/*=============================================================================
 * Authors: Marcos Raul Dominguez Shocron <mrds0690@gmail.com> - Pablo Javier Morzan
 * <pablomorzan@gmail.com> - Martin Julian Rios <jrios@fi.uba.ar>
 * Date: 03/12/2021
 * Version: 1.4
 *===========================================================================*/

/*=====[Inclusión de cabecera]=============================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "frame_processor.h"
#include "frame_packer.h"
#include "string.h"
#include "AO.h"

/*=====[Definición de macros de constantes privadas]==============================*/
typedef enum {
    ERROR_NONE,
    ERROR_INVALID_DATA,
    ERROR_INVALID_OPCODE,
    ERROR_SYSTEM,

    ERROR_QTY,
} error_t;

#define ERROR_MSG_SIZE                      3
#define POOL_PACKET_SIZE                    MAX_BUFFER_SIZE
#define POOL_PACKET_COUNT                   (QUEUE_SIZE)
#define POOL_SIZE_BYTES                     (POOL_PACKET_SIZE * POOL_PACKET_COUNT * sizeof(char))
#define CHECK_LOWERCASE(x)                  ((x >= 'a' && x <= 'z') ? TRUE : FALSE)
#define CHECK_UPPERCASE(x)                  ((x >= 'A' && x <= 'Z') ? TRUE : FALSE)
#define CHECK_OPCODE(x)                     ((x == 'S' || x == 'C' || x == 'P') ? ERROR_NONE : ERROR_INVALID_OPCODE)
#define TO_UPPERCASE(x)                     (x - 32)
#define TO_LOWERCASE(x)                     (x + 32)
#define INVALID_FRAME                       (-1)
#define ERROR_MSG_FORMAT                    "E%.2d"
#define ASCII_UNDERSCORE                     '_'
#define ASCII_SPACE                         ' '
#define TRANSITION_CHAR(x, current_char)    ((x == CASE_SNAKE) ? ASCII_UNDERSCORE : current_char)
/*=====[Definición de tipos de datos privados]===================================*/

typedef int8_t (*WordProcessorCallback)(char *, char *);
typedef void (*FrameProcessorCallback)(void *, void *);

typedef enum {
    CASE_PASCAL,
    CASE_CAMEL,
    CASE_SNAKE,

    CASE_QTY,
} case_t;

/*=====[Definición de variables globales publicas externas]=====================*/

/*=====[Definición de variables globales públicas]==============================*/

/*=====[Definición de variables globales privadas]=============================*/
static const char command_map[CASE_QTY] = {
    [CASE_SNAKE]  = 'S',
    [CASE_CAMEL]  = 'C',
    [CASE_PASCAL] = 'P',
};

static const char *const task_name_map[CASE_QTY] = {
    [CASE_SNAKE]  = "Snake Frame Processor",
    [CASE_CAMEL]  = "Camel Frame Processor",
    [CASE_PASCAL] = "Pascal Frame Processor",
};


/*=====[Declaración de prototipos de funciones privadas]======================*/
static void C3_FRAME_PROCESSOR_Callback(void *taskParmPtr);

/**
 * @brief función para procesar el frame según el comando que recibe.
 *
 * @param frame puntero al frame recibido para procesar.
 * @param cmd_case referencia al formato de conversión de la trama.
 *
 */
static void C3_FRAME_PROCESSOR_Transform(frame_t *frame, case_t cmd_case);

/**
 * @brief Tarea que recibe las tramas, llama a la función de callbak y envía las tramas para transmitir.
 *
 * @param taskParmPtr puntero de tipo main_processor que contiene contexto de la instancia del objeto.
 */
static void C3_FRAME_PROCESSOR_FrameTransformerObject(void *taskParmPtr);

/**
 * @brief Función que recibe la trama y llama a la función de procesamiento de trama con el comando de tranformación a camel
 *
 * @param frame Trama recibida a ser transformada
 */
