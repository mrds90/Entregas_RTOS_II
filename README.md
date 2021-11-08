# TP RTOSII - CESE15Co 2021

#### Fecha: 05/11/2021

#### Versión: 1.1

#### Integrantes:
- DOMINGUEZ SHOCRÓN, Marcos Raúl - <mrds0690@gmail.com>
- MORZÁN, Pablo Javier - <pablomorzan@gmail.com>
- RÍOS, Martín Julián - <jrios@fi.uba.ar>

#### Tabla de versiones
| Versión | Fecha | Modificaciones |
|:--- |:---|:---|
| v 1.0 |  5/11/2021  | **Versión inicial: se entregan requisitos R_C2_1 a R_C2_9**  |
|  |  | - Se elige estructura de memoria dinámica | 
|  |  | - Se define estructura de capas y contexto de cada función según requisitos |
|  |  | - Se establece comunicación entre capas y funciones buscando mantener modularidad |
|  |  | - Se implementa recepción de datos en contexto de interrupción |
| v 1.1 |  8/11/2021  | **Se entregan requisitos R_C2_10 a R_C2_16**  |
|  |  | - Chequeo de validez de ID | 
|  |  | - Se prueba con 2 instancias a máxima velocidad (Se pierden 10% de datos |
|  |  | - Se modifica procesamiento de datos de entrada de tipo "if-else" por máquina de estados switch-case anidado con chequeo de SOM-EOM en C2_FRAME_CAPTURE_UartRxInit |
|  |  | - Se agregan secciones críticas al gestionar el pool en contexto de interrupción taskENTER_CRITICAL_FROM_ISR/taskEXIT_CRITICAL_FROM_ISR |
|  |  | - Se agrega lectura de UART en caso de acumulación en buffer |
|  |  | - Se valida CRC en recepción |
|  |  | - Se calcula y agrega CRC a dato de salida |
|  |  | - Se prueba con 4000 mensajes en una instancia a máxima velocidad y no se pierden datos |




#### Justificación de arquitectura del TP
- ***Gestión de datos:*** aplicamos un patrón de asignación de objetos desde un pool de memoria _(Quantum Leaps - QMPool v6.2.0)_, el cual crea diferentes colecciones estáticas de objetos del mismo tipo. Si se requiere un objeto o bloque de objetos se lo puede solicitar de la colección disponible, y cuando ya no se lo precisa se puede liberar, quedando nuevamente disponible. En nuestro caso utilizamos una estructura pero lo inicializamos a través de un puntero pedido por pvPortMalloc, por lo que queda establecido en zona de memoria dinámica; en esta versión se crean 7 bloques de 200 unidades de datos de tipo _uint_8t_, estos bloques funcionan como buffer de recepción de datos. Con esto buscamos que al crear una nueva instancia se pueda reservar un nuevo pool en tiempo de ejecución.
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
| C2 | FRAME_PACKER | C2_FRAME_PACKER_PrinterInit | Recibe como parámetro el contexto necesario para la tarea _C2_FRAME_PACKER_PrinterTask_ , carga el contexto en memoria dinámica y la crea pasandole estos datos a través de un puntero a esta. |  
| | | C2_FRAME_PACKER_ReceiverInit | Recibe como parámetro el contexto necesario para la tarea _C2_FRAME_PACKER_ReceiverTask_ , carga el contexto en memoria dinámica y la crea pasandole estos datos a través de un puntero a esta. |
| | | C2_FRAME_PACKER_PrinterTask | Recibe el conexto (uart, Queue y bloque pool), imprime el paquete devuelto por la capa 3 agregando ID y CRC de procesamiento y libera el bloque del pool | 
| | | C2_FRAME_PACKER_ReceiverTask | Recibe el conexto (uart, Queue y bloque pool) y a través de una máquina de estados toma decisiones sobre los datos recibidos. (Chequea SOM y EOM, CRC, ID, envía a cola, separa los datos del paquete). |
| --- | --- | --- | --- |
| C2 | FRAME_CAPTURE | C2_FRAME_CAPTURE_UartRxInit |Es la funcion que inicializa la UART. Recibe la UART para la transmisión, la funcion de callback para la ISR y un puntero a una estructura de tipo frame_capture_t que contiene el contexto necesario para procesar cada frame. | 
| | | C2_FRAME_CAPTURE_UartRxISR | Es la función de callback a ser llamada en cada interrupción UART. Se utiliza el contexto pasado en la estructura frame_capture_t para decidir que hacer con el dato recibido que generó la interrupción. |
| | | C2_FRAME_CAPTURE_CheckCRC | Recibe la trama y el frame de la misma, chequea que el CRC sea válido y compara el recibido con el calculado. Devuelve TRUE si la son iguales.|
| | | C2_FRAME_CAPTURE_Atoh | Recibe un puntero a un char de tipo ASCII y devuelve un uint8_t con este valor convertido a Hexadecimal.|
| | |  \* C2_FRAME_CAPTURE_ObjInit | Inicializa el frame para la captura. Recibe el puntero al pool y la variable de tipo uartMap_t por la que se establece la comunicación. |
| --- | --- | --- | --- |
| C3 | FRAME_PROCESSOR | C3_FRAME_PROCESSOR_Init |  Esta función recibe desde el main cual es la uart que se usa para la instancia, pide espacio para el pool e inicia la tarea para procesar el dato recibido enviandole un puntero al contexto recién generado. | 
| | | C3_FRAME_PROCESSOR_Task | Procesa el dato recibido, valida el frame y manda información formateada de vuelta a la capa 2 para agregar ID y CRC, y luego ser enviada por C2_FRAME_PACKER_PrinterTask |
| CAux | crc8 | crc8_init | devuelve 0xFF que se utiliza como semilla. No utilizada  |
| | | crc8_calc | función para cálculo de CRC. Se utiliza enviándole la semilla y un trama de datos. En la captura se el envía cada dato que se recibe, pasandole como semilla el dato de crc previo. En la trama de salida se calcula el crc de la trama completa pasando 0 como semilla. Es de tipo CRC8-CCITT, con polinomio normal |
| | qf_mem | QMPool_init |  Inicializa un bloque de memoria de tamaño fijo, proporcionándole la memoria del pool para administrar, el tamaño de esta memoria y del bloque. |
| | | QMPool_put | Recicla un bloque de memoria del pool poniendolo disponible nuevamente. Se modifica la función para poder ser llamada en contexto de interrupción, agregando la sección crítica por fuera del llamado |
| | | QMPool_get | Selecciona un bloque de memoria disponible del pool devolviendo un puntero a al inicio del bloque solicitado, si no queda mas espacio duvuelve puntero a NULL. Se modifica la función para poder ser llamada en contexto de interrupción, agregando la sección crítica por fuera del llamado |
| | | QMPool_getMin |  Devuelve la minima cantidad de bloques que ha tenido el pool desde su creación. no utilizada |

![](./images/frame_trip.gif)
