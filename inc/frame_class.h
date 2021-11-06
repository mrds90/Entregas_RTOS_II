/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __FRAME_CLASS_H__
#define __FRAME_CLASS_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "sapi.h"
#include "qmpool.h"
#include "queue.h"

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/
#define MAX_BUFFER_SIZE         200
#define CHARACTER_SIZE_CRC      2
#define QUEUE_SIZE              7
/*=====[ Definitions of public data types ]==================================*/

typedef struct {
    char *data;
    uint8_t data_size;
} frame_t;

typedef struct {
    QMPool *pool;
    QueueHandle_t queue;
} frame_buffer_handler_t;

/*=====[Prototypes (declarations) of public functions]=======================*/

/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_CLASS_H__ */
