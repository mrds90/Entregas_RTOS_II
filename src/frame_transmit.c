/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.2
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "frame_transmit.h"
#include "string.h"
#include "task.h"
#include <stdio.h>
#include <ctype.h>

/*=====[Definición de macros de constantes privadas]==============================*/

#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) *sizeof(char))
/*=====[Definición de tipos de datos privados]===================================*/

/*=====[Definición de variables privadas]====================================*/

/*=====[Declaración de prototipos de funciones privadas]======================*/
/**
 * @brief Inicializa el contexto (puntero, estado, fcion de callback, UART) para
 * la transmisión de datos por ISR de la Tx de UART.
 *
 * @param UARTTxCallBackFunc puntero para pasar función de callback para atención de interrupción
 *
 * @param parameter puntero a estructura que pasa el contexto para procesar dato de salida.
 *
 * @param STATIC_FORCEINLINE para mejorar el rendimiento evitando saltos en la
 * ejecución de las instrucciones.
 */
__STATIC_FORCEINLINE void C2_FRAME_TRANSMIT_UartTxInit(void *UARTTxCallBackFunc, void *parameter);

/**
 * @brief Función de callback para atención de ISR para envío de dato procesado por UART. Utiliza
 * MEF para decisión a tomar en cada entrada a la función.
 *
 * @param taskParmPtr puntero a estructura de contexto para envio/impresión de información a
 * a través de UART Tx
 */
static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter);

/*=====[Implementación de funciones públicas]=================================*/


void C2_FRAME_TRANSMIT_InitTransmision(frame_transmit_t *frame_transmit) {
    frame_transmit->buff_ind = 0;
    frame_transmit->isr_printer_state = PRINT_FRAME;
    while (!uartTxReady(frame_transmit->uart)); //Espera a que se libere el buffer de transmisión para mandar el primer caracter
    uartTxWrite(frame_transmit->uart, START_OF_MESSAGE);
    C2_FRAME_TRANSMIT_UartTxInit(C2_FRAME_TRANSMIT_UartTxISR, (void *) frame_transmit);
    uartSetPendingInterrupt(frame_transmit->uart);
}

/*=====[Implementación de funciones privadas]================================*/
__STATIC_FORCEINLINE void C2_FRAME_TRANSMIT_UartTxInit(void *UARTTxCallBackFunc, void *parameter) {
    frame_transmit_t *printer_resources = (frame_transmit_t *) parameter;
    uartCallbackSet(printer_resources->uart, UART_TRANSMITER_FREE, UARTTxCallBackFunc, parameter); //función de capa 1 (SAPI) para inicializar interrupción UART Tx
}

/*=====[Implementación de funciones de interrupción]==============================*/

static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter) {
    frame_transmit_t *printer_resources = (frame_transmit_t *) parameter;

    while (uartTxReady(printer_resources->uart)) {                                                       //mientras espacio en buffer de transmisión
        if (*printer_resources->transmit_frame.data != '\0') {                                           //si hay dato a enviar
            uartTxWrite(printer_resources->uart, *printer_resources->transmit_frame.data);               //escribe en buffer de transmisión
            printer_resources->transmit_frame.data++;                                                    //avanza puntero de dato a enviar
        }
        else {
            uartTxWrite(printer_resources->uart, END_OF_MESSAGE);                                        //escribe en buffer de transmisión el fin de mensaje
            uartCallbackClr(printer_resources->uart, UART_TRANSMITER_FREE);                              //desactiva interrupción UART Tx
            UBaseType_t uxSavedInterruptStatus;
            uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            printer_resources->transmit_frame.data -= (printer_resources->transmit_frame.data_size - 1); //restaura puntero de dato a enviar al inicio del mensaje
            QMPool_put(printer_resources->pool, printer_resources->transmit_frame.data);                 // Se libera el bloque del pool de memoria
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            break;
        };
    }
}
