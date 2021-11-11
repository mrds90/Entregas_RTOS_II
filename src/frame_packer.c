/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 31/10/2021
 * Version: 1.1
 *===========================================================================*/

/*=====[Inclusion of own header]=============================================*/
#include "FreeRTOS.h"
#include "task.h"

#include "frame_packer.h"
#include "frame_capture.h"
#include "frame_transmit.h"


#include "string.h"
#include "crc8.h"

/*=====[Definition macros of private constants]==============================*/
#define CHARACTER_INDEX_ID               0
#define CHARACTER_INDEX_CMD              (CHARACTER_INDEX_ID + CHARACTER_SIZE_ID)
#define CHARACTER_SIZE_CMD               1
#define CHARACTER_INDEX_DATA             (CHARACTER_INDEX_CMD + CHARACTER_SIZE_CMD)
#define CHARACTER_BEFORE_DATA_SIZE       ((CHARACTER_SIZE_ID) *sizeof(uint8_t))
#define PRINT_FRAME_SIZE(size)           ((size) + (CHARACTER_INDEX_DATA + CHARACTER_SIZE_CRC) * sizeof(uint8_t))
#define FAKE_CRC                         "1B"
/*=====[ Definitions of private data types ]===================================*/
/**
 * @brief Resourse used to store the frame to be packed
 *
 */
typedef struct {
    frame_buffer_handler_t *buffer_handler;
    uartMap_t uart;
} frame_packer_resources_t;


/*=====[Definitions of private variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/
/**
 * @brief Create a frame ready to be printed
 *
 * @param taskParmPtr
 */
static void C2_FRAME_PACKER_PrinterTask(void *taskParmPtr);

/**
 * @brief Create a frame ready to be processed
 *
 * @param taskParmPtr
 */
static void C2_FRAME_PACKER_ReceiverTask(void *taskParmPtr);

/**
 * @brief Inicializa la interrupción de Tx de la UART.
 *
 * @param UARTRxCallBackFunc
 * @param parameter
 */
static void C2_FRAME_PACKER_UartTxInit(void *UARTTxCallBackFunc, void *parameter);

/**
 * @brief TX UART ISR function. Esta función se llama cuando el buffer de la UART está vacío.
 *
 * @param parameter QMPool*, frame_t y Uart inicializados.
 */
static void C2_FRAME_TRANSMIT_UartTxISR(void *parameter);

/*=====[Implementations of public functions]=================================*/

void C2_FRAME_PACKER_ReceiverInit(frame_buffer_handler_t *app_buffer_handler_receive, uartMap_t uart) {
    frame_packer_resources_t *frame_packer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
    configASSERT(frame_packer_resources != NULL);
    frame_packer_resources->uart = uart;
    frame_packer_resources->buffer_handler = app_buffer_handler_receive;
    BaseType_t xReturned = xTaskCreate(
        C2_FRAME_PACKER_ReceiverTask,
        (const char *)"Frame Packer",
        configMINIMAL_STACK_SIZE * 5,
        (void *)frame_packer_resources,
        tskIDLE_PRIORITY + 1,
        NULL
        );
    configASSERT(xReturned == pdPASS);
}

void C2_FRAME_PACKER_PrinterInit(frame_buffer_handler_t *app_buffer_handler_send, uartMap_t uart) {
    frame_packer_resources_t *printer_resources = pvPortMalloc(sizeof(frame_packer_resources_t));
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

/*=====[Implementations of private functions]================================*/

static void C2_FRAME_PACKER_ReceiverTask(void *taskParmPtr) {
    frame_packer_resources_t *frame_packer_resources = (frame_packer_resources_t *) taskParmPtr;
    frame_buffer_handler_t *buffer_handler_app = frame_packer_resources->buffer_handler;
    uartMap_t uart = frame_packer_resources->uart;
    frame_buffer_handler_t *buffer_handler_capture = C2_FRAME_CAPTURE_ObjInit(buffer_handler_app->pool, uart);
    vPortFree(frame_packer_resources);

    frame_t raw_frame;
    frame_t frame_app;

    while (TRUE) {
        xQueueReceive(buffer_handler_capture->queue, &raw_frame, portMAX_DELAY);
        // Chequear que tanto los caracteres del ID como del CRC esten en mayusculas, de otro modo el paquete seria invalido
        frame_app.data = &raw_frame.data[CHARACTER_INDEX_CMD];
        frame_app.data_size = raw_frame.data_size - CHARACTER_SIZE_ID;
        frame_app.data[frame_app.data_size] = '\0';
        xQueueSend(buffer_handler_app->queue, &frame_app, portMAX_DELAY);
    }
}

static void C2_FRAME_PACKER_PrinterTask(void *taskParmPtr) {
    frame_packer_resources_t *printer_resources = (frame_packer_resources_t *) taskParmPtr;
    frame_buffer_handler_t *buffer_handler_print = printer_resources->buffer_handler;
    frame_transmit_t printer_isr;
    printer_isr.pool = buffer_handler_print->pool;
    printer_isr.uart = printer_resources->uart;;

    vPortFree(printer_resources);


    // ----- Task repeat for ever -------------------------
    while (TRUE) {
        // Se espera a que llegue el paquete procesado
        xQueueReceive(buffer_handler_print->queue, &printer_isr.transmit_frame.data, portMAX_DELAY);
        // Se calcula el CRC del paquete procesado
        uint8_t crc = crc8_calc(0, printer_isr.transmit_frame.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size) - CHARACTER_SIZE_CRC - 1);
        // Se arma el paquete con los datos procesados, agregando los delimitadores, el ID y el nuevo CRC
        snprintf(printer_isr.transmit_frame.data, PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size), "%s%2X", printer_isr.transmit_frame.data - CHARACTER_SIZE_ID * sizeof(char), crc);
        printer_isr.transmit_frame.data_size = PRINT_FRAME_SIZE(printer_isr.transmit_frame.data_size);

        C2_FRAME_TRANSMIT_InitTransmision(&printer_isr);
        // Se habilita la interrupcion para enviar el paquete a la capa de transmision C1
    }
}

/*=====[Implementations of interrupt functions]==============================*/
