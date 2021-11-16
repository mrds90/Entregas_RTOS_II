/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.2
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "frame_capture.h"
#include "string.h"
#include "task.h"
#include "crc8.h"
#include <stdio.h>
#include <ctype.h>

/*=====[Definición de macros de constantes privadas]==============================*/

#define CHECK_HEXA(character)  (((character >= 'A' && character <= 'F') || (character >= '0' && character <= '9')) ? TRUE : FALSE)
#define FRAME_MIN_SIZE         (CHARACTER_SIZE_ID + CHARACTER_SIZE_CRC)
#define A_HEXA_VALUE           (10)
#define NIBBLE_SIZE            (4)
#define NIBBLE_BIT(nibble, max_nibbles) (NIBBLE_SIZE * (max_nibbles - (nibble + 1)))

/*=====[Definición de tipos de datos privados]===================================*/

/**
 * @brief Estados de MEF de recepción de datos.
 */
typedef enum {
    FRAME_CAPTURE_STATE_IDLE,
    FRAME_CAPTURE_STATE_ID_CHECK,
    FRAME_CAPTURE_STATE_FRAME,
} frame_capture_state_t;


/**
 * @brief Recurso utilizado para almacenar el contexto de la recepción de datos
 * 
 * @param buffer_handler puntero a pool y queue
 * @param raw_frame puntero a char de trama y tamaño de trama
 * @param uart uart usada en la instancia
 * @param state MEF de función de recepción de datos
 * @param frame_active flag que indica si se encuentra activo el procesamiento en recepción de datos
 * @param buff_ind posición de íncice de recepción
 * @param crc variable para calculo de crc en recepción
 */
typedef struct {
    frame_buffer_handler_t buffer_handler;
    frame_t raw_frame;
    uartMap_t uart;
    frame_capture_state_t state;
    bool_t frame_active;
    uint8_t buff_ind;
    uint8_t crc;
} frame_capture_t;

/*=====[Definición de variables privadas]====================================*/

/*=====[Declaración de prototipos de funciones privadas]======================*/
/**
 * @brief Inicializa la función de captura configurando los parámetros necesarios
 * a las funciones de la capa 1 que maneja la comunicación UART.
 * @note Se utiliza STATIC_FORCEINLINE para mejorar el rendimiento evitando saltos en la
 * ejecución de las instrucciones.
 * 
 * @param UARTRxCallBackFunc Función callback ISR.
 * @param parameter Puntero a la estructura de contexto de la instancia. 
 */
__STATIC_FORCEINLINE void C2_FRAME_CAPTURE_UartRxInit(void *UARTRxCallBackFunc, void *parameter);

/**
 * @brief Función que se llama en contexto de interrupción UART Rx, es llamada cuando se recibe
 * un caracter. La acción de procesamiento quedará determinada por la MEF.
 * 
 * @param parameter frame_buffer_handler_t* con QueueHandle_t y un QMPool* incializado
 */
static void C2_FRAME_CAPTURE_UartRxISR(void *parameter);

/**
 * @brief Función para validar CRC. Valida que sea un caracter ASCII que represente
 * hexadecimal y luego compara el CRC calculado con el recibido.
 * @note Se utiliza STATIC_FORCEINLINE para mejorar el rendimiento evitando saltos en la
 * ejecución de las instrucciones. 
 * 
 * @param frame Recibe el puntero a la posición del primer caracter del CRC
 * @param crc   Recibe el CRC calculado
 */ 
__STATIC_FORCEINLINE bool_t C2_FRAME_CAPTURE_CheckCRC(frame_t frame, uint8_t crc);

/**
 * @brief Función que convierte caracteres Ascii que representan Hexadecimales a su
 * valor respectivo de tipo int.
 * @note Se utiliza STATIC_FORCEINLINE para mejorar el rendimiento evitando saltos en la
 * ejecución de las instrucciones.
 * 
 * @param ascii Recibe el puntero a la posición del primer caracter del ascii
 * @param n     Cantidad de valores a ser convertidos
 */ 
__STATIC_FORCEINLINE uint8_t C2_FRAME_CAPTURE_AsciiHexaToInt(char *ascii, uint8_t n);

/*=====[Implementación de funciones públicas]=================================*/

QueueHandle_t C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart) {
    frame_capture_t *frame_capture = pvPortMalloc(sizeof(frame_capture_t)); //se libera luego de la llamada a la función en C2_FRAME_PACKER_Receive
    configASSERT(frame_capture != NULL);
    // inicialización del objeto
    frame_capture->buff_ind = 0;    
    frame_capture->crc = 0;
    frame_capture->frame_active = FALSE;
    frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
    frame_capture->buffer_handler.queue_receive = xQueueCreate(QUEUE_SIZE, sizeof(frame_t));
    configASSERT(frame_capture->buffer_handler.queue_receive != NULL);
    frame_capture->buffer_handler.pool = pool;
    frame_capture->uart = uart;
    C2_FRAME_CAPTURE_UartRxInit(C2_FRAME_CAPTURE_UartRxISR, (void *) frame_capture);
    return (QueueHandle_t) frame_capture->buffer_handler.queue_receive; // se devuelve cargado en el contexto el puntero al pool y la cola
}

