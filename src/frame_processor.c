/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 23/11/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "frame_processor.h"
#include "frame_packer.h"

/*=====[Definición de macros de constantes privadas]==============================*/

#define POOL_PACKET_SIZE      MAX_BUFFER_SIZE
#define POOL_PACKET_COUNT     (QUEUE_SIZE)
#define POOL_SIZE_BYTES       (POOL_PACKET_SIZE * POOL_PACKET_COUNT * sizeof(char))
#define INVALID_UART_ID       (-1)
/*=====[Definición de tipos de datos privados]===================================*/

/*=====[Definición de variables globales publicas externas]=====================*/

/*=====[Definición de variables globales públicas]==============================*/

/*=====[Definición de variables globales privadas]=============================*/

/*=====[Declaración de prototipos de funciones privadas]======================*/
/**
 * @brief función de capa 3 que envía contexto para inicializar Objeto de la instancia.
 * Recibe las tramas del packer de capa 2, los procesa y envía a capa 2 para transmitirlos.
 *
 * @param taskParmPtr Se envia a tarea puntero a pool de memoria y uart de instancia. 
 */
static void C3_FRAME_PROCESSOR_Task(void *taskParmPtr);

/*=====[Implementación de funciones públicas]=================================*/

bool_t C3_FRAME_PROCESSOR_Init(uartMap_t uart) {
    bool_t ret;
    static bool_t uart_used[UART_MAXNUM] = {FALSE};
    if (uart_used[uart]) { 
        ret = FALSE;
    }
    else {
        ret = TRUE;
        uart_used[uart] = TRUE;
        BaseType_t xReturned = xTaskCreate(
            C3_FRAME_PROCESSOR_Task,
            (const char *)"Frame Processor",
            configMINIMAL_STACK_SIZE * 3,
            (void *) uart,
            tskIDLE_PRIORITY + 1,
            NULL
            );
        configASSERT(xReturned == pdPASS);
    }
    return ret;
}

/*=====[Implementación de funciones privadas]================================*/

static void C3_FRAME_PROCESSOR_Task(void *taskParmPtr) {
    uint8_t *memory_pool = (char *)pvPortMalloc(POOL_SIZE_BYTES);
    configASSERT(memory_pool != NULL);
    QMPool pool;
    frame_class_t frame_obj;
    frame_obj.buffer_handler.pool = &pool;
    frame_obj.buffer_handler.queue = NULL,
    frame_obj.frame.data = NULL;
    frame_obj.frame.data_size = 0;
    frame_obj.uart = (uartMap_t) taskParmPtr;
    QMPool_init(&pool, (uint8_t *) memory_pool, POOL_SIZE_BYTES * sizeof(char), POOL_PACKET_SIZE);

    C2_FRAME_PACKER_Init(&frame_obj.buffer_handler, frame_obj.uart); // Se inicializa el objeto de la instancia

    while (TRUE) {
        C2_FRAME_PACKER_Receive(&frame_obj.frame, &frame_obj.buffer_handler);

        // Aquí se procesará la trama según el comando...
        
        C2_FRAME_PACKER_Print(&frame_obj);
    }
}

/*=====[Implementación de funciones de interrupción]==============================*/
