/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 2021/10/24
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

/*=====[Inclusions of private function dependencies]=========================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "frame_processor.h"

/*=====[Definition macros of private constants]==============================*/

/*=====[Private function-like macros]========================================*/

/*=====[Definitions of private data types]===================================*/

/*=====[Definitions of external public global variables]=====================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/

/*=====[Implementations of public functions]=================================*/

int main(void)
{
   boardConfig();

   C3_FRAME_PROCESSOR_Init(UART_USB);  //Init an instance of the frame processor on UART_USB
   C3_FRAME_PROCESSOR_Init(UART_GPIO); //Init an instance of the frame processor on UART_GPIO

   vTaskStartScheduler();
   return 0;
}


