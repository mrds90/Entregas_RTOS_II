/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __FRAME_PACKER_H__
#define __FRAME_PACKER_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "sapi.h"
#include "qmpool.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "frame_class.h"

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/

/*=====[ Definitions of public data types ]==================================*/

/*=====[Prototypes (declarations) of public functions]=======================*/
/**
 * @brief Initialize the frame packer printer
 * 
 * @param app_buffer_handler_send 
 */
void FRAME_PACKER_PrinterInit(frame_buffer_handler_t *app_buffer_handler_send);
/**
 * @brief Initialize the frame packer receiver
 * 
 * @param app_buffer_handler_receive 
 */
void FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *app_buffer_handler_receive, uartMap_t uart);


/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_PACKER_H__ */
