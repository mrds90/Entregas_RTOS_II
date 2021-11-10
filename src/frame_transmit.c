/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 10/11/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "FreeRTOS.h"
#include "frame_transmit.h"
#include "string.h"
#include "task.h"
#include <stdio.h>
#include <ctype.h>

/*=====[Definition macros of private constants]==============================*/

#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) *sizeof(char))
/*=====[Definitions of private data types]===================================*/

/*=====[Definitions of private variables]====================================*/

/*=====[Prototypes (declarations) of private functions]======================*/

static void C2_FRAME_TRANSMIT_UartTxInit(void *UARTTxCallBackFunc, void *parameter);

static void C2_FRAME_PACKER_UartTxISR(void *parameter);

/*=====[Implementations of public functions]=================================*/


void C2_FRAME_TRANSMIT_InitTransmision(frame_transmit_t *frame_transmit) {
    frame_transmit->buff_ind = 0;
    frame_transmit->isr_printer_state = START_FRAME;
    C2_FRAME_TRANSMIT_UartTxInit(C2_FRAME_PACKER_UartTxISR, (void *) frame_transmit);
    uartSetPendingInterrupt(frame_transmit->uart);
}

/*=====[Implementations of private functions]================================*/
static void C2_FRAME_TRANSMIT_UartTxInit(void *UARTTxCallBackFunc, void *parameter) {
    frame_transmit_t *printer_resources = (frame_transmit_t *) parameter;
    uartCallbackSet(printer_resources->uart, UART_TRANSMITER_FREE, UARTTxCallBackFunc, parameter);
}

/*=====[Implementations of interrupt functions]==============================*/

static void C2_FRAME_PACKER_UartTxISR(void *parameter) {
    frame_transmit_t *printer_resources = (frame_transmit_t *) parameter;

    gpioWrite(LED1, ON);       // Para Debug

    switch (printer_resources->isr_printer_state) {
        case START_FRAME:                                           // Se imprime el caracter de comienzo de paquete
            if (uartTxReady(printer_resources->uart) && (0 == printer_resources->buff_ind)) {
                uartTxWrite(printer_resources->uart, START_OF_MESSAGE);
                printer_resources->isr_printer_state = PRINT_FRAME;
            }
            break;

        case PRINT_FRAME:
            while (printer_resources->buff_ind < printer_resources->transmit_frame.data_size - 1) { // Se imprime el contenido del paquete
                if (uartTxReady(printer_resources->uart)) {
                    uartTxWrite(printer_resources->uart, printer_resources->transmit_frame.data[printer_resources->buff_ind]);
                    printer_resources->buff_ind++;
                }
                else break;
            }
            if (printer_resources->buff_ind == printer_resources->transmit_frame.data_size - 1) {  // Si es el ultimo caracter del paquete se manda a imprimir el fin de paquete
                printer_resources->isr_printer_state = LAST_FRAME_CHAR;
            }
            break;

        case LAST_FRAME_CHAR:                                       // Se imprime el caracter de fin de paquete
            if (uartTxReady(printer_resources->uart)) {
                uartTxWrite(printer_resources->uart, END_OF_MESSAGE);
                printer_resources->isr_printer_state = END_OF_FRAME;
            }
            break;

        case END_OF_FRAME:                                          // Se deshalibilita la interrupcion de Tx de la Uart y se libera el bloque del pool de memoria
            uartCallbackClr(printer_resources->uart, UART_TRANSMITER_FREE);
            UBaseType_t uxSavedInterruptStatus;
            uxSavedInterruptStatus = taskENTER_CRITICAL_FROM_ISR();
            QMPool_put(printer_resources->pool, printer_resources->transmit_frame.data - CHARACTER_BEFORE_DATA_SIZE); //< Se libera el bloque del pool de memoria
            taskEXIT_CRITICAL_FROM_ISR(uxSavedInterruptStatus);
            break;

        default:
            break;
    }

    gpioWrite(LED1, OFF);       // Para Debug
}