static void C3_FRAME_PROCESSOR_ToCamel(void *caller_ao, void *data);

/**
 * @brief Función que recibe la trama y llama a la función de procesamiento de trama con el comando de tranformación a pascal
 *
 * @param frame Trama recibida a ser transformada
 */
static void C3_FRAME_PROCESSOR_ToPascal(void *caller_ao, void *data);

/**
 * @brief Función que recibe la trama y llama a la función de procesamiento de trama con el comando de tranformación a snake
 *
 * @param frame Trama recibida a ser transformada
 */
static void C3_FRAME_PROCESSOR_ToSnake(void *caller_ao, void *data);

/**
 * @brief Procesa una palabra cualquiera y le pone la primera letra en minúscula.
 *
 * @param word_in Puntero de la cadena recibida que está siendo procesada.
 * @param word_out Puntero de la cadena de salida con los datos ya procesados.
 */
static int8_t C3_FRAME_PROCESSOR_WordLowerInitial(char *word_in, char *word_out);

/**
 * @brief Procesa una palabra cualquiera y le pone la primera letra en mayúscula.
 *
 * @param word_in Puntero de la cadena recibida que está siendo procesada.
 * @param word_out Puntero de la cadena de salida con los datos ya procesados.
 */
static int8_t C3_FRAME_PROCESSOR_WordUpperInitial(char *word_in, char *word_out);

/*=====[Implementación de funciones públicas]=================================*/

bool_t C3_FRAME_PROCESSOR_Init(uartMap_t uart , cosa_completa) {
    bool_t ret = FALSE;
    static bool_t uart_used[UART_MAXNUM] = {FALSE};

    if (uart < UART_MAXNUM && uart >= 0) {
        frame_class_t *frame_object = (frame_class_t *)pvPortMalloc(sizeof(frame_class_t));
        if (frame_object == NULL) {
            return FALSE;
        }


        activeObject_t *app_object = (activeObject_t *) pvPortMalloc(sizeof(activeObject_t));
        if (app_object == NULL) {
            return FALSE;
        }

        activeObjectOperationCreate(app_object, C3_FRAME_PROCESSOR_Callback, app_object->ReceiveQueue, FALSE);

        C2_FRAME_PACKER_Init(app_object, uart); // Se inicializa el objeto de la instancia
    }
    return ret;
}

/*=====[Implementación de Tareas]================================*/

