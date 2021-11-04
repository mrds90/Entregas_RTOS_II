/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "FreeRTOS.h"
#include "frame_capture.h"
#include "string.h"
#include "task.h"

/*=====[Definition macros of private constants]==============================*/
#define START_OF_MESSAGE         '('
#define END_OF_MESSAGE           ')'
/*=====[ Definitions of private data types ]===================================*/

/*=====[Definitions of private variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Initializes the UART
 * 
 * @param UARTCallBackFunc 
 * @param parameter 
 */
static void FRAME_CAPTURE_UartRxInit( void *UARTCallBackFunc, void *parameter, uartMap_t uart);

/**
 * @brief RX UART ISR function. This function is called when a character is received and is stored in the buffer if the start of the message is received.
 * 
 * @param parameter frame_buffer_handler_t* with a QueueHandle_t and a QMPool* initialized
 */
static void FRAME_CAPTURE_UartRxISR(void *parameter);

/*=====[Implementations of public functions]=================================*/

void *FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart) {
    frame_capture_t *frame_capture = pvPortMalloc(sizeof(frame_capture_t));
    configASSERT(frame_capture != NULL);
    frame_capture->buff_ind = 0;
    frame_capture->frame_active = 0;
    frame_capture->buffer_handler.queue = xQueueCreate( QUEUE_SIZE, sizeof( frame_t ) );
    configASSERT(frame_capture->buffer_handler.queue != NULL);
    frame_capture->buffer_handler.pool = pool;
    FRAME_CAPTURE_UartRxInit(FRAME_CAPTURE_UartRxISR, (void*) frame_capture, uart);
    return (void *) frame_capture;
}

/*=====[Implementations of private functions]================================*/

static void FRAME_CAPTURE_UartRxInit( void *UARTCallBackFunc, void *parameter, uartMap_t uart) {  // Deberiamos pasarle tambien como parametro la UART a utilizar
   uartConfig(uart, 115200);
   uartCallbackSet(uart, UART_RECEIVE, UARTCallBackFunc, parameter);
   uartInterrupt(uart, true);
}

/*=====[Implementations of interrupt functions]==============================*/

static void FRAME_CAPTURE_UartRxISR( void *parameter ) {
    
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;
    frame_capture->frame_active;
    frame_capture->buff_ind;
    BaseType_t px_higher_priority_task_woken = pdFALSE;
        
    uint8_t character = uartRxRead(UART_USB);

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




