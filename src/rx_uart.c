/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan <
pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.0
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "../inc/rx_uart.h"
#include "string.h"

/*=====[Definitions of private global variables]=============================*/
static char RxBuff[MAX_BUFF] = {0};
static uint8_t buff_ind = 0;
static char startDelimiter = '>';
static char endDelimiter = '<';

/*=====[Prototypes (declarations) of private functions]======================*/

/*=====[Implementations of public functions]=================================*/

/**
   	@brief Funcion para inicializar la UART y sus interrupciones.
	@param UART por donde se recibiran los datos
 */
void uart_Init( uartMap_t uart )
{
   /* Inicializar la UART_USB junto con las interrupciones de Tx y Rx */
   uartConfig(uart, 115200);
   // Seteo un callback al evento de recepcion y habilito su interrupcion
   uartCallbackSet(uart, UART_RECEIVE, onRx, NULL);
   // Habilito todas las interrupciones de UART_USB
   uartInterrupt(uart, true);
}

/**
   	@brief Funcion para inicializar los parametros para la deteccion de los paquetes.
	@param Caracter que determina el comienzo del paquete
	@param Caracter que determina el fin del paquete
 */
void checkPckgInit( char sDelimiter, char eDelimiter )
{
	startDelimiter = sDelimiter;
	endDelimiter = eDelimiter;
	cleanBuffer();
}

/**
   	@brief 	Detecta cuando hay un paquete valido en el buffer de recepcion y devuelve su longitud y contenido.
	@param 	Array en donde se almacenara el paquete
	@return La longitud del paquete de datos ingresado
 */
uint8_t checkPckg( char* pckg )
{
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

/**
   	@brief Funcion para limpiar el buffer de recepcion.

 */
void cleanBuffer( void )
{
	uint8_t i;

	for(i = 0; i < MAX_BUFF; i++)
		RxBuff[i] = 0;

	buff_ind = 0;
}


/*=====[Implementations of interrupt functions]==============================*/

/**
   	@brief 	Callback para la recepcion por UART.

 */
void onRx( void *noUsado )
{
	RxBuff[buff_ind] = uartRxRead( UART_USB );

	if(buff_ind < MAX_BUFF-1) buff_ind++;
	else buff_ind = 0;
}

