/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "../inc/rx_uart.h"
#include "string.h"
#include "qmpool.h"


/*=====[Definition macros of private constants]==============================*/
#define MAX_BUFF				200
#define START_OF_MESSAGE 		'('
#define END_OF_MESSAGE 			')'
#define CHARACTER_SIZE_ID		4
#define CHARACTER_SIZE_CMD		1
#define CHARACTER_SIZE_CRC		2
/*=====[ Definitions of private data types ]===================================*/


typedef struct {
	frame_state_t state;
	uint8_t buffer[MAX_BUFF];
	uint8_t index;
} frame_t;
/*=====[Definitions of private global variables]=============================*/
static char RxBuff[MAX_BUFF] = {0};
static uint8_t buff_ind = 0;

/*=====[Prototypes (declarations) of private functions]======================*/

/*=====[Implementations of public functions]=================================*/

/**
   	@brief Funcion para inicializar la UART y sus interrupciones.
	@param UART por donde se recibiran los datos
 */
void uart_Init( uartMap_t uart ) {
   /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
   uartConfig(uart, 115200);
   // Seteo un callback al evento de recepcion y habilito su interrupcion
   uartCallbackSet(uart, UART_RECEIVE, onRx, NULL);
   // Habilito todas las interrupciones de UART_USB
   uartInterrupt(uart, true);
}

/**
   	@brief 	Detecta cuando hay un paquete valido en el buffer de recepcion y devuelve su longitud y contenido.
	@param 	Array en donde se almacenara el paquete
	@return La longitud del paquete de datos ingresado
 */
uint8_t checkPckg( char* pckg ) {
	char* startPointer;
	char* endPointer;
	uint8_t startPos, endPos, pckgLen = 0;

	startPointer = strchr(RxBuff, startDelimiter);  // Obtengo la posicion de memoria donde esta el caracter de comienzo de paquete
	endPointer = strchr(RxBuff, endDelimiter);		// Obtengo la posicion de memoria donde esta el caracter de fin de paquete

	if( startPointer != NULL && endPointer != NULL)
	{
		startPos = startPointer - RxBuff;  	// Obtengo la posicion dentro del buffer donde esta el caracter de comienzo de paquete
		endPos = endPointer - RxBuff;		// Obtengo la posicion dentro del buffer donde esta el caracter de fin de paquete

		if( endPos > startPos )
		{
			pckgLen = endPos - startPos - 1;
			if( pckgLen > 0 && pckgLen <= MAX_BUFF)  memcpy(pckg, startPointer+1, pckgLen);  // Almaceno el paquete en un array
			else cleanBuffer();
			return pckgLen;
		}
		else cleanBuffer();
	}

	return pckgLen;
}
void FrameCreator(frame_t *frame) {

	// queueRecived 
	switch (frame->state) {
		case FRAME_WAITING:
			if (*RxBuff == START_OF_MESSAGE) {
				frame->buffer[0] = *RxBuff; 
				frame->state = FRAME_PROSESSING;
			}
			break;
		case FRAME_PROSESSING:

   
			
			break;
		case FRAME_COMPLETE:

			break;
	}
}
/**
   	@brief Funcion para limpiar el buffer de recepcion.

 */
void cleanBuffer( void ) {
	uint8_t i;

	for(i = 0; i < MAX_BUFF; i++)
		RxBuff[i] = 0;

	buff_ind = 0;
}


/*=====[Implementations of interrupt functions]==============================*/

/**
   	@brief 	Callback para la recepcion por UART.

 */
void onRx( void *noUsado ) {
	static uint8_t frame_active = 0;
	char *ptr_msg; 
	char character;
	character = uartRxRead(UART_USB);
	if (character == START_OF_MESSAGE) {
		if(frame_active == 0) {
			// ptr_msg = (char*) QMPool_get(); //TODO Crear pool
		}
		if (ptr_msg != NULL) {
			buff_ind = 0;
			frame_active = 1;
		}
	}
	else if ((character == END_OF_MESSAGE) && frame_active) {
		frame_active = 0;
		//GIVE queue ptr_msg
	}
	else if (frame_active) {
		RxBuff[buff_ind++] = character;
	}
	else if (buff_ind >= MAX_BUFF) {
		buff_ind = 0;
		frame_active = 0;
		// put(ptr_msg)
	}

}




