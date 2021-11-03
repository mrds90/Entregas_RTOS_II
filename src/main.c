/*=============================================================================
 * Author: Marcos Dominguez <mrds0690@gmail.com>
 * Date: 2021/10/24
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

/*=====[Inclusions of private function dependencies]=========================*/
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "sapi.h"
#include "frame_processor.h"

/*=====[Definition macros of private constants]==============================*/
/*=====[Private function-like macros]========================================*/

/*=====[Definitions of private data types]===================================*/

/*=====[Definitions of external public global variables]=====================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/

/*=====[Implementations of public functions]=================================*/



int main( void )
{
   /* Inicializar la placa */
   boardConfig();

   FrameProcessorInit(UART_USB);

   /* arranco el scheduler */
   vTaskStartScheduler();
   return 0;
}


