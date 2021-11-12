/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "frame_processor.h"


/*=====[Implementations of public functions]=================================*/

int main(void) {
    boardConfig();

    C3_FRAME_PROCESSOR_Init(UART_USB); //Init an instance of the frame processor on UART_USB
    C3_FRAME_PROCESSOR_Init(UART_GPIO); //Init an instance of the frame processor on UART_GPIO

    vTaskStartScheduler();
    return 0;
}
