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

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/
#define MAX_BUFFER_SIZE				200

/*=====[ Definitions of public data types ]==================================*/

typedef enum {
	FRAME_WAITING,
	FRAME_CRC_CHECK,
	FRAME_PROSESSING,
	FRAME_COMPLETE,
	FRAME_STATE_QTY
} frame_state_t;

typedef struct {
	uint8_t *data;
	uint8_t data_size;
} frame_t;

typedef struct {
	QMPool *pool;
	QueueHandle_t queue;
} buffer_handler_t;

typedef struct {
	buffer_handler_t *buffer_handler;
	uartMap_t uart;
} frame_packer_resources_t;
/*=====[Prototypes (declarations) of public functions]=======================*/
BaseType_t FramePackerInit(buffer_handler_t *app_buffer_handler_receive, uartMap_t uart);
void TASK_FramePrinter(void* taskParmPtr);

/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_PACKER_H__ */
