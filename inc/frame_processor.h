/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 19/11/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_PROCESSOR_H__
#define __FRAME_PROCESSOR_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/
#include "sapi.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Declaración de prototipos de funciones públicas]=====================*/
/**
 * @brief Se inicializa la tarea procesadora de tramas, reserva espacio para
 * pool de memoria y pasa el contexto a tarea.
 *
 * @param uart puerto de comunicación UART pasado desde int main para inicializar
 * la instancia
 */
bool_t C3_FRAME_PROCESSOR_Init(uartMap_t uart);

/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_PROCESSOR_H__ */
