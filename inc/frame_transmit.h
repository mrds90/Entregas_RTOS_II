/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 10/11/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __FRAME_TRANSMIT_H__
#define __FRAME_TRANSMIT_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "frame_class.h"

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/


/*=====[ Definitions of public data types ]==================================*/
typedef enum {
    START_FRAME,
    PRINT_FRAME,
    LAST_FRAME_CHAR,
    END_OF_FRAME,
} isr_printer_state_t;

typedef struct {
    QMPool *pool;
    frame_t transmit_frame;
    uartMap_t uart;
    uint8_t buff_ind;
    isr_printer_state_t isr_printer_state;    
} frame_transmit_t;

/*=====[Prototypes (declarations) of public functions]=======================*/
/**
 * @brief Inicia una transmision de datos con interrupciones
 *
 *
 * @param frame_transmit_t* puntero a la estructura con los recursos para imprimir. uart, pool y frame deben estar inicializados
 */
// void C2_FRAME_TRANSMIT_InitTransmision(frame_transmit_t *frame_transmit);

/*=====[Prototypes (declarations) of public interrupt functions]=============*/

void C2_FRAME_TRANSMIT_InitTransmision(frame_transmit_t *frame_transmit);

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __FRAME_TRANSMIT_H__ */
