/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.2
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
#define CHARACTER_SIZE_ID       4
#define CHARACTER_SIZE_CRC      2
#define CHARACTER_SIZE_CMD      1
#define QUEUE_SIZE              7
#define START_OF_MESSAGE        '('
#define END_OF_MESSAGE          ')'

#define WORD_MAX_SIZE           10
#define WORD_MAX_QTY            15
#define UNDERSCORE_MAX_QTY      WORD_MAX_QTY // no se puede al final pero si al principio segun los requerimientos. por lo que puede haber un guion por palabra
#if ((WORD_MAX_SIZE * WORD_MAX_QTY + CHARACTER_SIZE_ID + CHARACTER_SIZE_CMD + UNDERSCORE_MAX_QTY + CHARACTER_SIZE_CRC) > 200)
    #define MAX_BUFFER_SIZE         (WORD_MAX_SIZE * WORD_MAX_QTY + CHARACTER_SIZE_ID + CHARACTER_SIZE_CMD + UNDERSCORE_MAX_QTY + CHARACTER_SIZE_CRC)
#else
    #define MAX_BUFFER_SIZE         200
#endif
/*=====[Definiciones de macros de constantes publicas]=======================*/
/**
 * @brief Estructura usada para atributos de la trama. Puntero-indice y tamaño
 */
typedef struct {
    char *data;
    uint8_t data_size;
} frame_t;

/**
 * @brief Estructura para manejar la trama. Puntero para funciones de pool y cola
 */
typedef struct {
    QMPool *pool;
    QueueHandle_t queue;
    SemaphoreHandle_t semaphore;
} frame_buffer_handler_t;

typedef struct {
    frame_t frame;
    frame_buffer_handler_t buffer_handler;
    uartMap_t uart;
} frame_class_t;
/*=====[Declaración de prototipos de funciones públicas]=====================*/

/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_CLASS_H__ */
