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
 * @note STATIC_FORCEINLINE para mejorar el rendimiento evitando saltos en la
 * ejecución de las instrucciones.
 * 
 * @param UARTTxCallBackFunc puntero para pasar función de callback para atención de interrupción
 *
 * @param parameter puntero a estructura que pasa el contexto para procesar dato de salida.
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


void C2_FRAME_TRANSMIT_InitTransmision(frame_class_t *frame_obj) {
    while (!uartTxReady(frame_obj->uart));                                         //Espera a que se libere el buffer de transmisión para mandar el primer caracter
    uartTxWrite(frame_obj->uart, START_OF_MESSAGE);                                //Envía el caracter de inicio de mensaje
    C2_FRAME_TRANSMIT_UartTxInit(C2_FRAME_TRANSMIT_UartTxISR, (void *) frame_obj); //Inicializa el contexto para la transmisión de datos por ISR de la Tx de UART
    uartSetPendingInterrupt(frame_obj->uart);
}

/*=====[Implementación de funciones privadas]================================*/
__STATIC_FORCEINLINE void C2_FRAME_TRANSMIT_UartTxInit(void *UARTTxCallBackFunc, void *parameter) {
    frame_class_t *frame_obj = (frame_class_t *) parameter;
    uartCallbackSet(frame_obj->uart, UART_TRANSMITER_FREE, UARTTxCallBackFunc, parameter); //función de capa 1 (SAPI) para inicializar interrupción UART Tx
}

/*=====[Implementación de funciones de interrupción]==============================*/

static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter) {
    frame_class_t *frame_obj = (frame_class_t *) parameter;

    while (uartTxReady(frame_obj->uart)) {                                          // Mientras haya espacio en el buffer de transmisión
        if (*frame_obj->frame.data != '\0') {                                       // Si no se ha terminado de imprimir el paquete
            uartTxWrite(frame_obj->uart, *frame_obj->frame.data);                   // Imprime el caracter
            frame_obj->frame.data++;                                                // Avanza puntero al siguiente caracter
        }
        else {
            uartTxWrite(frame_obj->uart, END_OF_MESSAGE);                           // Imprime el caracter de fin de paquete
            uartCallbackClr(frame_obj->uart, UART_TRANSMITER_FREE);                 // Desactiva interrupción UART Tx
            UBaseType_t uxSavedInterruptStatus;
            uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            frame_obj->frame.data -= (frame_obj->frame.data_size - 1);
            QMPool_put(frame_obj->buffer_handler.pool, frame_obj->frame.data);      // Se libera el bloque del pool de memoria
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            break;
        }
    }
}
