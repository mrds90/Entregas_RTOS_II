/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.1
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
 * @brief Recibe el contexto necesario para inciar la tarea que envía los datos por Tx. Se envía el contexto 
 * 
 * @param app_buffer_handler_send  manejador que contiene el contexto (cola y puntero a pool)
 * @param uart  puerto con el que se inicializó la instancia que está imprimiendo los datos.
 */
void C2_FRAME_PACKER_PrinterInit(frame_buffer_handler_t *app_buffer_handler_send, uartMap_t uart);

/**
 * @brief Recibe el contexto necesario para  
 * 
 * @param app_buffer_handler_receive 
 * @param uart
 */
void C2_FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *app_buffer_handler_receive, uartMap_t uart);


/*=====[Declaración de prototipos de funciones publicas de interrupción]====*/

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_PACKER_H__ */
