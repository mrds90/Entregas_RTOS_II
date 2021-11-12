/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Evita la inclusión múltiple - comienzo]==============================*/

#ifndef __FRAME_TRANSMIT_H__
#define __FRAME_TRANSMIT_H__

/*=====[Inclusión de dependencias de funciones públicas]=====================*/

#include "frame_class.h"

/*=====[C++ - comienzo]======================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Declaración de prototipos de funciones públicas]=====================*/


/*=====[Definición de tipos de datos públicos]===============================*/
/**
 * @brief Estados de MEF para función de callback de ISR que se encarga de enviar datos por Tx
 */
typedef enum {
    START_FRAME,
    PRINT_FRAME,
    LAST_FRAME_CHAR,
    END_OF_FRAME,
} isr_printer_state_t;

/**
 * @brief Recurso para envío de contexto a función que se encarga de enviar por Tx los datos procesados.
 */
typedef struct {
    QMPool *pool;
    frame_t transmit_frame;
    uartMap_t uart;
    uint8_t buff_ind;
    isr_printer_state_t isr_printer_state;
} frame_transmit_t;

/*=====[Declaración de prototipos de funciones públicas]=====================*/


/*=====[Declaración de prototipos de funciones publicas de interrupción]=====*/
/**
 * @brief Inicia una transmision de datos con interrupciones
 *
 * @param frame_transmit_t* puntero a la estructura con los recursos para 
 * imprimir. uart, pool y frame deben estar inicializados
 */
void C2_FRAME_TRANSMIT_InitTransmision(frame_transmit_t *frame_transmit);

/*=====[C++ - fin]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Evita la inclusión múltiple - fin]===================================*/

#endif /* __FRAME_TRANSMIT_H__ */
