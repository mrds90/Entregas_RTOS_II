/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 03/12/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "frame_processor.h"

#define ERROR_APP_CREATE    "ERROR: No se pudo instanciar la aplicacion!\n"

/*=====[Implementacion de funciones públicas]=================================*/

int main(void) {
    boardConfig();

    
    if( !C3_FRAME_PROCESSOR_Init(UART_USB) ) { //Iniciar una instancia de frame_processor en UART_USB
        printf(ERROR_APP_CREATE);
    }
    // static app_t gpio_app = {
    //     .uart = UART_GPIO,
    // };
    //if ( !C3_FRAME_PROCESSOR_Init(&gpio_app) ) { //Iniciar una instancia de frame processor en UART_GPIO
    //     printf("ERROR: No se pudo instanciar la aplicacion!\n");
    // }
    
    vTaskStartScheduler();
    return 0;
}
