/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 03/12/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_PACKER_H__
#define __FRAME_PACKER_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/

#include "frame_class.h"
#include "AO.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif



/*=====[Declaración de prototipos de funciones públicas]=====================*/

/**
 * @brief Encola en el buffer_handler un puntero a los datos inicializados de la instancia.
 *
  */
bool_t C2_FRAME_PACKER_Init(activeObject_t *ao_obj, uartMap_t uart);



/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_PACKER_H__ */
