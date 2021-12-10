/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 03/12/2021
 * Version: 1.4
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
#define START_OF_MESSAGE_SIZE            1 * sizeof(char)
#define PRINT_FRAME_SIZE(size)           ((size) + (CHARACTER_INDEX_DATA + CHARACTER_SIZE_CRC) * sizeof(uint8_t))
#define CRC_MSG_FORMAT                   "%s%0.2X"
#define POOL_PACKET_SIZE                    MAX_BUFFER_SIZE
#define POOL_PACKET_COUNT                   (QUEUE_SIZE)
#define POOL_SIZE_BYTES                     (POOL_PACKET_SIZE * POOL_PACKET_COUNT * sizeof(char))
/*=====[Definición de tipos de datos privados]================================*/
typedef struct {
    activeObject_t *activeObject;
    uartMap_t uart;
} frame_packer_t;
/*=====[Definición de variables privadas]====================================*/
/**
 * @brief Recibe los datos de FRAME_CAPTURE y los empaqueta (enmascara el ID y saca el CRC) y los envia a la capa superior.
 *
 * @param frame          Puntero al objeto que cargara el frame recibido.
 * @param buffer_handler Buffer que contiene el contexto (cola y puntero a pool)
 */
static void C2_FRAME_PACKER_ReceiveTask(void *task_parameter);
/**
 * @brief Función que recibe y procesa los datos recibidos desde C3, le inserta el ID y CRC para ser impresos/enviados por la función de callback de
 * la ISR de Tx.
 *
 * @param frame_obj Estructura del tipo frame_class_t.
 */

static void C2_FRAME_PACKER_PrintTask(void *task_parameter);
/*=====[Declaración de prototipos de funciones privadas]======================*/

/*=====[Implementación de funciones públicas]=================================*/

bool_t C2_FRAME_PACKER_Init(activeObject_t *ao_obj, uartMap_t uart) {
    bool_t no_error = TRUE;
    frame_packer_t *frame_packer = pvPortMalloc(sizeof(frame_packer_t));
    if (frame_packer == NULL) {
        no_error = FALSE;
    }
    frame_packer->activeObject = ao_obj;
    frame_packer->uart = uart;
    BaseType_t res = xTaskCreate(
        C2_FRAME_PACKER_ReceiveTask,
        (const char *)"C2_FRAME_PACKER_ReceiveTask",
        configMINIMAL_STACK_SIZE * 3,
        (void *) frame_packer,
        tskIDLE_PRIORITY + 1,
        NULL
        );
    if (res != pdPASS) {
        no_error = FALSE;
    }
    return no_error;
}

void C2_FRAME_PACKER_ReceiveTask(void *task_parameter) {
    frame_packer_t *frame_packer = (frame_packer_t *) task_parameter;
    activeObject_t *ao_obj = frame_packer->activeObject;


    QMPool pool;
    event_t event = EVENT_RECEIVE;
    frame_buffer_handler_t buffer_handler;
    buffer_handler.pool = &pool;

    buffer_handler.uart = frame_packer->uart;

    vPortFree(frame_packer);

    char *memory_pool = (char *)pvPortMalloc(POOL_SIZE_BYTES);
    configASSERT(memory_pool != NULL);
    QMPool_init(buffer_handler.pool, (char *) memory_pool, POOL_SIZE_BYTES * sizeof(char), POOL_PACKET_SIZE);

    C2_FRAME_TRANSMIT_ObjInit(&buffer_handler);     // Se envía para asignación de semáforo de buffer_handler de transmisión

    QueueHandle_t queue_receive = C2_FRAME_CAPTURE_ObjInit(buffer_handler.pool,  buffer_handler.uart)->queue_receive;
    buffer_handler.queue_transmit = xQueueCreate(QUEUE_SIZE, sizeof(frame_t));
    
    configASSERT(buffer_handler.queue_transmit != NULL);

    BaseType_t res = xTaskCreate(
        C2_FRAME_PACKER_PrintTask,
        (const char *)"C2_FRAME_PACKER_PrintTask",
        configMINIMAL_STACK_SIZE * 3,
        (void *) &buffer_handler,
        tskIDLE_PRIORITY + 1,
        NULL
        );

    configASSERT(res == pdPASS);

    frame_t frame;
    while (TRUE) {
        xQueueReceive(queue_receive, &frame, portMAX_DELAY);
        frame.data = &frame.data[CHARACTER_INDEX_CMD];                         //Se posiciona en el comienzo de los datos y se enmascara el ID
        frame.data_size = frame.data_size - CHARACTER_SIZE_ID;                 //Se descuenta el ID en el tamaño de los datos
        frame.data[frame.data_size] = CHARACTER_END_OF_PACKAGE;                //Se agrega el '\0' al final de los datos sacando el CRC
        frame.event = EVENT_RECEIVE;
        frame.send_queue = buffer_handler.queue_transmit;
        activeObjectEnqueue(ao_obj, &frame);                      //Recibe luego de un EOM en frame_capture
    }
}

void C2_FRAME_PACKER_PrintTask(void *task_parameter) {
    frame_buffer_handler_t *buffer_handler = (frame_buffer_handler_t *) task_parameter;
    frame_t frame;

    while (TRUE) {
        xQueueReceive(buffer_handler->queue_transmit, &frame, portMAX_DELAY);

        uint8_t crc = crc8_calc(0, frame.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(frame.data_size) - CHARACTER_SIZE_CRC - 1); // Se calcula el CRC del paquete procesado
        snprintf(frame.data + START_OF_MESSAGE_SIZE - CHARACTER_SIZE_ID * sizeof(char), // puntero a posición de cadena de destino
                 PRINT_FRAME_SIZE(frame.data_size),                                    // tamaño de los datos a escribir
                 CRC_MSG_FORMAT,                                                                       // formato de dato [[frame + crc de 2 elementos]]
                 frame.data - CHARACTER_SIZE_ID * sizeof(char),                        // datos sin crc
                 crc);                                                                            // nuevo crc

        frame.data -= CHARACTER_SIZE_ID * sizeof(char);                                      // Se resta el ID al puntero de datos para apuntar al comienzo del paquete
        frame.data[0] = START_OF_MESSAGE;                                                    // Se agraga el SOM
        frame.data_size = PRINT_FRAME_SIZE(frame.data_size) + START_OF_MESSAGE_SIZE; // Se actualiza el tamaño del paquete incluyendo el CRC y el ID

        frame_class_t frame_obj = {
            .frame = frame,
            .buffer_handler = *buffer_handler
        };

        C2_FRAME_TRANSMIT_InitTransmision(&frame_obj);                           // Se inicializa la transmisión del paquete procesado por la ISR
    }
}

/*=====[Implementación de funciones privadas]================================*/

/*=====[Implementación de funciones de interrupción]==============================*/