static void C3_FRAME_PROCESSOR_Callback(void *caller_ao, void *parameter) {
    activeObject_t *caller_object = (activeObject_t *)caller_ao;
    frame_t *frame = (frame_t *) parameter;


    FrameProcessorCallback CallBackAo[CASE_QTY] = {            // Se cargan los callback para transformar la primera letra de cada palabra segun el caso
        [CASE_SNAKE] = C3_FRAME_PROCESSOR_ToSnake,
        [CASE_CAMEL]  = C3_FRAME_PROCESSOR_ToCamel,
        [CASE_PASCAL]  = C3_FRAME_PROCESSOR_ToPascal,
    };

    activeObject_t frame_ao[CASE_QTY] = {
        [CASE_SNAKE]    = {.itIsAlive = FALSE},
        [CASE_CAMEL]    = {.itIsAlive = FALSE},
        [CASE_PASCAL]   = {.itIsAlive = FALSE},
    };

    // Se utilizará para guardar el paquete proveniente de C2
    if (frame->event == EVENT_RECEIVE) {
        case_t command = CASE_PASCAL;

        // Se obtiene el evento
        while (command_map[command] != *frame.data && command < CASE_QTY) {     // Se obtiene el comando mapeado a número de la primera posición del paquete que arribó de C2
            command++;
        }

        // Si el comando es uno válido se procesa el paquete
        if (command < CASE_QTY) {
            vTaskSuspendAll();
            if (frame_ao[command].itIsAlive == FALSE) {
                if (!activeObjectOperationCreate(&frame_ao[command], CallBackAo[command], caller_object->ReceiveQueue)) {     // Se crea el objeto activo, con el comando correspondiente y tarea asociada.
                    snprintf(frame->data, ERROR_MSG_SIZE + (sizeof((char)CHARACTER_END_OF_PACKAGE)), ERROR_MSG_FORMAT, ERROR_SYSTEM - 1);
                    frame->data_size = ERROR_MSG_SIZE;
                    frame->event = EVENT_TRANSMIT;
                    activeObjectEnqueue(caller_ao, frame);
                }
            }
            if (frame_ao[command].itIsAlive == TRUE) {     // Se consulta si está activo el AO por si no se pudo crear en la condición anterior
                activeObjectEnqueue(&frame_ao[command], frame);      // Y enviamos el dato a la cola para procesar.
            }
            xTaskResumeAll();
        }
        else {     // En caso de que el comando no sea válido se genera el mensaje de error
            snprintf(frame->data, ERROR_MSG_SIZE + (sizeof((char)CHARACTER_END_OF_PACKAGE)), ERROR_MSG_FORMAT, ERROR_INVALID_OPCODE - 1);
            frame->data_size = ERROR_MSG_SIZE;
            frame->event = EVENT_TRANSMIT;
            activeObjectEnqueue(caller_ao, frame);
        }
    }
    else if (frame->event == EVENT_TRANSMIT) {
        xQueueSend(frame->send_queue, &frame, portMAX_DELAY);
    }
}

/*=====[Implementación de funciones privadas]================================*/
static void C3_FRAME_PROCESSOR_ToCamel(void *caller_ao, void *data) {
    frame_t *frame = (frame_t *) data;
    activeObject_t *me_ao = (activeObject_t *) caller_ao;
    C3_FRAME_PROCESSOR_Transform(frame, CASE_CAMEL);
    frame->event = EVENT_TRANSMIT;
    xQueueSend(me_ao->TransmitQueue, &frame, portMAX_DELAY);
}

static void C3_FRAME_PROCESSOR_ToPascal(void *caller_ao, void *data) {
    frame_t *frame = (frame_t *) data;
    activeObject_t *me_ao = (activeObject_t *) caller_ao;
    C3_FRAME_PROCESSOR_Transform(frame, CASE_PASCAL);
    frame->event = EVENT_TRANSMIT;
    xQueueSend(me_ao->TransmitQueue, &frame, portMAX_DELAY);
}

static void C3_FRAME_PROCESSOR_ToSnake(void *caller_ao, void *data) {
    frame_t *frame = (frame_t *) data;
    activeObject_t *me_ao = (activeObject_t *) caller_ao;
    C3_FRAME_PROCESSOR_Transform(frame, CASE_SNAKE);
    frame->event = EVENT_TRANSMIT;
    xQueueSend(me_ao->TransmitQueue, &frame, portMAX_DELAY);
}

