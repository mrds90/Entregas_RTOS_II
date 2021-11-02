/*=============================================================================
 * Author: Marcos Dominguez <mrds0690@gmail.com>
 * Date: 2021/10/31
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/

#include "frame_processor.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "qmpool.h"
#include "frame_packer.h"

 
/*=====[Inclusions of private function dependencies]=========================*/

/*=====[Definition macros of private constants]==============================*/
#define POOL_PACKET_SIZE      MAX_BUFFER_SIZE
#define POOL_PACKET_COUNT     (5)
#define POOL_SIZE_BYTES       (POOL_PACKET_SIZE * POOL_PACKET_COUNT)
#define QUEUE_SIZE            10
/*=====[Private function-like macros]========================================*/

/*=====[Definitions of private data types]===================================*/

/*=====[Definitions of external public global variables]=====================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/

/*=====[Implementations of public functions]=================================*/

void FrameProcessorInit() {
static uint8_t memory_pool[POOL_SIZE_BYTES];
   BaseType_t xReturned = xTaskCreate(
      TASK_FrameProcessor,
      (const char *)"Frame Processor",
      configMINIMAL_STACK_SIZE,
      (void*) memory_pool,
      tskIDLE_PRIORITY + 1,
      NULL
   );
   configASSERT(xReturned == pdPASS);
}

// Task implementation
void TASK_FrameProcessor( void* taskParmPtr ) {
   uint8_t *memory_pool = (uint8_t*) taskParmPtr;
   static QMPool pool;

   static buffer_handler_t app_buffer_handler_receive = {
      .queue = NULL,
      .pool = &pool,
   };

   static buffer_handler_t app_buffer_handler_send = {
      .queue = NULL,
      .pool = &pool,
   };

   QMPool_init( &pool, (uint8_t*) memory_pool, POOL_SIZE_BYTES , POOL_PACKET_SIZE);
   if ( app_buffer_handler_receive.queue == NULL ) {
      app_buffer_handler_receive.queue = xQueueCreate( QUEUE_SIZE, sizeof( frame_t ) );
   }
   configASSERT( app_buffer_handler_receive.queue != NULL );

   if ( app_buffer_handler_send.queue == NULL ) {
      app_buffer_handler_send.queue = xQueueCreate( QUEUE_SIZE, sizeof( frame_t ) );
   }
   configASSERT( app_buffer_handler_send.queue != NULL );

   BaseType_t xReturned = xTaskCreate(
      TASK_FramePrinter,
      (const char *)"Print Function",
      configMINIMAL_STACK_SIZE * 4,
      (void *)&app_buffer_handler_send,
      tskIDLE_PRIORITY + 1,
      NULL
   );

   configASSERT( xReturned == pdPASS );

   xReturned = xTaskCreate(
      TASK_FramePacker,
      (const char *)"Frame Packer",
      configMINIMAL_STACK_SIZE * 5,
      (void *)&app_buffer_handler_receive,
      tskIDLE_PRIORITY + 1,
      NULL
   );

   configASSERT( xReturned == pdPASS );

   frame_t *frame;
   
   while( true ) {
      xQueueReceive(app_buffer_handler_receive.queue, &frame, portMAX_DELAY);
      // Do something with the frame

      xQueueSend(app_buffer_handler_send.queue, &frame, portMAX_DELAY);
   }

}

/*=====[Implementations of interrupt functions]==============================*/

/*=====[Implementations of private functions]================================*/

