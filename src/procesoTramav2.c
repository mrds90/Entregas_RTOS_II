#include <string.h>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define EOC '\0'
#define CHECK_MINUSCULA(x) ((x >= 'a' && x <= 'z') ? TRUE : FALSE)
#define CHECK_MAYUSCULA(x) ((x >= 'A' && x <= 'Z') ? TRUE : FALSE)
#define CHECK_NUMERO(x)    ((x >= '0' && x <= '9') ? TRUE : FALSE)
#define A_MAYUSCULA(x)      (x - 32)
#define A_MINUSCULA(x)      (x + 32)
#define MAX_POR_PALABRA     10
#define MAX_WORDS_FRAME     15


//char cadena[] = "Esto_Es_Unaaaaa_Pablopablopablopablopablo_Pascal_Para_Procesar_Agregando_Mas_Palabras_Para_Llegar_A_Las_Quince_Ehhhh_Y_Algo_Mas";
//char cadena[] = "estoEsUnaCadenaCamelParaProcesar";
//char cadena[] = "esto_es_una_cadena_snake_para_procesar";
char cadena[] = "esto es una cadena con espacios para procesar";

int main (void) {

int i=0;
int j=0;
int qty_words=0;
int error_flag = 0;
int countChar=0;
char comando='S';
char cadenaOut[150];
error_flag = FALSE;

memset(cadenaOut,0,strlen(cadenaOut));

switch(comando){
        case 'P':       //======================== TRANSFORMAR A PASCAL CASE =========================

            if(CHECK_MAYUSCULA(cadena[i])){
                cadenaOut[j] = cadena[i];
                i++;
                j++;
                countChar++;
                
            } else if (CHECK_MINUSCULA(cadena[i])){
                cadenaOut[j] = A_MAYUSCULA(cadena[i]);
                i++;
                j++;
                countChar++;
                
            } else {
                error_flag = 1;
            }

            while((CHECK_MINUSCULA(cadena[i])) && (countChar < MAX_POR_PALABRA)){
                cadenaOut[j] = cadena[i];
                i++;
                j++;
                countChar++;
            }

            qty_words++;

            while(cadena[i]!=EOC){
                
                countChar=0;

                if(cadena[i] == '_' || cadena[i] == ' '){       //CHEQUEO DE TRANSICIÓN DE PALABRA 
                    cadenaOut[j] = '_';
                    i++;
                    j++;

                    if(CHECK_MAYUSCULA(cadena[i])){
                        cadenaOut[j]=cadena[i];
                        i++;
                        j++;
                        countChar++;
                    } else if(CHECK_MINUSCULA(cadena[i])){
                        cadenaOut[j] = A_MAYUSCULA(cadena[i]);
                        i++;
                        j++;
                        countChar++;
                    } else {
                        error_flag = 1; //ERROR DE CARACTER DE TRAMA
                }

                } else if(CHECK_MAYUSCULA(cadena[i])){
                    cadenaOut[j] = '_';
                    j++;
                    cadenaOut[j] = cadena[i];
                    j++;
                    i++;
                    countChar++;

                } else{
                    error_flag = 3; // ERROR DE CARACTER DE TRANSICIÓN
                    }   

                while((CHECK_MINUSCULA(cadena[i])) && (countChar < MAX_POR_PALABRA)){
                    cadenaOut[j] = cadena[i];
                    i++;
                    j++;
                    countChar++;
                }

                qty_words++;

                if(countChar >= MAX_POR_PALABRA){
                    error_flag = 4;
                }    
                
                if(qty_words>=MAX_WORDS_FRAME){
                    error_flag = 5;
                }

                if(error_flag > 0){
                    //Envío el error
                    break;
                }
            }
            cadenaOut[j]=EOC;

            break;

        case 'C':    //======================== TRANSFORMAR A CAMEL CASE =========================
            
            if(CHECK_MAYUSCULA(cadena[0])){
                cadenaOut[0] = A_MINUSCULA(cadena[0]);
                i++;
                j++;
                countChar++;
                
            } else if (CHECK_MINUSCULA(cadena[0])){
                cadenaOut[0] = (cadena[0]);
                i++;
                j++;
                countChar++;
                
            } else {
                error_flag = 1;
            }

            while((CHECK_MINUSCULA(cadena[i]) && (countChar<MAX_POR_PALABRA))){
                cadenaOut[j] = cadena[i];
                i++;
                j++;
                countChar++;
            }

            qty_words++;

            while(cadena[i]!=EOC){
                
                countChar=0;
                
                if(cadena[i] == '_' || cadena[i] == ' '){       //========Transisión de palabra ======
                    i++;

                    if(CHECK_MAYUSCULA(cadena[i])){
                        cadenaOut[j]=cadena[i];
                        i++;
                        j++;
                        countChar++;
                    } else if(CHECK_MINUSCULA(cadena[i])){
                        cadenaOut[j] = A_MAYUSCULA(cadena[i]);
                        i++;
                        j++;
                        countChar++;
                    } else {
                        error_flag = 2;
                    } 
                } else if(CHECK_MAYUSCULA(cadena[i])){
                    cadenaOut[j] = cadena[i];
                    j++;
                    i++;
                    countChar++;
                } else{
                    error_flag = 3;
                }
            
                while((CHECK_MINUSCULA(cadena[i])) && (countChar < MAX_POR_PALABRA)){
                    cadenaOut[j] = cadena[i];
                    i++;
                    j++;
                    countChar++;
                }

                qty_words++;

                if(countChar >= MAX_POR_PALABRA){
                    error_flag = 4;
                }    

                if(qty_words>=MAX_WORDS_FRAME){
                    error_flag = 5;
                }
                
                if(error_flag > 0){
                    //Envío el error
                    break;
                }
            }
            cadenaOut[j]=EOC;

            break;

        case 'S': //======================== TRANSFORMAR A SNAKE CASE =========================
            
            if(CHECK_MAYUSCULA(cadena[i])){
                cadenaOut[j] = A_MINUSCULA(cadena[i]);
                i++;
                j++;
                countChar++;
                
            } else if (CHECK_MINUSCULA(cadena[i])){
                cadenaOut[j] = (cadena[i]);
                i++;
                j++;
                countChar++;
                
            } else {
                error_flag = 1;
            }
           
            while((CHECK_MINUSCULA(cadena[i]) && (countChar < MAX_POR_PALABRA))){
                cadenaOut[j] = cadena[i];
                i++;
                j++;
                countChar++;
            }

            qty_words++;

            while(cadena[i]!=EOC){
                
                countChar=0;

                if(cadena[i] == '_' || cadena[i] == ' '){       //Transisión de palabra 
                    cadenaOut[j] = '_';
                    i++;
                    j++;

                    if(CHECK_MAYUSCULA(cadena[i])){
                        cadenaOut[j]=A_MINUSCULA(cadena[i]);
                        i++;
                        j++;
                        countChar++;
                    } else if(CHECK_MINUSCULA(cadena[i])){
                        cadenaOut[j] = (cadena[i]);
                        i++;
                        j++;
                        countChar++;
                    } else {
                        error_flag = 2;
                    } 
                } else if(CHECK_MAYUSCULA(cadena[i])){
                    cadenaOut[j] = '_';
                    j++;
                    countChar++;
                    cadenaOut[j] = A_MINUSCULA(cadena[i]);
                    j++;
                    i++;
                    countChar++;
                } else{
                    error_flag = 3;
                }

                while(CHECK_MINUSCULA(cadena[i]) && countChar < MAX_POR_PALABRA){
                    cadenaOut[j] = cadena[i];
                    i++;
                    j++;
                    countChar = countChar + 1;
                }

                qty_words++;

                if(countChar >= MAX_POR_PALABRA){
                    error_flag = 4;
                }    

                if(qty_words>=MAX_WORDS_FRAME){
                    error_flag = 5;
                }
                
                if(error_flag > 0){
                    //Envío el error
                    break;
                }

            }

            cadenaOut[j]=EOC;

            break;

        default:
            printf("default");
    }




for(int n=0;n<strlen(cadena);n++){
    printf("%c", cadena[n]);
}

printf("\n");

for(int n=0;n<strlen(cadenaOut);n++){
    printf("%c", cadenaOut[n]);
}

printf("\nse contaron %d \n i=%d \n", qty_words, i);
printf("error flag: %d \n", error_flag);

}








            

