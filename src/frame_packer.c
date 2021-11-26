/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 23/11/2021
 * Version: 1.3
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
/*=====[Definición de tipos de datos privados]================================*/

/*=====[Definición de variables privadas]====================================*/

/*=====[Declaración de prototipos de funciones privadas]======================*/

/*=====[Implementación de funciones públicas]=================================*/

void C2_FRAME_PACKER_Init(frame_buffer_handler_t *buffer_handler, uartMap_t uart) {
    buffer_handler->queue = C2_FRAME_CAPTURE_ObjInit(buffer_handler->pool, uart)->queue;
    C2_FRAME_TRANSMIT_ObjInit(buffer_handler); // Se envía para asignación de semáforo de buffer_handler de transmisión
}

void C2_FRAME_PACKER_Receive(frame_t *frame, frame_buffer_handler_t *buffer_handler) {
    xQueueReceive(buffer_handler->queue, frame, portMAX_DELAY);                  //Recibe luego de un EOM en frame_capture
    frame->data = &frame->data[CHARACTER_INDEX_CMD];                             //Se posiciona en el comienzo de los datos y se enmascara el ID
    frame->data_size = frame->data_size - CHARACTER_SIZE_ID;                     //Se descuenta el ID en el tamaño de los datos
    frame->data[frame->data_size] = CHARACTER_END_OF_PACKAGE;                    //Se agrega el '\0' al final de los datos sacando el CRC
}

void C2_FRAME_PACKER_Print(frame_class_t *frame_obj) {
    uint8_t crc = crc8_calc(0, frame_obj->frame.data - CHARACTER_SIZE_ID * sizeof(char), PRINT_FRAME_SIZE(frame_obj->frame.data_size) - CHARACTER_SIZE_CRC - 1); // Se calcula el CRC del paquete procesado

    snprintf(frame_obj->frame.data + START_OF_MESSAGE_SIZE - CHARACTER_SIZE_ID * sizeof(char),  // puntero a posición de cadena de destino
             PRINT_FRAME_SIZE(frame_obj->frame.data_size),                                      // tamaño de los datos a escribir
             "%s%0.2X",                                                                         // formato de dato [[frame + crc de 2 elementos]]
             frame_obj->frame.data - CHARACTER_SIZE_ID * sizeof(char),                          // datos sin crc
             crc);                                                                              // nuevo crc 
                    
    frame_obj->frame.data -= CHARACTER_SIZE_ID * sizeof(char);                                          // Se resta el ID al puntero de datos para apuntar al comienzo del paquete
    frame_obj->frame.data[0] = START_OF_MESSAGE;                                                        // Se agraga el SOM
    frame_obj->frame.data_size = PRINT_FRAME_SIZE(frame_obj->frame.data_size) + START_OF_MESSAGE_SIZE;  // Se actualiza el tamaño del paquete incluyendo el CRC y el ID


    C2_FRAME_TRANSMIT_InitTransmision(frame_obj);                               // Se inicializa la transmisión del paquete procesado por la ISR
}

/*=====[Implementación de funciones privadas]================================*/

/*=====[Implementación de funciones de interrupción]==============================*/
