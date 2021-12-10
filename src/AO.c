/*=============================================================================
 * Copyright (c) 2021, Franco Bucafusco <franco_bucafusco@yahoo.com.ar>
 *                     Martin N. Menendez <mmenendez@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2021/05/03
 * Version: v1.3
 *===========================================================================*/
/*=====[Inclusion of own header]=============================================*/

#include "AO.h"

/*=====[Inclusions of private function dependencies]=========================*/

/*=====[Definition macros of private constants]==============================*/

/*=====[Private function-like macros]========================================*/

/*=====[Definitions of private data types]===================================*/

/*=====[Definitions of external public global variables]=====================*/

/*=====[Definitions of public global variables]==============================*/

/*=====[Definitions of private global variables]=============================*/

/*=====[Prototypes (declarations) of private functions]======================*/

/*===== Tarea activeObjectTask()===============================================
 *
 * (+) Descripci�n: Esta es la tarea asociada al objeto activo. Leer� datos de
 * la cola del objeto y cuando los procese, se ejecutar� el callback asociado.
 *
 * (+) Recibe: Un puntero del tipo "void" por donde se enviar� el puntero al
 * objeto activo.
 *
 * (+) Devuelve: Nada.
 *
 *===========================================================================*/
static void activeObjectTask(void *pvParameters);


/*===== Funci�n activeObjectCreate()===========================================
 *
 * (+) Descripci�n: Esta funci�n se encarga de crear el objeto activo; es decir,
 * crear su cola de procesamiento y su tarea asociada. Adicionalmente, se le
 * asignar� una funci�n de callback que es la que se ejecutar� en la tarea.
 *
 * (+) Recibe: Un puntero del tipo "activeObject_t" al objeto activo y el
 * evento del tipo "activeObjectEvent_t". Adicionalmente, se le debe pasar el
 * nombre de la tarea asociada al objeto activo que se va a crear, del tipo
 * "TaskFunction_t".
 *
 * (+) Devuelve: True o False dependiendo de si el objeto activo se cre�
 * correctamente o no.
 *
 *===========================================================================*/

static bool_t activeObjectCreate(activeObject_t *ao, callBackActObj_t callback, QueueHandle_t queue);

/*=====[Implementations of public functions]=================================*/

/*===== Funci�n activeObjectCreate()===========================================
 *
 * (+) Descripci�n: Esta funci�n se encarga de crear el objeto activo; es decir,
 * crear su cola de procesamiento y su tarea asociada. Adicionalmente, se le
 * asignar� una funci�n de callback que es la que se ejecutar� en la tarea.
 *
 * (+) Recibe: Un puntero del tipo "activeObject_t" al objeto activo y el
 * evento del tipo "activeObjectEvent_t".
 *
 * (+) Devuelve: True o False dependiendo de si el objeto activo se cre�
 * correctamente o no.
 *
 *===========================================================================*/

bool_t activeObjectCreate(activeObject_t *ao, callBackActObj_t callback, QueueHandle_t queue) {
    // Una variable local para saber si hemos creado correctamente los objetos.
    BaseType_t retValue = pdFALSE;

    // Creamos la cola asociada a este objeto activo.
    if(ao->ReceiveQueue == NULL) {
        ao->ReceiveQueue = xQueueCreate(N_QUEUE_AO, sizeof(void *));
    }
    ao->TransmitQueue = queue;

    // Si la cola se cre� sin inconvenientes.
    if (ao->ReceiveQueue != NULL) {
        // Asignamos el callback al objeto activo.
        ao->callbackFunc = callback;

        // Creamos la tarea asociada al objeto activo. A la tarea se le pasar� el objeto activo como par�metro.
        retValue = xTaskCreate(activeObjectTask, ( const char * )"Task For AO", configMINIMAL_STACK_SIZE * 4, ao, tskIDLE_PRIORITY + 1, NULL);
    }

    // Chequeamos si la tarea se cre� correctamente o no.
    if (retValue == pdPASS) {
        // Cargamos en la variable de estado del objeto activo el valor "true" para indicar que se ha creado.
        ao->itIsAlive = TRUE;

        // Devolvemos "true" para saber que el objeto activo se instanci� correctamente.
        return(TRUE);
    }
    else {
        // Caso contrario, devolvemos "false".
        return(FALSE);
    }
}

/*===========================================================================*/

/*===== Tarea activeObjectTask()===============================================
 *
 * (+) Descripci�n: Esta es la tarea asociada al objeto activo. Leer� datos de
 * la cola del objeto y cuando los procese, se ejecutar� el callback asociado.
 *
 * (+) Recibe: Un puntero del tipo "void" por donde se enviar� el puntero al
 * objeto activo.
 *
 * (+) Devuelve: Nada.
 *
 *===========================================================================*/

void activeObjectTask(void *pvParameters) {
    // Una variable para evaluar la lectura de la cola.
    BaseType_t retQueueVal;

    // Una variable local para almacenar el dato desde la cola.

    // Obtenemos el puntero al objeto activo.
    activeObject_t *actObj = ( activeObject_t * ) pvParameters;

    // Cuando hay un evento, lo procesamos.
    while (TRUE) {
        void *auxValue;
        // Verifico si hay elementos para procesar en la cola. Si los hay, los proceso.
        if (uxQueueMessagesWaiting(actObj->ReceiveQueue)) {
            // Hago una lectura de la cola.
            retQueueVal = xQueueReceive(actObj->ReceiveQueue, &auxValue, portMAX_DELAY);

            // Si la lectura fue exitosa, proceso el dato.
            if (retQueueVal) {
                // Llamamos al callback correspondiente en base al comando que se le pas�.
                /* TODO:INFO a la funcion llamante le mando el ao que la llamo coo referenca porq
                   es necesario */
                (actObj->callbackFunc)(actObj,  auxValue);
            }
        }

        // Caso contrario, la cola est� vac�a, lo que significa que debo eliminar la tarea.
        else if (actObj->isDestructible){
            // Cambiamos el estado de la variable de estado, para indicar que el objeto activo no existe m�s.
            actObj->itIsAlive = FALSE;

            // Borramos la cola del objeto activo.
            vQueueDelete(actObj->ReceiveQueue);
            actObj->ReceiveQueue = NULL;
            // Y finalmente tenemos que eliminar la tarea asociada (suicidio).
            vTaskDelete(NULL);
        }
    }
}

/*===========================================================================*/

/*===== Funci�n activeObjectEnqueue()==========================================
 *
 * (+) Descripci�n: Esta funci�n se encargar� de ingresar en la cola del objeto
 * activo un evento que deber� procesarse.
 *
 * (+) Recibe: Un puntero del tipo "activeObject_t" por donde se enviar� el
 * puntero al objeto activo y un puntero a "char" donde se le pasar� el dato a
 * encolar.
 *
 * (+) Devuelve: Nada.
 *
 *===========================================================================*/

void activeObjectEnqueue(activeObject_t *ao, void *value) {
    // Y lo enviamos a la cola.
    xQueueSend(ao->ReceiveQueue, &value, 0);
}

/*===========================================================================*/

bool_t activeObjectOperationCreate(activeObject_t *ao, callBackActObj_t callback, QueueHandle_t response_queue, bool_t is_destroyable) {
    /* cargo miembro que no estaba */
    ao->isDestructible = is_destroyable;
    /* creo oa padre */
    return (activeObjectCreate(ao, callback, response_queue));
}
