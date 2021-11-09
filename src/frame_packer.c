/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/
#include "FreeRTOS.h"
#include "task.h"

#include "frame_packer.h"
#include "frame_capture.h"

#include "string.h"
#include "crc8.h"

/*=====[Definition macros of private constants]==============================*/
#define CHARACTER_INDEX_ID               0
#define CHARACTER_INDEX_CMD              (CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_SIZE_CMD               1
#define CHARACTER_INDEX_DATA             (CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) * sizeof(uint8_t))
#define PRINT_FRAME_SIZE(size)           ((size) + (CHARACTER_INDEX_DATA + CHARACTER_SIZE_CRC) * sizeof(uint8_t))
#define FAKE_CRC                         "1B"
/*=====[ Definitions of private data types ]===================================*/
/**
 * @brief Resourse used to store the frame to be packed
 * 
 */
typedef struct {
    frame_buffer_handler_t *buffer_handler;
    uartMap_t uart;
} frame_packer_resources_t;

typedef struct {
    QMPool *pool;
    frame_t raw_frame;
    uartMap_t uart;
} frame_printer_resources_t;

typedef enum {
    START_FRAME,
    PRINT_FRAME,
    LAST_FRAME_CHAR,
    END_OF_FRAME,
} isr_printer_state_t;

/*=====[Definitions of private variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Create a frame ready to be printed
 * 
 * @param taskParmPtr 
 */
static void C2_FRAME_PACKER_PrinterTask(void* taskParmPtr);
/**
 * @brief Create a frame ready to be processed
 * 
 * @param taskParmPtr 
 */
static void C2_FRAME_PACKER_ReceiverTask(void* taskParmPtr);

static void C2_FRAME_PACKER_UartTxInit(void *UARTTxCallBackFunc, void *parameter);

static void C2_FRAME_PACKER_UartTxISR(void *parameter);

/*=====[Implementations of public functions]=================================*/

