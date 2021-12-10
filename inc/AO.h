/*=============================================================================
 * Copyright (c) 2021, Franco Bucafusco <franco_bucafusco@yahoo.com.ar>
 *                     Martin N. Menendez <mmenendez@fi.uba.ar>
 * All rights reserved.
 * License: Free
 * Date: 2021/05/03
 * Version: v1.3
 *===========================================================================*/
/*=====[Avoid multiple inclusion - begin]====================================*/

#ifndef __AO_H__
#define __AO_H__

/*=====[Inclusions of public function dependencies]==========================*/

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "sapi.h"

#define N_QUEUE_AO      10

/*=====[C++ - begin]=========================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====[Definition macros of public constants]===============================*/

/*=====[Public function-like macros]=========================================*/

/*=====[Definitions of public data types]====================================*/

/*===== Tipo de dato activeObjectEvent_t ======================================
 *
 * (+) Descripci�n: Este tipo de dato se emplear� para que el objeto activo
 * reaccione a cada uno de los tipos de eventos disponibles. Consiste en:
 *
 * - UPPER_CASE: Para indicar la funci�n de mayusculizar.
 * - LOWER_CASE: Para indicar la funci�n de minusculizar.
 *
 *===========================================================================*/

/*===== Tipo de dato callBackActObj_t =========================================
 *
 * (+) Descripci�n: Tipo de dato para funciones callback que se ejecutar�n en
 * el objeto activo.
 *
 *===========================================================================*/

typedef void ( *callBackActObj_t )(void *caller_ao, void *data);

/*===== Objeto activeObjectEvent_t ============================================
 *
 * (+) Descripci�n: Este tipo de dato se utiliza para almacenar todos los
 * elementos asociados con el funcionamiento del objeto activo. Este tipo de
 * variable esta conformado por:
 *
 * - TaskFunction_t: Una variable para poder crear la tarea asociada al OA.
 * - QueueHandle_t: La cola del objeto activo en cuesti�n.
 * - callBackActObj_t: Callback que se deber� ejecutar en la tarea del OA.
 * - bool: Una variable de tipo booleana para saber si el objeto activo existe
 * o no.
 *
 *===========================================================================*/
typedef struct
{
    QueueHandle_t ReceiveQueue;
    QueueHandle_t TransmitQueue;        // Es una única cola para todos los AO creados que se desencolará en la tarea Tx
    callBackActObj_t callbackFunc;
    bool_t itIsAlive;
    bool_t isDestructible;
} activeObject_t;

/*=====[Prototypes (declarations) of public functions]=======================*/

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

void activeObjectEnqueue(activeObject_t *ao, void *value);

bool_t activeObjectOperationCreate(activeObject_t *ao, callBackActObj_t callback, QueueHandle_t response_queue, bool_t is_destroyable);


/*=====[Prototypes (declarations) of public interrupt functions]=============*/

/*=====[C++ - end]===========================================================*/

#ifdef __cplusplus
}
#endif

/*=====[Avoid multiple inclusion - end]======================================*/

#endif /* __AO_H__ */
