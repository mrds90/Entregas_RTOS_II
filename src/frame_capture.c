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
#include "crc8.h"
#include <stdio.h>
#include <ctype.h>

/*=====[Definition macros of private constants]==============================*/
#define CHECK_HEXA(character)  (((character >= 'A' && character <= 'F') || (character >= '0' && character <= '9')) ? TRUE : FALSE)
#define FRAME_MIN_SIZE         (CHARACTER_SIZE_ID + CHARACTER_SIZE_CRC)
#define A_HEXA_VALUE           (10)
#define NIBBLE_SIZE            (4)
#define NIBBLE_BIT(nibble, max_nibbles) (NIBBLE_SIZE * (max_nibbles - (nibble + 1)))
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
 * @param UARTRxCallBackFunc
 * @param parameter
 */
static void C2_FRAME_CAPTURE_UartRxInit(void *UARTRxCallBackFunc, void *parameter);

/**
 * @brief RX UART ISR function. This function is called when a character is received and is stored in the buffer if the start of the message is received.
 *
 * @param parameter frame_buffer_handler_t* with a QueueHandle_t and a QMPool* initialized
 */
static void C2_FRAME_CAPTURE_UartRxISR(void *parameter);

__STATIC_FORCEINLINE bool_t C2_FRAME_CAPTURE_CheckCRC(frame_t frame, uint8_t crc);

static uint8_t C2_FRAME_CAPTURE_AsciiHexaToInt(char *ascii, uint8_t n);

/*=====[Implementations of public functions]=================================*/

void*C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart) {
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
    C2_FRAME_CAPTURE_UartRxInit(C2_FRAME_CAPTURE_UartRxISR, (void *) frame_capture);
    return (void *) &frame_capture->buffer_handler;
}

/*=====[Implementations of private functions]================================*/

static void C2_FRAME_CAPTURE_UartRxInit(void *UARTRxCallBackFunc, void *parameter) {
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;
    uartConfig(frame_capture->uart, 115200);
    uartCallbackSet(frame_capture->uart, UART_RECEIVE, UARTRxCallBackFunc, parameter);
    uartInterrupt(frame_capture->uart, true);
}

__STATIC_FORCEINLINE bool_t C2_FRAME_CAPTURE_CheckCRC(frame_t frame, uint8_t crc) {
    bool_t ret = FALSE;

    if ((CHECK_HEXA(frame.data[frame.data_size])) && (CHECK_HEXA(frame.data[frame.data_size + 1]))) {
        if (C2_FRAME_CAPTURE_AsciiHexaToInt(&frame.data[frame.data_size], CHARACTER_SIZE_CRC) == crc) {
            ret = TRUE;
        }
    }

    return ret;
}

__STATIC_FORCEINLINE uint8_t C2_FRAME_CAPTURE_AsciiHexaToInt(char *ascii, uint8_t digit_qty) {
    uint8_t ret = 0, hex_digit = 0;

    for (uint8_t nibble = 0; nibble < digit_qty; nibble++) {
        if (isdigit(ascii[nibble])) hex_digit = ascii[nibble] - '0';
        else hex_digit = ascii[nibble] - 'A' + A_HEXA_VALUE;
        ret += hex_digit << NIBBLE_BIT(nibble, digit_qty);
    }

    return ret;
}

/*=====[Implementations of interrupt functions]==============================*/

static void C2_FRAME_CAPTURE_UartRxISR(void *parameter) {
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;

    while (uartRxReady(frame_capture->uart)) {
        bool_t error = FALSE;
        BaseType_t px_higher_priority_task_woken = pdFALSE;

        char character = uartRxRead(frame_capture->uart); //!< Read the character from the UART (function of layer C1)

        switch (character) {
            case START_OF_MESSAGE:
                if (!frame_capture->frame_active) {
                    UBaseType_t uxSavedInterruptStatus;
                    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
                    frame_capture->raw_frame.data = (char *) QMPool_get(frame_capture->buffer_handler.pool, 0);
                    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                }
                if (frame_capture->raw_frame.data != NULL) {
                    frame_capture->state = FRAME_CAPTURE_STATE_ID_CHECK;
                    frame_capture->frame_active = TRUE;
                    frame_capture->buff_ind = 0;
                    frame_capture->crc = 0;
                }
                break;

            case END_OF_MESSAGE:
                if (frame_capture->frame_active) {
                    error = TRUE;
                    if (frame_capture->buff_ind >= FRAME_MIN_SIZE) {
                        frame_capture->raw_frame.data_size = frame_capture->buff_ind - CHARACTER_SIZE_CRC; // Es el tamaño de los datos
                        if (C2_FRAME_CAPTURE_CheckCRC(frame_capture->raw_frame, frame_capture->crc)) {
                            if (frame_capture->buffer_handler.queue != NULL) {
                                frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
                                frame_capture->frame_active = FALSE;
                                if (xQueueSendFromISR(frame_capture->buffer_handler.queue, &frame_capture->raw_frame, &px_higher_priority_task_woken) == pdTRUE) {
                                    if (px_higher_priority_task_woken == pdTRUE) {
                                        portYIELD_FROM_ISR(px_higher_priority_task_woken);
                                    }

                                    error = FALSE;
                                }
                            }
                        }
                    }
                }
                break;

            default:
                switch (frame_capture->state) {
                    case FRAME_CAPTURE_STATE_IDLE:

                        break;

                    case FRAME_CAPTURE_STATE_ID_CHECK:
                        if (CHECK_HEXA(character)) {
                            frame_capture->raw_frame.data[frame_capture->buff_ind++] = character;
                            if (frame_capture->buff_ind == CHARACTER_SIZE_ID) {
                                frame_capture->crc = crc8_calc(frame_capture->crc, frame_capture->raw_frame.data, 2);
                                frame_capture->state = FRAME_CAPTURE_STATE_FRAME;
                            }
                        }
                        else {
                            error = TRUE;
                        }
                        break;

                    case FRAME_CAPTURE_STATE_FRAME:
                        frame_capture->raw_frame.data[frame_capture->buff_ind] = character;
                        frame_capture->crc = crc8_calc(frame_capture->crc, &frame_capture->raw_frame.data[frame_capture->buff_ind - 2], 1);
                        frame_capture->buff_ind++;
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
            UBaseType_t uxSavedInterruptStatus;
            uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            QMPool_put(frame_capture->buffer_handler.pool, (void *) frame_capture->raw_frame.data);
            frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
            frame_capture->frame_active = FALSE;
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        }
    }
}
