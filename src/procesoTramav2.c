#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define TRUE 1
#define FALSE 0
#define EOC '\0'
#define ERROR_NONE              0
#define ERROR_INVALID_DATA      1
#define ERROR_INVALID_OPCODE    2
#define ERROR_MSG_SIZE          4
#define CHECK_MINUSCULA(x) ((x >= 'a' && x <= 'z') ? TRUE : FALSE)
#define CHECK_MAYUSCULA(x) ((x >= 'A' && x <= 'Z') ? TRUE : FALSE)
#define CHECK_OPCODE(x) ((x == 'S' || x == 'C' || x == 'P') ? ERROR_NONE : ERROR_INVALID_OPCODE)
#define A_MAYUSCULA(x) (x - 32)
#define A_MINUSCULA(x) (x + 32)
#define MAX_POR_PALABRA         10
#define MAX_WORDS_FRAME         15 
#define INVALID_FRAME           -1

int8_t C3_FRAME_PROCESSOR_WordProcessor(char *word_in, char *word_out, char command);

//char frame_in[] = "SEsto_Es_Unaaaaa_Pablopablopablopablopablo_Pascal_Para_Procesar_Agregando_Mas_Palabras_Para_Llegar_A_Las_Quince_Ehhhh_Y_Algo_Mas";
//char frame_in[] = "SestoEsUnaCadenaCamelParaProcesar";
//char frame_in[] = "Sesto_es_una_cadena_snake_para_procesar";
char frame_in[] = "C_esto es una frame_in con espacios para procesar";
//char frame_in[] = "SestoEsUnaCadenaCamelParaProcesarSestoEsUnaCadenaCamelParaProcesarSestoEsUnaCadenaCamelParaProcesar";

int main(void)
{
    int index_in = 1;
    int index_out = 1;
    int qty_words = 0;
    char command;
    char frame_out[150];
    int8_t error_flag = 0;

    command = *frame_in;
    *frame_out = *frame_in;
    error_flag = CHECK_OPCODE(command);

    while (!error_flag)
    {   
        int8_t character_count; 
        
        character_count = C3_FRAME_PROCESSOR_WordProcessor(&frame_in[index_in], &frame_out[index_out], command);
        
        
        //printf("character count: %d\n",character_count);
        if(character_count == INVALID_FRAME)
        {
            error_flag = ERROR_INVALID_DATA;
            break;
        }

        index_in += character_count;
        index_out += character_count;

        if(frame_in[index_in]==EOC){
            break;
        }
        
        if(++qty_words>MAX_WORDS_FRAME){
            error_flag = ERROR_INVALID_DATA;
            break;
        }

        if(command == 'S')
        {
            if (frame_in[index_in] == '_' || frame_in[index_in] == ' ')
            { //Transisión de palabra
                frame_out[index_out] = '_';
                index_in++;
                index_out++;
            }
            else if (CHECK_MAYUSCULA(frame_in[index_in]))
            {
                frame_out[index_out] = '_';
                index_out++;
            }
            else
            {
                error_flag = ERROR_INVALID_DATA;
                break;
            }
        }
        else 
        {
            if (frame_in[index_in] == '_' || frame_in[index_in] == ' ')
            { //CHEQUEO DE TRANSICIÓN DE PALABRA
                index_in++;

            }
            else if (!CHECK_MAYUSCULA(frame_in[index_in]))
            {
                error_flag = ERROR_INVALID_DATA;
                break;
            }
        }

    }

    if(error_flag){
        snprintf(frame_out, ERROR_MSG_SIZE, "E%.2d",error_flag-1);
        index_out = ERROR_MSG_SIZE;
    }else if('C' == command)
        {
            frame_out[1] = A_MINUSCULA(frame_out[1]);
        }

    frame_out[index_out] = EOC;



    //================================================

    for (int n = 0; n < strlen(frame_in); n++)
    {
        printf("%c", frame_in[n]);
    }

    printf("\n");

    for (int n = 0; n < strlen(frame_out); n++)
    {
        printf("%c", frame_out[n]);
    }

    printf("\nse contaron %d \n index_in=%d \n", qty_words, index_in);
    printf("error flag: %d \n", error_flag);
}



int8_t C3_FRAME_PROCESSOR_WordProcessor(char *word_in, char *word_out, char command)
{
    
    if (CHECK_MAYUSCULA(*word_in))
    {
        if (command == 'S')
        {
            *word_out = A_MINUSCULA(*word_in);
        }
        else
        {
            *word_out = *word_in;
        }
    }
    else if (CHECK_MINUSCULA(*word_in))
    {
        if (command == 'S')
        {
            *word_out = *word_in;
        }
        else
        {
            *word_out = A_MAYUSCULA(*word_in);
        }
    }
    else
    {
        return INVALID_FRAME; //PONER ETIQUETA INVALID FRAME
    }
    
    int8_t index_in = 1;

    while (CHECK_MINUSCULA(word_in[index_in]) && index_in < MAX_POR_PALABRA)
            {
                word_out[index_in] = word_in[index_in];
                index_in++;
            }

    if(index_in >= MAX_POR_PALABRA)
    {
        index_in = INVALID_FRAME;
    }
    
    return index_in;
}
