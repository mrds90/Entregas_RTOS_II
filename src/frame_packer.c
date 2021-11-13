/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 11/11/2021
 * Version: 1.2
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/
#include "FreeRTOS.h"
#include "task.h"

#include "frame_packer.h"
#include "frame_capture.h"
#include "frame_transmit.h"

#include "string.h"
#include "crc8.h"

/*=====[Definición de macros de constantes privadas]==========================*/
#define CHARACTER_INDEX_ID               0
#define CHARACTER_INDEX_CMD              (CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_INDEX_DATA             (CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) *sizeof(uint8_t))
#define CHARACTER_END_OF_PACKAGE         '\0'
#define PRINT_FRAME_SIZE(size)           ((size) + (CHARACTER_INDEX_DATA + CHARACTER_SIZE_CRC) * sizeof(uint8_t))
#define FAKE_CRC                         "1B"
/*=====[Definición de tipos de datos privados]================================*/
/**
 * @brief Recurso usado para pasar el contexto a las tareas de recepción y envío de datos.
 */
typedef struct {
    frame_buffer_handler_t *buffer_handler;
    uartMap_t uart;
} frame_packer_resources_t;


/*=====[Definición de variables privadas]====================================*/

/*=====[Declaración de prototipos de funciones privadas]======================*/
/**
 * @brief Tarea que recibe y procesa los datos recibidos desde C3 a través
 * de la cola, para ser impresos/enviados por la función de callback de
 * la ISR de Tx.
 *
 * @param taskParmPtr Estructura del tipo frame_packer_resources_t.
 */
static void C2_FRAME_PACKER_PrinterTask(void *taskParmPtr);

/*=====[Implementación de funciones públicas]=================================*/

void C2_FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *buffer_handler, uartMap_t uart) {
    buffer_handler->queue = C2_FRAME_CAPTURE_ObjInit(buffer_handler->pool, uart)->queue;
}

void C2_FRAME_PACKER_ReceiverTask(frame_t *frame, frame_buffer_handler_t *buffer_handler) {
    xQueueReceive(buffer_handler->queue, frame, portMAX_DELAY); //Recibe luego de un EOM en frame_capture
    frame->data = &frame->data[CHARACTER_INDEX_CMD];
    frame->data_size = frame->data_size - CHARACTER_SIZE_ID;
    frame->data[frame->data_size] = CHARACTER_END_OF_PACKAGE;
}

void C2_FRAME_PACKER_PrinterInit(frame_buffer_handler_t *app_buffer_handler_send, uartMap_t uart) {
    frame_packer_resources_t *printer_resources = pvPortMalloc(sizeof(frame_packer_resources_t)); // se libera al iniciar la tarea C2_FRAME_PACKER_PrinterTask
    configASSERT(printer_resources != NULL);
    printer_resources->uart = uart;
    printer_resources->buffer_handler = app_buffer_handler_send;

    BaseType_t xReturned = xTaskCreate(
        C2_FRAME_PACKER_PrinterTask,
        (const char *)"Print Function",
        configMINIMAL_STACK_SIZE * 4,
        (void *)printer_resources,
        tskIDLE_PRIORITY + 1,
        NULL
        );

    configASSERT(xReturned == pdPASS);
}

/*=====[Implementación de funciones privadas]================================*/

static void C2_FRAME_PACKER_PrinterTask(void *taskParmPtr) {
    frame_packer_resources_t *printer_resources = (frame_packer_resources_t *) taskParmPtr;
    frame_buffer_handler_t *buffer_handler_print = printer_resources->buffer_handler;
    frame_transmit_t printer_isr;
    printer_isr.pool = buffer_handler_print->pool;
    printer_isr.uart = printer_resources->uart;;

    vPortFree(printer_resources);


    // ----- Se repite tarea por siempre -------------------------
    while (TRUE) {
        // Se espera a que llegue el paquete procesado
        xQueueReceive(buffer_handler_print->queue, &printer_isr.transmit_frame.data, portMAX_DELAY);
        // Se calcula el CRC del paquete procesado
        uint8_t crc = crc8_calc(0, printer_isr.transmit_frame.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size) - CHARACTER_SIZE_CRC - 1);
        // Se arma el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
        snprintf(printer_isr.transmit_frame.data, PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size), "%s%2X", printer_isr.transmit_frame.data - CHARACTER_SIZE_ID * sizeof(char), crc);
        printer_isr.transmit_frame.data_size = PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size);

        // Se habilita la interrupcion para enviar el paquete a la capa de transmision C1
        C2_FRAME_TRANSMIT_InitTransmision(&printer_isr);
    }
}

/*=====[Implementación de funciones de interrupción]==============================*/
