/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.2
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_TRANSMIT_H__
#define __FRAME_TRANSMIT_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/

#include "frame_class.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Declaración de prototipos de funciones públicas]=====================*/


/*=====[Definición de tipos de datos públicos]===============================*/

/*=====[Declaración de prototipos de funciones públicas]=====================*/


/*=====[Declaración de prototipos de funciones publicas de interrupción]=====*/
/**
 * @brief Inicia una transmision de datos con interrupciones
 *
 * @param frame_class_t* puntero a la estructura con los recursos para 
 * imprimir. uart, pool y frame deben estar inicializados
 */
void C2_FRAME_TRANSMIT_InitTransmision(frame_class_t *frame_obj);

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_TRANSMIT_H__ */
