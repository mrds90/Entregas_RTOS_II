/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/
#include "FreeRTOS.h"
#include "task.h"

#include "frame_packer.h"
#include "frame_capture.h"

#include "string.h"

/*=====[Definition macros of private constants]==============================*/
#define CHARACTER_INDEX_ID               0
#define CHARACTER_SIZE_ID                4
#define CHARACTER_INDEX_CMD              (CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_SIZE_CMD               1
#define CHARACTER_INDEX_DATA             (CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) * sizeof(uint8_t))
#define PRINT_FRAME_SIZE(size)           ((size) + (CHARACTER_INDEX_DATA + CHARACTER_SIZE_CRC) * sizeof(uint8_t))
#define FAKE_CRC                         "1B"
/*=====[ Definitions of private data types ]===================================*/
/**
 * @brief States of the packer state machine
 * 
 */
typedef enum {
    FRAME_WAITING,
    FRAME_CRC_CHECK,
    FRAME_PROSESSING,
    FRAME_COMPLETE,
    FRAME_STATE_QTY
} frame_state_t;

/**
 * @brief Resourse used to store the frame to be packed
 * 
 */
typedef struct {
    frame_buffer_handler_t *buffer_handler;
    uartMap_t uart;
} frame_packer_resources_t;

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
    frame_capture_t *frame_capture = C2_FRAME_CAPTURE_ObjInit(buffer_handler_app->pool, uart);
    vPortFree(frame_packer_resources);

    frame_t raw_frame;
    frame_t frame_app;
    frame_state_t state = FRAME_WAITING;

    while (TRUE) {
        uint8_t frame_correct = 0;

        switch (state) {
            case FRAME_WAITING:
                xQueueReceive(frame_capture->buffer_handler.queue, &raw_frame, portMAX_DELAY);
                // Chequear que tanto los caracteres del ID como del CRC esten en mayusculas, de otro modo el paquete seria invalido
                state = FRAME_CRC_CHECK;
                break;
            case FRAME_CRC_CHECK:
                // CHECK CRC
                // Si el CRC es valido, 
                state = FRAME_PROSESSING;
                break;
            case FRAME_PROSESSING:
                // PROCESS FRAME
                // TODO: Routine that validate the ID
                // Separar aca los campos ID, C+DATA y CRC del paquete recibido
                frame_correct = 1;
                if(frame_correct) {
                    frame_app.data = &raw_frame.data[CHARACTER_INDEX_CMD];
                    frame_app.data_size = raw_frame.data_size - CHARACTER_SIZE_ID;
                    state = FRAME_COMPLETE;
                }
                else {
                    state = FRAME_WAITING;
                    taskENTER_CRITICAL(); 
                    QMPool_put(frame_capture->buffer_handler.pool, raw_frame.data);
                    taskEXIT_CRITICAL(); 
                    raw_frame.data = NULL;
                }
                break;
            case FRAME_COMPLETE:
                // enviar C+DATA a la capa C3
                frame_app.data[frame_app.data_size] = '\0';
                xQueueSend(buffer_handler_app->queue, &frame_app, portMAX_DELAY);
                state = FRAME_WAITING;
                break;
            default:
                break;
        }

    }
}


static void C2_FRAME_PACKER_PrinterTask(void* taskParmPtr) {
    frame_packer_resources_t *printer_resources = (frame_packer_resources_t*) taskParmPtr;
    frame_buffer_handler_t* buffer_handler_print = printer_resources->buffer_handler;
    uartMap_t uart = printer_resources->uart;
    vPortFree(printer_resources);

    frame_t frame_print;

	// ----- Task repeat for ever -------------------------
	while(TRUE) {
        // Wait for message
        xQueueReceive(buffer_handler_print->queue, &frame_print, portMAX_DELAY);
        // TODO: Volver a armar el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
        snprintf(frame_print.data, PRINT_FRAME_SIZE(frame_print.data_size), "%s%s",  frame_print.data - CHARACTER_SIZE_ID * sizeof(char), FAKE_CRC);
        // Enviar el paquete a la capa de transmision C1
        uartWriteByte(uart, START_OF_MESSAGE);
        uartWriteString(uart, frame_print.data);    // Print the frame
        uartWriteByte(uart, END_OF_MESSAGE);
        uartWriteString(uart, "\n");

        taskENTER_CRITICAL();
        QMPool_put(buffer_handler_print->pool, frame_print.data - CHARACTER_BEFORE_DATA_SIZE); //< Free the memory
        taskEXIT_CRITICAL();
    }
}




