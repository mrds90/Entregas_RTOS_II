/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
 * pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "frame_packer.h"
#include "string.h"
#include "qmpool.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "sapi.h"

/*=====[Definition macros of private constants]==============================*/
#define START_OF_MESSAGE 				'('
#define END_OF_MESSAGE 					')'
#define CHARACTER_INDEX_ID 				0
#define CHARACTER_SIZE_ID				4
#define CHARACTER_INDEX_CMD 			(CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_SIZE_CMD				1
#define CHARACTER_INDEX_DATA 			(CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_SIZE_CRC				2
#define QUEUE_SIZE						5
#define CHARACTER_BEFORE_DATA_SIZE 		((CHARACTER_SIZE_ID) * sizeof(uint8_t))
/*=====[ Definitions of private data types ]===================================*/

typedef struct {
	buffer_handler_t buffer_handler;
	frame_t raw_frame;
	uint8_t frame_active;
	uint8_t buff_ind;
} frame_capture_t;
/*=====[Definitions of private variables]=============================*/


/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Initializes the UART
 * 
 * @param UARTCallBackFunc 
 * @param parameter 
 */
static void UART_RX_Init( void *UARTCallBackFunc, void *parameter, uartMap_t uart);

/**
 * @brief RX UART ISR function. This function is called when a character is received and is stored in the buffer if the start of the message is received.
 * 
 * @param parameter buffer_handler_t* with a QueueHandle_t and a QMPool* initialized
 */
static void UART_RX_ISRFunction(void *parameter);

static void TASK_FramePacker(void* taskParmPtr);

/*=====[Implementations of public functions]=================================*/

BaseType_t FramePackerInit(buffer_handler_t *app_buffer_handler_receive, uartMap_t uart) {
	frame_packer_resources_t *frame_packer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
	configASSERT(frame_packer_resources != NULL);
	frame_packer_resources->uart = uart;
	frame_packer_resources->buffer_handler = app_buffer_handler_receive;
	BaseType_t xReturned = xTaskCreate(
		TASK_FramePacker,
		(const char *)"Frame Packer",
		configMINIMAL_STACK_SIZE * 5,
		(void *) frame_packer_resources,
		tskIDLE_PRIORITY + 1,
		NULL
	);

	return xReturned;
}

void TASK_FramePrinter(void* taskParmPtr) {
   buffer_handler_t *buffer_handler_print = (buffer_handler_t*) taskParmPtr;
   frame_t frame_print;

   // ----- Task repeat for ever -------------------------
   while(TRUE) {
      // Wait for message
      xQueueReceive(buffer_handler_print->queue, &frame_print, portMAX_DELAY);
	  // snprintf(frame_print.id, frame_print.data_size + 1, "%s%s%s", frame_print.id, frame_print.cmd, frame_print.data); 
	  uartWriteString(UART_USB, frame_print.data);
	  uartWriteString(UART_USB, "\n");
	  //printf("%s\n", frame_print.id);

	  taskENTER_CRITICAL();
	  QMPool_put(buffer_handler_print->pool, frame_print.data - CHARACTER_BEFORE_DATA_SIZE);
	  taskEXIT_CRITICAL();
   }
}
/*=====[Implementations of private functions]================================*/

static void TASK_FramePacker(void* taskParmPtr) {
	frame_packer_resources_t *frame_packer_resources = (frame_packer_resources_t*) taskParmPtr;
	frame_capture_t *frame_capture = pvPortMalloc(sizeof(frame_capture_t));
	configASSERT(frame_capture != NULL);
	
    buffer_handler_t* buffer_handler_app = frame_packer_resources->buffer_handler;
	uartMap_t uart = frame_packer_resources->uart;
	vPortFree(frame_packer_resources);
	frame_capture->buff_ind = 0;
	frame_capture->frame_active = 0;
    frame_capture->buffer_handler.queue = xQueueCreate( QUEUE_SIZE, sizeof( frame_t ) );
	configASSERT(frame_capture->buffer_handler.queue != NULL);

	frame_capture->buffer_handler.pool = buffer_handler_app->pool;

	UART_RX_Init(UART_RX_ISRFunction, (void*) frame_capture, uart);

	frame_t raw_frame;
	frame_t frame_app;
	frame_state_t state = FRAME_WAITING;

	while (1) {
		uint8_t frame_correct = 0;

		switch (state) {
			case FRAME_WAITING:
				xQueueReceive(frame_capture->buffer_handler.queue, &raw_frame, portMAX_DELAY);
							// Separar aca los campos ID, C+DATA y CRC del paquete recibido
							// Chequear que tanto los caracteres del ID como del CRC esten en mayusculas, de otro modo el paquete seria invalido
				state = FRAME_CRC_CHECK;
				break;
			case FRAME_CRC_CHECK:
				// CHECK CRC
							// Si el CRC es valido, enviar C+DATA a la capa C3
				state = FRAME_PROSESSING;
				break;
			case FRAME_PROSESSING:
				// PROCESS FRAME
				//TODO: Routine that validate the frame
				frame_correct = 1;
				if(frame_correct) {
					frame_app.data = &raw_frame.data[CHARACTER_INDEX_CMD];
					frame_app.data_size = raw_frame.data_size - CHARACTER_SIZE_ID;
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
				// SEND TO API
				frame_app.data[frame_app.data_size] = '\0';
				xQueueSend(buffer_handler_app->queue, &frame_app, portMAX_DELAY);
				state = FRAME_WAITING;
				break;
			default:
				break;
		}
		
	}
}

static void UART_RX_Init( void *UARTCallBackFunc, void *parameter, uartMap_t uart) {  // Deberiamos pasarle tambien como parametro la UART a utilizar
   uartConfig(uart, 115200);
   uartCallbackSet(uart, UART_RECEIVE, UARTCallBackFunc, parameter);
   uartInterrupt(uart, true);
}

/*=====[Implementations of interrupt functions]==============================*/

static void UART_RX_ISRFunction( void *parameter ) {
	
	frame_capture_t *frame_capture = (frame_capture_t *) parameter;
	frame_capture->frame_active;
	frame_capture->buff_ind;
	BaseType_t px_higher_priority_task_woken = pdFALSE;
		
	char character;
	character = uartRxRead(UART_USB);

	if (character == START_OF_MESSAGE) {
		if(frame_capture->frame_active == 0) {
			frame_capture->raw_frame.data = (uint8_t*) QMPool_get(frame_capture->buffer_handler.pool,0);
		}
		if (frame_capture->raw_frame.data != NULL) {
			frame_capture->buff_ind = 0;
			frame_capture->frame_active = 1;
		}
	}
	else if ((character == END_OF_MESSAGE) && frame_capture->frame_active) {
		frame_capture->frame_active = 0;
		frame_capture->raw_frame.data_size = frame_capture->buff_ind - CHARACTER_SIZE_CRC;
		if(frame_capture->buffer_handler.queue != NULL) {
			xQueueSendFromISR(frame_capture->buffer_handler.queue, &frame_capture->raw_frame, &px_higher_priority_task_woken);
			if (px_higher_priority_task_woken == pdTRUE) {
				portYIELD_FROM_ISR(px_higher_priority_task_woken);
			}
		}
	}
	else if (frame_capture->buff_ind >= MAX_BUFFER_SIZE) {
		frame_capture->buff_ind = 0;
		frame_capture->frame_active = 0;
		QMPool_put(frame_capture->buffer_handler.pool, (void*) frame_capture->raw_frame.data);
	}
	else if (frame_capture->frame_active) {
		frame_capture->raw_frame.data[frame_capture->buff_ind++] = character;
	}
}




