# tp-2020-1c-aislamientoSOcial

### Aspectos generales
El presente repositorio comprende al Trabajo Práctico Integrador concerniente a la materia Sistemas Operativos del 3er año de Ingeniería en Sistemas de Información - UTN FRBA.
La misión de este Trabajo Práctico comprende unificar los conocimientos teóricos y materializarlos a través de un sistema que se divide en cuatro módulos, y que intentar representar el comportamiento y manejo que realizan los Sistemas Operativos en general. 
Los cuatro módulos que lo componen son:

### Módulo Gameboy:
Permite interactuar con el sistema a través de una consola de comandos.

### Módulo Team:
Representa al planificador de procesos de un sistema operativo. Coordina la multiprogramación, organizando las colas de procesos "listos para ejectuar", enviándolos a la CPU para que ejecuten sus ráfagas o swapeándolos cuando corresponde.

### Modulo Broker: 
Representa el manejo de la memoria en un sistema operativo. Comprende la administración de memoria mediante particiones dinámicas con compactación y buddy system. Además administra colas de mensajes mediante programación multihilos. Coordina la comunicación entre todos los módulos del sistema de forma simultánea.

### Módulo Gamecard: 
Representa el file system de un sistema operativo. Permite la persistencia de los archivos en disco. Además de ser una abstracción de un sistema real, resulta necesario para el funcionamiento general del sistema, ya que ante la terminación abrupta del programa, es quien permite a los demás módulos recuperar el estado anterior al cierre inesperado.

### Tecnologías y conceptos aplicados
El desarrollo de todos los módulos fue realizado en lenguaje C, e implementa los siguientes conceptos:

Procesos, concurrencia, mútua exclusión, semáforos, planificación de procesos, threads, sockets, serialización, gestión de memoria, multiprogramación, sistema de archivos.


