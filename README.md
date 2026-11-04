
# Torneo de Komikan Concurrente

**Tarea 2 — Sistemas Operativos | Universidad Diego Portales**

Simulador concurrente de partidas de Komikan desarrollado en C++17 para sistemas Unix/Linux. Organiza jugadores por parejas (Puma y Perros), asigna hasta M mesas simultáneas y ejecuta cada partida sin turnos estrictos usando hilos POSIX.

Hecho por: Javier Santibáñez - 219584314.

## Requisitos

- Linux o entorno Unix con soporte para POSIX threads (por ejemplo, Ubuntu/WSL).
- Compilador g++ con soporte C++17 y make (recomendado).
- No requiere bibliotecas externas.

## Compilación

Desde la raíz del repositorio:

```bash
make
```

Equivalente manual:

```bash
g++ -Wall -Wextra -std=c++17 -pthread Codigous/main.cpp Codigous/torneo.cpp Codigous/mapa.cpp Codigous/mesa.cpp Codigous/matchmaker.cpp Codigous/configuracion.cpp -o torneo
```

Para eliminar el ejecutable:

```bash
make clean
```

## Ejecución

```bash
./torneo N M [config.txt]
```

- N: cantidad de jugadores.
- M: máximo de mesas simultáneas.
- config.txt: archivo de configuración opcional.

Ejemplos:

```bash
./torneo 4 2
./torneo 6 2 config.txt
./torneo 2 1 config_abandono.txt
./torneo 20 3
```

Si N es impar, un jugador queda sin pareja. Los emparejamientos pueden variar por la llegada concurrente.

Las partidas no tienen límite artificial de tiempo. Para detener el torneo, utilizar Ctrl+C.

## Configuración

El archivo utiliza el formato CLAVE=VALOR. Se permiten comentarios iniciados por #.

```ini
MIN_DELAY=50
MAX_DELAY=300
SEED=12345

PUMA=REY
PERROS=13,14,15,16,17,18,19,20,21,22,23,24
```

Parámetros disponibles:

| Parámetro | Descripción |
| --- | --- |
| MIN_DELAY | Retardo mínimo por movimiento (ms) |
| MAX_DELAY | Retardo máximo por movimiento (ms) |
| SEED | Semilla aleatoria |
| PUMA | Posición inicial del Puma |
| PERROS | Doce posiciones iniciales diferentes |
| ABANDONO | NINGUNO, PUMA o PERROS |
| ABANDONO_MS | Momento del abandono simulado (ms) |

El tablero utiliza posiciones del 0 al 24 y tres posiciones especiales: TopL, TopR y REY.

La configuración se valida antes de comenzar. Los valores incorrectos, posiciones repetidas o archivos inexistentes producen un mensaje de error.

## Arquitectura

### Matchmaker

Recibe jugadores mediante hilos concurrentes, los almacena en una cola FIFO y forma parejas de Puma y Perros.

La cola está protegida mediante un mutex POSIX. Cuando no existen jugadores suficientes, las mesas esperan pasivamente utilizando pthread_cond_wait().

El sistema crea hasta M trabajadores de mesa reutilizables.

### Mesa

Cada Mesa mantiene:

- Un tablero independiente.
- Un hilo Juez.
- Un hilo para el Puma.
- Doce hilos para los Perros.
- Un mutex para sincronizar movimientos.
- Una variable de condición para notificar al Juez.

El mutex expansionDeDominio protege la consulta, validación y actualización del tablero.

### Mapa

Representa las 28 posiciones del tablero y valida los movimientos legales y las capturas.

El Puma gana después de capturar seis Perros. Los Perros ganan cuando dejan al Puma sin movimientos válidos.

### SIGINT

Un hilo vigilante recibe SIGINT mediante sigwait().

La bandera atómica zaWarudo notifica la interrupción a las mesas activas. Los hilos terminan ordenadamente antes de liberar los recursos de sincronización.

## Pruebas

Comandos sugeridos:

```bash
./torneo 2 1
./torneo 5 2 config_abandono.txt
./torneo 6 2 config.txt
./torneo 20 3
```

En el último caso, presionar Ctrl+C mientras existen partidas activas.

El archivo config_abandono.txt permite comprobar la victoria por abandono mediante un evento artificial.

## Limitaciones conocidas

- El abandono simulado no detecta automáticamente fallos inesperados de hilos o desconexiones reales.
- La salida de distintas mesas puede entremezclarse en consola.
- Los movimientos aleatorios pueden producir partidas prolongadas o livelock.
- Los jugadores se incorporan en tandas de hasta 128 hilos para controlar el consumo de recursos.
- La geometría de algunas capturas del trono requiere validación final contra el enunciado.

## Documentación

El archivo informe.pdf contiene el análisis del esquema de sincronización, los riesgos de concurrencia y las decisiones de diseño.

La versión editable se encuentra en informe.tex.