void C2_FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *app_buffer_handler_receive, uartMap_t uart) {
    frame_packer_resources_t *frame_packer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
    configASSERT(frame_packer_resources != NULL);
    frame_packer_resources->uart = uart;
    frame_packer_resources->buffer_handler = app_buffer_handler_receive;
    BaseType_t xReturned = xTaskCreate(
        C2_FRAME_PACKER_ReceiverTask,
        (const char *)"Frame Packer",
        configMINIMAL_STACK_SIZE * 5,
        (void *)frame_packer_resources,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(xReturned == pdPASS);
}

void C2_FRAME_PACKER_PrinterInit(frame_buffer_handler_t *app_buffer_handler_send, uartMap_t uart) {
    frame_packer_resources_t *printer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
    configASSERT(printer_resources != NULL);
    printer_resources->uart = uart;
    printer_resources->buffer_handler = app_buffer_handler_send;

    BaseType_t xReturned = xTaskCreate(
        C2_FRAME_PACKER_PrinterTask,
        (const char *)"Print Function",
        configMINIMAL_STACK_SIZE * 4,
        (void *)printer_resources,
        tskIDLE_PRIORITY + 1,
        NULL
    );

   configASSERT(xReturned == pdPASS);
}

/*=====[Implementations of private functions]================================*/

static void C2_FRAME_PACKER_ReceiverTask(void* taskParmPtr) {
    frame_packer_resources_t *frame_packer_resources = (frame_packer_resources_t*) taskParmPtr;
    frame_buffer_handler_t* buffer_handler_app = frame_packer_resources->buffer_handler;
    uartMap_t uart = frame_packer_resources->uart;
    frame_buffer_handler_t *buffer_handler_capture = C2_FRAME_CAPTURE_ObjInit(buffer_handler_app->pool, uart);
    vPortFree(frame_packer_resources);

    frame_t raw_frame;
    frame_t frame_app;

    while (TRUE) {
            xQueueReceive(buffer_handler_capture->queue, &raw_frame, portMAX_DELAY);
            // Chequear que tanto los caracteres del ID como del CRC esten en mayusculas, de otro modo el paquete seria invalido
            frame_app.data = &raw_frame.data[CHARACTER_INDEX_CMD];
            frame_app.data_size = raw_frame.data_size - CHARACTER_SIZE_ID;
            frame_app.data[frame_app.data_size] = '\0';
            xQueueSend(buffer_handler_app->queue, &frame_app, portMAX_DELAY);
    }
}


static void C2_FRAME_PACKER_PrinterTask(void* taskParmPtr) {
    frame_packer_resources_t *printer_resources = (frame_packer_resources_t*) taskParmPtr;
    frame_buffer_handler_t* buffer_handler_print = printer_resources->buffer_handler;
    uartMap_t uart = printer_resources->uart;

    frame_printer_resources_t printer_isr;
    printer_isr.pool = buffer_handler_print->pool;
    printer_isr.uart = uart;

    vPortFree(printer_resources);

    frame_t frame_print;

	// ----- Task repeat for ever -------------------------
	while(TRUE) {
        // Wait for message
        xQueueReceive(buffer_handler_print->queue, &frame_print, portMAX_DELAY);
        printer_isr.raw_frame.data = frame_print.data;
        uint8_t crc = crc8_calc(0, frame_print.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(frame_print.data_size) - CHARACTER_SIZE_CRC - 1);
        // Se arma el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
        snprintf(printer_isr.raw_frame.data, PRINT_FRAME_SIZE(frame_print.data_size), "%s%2X", frame_print.data - CHARACTER_SIZE_ID * sizeof(char), crc);
        printer_isr.raw_frame.data_size = PRINT_FRAME_SIZE(frame_print.data_size);

        // Se habilita la interrupcion para enviar el paquete a la capa de transmision C1
        C2_FRAME_PACKER_UartTxInit(C2_FRAME_PACKER_UartTxISR, (void*) &printer_isr);
        uartSetPendingInterrupt(uart);
    }
}

static void C2_FRAME_PACKER_UartTxInit(void *UARTTxCallBackFunc, void *parameter) {
   frame_printer_resources_t *printer_resources = (frame_printer_resources_t *) parameter;
   uartCallbackSet(printer_resources->uart, UART_TRANSMITER_FREE, UARTTxCallBackFunc, parameter);
}

/*=====[Implementations of interrupt functions]==============================*/

static void C2_FRAME_PACKER_UartTxISR(void *parameter) {
    frame_printer_resources_t *printer_resources = (frame_printer_resources_t *) parameter;
    static isr_printer_state_t isr_printer_state = START_FRAME;
    static uint8_t n = 0;

    gpioWrite(LED1, ON);

    switch (isr_printer_state)
    {
    case START_FRAME:
        if (uartTxReady(printer_resources->uart) && (0 == n)) {
            uartTxWrite(printer_resources->uart, START_OF_MESSAGE);
            isr_printer_state = PRINT_FRAME;
        }
        break;

    case PRINT_FRAME:
        if(n < printer_resources->raw_frame.data_size - 1) {
            if (uartTxReady(printer_resources->uart)) {
                uartTxWrite(printer_resources->uart, printer_resources->raw_frame.data[n]);
                n++;
            }     
        }  
        if(n == printer_resources->raw_frame.data_size - 1) {
            isr_printer_state = LAST_FRAME_CHAR;
        }
        break;

    case LAST_FRAME_CHAR:
        if(uartTxReady(printer_resources->uart)) {   
            uartTxWrite(printer_resources->uart, END_OF_MESSAGE);  
            isr_printer_state = END_OF_FRAME;
        }    
        break;

    case END_OF_FRAME:
        uartCallbackClr( printer_resources->uart, UART_TRANSMITER_FREE);
        UBaseType_t uxSavedInterruptStatus;
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        QMPool_put(printer_resources->pool, printer_resources->raw_frame.data - CHARACTER_BEFORE_DATA_SIZE); //< Se libera el bloque del pool de memoria
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);  
        n = 0;        
        isr_printer_state = START_FRAME;    
        break;
    
    default:
        break;
    }

    gpioWrite(LED1, OFF);
}



