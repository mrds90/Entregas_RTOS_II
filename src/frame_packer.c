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
typedef enum {
    START_FRAME,
    PRINT_FRAME,
    LAST_FRAME_CHAR,
    END_OF_FRAME,
} isr_printer_state_t;
typedef struct {
    QMPool *pool;
    frame_t transmit_frame;
    uartMap_t uart;
    uint8_t buff_ind;
    isr_printer_state_t isr_printer_state;    
} frame_printer_resources_t;

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

/**
 * @brief Inicializa la interrupción de Tx de la UART.
 * 
 * @param UARTRxCallBackFunc
 * @param parameter 
 */
static void C2_FRAME_PACKER_UartTxInit(void *UARTTxCallBackFunc, void *parameter);

/**
 * @brief TX UART ISR function. Esta función se llama cuando el buffer de la UART está vacío.
 * 
 * @param parameter QMPool*, frame_t y Uart inicializados.
 */
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
        // Se espera a que llegue el paquete procesado
        xQueueReceive(buffer_handler_print->queue, &frame_print, portMAX_DELAY);
        printer_isr.transmit_frame.data = frame_print.data;
        // Se calcula el CRC del paquete procesado
        uint8_t crc = crc8_calc(0, frame_print.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(frame_print.data_size) - CHARACTER_SIZE_CRC - 1);
        // Se arma el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
        snprintf(printer_isr.transmit_frame.data, PRINT_FRAME_SIZE(frame_print.data_size), "%s%2X", frame_print.data - CHARACTER_SIZE_ID * sizeof(char), crc);
        printer_isr.transmit_frame.data_size = PRINT_FRAME_SIZE(frame_print.data_size);
        printer_isr.buff_ind = 0;
        printer_isr.isr_printer_state = START_FRAME;

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

    gpioWrite(LED1, ON);       // Para Debug

    switch (printer_resources->isr_printer_state)
    {
    case START_FRAME:                                               // Se imprime el caracter de comienzo de paquete
        if (uartTxReady(printer_resources->uart) && (0 == printer_resources->buff_ind)) {
            uartTxWrite(printer_resources->uart, START_OF_MESSAGE);
            printer_resources->isr_printer_state = PRINT_FRAME;
        }
        break;

    case PRINT_FRAME:
        while(printer_resources->buff_ind < printer_resources->transmit_frame.data_size - 1) {     // Se imprime el contenido del paquete
            if (uartTxReady(printer_resources->uart)) {
                uartTxWrite(printer_resources->uart, printer_resources->transmit_frame.data[printer_resources->buff_ind]);
                printer_resources->buff_ind++;
            }   
            else break;  
        }  
        if(printer_resources->buff_ind == printer_resources->transmit_frame.data_size - 1) {       // Si es el ultimo caracter del paquete se manda a imprimir el fin de paquete
            printer_resources->isr_printer_state = LAST_FRAME_CHAR;
        }
        break;

    case LAST_FRAME_CHAR:                                           // Se imprime el caracter de fin de paquete
        if(uartTxReady(printer_resources->uart)) {   
            uartTxWrite(printer_resources->uart, END_OF_MESSAGE);  
            printer_resources->isr_printer_state = END_OF_FRAME;
        }    
        break;

    case END_OF_FRAME:                                              // Se deshalibilita la interrupcion de Tx de la Uart y se libera el bloque del pool de memoria
        uartCallbackClr( printer_resources->uart, UART_TRANSMITER_FREE);
        UBaseType_t uxSavedInterruptStatus;
        uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
        QMPool_put(printer_resources->pool, printer_resources->transmit_frame.data - CHARACTER_BEFORE_DATA_SIZE); //< Se libera el bloque del pool de memoria
        taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);  
        break;
    
    default:
        break;
    }

    gpioWrite(LED1, OFF);       // Para Debug
}



