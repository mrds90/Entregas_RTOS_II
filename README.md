# TP RTOSII - CESE15Co 2021

#### Fecha: 05/11/2021

#### Revisión: 1.0

#### Integrantes:
- DOMINGUEZ SHOCRÓN, Marcos Raúl - <mrds0690@gmail.com>
- MORZÁN, Pablo Javier - <pablomorzan@gmail.com>
- RÍOS, Martín Julián - <jrios@fi.uba.ar>


#### Justificación de arquitectura del TP
- ***Gestión de datos:*** aplicamos un patrón de asignación de objetos desde un pool memoria _(Quantum Leaps - QMPool v6.2.0)_, el cual crea en compilación diferentes colecciones estáticas de objetos del mismo tipo. Si se requiere un objeto o bloque de objetos se lo puede solicitar de la colección disponible y cuando ya no se lo precisa se puedo liberar, quedando nuevamente disponible. En nuestro caso utilizamos un pool de 5 bloques de 200 unidades de datos de tipo _uint_8t_. Esto está sujeto a modificaciones según sea necesario en el avance de la implementación.
Esto se podría haber resuelto con pvPortMalloc y vPortFree, pero sería propenso a la fragmentación del espacio de memoria por la frecuencia de pedido/liberación que se espera. Otra alternativa que no es aceptable es la asignación de memoria estática al momento de compilación ya que nos quedaríamos sin memoria con facilidad.

- ***Taréas:***
- ***Técnicas:***


#### Justificación del esquema de memoria dinámica utilizado
En la primera etapa de este trabajo práctico se decide utilizar un esquema de tipo **Heap4**. Las caracteristicas particulares que buscamos en este esquema son los siguientes:
- Fusión de espacios de memoria adyacentes liberados. Esto previene en gran medida la fragmentación del espacio de memoria.
- La capacidad de configurar el espacio de memoria necesario con **configTOTAL_HEAP_SIZE**
- Tener la capacidad de eliminar tareas, semaforos, colas, etc..

Si en un futuro se observa que el sistema puede resolverse sin eliminaciones se optaría por un esquema de tipo **Heap1** y en caso de no tener suficiente memoria para resolver el TP se elijiría **Heap5**.


#### Tabla con resumen de estructura de capas.

| Capa de Abstracción | Clase    | Métodos/Funciones  | Detalle        |
|:---:            |:---:         |:---:               |:---:           |
|                 |              |                    |                |
|  C2             |              |                    |                | 
|                 |              |                    |                |
|                 |              |                    |                |
|                 |              |                    |                |

