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

/*=====[Definición de macros de constantes privadas]==============================*/
typedef enum {
    ERROR_NONE,
    ERROR_INVALID_DATA,
    ERROR_INVALID_OPCODE,
    ERROR_MSG_SIZE,

    ERROR_QTY,
} error_t;

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
typedef void (*FrameProcessorCallback)(frame_t *frame_obj);
typedef struct {
    TaskFunction_t task_function;
    uartMap_t uart;
    bool_t is_active;
} main_processor_t;

typedef struct {
    FrameProcessorCallback callback;
    QueueHandle_t queue_receive;
    QueueHandle_t queue_send;
    bool_t is_active;
} frame_processor_t;

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
/**
 * @brief función de capa 3 que envía contexto para inicializar Objeto de la instancia.
 * Recibe las tramas del packer de capa 2, los procesa y envía a capa 2 para transmitirlos.
 *
 * @param taskParmPtr Se envia a tarea puntero a pool de memoria y uart de instancia.
 */
static void C3_FRAME_PROCESSOR_Task(void *taskParmPtr);

static void C3_FRAME_PROCESSOR_Transform(frame_t *frame_obj, case_t cmd_case);

static void C3_FRAME_PROCESSOR_FrameTransformerObject(void *taskParmPtr);

static void C3_FRAME_PROCESSOR_ToCamel(frame_t *frame_obj);

static void C3_FRAME_PROCESSOR_ToPascal(frame_t *frame_obj);

static void C3_FRAME_PROCESSOR_ToSnake(frame_t *frame_obj);

static int8_t C3_FRAME_PROCESSOR_WordLowerInitial(char *word_in, char *word_out);

static int8_t C3_FRAME_PROCESSOR_WordUpperInitial(char *word_in, char *word_out);

/*=====[Implementación de funciones públicas]=================================*/

bool_t C3_FRAME_PROCESSOR_Init(uartMap_t uart) {
    main_processor_t *main_app_instance = (main_processor_t *)pvPortMalloc(sizeof(frame_processor_t));

    main_app_instance->task_function = C3_FRAME_PROCESSOR_Task;
    main_app_instance->uart = uart;
    bool_t ret = FALSE;
    static bool_t uart_used[UART_MAXNUM] = {FALSE};
    if (uart < UART_MAXNUM && uart >= 0) {
        if (!uart_used[uart]) {
            uart_used[uart] = TRUE;
            BaseType_t xReturned = xTaskCreate(
                main_app_instance->task_function,
                (const char *)"Main Processor",
                configMINIMAL_STACK_SIZE * 3,
                (void *) main_app_instance,
                tskIDLE_PRIORITY + 1,
                NULL
                );
            if (xReturned == pdPASS) {
                main_app_instance->is_active = TRUE;
            }
        }
    }
    return ret;
}

/*=====[Implementación de Tareas]================================*/

static void C3_FRAME_PROCESSOR_Task(void *taskParmPtr) {
    main_processor_t *main_app_instance = (main_processor_t *)taskParmPtr;

    char *memory_pool = (char *)pvPortMalloc(POOL_SIZE_BYTES);
    configASSERT(memory_pool != NULL);

    frame_processor_t frame_processor_instance[CASE_QTY];

    frame_processor_instance[CASE_SNAKE].callback = C3_FRAME_PROCESSOR_ToSnake;
    frame_processor_instance[CASE_CAMEL].callback = C3_FRAME_PROCESSOR_ToCamel;
    frame_processor_instance[CASE_PASCAL].callback = C3_FRAME_PROCESSOR_ToPascal;
    for (int i = 0; i < CASE_QTY; i++) {
        frame_processor_instance[i].is_active = FALSE;
    }

    QMPool pool;
    frame_class_t frame_obj;
    frame_obj.buffer_handler.pool = &pool;
    frame_obj.buffer_handler.queue_receive = NULL,
    frame_obj.buffer_handler.queue_transmit = NULL,
    frame_obj.frame.data = NULL;
    frame_obj.frame.data_size = 0;
    frame_obj.uart = (uartMap_t) main_app_instance->uart;
    frame_obj.buffer_handler.queue_transmit  = xQueueCreate(QUEUE_SIZE, sizeof(frame_t));
    configASSERT(frame_obj.buffer_handler.queue_transmit  != NULL);
    QMPool_init(&pool, (char *) memory_pool, POOL_SIZE_BYTES * sizeof(char), POOL_PACKET_SIZE);

    C2_FRAME_PACKER_Init(&frame_obj); // Se inicializa el objeto de la instancia
    
    while (TRUE) {
        frame_t frame;
        C2_FRAME_PACKER_Receive(&frame, frame_obj.buffer_handler.queue_receive);    // Se espera el paquete de la capa C2

        case_t command = CASE_PASCAL;

        while (command_map[command] != *frame.data && command < CASE_QTY) {  // Se obtiene el comando mapeado a número de la primera posición del paquete que arribó de C2
            command++;
        }

        // Si el comando es uno válido se procesa el paquete
        if (command < CASE_QTY) {
            vTaskSuspendAll();
            if (frame_processor_instance[command].is_active == FALSE) {  // Se crea el objeto sólo si no está activo
                xTaskResumeAll();
                BaseType_t ret = xTaskCreate(
                    C3_FRAME_PROCESSOR_FrameTransformerObject,
                    (const char *) task_name_map[command],
                    configMINIMAL_STACK_SIZE * 3,
                    (void *) &frame_processor_instance[command],
                    tskIDLE_PRIORITY + 1,
                    NULL
                    );
                if (ret == pdPASS) {
                    vTaskSuspendAll();
                    frame_processor_instance[command].queue_receive = xQueueCreate(QUEUE_SIZE, sizeof(frame_t));
                    if (frame_processor_instance[command].queue_receive != NULL) {
                        frame_processor_instance[command].queue_send = frame_obj.buffer_handler.queue_transmit;
                        frame_processor_instance[command].is_active = TRUE;
                    }
                    xTaskResumeAll();
                }
            }
            else {
                xTaskResumeAll();
            }
            vTaskSuspendAll();
            if (frame_processor_instance[command].is_active == TRUE) {
                xQueueSend(frame_processor_instance[command].queue_receive, &frame, 0);     // Se envía el paquete al objeto correspondiente para procesar
            }
            xTaskResumeAll();
        }
        else {  // En caso de que el comando no sea válido se genera el mensaje de error
            snprintf(frame.data, ERROR_MSG_SIZE + (sizeof((char)CHARACTER_END_OF_PACKAGE)), ERROR_MSG_FORMAT, ERROR_INVALID_OPCODE - 1);
            frame.data_size = ERROR_MSG_SIZE;
            xQueueSend(frame_obj.buffer_handler.queue_transmit, &frame, 0);     // Se envía el paquete para trasmitir por UART
        }
        
        
    }
}

