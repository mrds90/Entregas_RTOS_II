/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 19/11/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "semphr.h"

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
 * @brief Función de callback para atención de ISR para envío de dato procesado por UART.
 *
 * @param taskParmPtr puntero a estructura de contexto para envio/impresión de información a
 * a través de UART Tx
 */
static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter);


/*=====[Implementación de funciones públicas]=================================*/
void C2_FRAME_TRANSMIT_ObjInit(frame_buffer_handler_t *buffer_handler) {
    buffer_handler->semaphore = xSemaphoreCreateBinary();// Se crea el semaforo para evitar que se empiece a transmitir un paquete antes de que se termine de transmitir el anterior
    configASSERT(buffer_handler->semaphore != NULL);
}

void C2_FRAME_TRANSMIT_InitTransmision(frame_class_t *frame_obj) {
    uartCallbackSet(frame_obj->uart, UART_TRANSMITER_FREE, C2_FRAME_TRANSMIT_UartTxISR, (void *) frame_obj); //función de capa 1 (SAPI) para inicializar interrupción UART Tx
    uartSetPendingInterrupt(frame_obj->uart);
    xSemaphoreTake(frame_obj->buffer_handler.semaphore, portMAX_DELAY);             // No avanza si esta enviando un paquete
}

/*=====[Implementación de funciones privadas]================================*/

/*=====[Implementación de funciones de interrupción]==============================*/

static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter) {
    frame_class_t *frame_obj = (frame_class_t *) parameter;

    while (uartTxReady(frame_obj->uart)) {                                          // Mientras haya espacio en el buffer de transmisión
        BaseType_t px_higher_priority_task_woken = pdFALSE;

        if (*frame_obj->frame.data != CHARACTER_END_OF_PACKAGE) {                   // Si no se ha terminado de imprimir el paquete
            uartTxWrite(frame_obj->uart, *frame_obj->frame.data);                   // Imprime el caracter
            frame_obj->frame.data++;                                                // Avanza puntero al siguiente caracter
        }
        else {                                                                      // Si se ha terminado de imprimir el paquete
            uartTxWrite(frame_obj->uart, END_OF_MESSAGE);                           // Imprime el caracter de fin de paquete
            uartCallbackClr(frame_obj->uart, UART_TRANSMITER_FREE);                 // Desactiva interrupción UART Tx
            UBaseType_t uxSavedInterruptStatus;
            uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            frame_obj->frame.data -= (frame_obj->frame.data_size - 1);              // Retrocede puntero al inicio del paquete
            QMPool_put(frame_obj->buffer_handler.pool, frame_obj->frame.data);      // Se libera el bloque del pool de memoria
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            xSemaphoreGiveFromISR(frame_obj->buffer_handler.semaphore, &px_higher_priority_task_woken);     // Se libera el semaforo para permitir que se transmita otro paquete
            if (px_higher_priority_task_woken == pdTRUE) {
                portYIELD_FROM_ISR(px_higher_priority_task_woken);
            }
            break;
        }
    }
}
