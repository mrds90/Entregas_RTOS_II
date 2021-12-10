/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 03/12/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_CLASS_H__
#define __FRAME_CLASS_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/

#include "sapi.h"
#include "qmpool.h"
#include "queue.h"
#include "semphr.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definiciones de macros de constantes publicas]=======================*/
#define CHARACTER_SIZE_ID           4
#define CHARACTER_SIZE_CRC          2
#define CHARACTER_SIZE_CMD          1
#define CHARACTER_SIZE_SOM          1
#define QUEUE_SIZE                  10
#define START_OF_MESSAGE            '('
#define END_OF_MESSAGE              ')'
#define CHARACTER_END_OF_PACKAGE    '\0'

#define WORD_MAX_SIZE               10
#define WORD_MAX_QTY                15
#define WORD_MIN_QTY                1
#define UNDERSCORE_MAX_QTY      WORD_MAX_QTY // no se puede al final pero si al principio segun los requerimientos. por lo que puede haber un guion por palabra
#if ((CHARACTER_SIZE_SOM + WORD_MAX_SIZE * WORD_MAX_QTY + CHARACTER_SIZE_ID + CHARACTER_SIZE_CMD + UNDERSCORE_MAX_QTY + CHARACTER_SIZE_CRC) > 200)
    #define MAX_BUFFER_SIZE         (CHARACTER_SIZE_SOM + WORD_MAX_SIZE * WORD_MAX_QTY + CHARACTER_SIZE_ID + CHARACTER_SIZE_CMD + UNDERSCORE_MAX_QTY + CHARACTER_SIZE_CRC)
#else
    #define MAX_BUFFER_SIZE         200
#endif
/*=====[Definiciones de macros de constantes publicas]=======================*/


typedef enum {
    EVENT_RECEIVE,
    EVENT_TRANSMIT,

    EVENT_QTY,
} event_t;

/**
 * @brief Contiene atributos de la instancia de la trama. Puntero de indice y tamaño
 */
typedef struct {
    uint8_t data_size;
    char *data;
    event_t event;
    QueueHandle_t send_queue;
} frame_t;
/**
 * @brief Estructura para manejar la trama. Puntero para funciones de pool, cola y
 * semaforo para espera en envío
 */
typedef struct {
    QMPool *pool;
    QueueHandle_t queue_receive;
    QueueHandle_t queue_transmit;
    SemaphoreHandle_t semaphore;
    uartMap_t uart;
} frame_buffer_handler_t;

/**
 * @brief Tipo de dato con el contexto completo de la trama.
 * @param frame_t -> Puntero(índice) y tamaño
 * @param frame_buffer_handler -> Puntero al pool, Cola y semáforo de transmit
 * @param uartMap_t -> uart de transmisión de la instancia
 */
typedef struct {
    frame_buffer_handler_t buffer_handler;
    frame_t frame;
} frame_class_t;
/*=====[Declaración de prototipos de funciones públicas]=====================*/

/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_CLASS_H__ */
