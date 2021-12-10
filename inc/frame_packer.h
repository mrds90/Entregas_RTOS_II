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

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  event_t event;
  frame_t frame;
} data_t;

/*=====[Declaración de prototipos de funciones públicas]=====================*/

/**
 * @brief Encola en el buffer_handler un puntero a los datos inicializados de la instancia.
 *
  */
void C2_FRAME_PACKER_Init(frame_class_t *frame_obj, activeObject_t *ao_obj);

/**
 * @brief Función que recibe y procesa los datos recibidos desde C3, le inserta el ID y CRC para ser impresos/enviados por la función de callback de
 * la ISR de Tx.
 *
 * @param frame_obj Estructura del tipo frame_class_t.
 */

void C2_FRAME_PACKER_Print(frame_class_t *frame_obj);

/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_PACKER_H__ */
