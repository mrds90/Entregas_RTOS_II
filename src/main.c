/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 19/11/2021
 * Version: 1.3
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "frame_processor.h"


/*=====[Implementacion de funciones públicas]=================================*/

int main(void) {
    boardConfig();

    C3_FRAME_PROCESSOR_Init(UART_USB); //Iniciar una instancia de frame_processor en UART_USB
    //C3_FRAME_PROCESSOR_Init(UART_GPIO); //Iniciar una instancia de frame processor en UART_GPIO

    vTaskStartScheduler();
    return 0;
}