void C3_FRAME_PROCESSOR_FrameTransformerObject(void *taskParmPtr) {
    frame_processor_t *frame_processor_instance = (frame_processor_t *)taskParmPtr;
    frame_t frame;
    
    while (TRUE) {
        if(frame_processor_instance->is_active == TRUE) {
            xQueueReceive(frame_processor_instance->queue_receive, &frame, portMAX_DELAY);
            frame_processor_instance->callback(&frame);
            xQueueSend(frame_processor_instance->queue_send, &frame, 0);  // Se envía en paquete para transmitir
            if(uxQueueMessagesWaiting( frame_processor_instance->queue_receive ) == 0) {
                vTaskSuspendAll();
                frame_processor_instance->is_active = FALSE;
                vQueueDelete(frame_processor_instance->queue_receive);
                frame_processor_instance->queue_receive = NULL;
                xTaskResumeAll();
                vTaskDelete(NULL);
            }            
        }
    }       
}
    
/*=====[Implementación de funciones privadas]================================*/
static void C3_FRAME_PROCESSOR_ToCamel(frame_t *frame_obj) {
    C3_FRAME_PROCESSOR_Transform(frame_obj, CASE_CAMEL);
}

static void C3_FRAME_PROCESSOR_ToPascal(frame_t *frame_obj) {
    C3_FRAME_PROCESSOR_Transform(frame_obj, CASE_PASCAL);
}

static void C3_FRAME_PROCESSOR_ToSnake(frame_t *frame_obj) {
    C3_FRAME_PROCESSOR_Transform(frame_obj, CASE_SNAKE);
}

static void C3_FRAME_PROCESSOR_Transform(frame_t *frame_obj, case_t cmd_case) {
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
    char frame_out[MAX_BUFFER_SIZE];    // su usa para armar la cadena de salida
    *frame_out = *frame_obj->data;  // se copia el comando

    int index_in = CHARACTER_SIZE_CMD;  // índice en cadena de entrada
    int index_out = CHARACTER_SIZE_CMD; // índice en cadena de salida
    int qty_words = 0;                  // para chequear la cantidad máxima de palabras
    int8_t error_flag = 0;              // flag de error para informar a capa 2
    while (!error_flag) {
        int8_t character_count = CallbackFunc[cmd_case](&frame_obj->data[index_in], &frame_out[index_out]);
        index_in += character_count;
        index_out += character_count;

        if (++qty_words > WORD_MAX_QTY) {
            error_flag = ERROR_INVALID_DATA;
            break;
        }
        if (frame_obj->data[index_in] == CHARACTER_END_OF_PACKAGE) {              // Condición de salida de un frame correcto
            if (qty_words < WORD_MIN_QTY) {
                error_flag = ERROR_INVALID_DATA;
            }
            break;
        }
        else if (frame_obj->data[index_in] == ASCII_UNDERSCORE || frame_obj->data[index_in] == ASCII_SPACE) {   // Si es snake y viene un '_' o ' ' se pone '_'. Si es camel o Pascal se descarta 
            frame_out[index_out] = TRANSITION_CHAR(cmd_case,frame_out[index_out]);
            index_out += out_increment_map[cmd_case];
            index_in++;
        }
        else if (CHECK_UPPERCASE(frame_obj->data[index_in])) {
            frame_out[index_out] = TRANSITION_CHAR(cmd_case,frame_out[index_out]);
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

    memcpy(frame_obj->data, frame_out, index_out);
    frame_obj->data[index_out] = CHARACTER_END_OF_PACKAGE;
    frame_obj->data_size = index_out;
}

static int8_t C3_FRAME_PROCESSOR_WordLowerInitial(char *word_in, char *word_out){
    if(CHECK_UPPERCASE(*word_in)){
        *word_out = TO_LOWERCASE(*word_in);
    }
    else if (CHECK_LOWERCASE(*word_in)){
        *word_out = *word_in;
    }
    else{
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
    
    if(CHECK_UPPERCASE(*word_in)){
        *word_out = *word_in;
    }
    else if (CHECK_LOWERCASE(*word_in)){
        *word_out = TO_UPPERCASE(*word_in);
    }
    else{
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