static void C3_FRAME_PROCESSOR_Transform(frame_t *frame, case_t cmd_case) {
    WordProcessorCallback CallbackFunc[CASE_QTY] = {    // Se cargan los callback para transformar la primera letra de cada palabra segun el caso
        [CASE_SNAKE]  = C3_FRAME_PROCESSOR_WordLowerInitial,
        [CASE_CAMEL]  = C3_FRAME_PROCESSOR_WordUpperInitial,
        [CASE_PASCAL] = C3_FRAME_PROCESSOR_WordUpperInitial,
    };
    uint8_t out_increment_map[CASE_QTY] = {
        [CASE_SNAKE]  = sizeof((char)ASCII_UNDERSCORE),
        [CASE_CAMEL]  = 0,
        [CASE_PASCAL] = 0,
    };
    char frame_out[MAX_BUFFER_SIZE];    // se usa para armar la cadena de salida
    *frame_out = *frame->data;      // se copia el comando

    int index_in = CHARACTER_SIZE_CMD;  // índice en cadena de entrada
    int index_out = CHARACTER_SIZE_CMD; // índice en cadena de salida
    int qty_words = 0;                  // para chequear la cantidad máxima de palabras
    int8_t error_flag = 0;              // flag de error para informar a capa 2
    while (!error_flag) {
        int8_t character_count = CallbackFunc[cmd_case](&frame->data[index_in], &frame_out[index_out]);
        index_in += character_count;
        index_out += character_count;

        if ((++qty_words > WORD_MAX_QTY) || (character_count == INVALID_FRAME)) {           // Si se pasa la cantidad máxima de palabras se sale
            error_flag = ERROR_INVALID_DATA;
            break;
        }
        if (frame->data[index_in] == CHARACTER_END_OF_PACKAGE) {        // Condición de salida de un frame correcto
            if (qty_words < WORD_MIN_QTY) {                                 // Si la cantidad de palabras es menor a la permitida se considera error.
                error_flag = ERROR_INVALID_DATA;
            }
            break;
        }
        else if (frame->data[index_in] == ASCII_UNDERSCORE || frame->data[index_in] == ASCII_SPACE) {   // Si es snake y viene un '_' o ' ' se pone '_'. Si es camel o Pascal se descarta
            frame_out[index_out] = TRANSITION_CHAR(cmd_case, frame_out[index_out]);
            index_out += out_increment_map[cmd_case];
            index_in++;
        }
        else if (CHECK_UPPERCASE(frame->data[index_in])) {
            frame_out[index_out] = TRANSITION_CHAR(cmd_case, frame_out[index_out]);
            index_out += out_increment_map[cmd_case];
        }
        else {
            error_flag = ERROR_INVALID_DATA;
            break;
        }
    }

    if (error_flag) {
        snprintf(frame_out, ERROR_MSG_SIZE + sizeof(CHARACTER_END_OF_PACKAGE), ERROR_MSG_FORMAT, error_flag - 1);
        index_out = ERROR_MSG_SIZE;
    }
    else if (cmd_case == CASE_CAMEL) {
        frame_out[CHARACTER_SIZE_CMD] = TO_LOWERCASE(frame_out[CHARACTER_SIZE_CMD]);
    }

    memcpy(frame->data, frame_out, index_out);
    frame->data[index_out] = CHARACTER_END_OF_PACKAGE;
    frame->data_size = index_out;
}

static int8_t C3_FRAME_PROCESSOR_WordLowerInitial(char *word_in, char *word_out) {
    if (CHECK_UPPERCASE(*word_in)) {
        *word_out = TO_LOWERCASE(*word_in);
    }
    else if (CHECK_LOWERCASE(*word_in)) {
        *word_out = *word_in;
    }
    else {
        return INVALID_FRAME; // Si la palabra no comienza con mayúscula o minúscula se considera error
    }

    int8_t index_in = CHARACTER_SIZE_CMD;

    while (CHECK_LOWERCASE(word_in[index_in]) && index_in <= WORD_MAX_SIZE) { // palabra de hasta 10 caracteres
        word_out[index_in] = word_in[index_in];
        index_in++;
    }

    if (index_in > WORD_MAX_SIZE) { // si se cuentan 11 caracteres o más se considera inválido
        index_in = INVALID_FRAME;
    }

    return index_in;
}

static int8_t C3_FRAME_PROCESSOR_WordUpperInitial(char *word_in, char *word_out) {
    if (CHECK_UPPERCASE(*word_in)) {
        *word_out = *word_in;
    }
    else if (CHECK_LOWERCASE(*word_in)) {
        *word_out = TO_UPPERCASE(*word_in);
    }
    else {
        return INVALID_FRAME; // Si la palabra no comienza con mayúscula o minúscula se considera error
    }

    int8_t index_in = CHARACTER_SIZE_CMD;

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
