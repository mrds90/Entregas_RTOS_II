/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __FRAME_CAPTURE_H__
#define __FRAME_CAPTURE_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "frame_class.h"

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/
#define START_OF_MESSAGE         '('
#define END_OF_MESSAGE           ')'
#define CHARACTER_SIZE_ID         4

/*=====[ Definitions of public data types ]==================================*/

/*=====[Prototypes (declarations) of public functions]=======================*/
/**
 * @brief Initializes the frame capture
 * 
 * @param pool* 
 * @param uart 
 */
void *C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart);


/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_CAPTURE_H__ */
