/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "frame_packer.h"
#include "string.h"
#include "qmpool.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "sapi.h"

/*=====[Definition macros of private constants]==============================*/
#define MAX_BUFF				200
#define START_OF_MESSAGE 		'('
#define END_OF_MESSAGE 			')'
#define CHARACTER_INDEX_ID 		0
#define CHARACTER_SIZE_ID		4
#define CHARACTER_INDEX_CMD 	(CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_SIZE_CMD		1
#define CHARACTER_INDEX_DATA 	(CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_SIZE_CRC		2
#define QUEUE_SIZE				10
/*=====[ Definitions of private data types ]===================================*/


typedef struct {
	char *data;
	uint8_t data_size;
} raw_frame_t;
/*=====[Definitions of private global variables]=============================*/
static char RxBuff[MAX_BUFF] = {0};
static uint8_t buff_ind = 0;

/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Initializes the UART
 * 
 * @param UARTCallBackFunc 
 * @param parameter 
 */
static void UART_RX_Init( void *UARTCallBackFunc, void *parameter );
/**
 * @brief RX UART ISR function. This function is called when a character is received and is stored in the buffer if the start of the message is received.
 * 
 * @param parameter buffer_handler_t* with a QueueHandle_t and a QMPool* initialized
 */
static void UART_RX_ISRFunction(void *parameter);
/*=====[Implementations of public functions]=================================*/

void TASK_FramePacker(void* taskParmPtr) {
	
	buffer_handler_t* buffer_handler_app = (buffer_handler_t*) taskParmPtr;

	static buffer_handler_t *buffer_handler_isr = {NULL};
	buffer_handler_isr->pool = buffer_handler_app->pool;
	buffer_handler_isr->queue = xQueueCreate(QUEUE_SIZE, sizeof(raw_frame_t*));
	configASSERT(buffer_handler_isr->queue != NULL);

	UART_RX_Init(UART_RX_ISRFunction, (void*) buffer_handler_isr);

	static raw_frame_t raw_frame;
	static frame_t frame_api;
	static frame_state_t state = FRAME_WAITING;
	while (1) {
		uint8_t frame_correct = 0;
		switch (state) {
		case FRAME_WAITING:
			xQueueReceive(buffer_handler_isr->queue, &raw_frame, portMAX_DELAY);
			state = FRAME_CRC_CHECK;
			break;
		case FRAME_CRC_CHECK:
			// CHECK CRC
			break;
		case FRAME_PROSESSING:
			// PROCESS FRAME
			//TODO: Routine that validate the frame
			if(frame_correct) {
				frame_api.id = &raw_frame.data[CHARACTER_INDEX_ID];
				frame_api.cmd = &raw_frame.data[CHARACTER_INDEX_CMD];
				frame_api.data = &raw_frame.data[CHARACTER_INDEX_DATA];
				frame_api.data_size = raw_frame.data_size - CHARACTER_SIZE_ID - CHARACTER_SIZE_CRC;
				state = FRAME_COMPLETE;
			}
			else {
				state = FRAME_WAITING;
				QMPool_put(buffer_handler_isr->pool, raw_frame.data);
				raw_frame.data = NULL;
			}
			break;
		case FRAME_COMPLETE:
			// SEND TO API
			xQueueSend(buffer_handler_app->queue, &frame_api, portMAX_DELAY);
			break;
		default:
			break;
		}
		
	}
}

void TASK_FramePrinter(void* taskParmPtr) {
   buffer_handler_t *buffer_handler_print = (buffer_handler_t*) taskParmPtr;
   frame_t frame_print;
   // ----- Task repeat for ever -------------------------
   while(TRUE) {
      // Wait for message
      xQueueReceive(buffer_handler_print->queue, &frame_print, portMAX_DELAY);
	  // Print message
	  snprintf(frame_print.id, frame_print.data_size + 1, "%s%s%s", frame_print.id, frame_print.cmd, frame_print.data); 
	  printf("%s\n", frame_print.id);
	  QMPool_put(buffer_handler_print->pool, frame_print.id);
   }
}
/*=====[Implementations of private functions]================================*/

static void UART_RX_Init( void *UARTCallBackFunc, void *parameter ) {
   uartConfig(UART_USB, 115200);
   uartCallbackSet(UART_USB, UART_RECEIVE, UARTCallBackFunc, parameter);
   uartInterrupt(UART_USB, true);
}

/*=====[Implementations of interrupt functions]==============================*/

static void UART_RX_ISRFunction( void *parameter ) {
	static uint8_t frame_active = 0;
	static raw_frame_t *raw_frame; 
	BaseType_t px_higher_priority_task_woken = pdFALSE;
	buffer_handler_t* buffer_handler = (buffer_handler_t*) parameter;
	char character;
	character = uartRxRead(UART_USB);

	if (character == START_OF_MESSAGE) {
		if(frame_active == 0) {
			raw_frame->data = (char*) QMPool_get(buffer_handler->pool,0);
		}
		if (raw_frame->data != NULL) {
			buff_ind = 0;
			frame_active = 1;
		}
	}
	else if ((character == END_OF_MESSAGE) && frame_active) {
		frame_active = 0;
		raw_frame->data_size = buff_ind;
		if(buffer_handler->queue != NULL) {
			xQueueSendFromISR(buffer_handler->queue, raw_frame, &px_higher_priority_task_woken);
			if (px_higher_priority_task_woken == pdTRUE) {
				portYIELD_FROM_ISR(px_higher_priority_task_woken);
			}
		}
	}
	else if (frame_active) {
		RxBuff[buff_ind++] = character;
	}
	else if (buff_ind >= MAX_BUFF) {
		buff_ind = 0;
		frame_active = 0;
		QMPool_put(buffer_handler->pool, (void*) raw_frame->data);
	}
}




