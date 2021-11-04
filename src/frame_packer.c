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
/*=====[ Definitions of private data types ]===================================*/
/**
 * @brief States of the packer state machine
 * 
 */
typedef enum {
    FRAME_WAITING,
    FRAME_CRC_CHECK,
    FRAME_PROSESSING_WAITING,
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
static void FRAME_PACKER_PrinterTask(void* taskParmPtr);
/**
 * @brief Create a frame ready to be processed
 * 
 * @param taskParmPtr 
 */
static void FRAME_PACKER_ReceiverTask(void* taskParmPtr);

/*=====[Implementations of public functions]=================================*/

void FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *app_buffer_handler, uartMap_t uart) {
    frame_packer_resources_t *frame_packer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
    configASSERT(frame_packer_resources != NULL);
    frame_packer_resources->uart = uart;
    frame_packer_resources->buffer_handler = app_buffer_handler;

    BaseType_t xReturned = xTaskCreate(
        FRAME_PACKER_ReceiverTask,
        (const char *)"Frame Packer",
        configMINIMAL_STACK_SIZE * 5,
        (void *)frame_packer_resources,
        tskIDLE_PRIORITY + 1,
        NULL
    );
    configASSERT(xReturned == pdPASS);
}

void FRAME_PACKER_PrinterInit(frame_buffer_handler_t *packer_buffer_handler) {
    BaseType_t xReturned = xTaskCreate(
        FRAME_PACKER_PrinterTask,
        (const char *)"Print Function",
        configMINIMAL_STACK_SIZE * 4,
        (void *)packer_buffer_handler,
        tskIDLE_PRIORITY + 1,
        NULL
    );

   configASSERT( xReturned == pdPASS );
}

/*=====[Implementations of private functions]================================*/

static void FRAME_PACKER_ReceiverTask(void* taskParmPtr) {
    frame_packer_resources_t *frame_packer_resources = (frame_packer_resources_t*) taskParmPtr;
    frame_buffer_handler_t* buffer_handler_app = frame_packer_resources->buffer_handler;
    uartMap_t uart = frame_packer_resources->uart;
    frame_capture_t *frame_capture = FRAME_CAPTURE_ObjInit(buffer_handler_app->pool, uart);
    vPortFree(frame_packer_resources);

    frame_t raw_frame;
    frame_t frame_app;
    frame_state_t state = FRAME_WAITING;

	frame_buffer_handler_t packer_buffer_handler = {
		.pool = buffer_handler_app->pool,
        .queue_receive = NULL,
        .queue_send = NULL,
    };   

    if ( packer_buffer_handler.queue_send == NULL ) {
        packer_buffer_handler.queue_send = xQueueCreate( QUEUE_SIZE, sizeof( frame_t ) );
    }
    configASSERT( packer_buffer_handler.queue_send != NULL ); 

	FRAME_PACKER_PrinterInit(&packer_buffer_handler);  			

    while (1) {
        bool frame_correct = false, crc_valid = false;

        switch (state) {
            case FRAME_WAITING:
                xQueueReceive(frame_capture->buffer_handler.queue_send, &raw_frame, portMAX_DELAY);
                // Separar aca los campos ID, C+DATA y CRC del paquete recibido
                // Chequear que tanto los caracteres del ID como del CRC esten en mayusculas, de otro modo el paquete seria invalido
                state = FRAME_CRC_CHECK;
                break;
            case FRAME_CRC_CHECK:
                // TODO: CHECK CRC
                crc_valid = true;
				
				// Si el CRC es valido, enviar C+DATA a la capa C3
				if(crc_valid) {
					frame_app.data = &raw_frame.data[CHARACTER_INDEX_CMD];
					frame_app.data_size = raw_frame.data_size - CHARACTER_SIZE_ID;
					frame_app.data[frame_app.data_size] = '\0';
					xQueueSend(buffer_handler_app->queue_receive, &frame_app, portMAX_DELAY);	
				}
                state = FRAME_PROSESSING_WAITING;
                break;
            case FRAME_PROSESSING_WAITING:
				// Se espera a que la aplicacion procese los datos
				xQueueReceive(buffer_handler_app->queue_send, &frame_app, portMAX_DELAY);
				if(frame_app.data_size > 0)
                	frame_correct = true;
                if(frame_correct) {
                    // TODO: Volver a armar el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
                    state = FRAME_COMPLETE;
                }
                else {
                    state = FRAME_WAITING;
                    portENTER_CRITICAL(); 
                    QMPool_put(frame_capture->buffer_handler.pool, raw_frame.data);
                    portEXIT_CRITICAL(); 
                    raw_frame.data = NULL;
                }
                break;
            case FRAME_COMPLETE:
                frame_app.data[frame_app.data_size] = '\0';
				// Se envia el paquete para imprimir
                xQueueSend(packer_buffer_handler.queue_send, &frame_app, portMAX_DELAY);
                state = FRAME_WAITING;
                break;
            default:
                break;
        }

    }
}


void FRAME_PACKER_PrinterTask(void* taskParmPtr) {
   frame_buffer_handler_t *packer_buffer_handler = (frame_buffer_handler_t*) taskParmPtr;
   frame_t frame_print;

   // ----- Task repeat for ever -------------------------
   while(TRUE) {
      // Wait for message
      xQueueReceive(packer_buffer_handler->queue_send, &frame_print, portMAX_DELAY);
      
	  uartWriteByte(UART_USB, START_OF_MESSAGE);
      uartWriteString(UART_USB, frame_print.data);
	  uartWriteByte(UART_USB, END_OF_MESSAGE);
      uartWriteString(UART_USB, "\n");

      taskENTER_CRITICAL();
      QMPool_put(packer_buffer_handler->pool, frame_print.data - CHARACTER_BEFORE_DATA_SIZE);
      taskEXIT_CRITICAL();
   }
}




