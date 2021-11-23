/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 23/11/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_CAPTURE_H__
#define __FRAME_CAPTURE_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/

#include "frame_class.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definición de macros de constantes públicas]===============================*/

/*=====[Definición de tipos de datos públicos]==================================*/

/*=====[Declaración de prototipos de funciones públicas]=====================*/
/**
 * @brief Se inicializa el objeto. Se inicilizan índices, flag, se crea la cola
 * se le pasa puntero al buffer_handler, crea timer y se inicializan los estados.
 *
 * @param pool* puntero inicializado utilizado por QMPool
 * @param uart  uart utilizada en la instancia.
 */
frame_buffer_handler_t *C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart);

/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_CAPTURE_H__ */
