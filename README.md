# TP RTOSII - CESE15Co 2021

#### Fecha: 05/11/2021

#### Revisión: 1.0

#### Integrantes:
- DOMINGUEZ SHOCRÓN, Marcos Raúl - <mrds0690@gmail.com>
- MORZÁN, Pablo Javier - <pablomorzan@gmail.com>
- RÍOS, Martín Julián - <jrios@fi.uba.ar>


#### Justificación de arquitectura del TP
- ***Gestión de datos:*** aplicamos un patrón de asignación de objetos desde un pool de memoria _(Quantum Leaps - QMPool v6.2.0)_, el cual crea diferentes colecciones estáticas de objetos del mismo tipo. Si se requiere un objeto o bloque de objetos se lo puede solicitar de la colección disponible, y cuando ya no se lo precisa se puedo liberar, quedando nuevamente disponible. En nuestro caso utilizamos un pool pero lo inicializamos a través de un puntero pedido por pvPortMalloc, por lo que queda establecido en memoria zona de memoria dinámica, este consta de 5 bloques de 200 unidades de datos de tipo _uint_8t_, estos bloques funcionan como buffer de recepción de datos. Con esto buscamos que al crear una nueva instancia podamos reservar un nuevo pool en tiempo de compilación.
Esto se podría haber resuelto con asignación dinámica de cada dato (malloc/free), pero sería propenso a la fragmentación del espacio de memoria por la frecuencia de pedido/liberación que se espera. Otra alternativa que no es aceptable es la asignación de memoria estática en momento de compilación para el almacenamiento de cada dato, pero nos quedaríamos sin espacio de almacenamiento rápidamente.

- ***Taréas:*** se crean tareas como métodos de los objetos modularizados en cada archivo. A su vez estos pertenecen a una capa y no conocen los atributos ni métodos privados de los objetos de otras capas, la estructura de las mismas se resume en la Tabla 1. Por el momento no se eliminan y sólo será necesario en caso de terminar una instancia de comunicación. Así mismo, si se crea una nueva instancia  de una clase se crearán nuevas tareas.

- ***Técnicas:***


#### Justificación del esquema de memoria dinámica utilizado
En la primera etapa de este trabajo práctico se decide utilizar un esquema de tipo **Heap4**. Las caracteristicas particulares que buscamos en este esquema son los siguientes:
- Fusión de espacios de memoria adyacentes liberados. Esto previene en gran medida la fragmentación del espacio de memoria.
- La capacidad de configurar el espacio de memoria necesario con **configTOTAL_HEAP_SIZE**. Al crear instancias nuevas con el pool en memoria dinámica podría ser necesario modificar este parámetro.
- Tener la capacidad de eliminar tareas, semaforos, colas, etc..

Si en un futuro se observa que el sistema puede resolverse sin eliminaciones se optaría por un esquema de tipo **Heap1** para poder tener un sistema determinista; en caso de no tener suficiente memoria para resolver el TP se elijiría **Heap5**.


#### Tabla 1: Resumen de estructura de capas.
Se implementan las capas 2 y 3. La capa 1 está comprendida por SAPI y UART.

| Capa de Abstracción | Clase          | Métodos/Funciones              | Descripción    |
|:---:                |:---:           |:---:                           |:---:           |
|                     |                |  C2_FRAME_PACKER_PrinterTask   | Recibe el conexto (uart, Queue y bloque pool) e imprime el paquete devuelto por la capa 3 de procesamiento y libera el bloque del pool | 
|  C2                 |  FRAME_PACKER  |  C2_FRAME_PACKER_ReceiverInit  | Recive como parámetro el contexto necesario para la tarea _C2_FRAME_PACKER_ReceiverTask_ , carga el contexto en memoria dinámica y la crea pasandole estos datos a través de un puntero a esta. |
|                     |                |  C2_FRAME_PACKER_PrinterInit   | Recive como parámetro el contexto necesario para la tarea _C2_FRAME_PACKER_PrinterTask_ , carga el contexto en memoria dinámica y la crea pasandole estos datos a través de un puntero a esta. |
|                     |                |  C2_FRAME_PACKER_ReceiverTask  | Recibe el conexto (uart, Queue y bloque pool) y a través de una máquina de estados toma decisiones sobre los datos recibidos. (Chequea SOM y EOM, ªCRC, ªID, envía a cola, ªsepara los datos del paquete) - (ª)Aún no implementados.  |
| --- | --- | --- | --- |
|                     |                |  C2_FRAME_CAPTURE_UartRxInit   |                | 
|  C2                 |  FRAME_CAPTURE |  C2_FRAME_CAPTURE_UartRxISR    |                |
|                     |                |  *C2_FRAME_CAPTURE_ObjInit     |                |
| --- | --- | --- | --- |
|                     |                |  C3_FRAME_PROCESSOR_Init       |                | 
|  C3                 |FRAME_PROCESSOR |  C3_FRAME_PROCESSOR_Task       |                |