/*=====[Implementación de funciones privadas]================================*/

__STATIC_FORCEINLINE void C2_FRAME_CAPTURE_UartRxInit(void *UARTRxCallBackFunc, void *parameter) {
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;
    uartConfig(frame_capture->uart, 115200);
    uartCallbackSet(frame_capture->uart, UART_RECEIVE, UARTRxCallBackFunc, parameter); // Función de capa 1 (SAPI) de atención a la interrupción. 
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

/*=====[Implementación de funciones de interrupción]==============================*/

static void C2_FRAME_CAPTURE_UartRxISR(void *parameter) {
    frame_capture_t *frame_capture = (frame_capture_t *) parameter;

    while (uartRxReady(frame_capture->uart)) {
        bool_t error = FALSE;
        BaseType_t px_higher_priority_task_woken = pdFALSE;

        char character = uartRxRead(frame_capture->uart); //Lee el caracter de la UART (función de la capa 1)

        switch (character) {
            case START_OF_MESSAGE:
                if (!frame_capture->frame_active) {
                    UBaseType_t uxSavedInterruptStatus;
                    uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
                    frame_capture->raw_frame.data = (char *) QMPool_get(frame_capture->buffer_handler.pool, 0);
                    taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
                } // si frame_active = 1 se estaba procesando dato válido, pero con el SOM se reinicia sobre el mismo pool

                if (frame_capture->raw_frame.data != NULL) {
                    frame_capture->state = FRAME_CAPTURE_STATE_ID_CHECK;
                    frame_capture->frame_active = TRUE;
                    frame_capture->buff_ind = 0;
                    frame_capture->crc = 0;
                }
                break;

            case END_OF_MESSAGE:
                if (frame_capture->frame_active) { // solo se procesa si se encontraba procesando datos válidos
                    error = TRUE;
                    if (frame_capture->buff_ind >= FRAME_MIN_SIZE) {  // se previene un EOM en ID o sin CRC
                        frame_capture->raw_frame.data_size = frame_capture->buff_ind - CHARACTER_SIZE_CRC; // Se toma el tamaño de datos sin CRC
                        if (C2_FRAME_CAPTURE_CheckCRC(frame_capture->raw_frame, frame_capture->crc)) {
                            if (frame_capture->buffer_handler.queue_receive != NULL) {
                                frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
                                frame_capture->frame_active = FALSE;
                                if (xQueueSendFromISR(frame_capture->buffer_handler.queue_receive, &frame_capture->raw_frame, &px_higher_priority_task_woken) == pdTRUE) {
                                    // Se envía la cola de FRAME_PACKER para ser empaquetado y enviado a la capa 3
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

            default:                                        //en caso de no ser un SOM o un EOM se implementa una MEF
                switch (frame_capture->state) {
                    case FRAME_CAPTURE_STATE_IDLE:          // se ignoran los datos.
                        break;

                    case FRAME_CAPTURE_STATE_ID_CHECK:      // se capturan y validan los datos ID y se comienza a calcular el CRC      
                        if (CHECK_HEXA(character)) {
                            frame_capture->raw_frame.data[frame_capture->buff_ind++] = character;
                            if (frame_capture->buff_ind == CHARACTER_SIZE_ID) {
                                frame_capture->crc = crc8_calc(frame_capture->crc, frame_capture->raw_frame.data, CHARACTER_SIZE_ID - CHARACTER_SIZE_CRC);
                                frame_capture->state = FRAME_CAPTURE_STATE_FRAME;
                            }
                        }
                        else {
                            error = TRUE;
                        }
                        break;

                    case FRAME_CAPTURE_STATE_FRAME:         // se capturan datos, calcula crc y chequea que no se supere el MAX_BUFFER_SIZE
                        frame_capture->raw_frame.data[frame_capture->buff_ind] = character;
                        frame_capture->crc = crc8_calc(frame_capture->crc, &frame_capture->raw_frame.data[frame_capture->buff_ind - CHARACTER_SIZE_CRC], sizeof(char));
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
            QMPool_put(frame_capture->buffer_handler.pool, (void *) frame_capture->raw_frame.data); // Ante un error en adquisición libero pool
            frame_capture->state = FRAME_CAPTURE_STATE_IDLE;
            frame_capture->frame_active = FALSE;
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
        }
    }
}
