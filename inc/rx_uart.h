/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __RX_UART_H__
#define __RX_UART_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "sapi.h"

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/

/*=====[ Definitions of public data types ]==================================*/

typedef enum {
	FRAME_WAITING,
	FRAME_PROSESSING,
	FRAME_COMPLETE,

	FRAME_STATE_QTY
} frame_state_t;
/*=====[Prototypes (declarations) of public functions]=======================*/

void uart_Init( uartMap_t uart );
void checkPckgInit( char sDelimiter, char eDelimiter );
uint8_t checkPckg( char* pckg );
void cleanBuffer( void );
void onRx( void *noUsado );

/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __RX_UART_H__ */
