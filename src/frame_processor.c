/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 23/11/2021
 * Version: 1.3
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "frame_processor.h"
#include "frame_packer.h"
#include "string.h"

/*=====[Definición de macros de constantes privadas]==============================*/

#define POOL_PACKET_SIZE      MAX_BUFFER_SIZE
#define POOL_PACKET_COUNT     (QUEUE_SIZE)
#define POOL_SIZE_BYTES       (POOL_PACKET_SIZE * POOL_PACKET_COUNT * sizeof(char))
#define INVALID_UART_ID       (-1)
#define ERROR_NONE              0
#define ERROR_INVALID_DATA      1
#define ERROR_INVALID_OPCODE    2
#define ERROR_MSG_SIZE          3
#define CHECK_LOWERCASE(x) ((x >= 'a' && x <= 'z') ? TRUE : FALSE)
#define CHECK_UPPERCASE(x) ((x >= 'A' && x <= 'Z') ? TRUE : FALSE)
#define CHECK_OPCODE(x) ((x == 'S' || x == 'C' || x == 'P') ? ERROR_NONE : ERROR_INVALID_OPCODE)
#define TO_UPPERCASE(x) (x - 32)
#define TO_LOWERCASE(x) (x + 32)
#define INVALID_FRAME           (-1)
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

/**
 * @brief Transforma una trama en el formato definido por el comando que es el primer
 * elemento de la misma.
 *
 * @param frame recibe un puntero al primer elemento del frame que debe procesar.
 */
static uint8_t C3_FRAME_PROCESSOR_Transform(char *frame);
/**
 * @brief Transfomra de formato una palabra según el comando recibido
 * 
 * @param word_in puntero al inicio de la palabra a procesar
 * @param word_out puntero donde se debe escribir la palabra procesada
 * @param command indica el formato al que se debe procesar
 * @return int8_t revuelve la cantidad de caracteres procesados. En caso de error devuelve -1.
 */
static int8_t C3_FRAME_PROCESSOR_WordProcessor(char *word_in, char *word_out, char command);

/*=====[Implementación de funciones públicas]=================================*/

bool_t C3_FRAME_PROCESSOR_Init(uartMap_t uart) {
    bool_t ret = FALSE;
    static bool_t uart_used[UART_MAXNUM] = {FALSE};

    if (uart_used[uart]) {
        ret = FALSE;
    }
    else {
        uart_used[uart] = TRUE;
        BaseType_t xReturned = xTaskCreate(
            C3_FRAME_PROCESSOR_Task,
            (const char *)"Frame Processor",
            configMINIMAL_STACK_SIZE * 3,
            (void *) uart,
            tskIDLE_PRIORITY + 1,
            NULL
            );
        if (xReturned == pdPASS) {
            ret = TRUE;
        }
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
        frame_obj.frame.data_size = C3_FRAME_PROCESSOR_Transform(frame_obj.frame.data);
        C2_FRAME_PACKER_Print(&frame_obj);
    }
}

static uint8_t C3_FRAME_PROCESSOR_Transform(char *frame_in) {
    
    char frame_out[MAX_BUFFER_SIZE];    // su usa para armar la cadena de salida
    int index_in = 1;                   // índice en cadena de entrada
    int index_out = 1;                  // índice en cadena de salida
    int qty_words = 0;                  // para chequear la cantidad máxima de palabras
    char command;                       // ref al formato que se debe transformar
    int8_t error_flag = 0;              // flag de error para informar a capa 2

    command = *frame_in;
    *frame_out = *frame_in;
    error_flag = CHECK_OPCODE(command);

    while (!error_flag) {
        int8_t character_count;

        character_count = C3_FRAME_PROCESSOR_WordProcessor(&frame_in[index_in], &frame_out[index_out], command);

        if (character_count == INVALID_FRAME) {
            error_flag = ERROR_INVALID_DATA;
            break;
        }

        index_in += character_count;
        index_out += character_count;

        if (frame_in[index_in] == CHARACTER_END_OF_PACKAGE) {  // Condición de salida de un frame correcto
        	frame_out[index_out] = CHARACTER_END_OF_PACKAGE;
            break;
        }

        if (++qty_words > WORD_MAX_QTY) {
            error_flag = ERROR_INVALID_DATA;
            break;
        }

        // Transición de palabra
        if (command == 'S') {
            if (frame_in[index_in] == '_' || frame_in[index_in] == ' ') { 
                frame_out[index_out] = '_';
                index_in++;
                index_out++;
            }
            else if (CHECK_UPPERCASE(frame_in[index_in])) {
                frame_out[index_out] = '_';
                index_out++;
            }
            else {
                error_flag = ERROR_INVALID_DATA;
                break;
            }
        }
        else {
            if (frame_in[index_in] == '_' || frame_in[index_in] == ' ') { //Chequeo de transición de palabra
                index_in++;
            }
            else if (!CHECK_UPPERCASE(frame_in[index_in])) {
                error_flag = ERROR_INVALID_DATA;
                break;
            }
        }
    }

    if (error_flag) {
        snprintf(frame_out, ERROR_MSG_SIZE + 1, "E%.2d", error_flag - 1);
        index_out = ERROR_MSG_SIZE;
    }
    else if ('C' == command) {         // Camel se procesa como Pascal y se modifica el primer caracter al final
        frame_out[1] = TO_LOWERCASE(frame_out[1]);
    }

    memcpy(frame_in, frame_out, strlen(frame_out));

    frame_in[index_out] = CHARACTER_END_OF_PACKAGE;

    return index_out;
}

static int8_t C3_FRAME_PROCESSOR_WordProcessor(char *word_in, char *word_out, char command) {
    
    // Pascal y Camel se procesan como Pascal y al final se modifica el primer caracter

    if (CHECK_UPPERCASE(*word_in)) {  
        if (command == 'S') {
            *word_out = TO_LOWERCASE(*word_in);
        }
        else {
            *word_out = *word_in;
        }
    }
    else if (CHECK_LOWERCASE(*word_in)) {
        if (command == 'S') {
            *word_out = *word_in;
        }
        else {
            *word_out = TO_UPPERCASE(*word_in);
        }
    }
    else {
        return INVALID_FRAME; // Si la palabra no comienza con mayúscula o minúscula se considera error
    }

    int8_t index_in = 1;

    while (CHECK_LOWERCASE(word_in[index_in]) && index_in <= WORD_MAX_SIZE) { // palabra de hasta 10 caracteres
        word_out[index_in] = word_in[index_in];
        index_in++;
    }

    if (index_in > WORD_MAX_SIZE) { // si se cuentan 11 caracteres o más se considera inválido
        index_in = INVALID_FRAME;
    }
    
    return index_in;
}

/*=====[Implementación de funciones de interrupción]==============================*/
