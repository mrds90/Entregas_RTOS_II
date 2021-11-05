/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
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

/*=====[ Definitions of public data types ]==================================*/


typedef struct {
    frame_buffer_handler_t buffer_handler;
    frame_t raw_frame;
    uint8_t frame_active;
    uint8_t buff_ind;
} frame_capture_t;

/*=====[Prototypes (declarations) of public functions]=======================*/
/**
 * @brief Initializes the frame capture
 * 
 * @param frame_capture_t* 
 * @param frame_buffer_handler_t* 
 */
void *C2_FRAME_CAPTURE_ObjInit(QMPool *pool, uartMap_t uart);


/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_CAPTURE_H__ */
