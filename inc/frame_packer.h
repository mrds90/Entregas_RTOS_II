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

/*=====[Declaración de prototipos de funciones públicas]=====================*/

/**
 * @brief Encola en el buffer_handler un puntero a los datos inicializados de la instancia.
 * 
 * @param app_buffer_handler_receive Manejador que contiene el contexto (cola, semaforo y puntero a pool) 
 * @param uart Uart por donde se recibirán los datos.
 */
void C2_FRAME_PACKER_Init(frame_buffer_handler_t *app_buffer_handler_receive, uartMap_t uart);

/**
 * @brief Recibe los datos de FRAME_CAPTURE y los empaqueta (enmascara el ID y saca el CRC) y los envia a la capa superior.
 *
 * @param frame          Puntero al objeto que cargara el frame recibido.
 * @param buffer_handler Buffer que contiene el contexto (cola y puntero a pool)
 */
void C2_FRAME_PACKER_Receive(frame_t *frame, frame_buffer_handler_t *buffer_handler) ;
/**
 * @brief Tarea que recibe y procesa los datos recibidos desde C3, le inserta el ID y CRC para ser impresos/enviados por la función de callback de
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
