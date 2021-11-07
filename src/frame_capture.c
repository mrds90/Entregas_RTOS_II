/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "FreeRTOS.h"
#include "frame_capture.h"
#include "string.h"
#include "task.h"

/*=====[Definition macros of private constants]==============================*/
#define CHECK_ID(character) ((character>='A' && character<='F') || (character>='0' && character<='9')) ? TRUE : FALSE
#define FRAME_MIN_SIZE      (CHARACTER_SIZE_ID + CHARACTER_SIZE_CRC)
/*=====[Definitions of private data types]===================================*/
typedef enum {
    FRAME_CAPTURE_STATE_IDLE,
    FRAME_CAPTURE_STATE_ID_CHECK,
    FRAME_CAPTURE_STATE_FRAME,
    FRAME_CAPTURE_STATE_END_OF_MESSAGE,
    FRAME_CAPTURE_STATE_ERROR,
} frame_capture_state_t;

typedef struct {
    frame_buffer_handler_t buffer_handler;
    frame_t raw_frame;
    uartMap_t uart;
    frame_capture_state_t state;
    bool_t frame_active;
    uint8_t buff_ind;
    uint8_t crc;
} frame_capture_t;

/*=====[Definitions of private variables]====================================*/

/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Initializes the UART
 * 
 * @param UARTCallBackFunc 
 * @param parameter 
 */
static void C2_FRAME_CAPTURE_UartRxInit(void *UARTCallBackFunc, void *parameter);

/**
 * @brief RX UART ISR function. This function is called when a character is received and is stored in the buffer if the start of the message is received.
 * 
 * @param parameter frame_buffer_handler_t* with a QueueHandle_t and a QMPool* initialized
 */
static void C2_FRAME_CAPTURE_UartRxISR(void *parameter);
 
static bool_t C2_FRAME_CAPTURE_CheckCRC(void);
/*=====[Implementations of public functions]=================================*/

void *C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart) {
    frame_capture_t *frame_capture = pvPortMalloc(sizeof(frame_capture_t));
    configASSERT(frame_capture != NULL);
    frame_capture->buff_ind = 0;
    frame_capture->crc = 0;
    frame_capture->frame_active = FALSE;
    frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
    frame_capture->buffer_handler.queue = xQueueCreate(QUEUE_SIZE, sizeof(frame_t));
    configASSERT(frame_capture->buffer_handler.queue != NULL);
    frame_capture->buffer_handler.pool = pool;
    frame_capture->uart = uart;
    C2_FRAME_CAPTURE_UartRxInit(C2_FRAME_CAPTURE_UartRxISR, (void*) frame_capture);
    return (void *) &frame_capture->buffer_handler;
}

/*=====[Implementations of private functions]================================*/

static void C2_FRAME_CAPTURE_UartRxInit(void *UARTCallBackFunc, void *parameter) {
   frame_capture_t *frame_capture = (frame_capture_t *) parameter;
   uartConfig(frame_capture->uart, 115200);
   uartCallbackSet(frame_capture->uart, UART_RECEIVE, UARTCallBackFunc, parameter);
   uartInterrupt(frame_capture->uart, true);
}
static bool_t C2_FRAME_CAPTURE_CheckCRC(void) {
    return TRUE;
}
/*=====[Implementations of interrupt functions]==============================*/

static void C2_FRAME_CAPTURE_UartRxISR(void *parameter) {
    bool_t error = FALSE;
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;
    BaseType_t px_higher_priority_task_woken = pdFALSE;

    char character = uartRxRead(frame_capture->uart); //!< Read the character from the UART (function of layer C1)

    switch (character) {
        case START_OF_MESSAGE:
            if(!frame_capture->frame_active) { 
                frame_capture->raw_frame.data = (uint8_t*) QMPool_get(frame_capture->buffer_handler.pool,0);
            }
            if (frame_capture->raw_frame.data != NULL) {
                frame_capture->crc = 0;
                frame_capture->raw_frame.data_size = 0;
                frame_capture->state = FRAME_CAPTURE_STATE_ID_CHECK;
                frame_capture->frame_active = TRUE;
            }
            break;
        case END_OF_MESSAGE:
            error = TRUE;
            if(frame_capture->raw_frame.data_size >= FRAME_MIN_SIZE) {
                //TODO: Chequeo de CRC
                if (C2_FRAME_CAPTURE_CheckCRC()) {
                    frame_capture->raw_frame.data_size -= CHARACTER_SIZE_CRC;
                    if(frame_capture->buffer_handler.queue != NULL) {
                        frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
                        frame_capture->frame_active = FALSE;
                        xQueueSendFromISR(frame_capture->buffer_handler.queue, &frame_capture->raw_frame, &px_higher_priority_task_woken);
                        if (px_higher_priority_task_woken == pdTRUE) {
                            portYIELD_FROM_ISR(px_higher_priority_task_woken);
                        }
                        error = FALSE;
                    }
                }
            }
        default:
            switch (frame_capture->state) {
                case FRAME_CAPTURE_STATE_IDLE:

                    break;
                case FRAME_CAPTURE_STATE_ID_CHECK:
                    if (CHECK_ID(character)) {
                        frame_capture->raw_frame.data[frame_capture->raw_frame.data_size++] = character;
                        if (frame_capture->raw_frame.data_size == CHARACTER_SIZE_ID) {
                            frame_capture->state = FRAME_CAPTURE_STATE_FRAME;
                        }
                    } 
                    else {
                        error = TRUE;
                    }
                    break;
                case FRAME_CAPTURE_STATE_FRAME:
                    frame_capture->raw_frame.data[frame_capture->raw_frame.data_size++] = character;
                    //TODO: Calculo de CRC
                    if (frame_capture->buff_ind >= MAX_BUFFER_SIZE) {
                        error = TRUE;
                    }
                    break;
                default:
                    error = TRUE;
                    break;
            }
            break;
    }

    if (error) {
        QMPool_put(frame_capture->buffer_handler.pool, (void*) frame_capture->raw_frame.data);
        frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
        frame_capture->frame_active = FALSE;
    }
}





