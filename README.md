# Proyecto_Semestral_Estructura_de_datos
_Luis Martinez Neira_<br>
_2023427985_

_Valentina Serón Canales_<br>
_2020901589_

_Benjamin Jimenez Ruiz_<br>
_2022445645_

# Sobre el entregable
Este proyecto escrito en C++ implementa un Tipo de Dato Abstracto (ADT) Grafo altamente eficiente mediante listas de adyacencia para el análisis de redes complejas. Permite cargar 2 datasets distintos y calcular diversas métricas de centralidad para determinar la importancia de los vértices dentro del sistema

# Funcionalidades

A través de la ejecución en consola mediante parámetros, el programa permite:

1. Cálculo de Métricas: Ejecutar y medir el rendimiento de 7 medidas de centralidad distintas (Degree, Betweenness con algoritmo de Brandes, Closeness con normalización de Wasserman & Faust, PageRank, Average Shortest Path, Harmonic y Eigenvector Centrality).

2. Experimentación de Impacto: Evaluar dinámicamente cómo cambian las métricas globales y locales al añadir o eliminar aristas estratégicas dentro de la red.

3. Análisis de Rendimiento: Exportar de manera automática los tiempos de ejecución (media y varianza) de cada algoritmo a archivos .csv y medir la estimación de memoria RAM consumida por el grafo.

# Requisitos y Configuración

* Compilador: El código requiere un compilador que soporte el estándar C++17 (ej. GCC o Clang).
* Dataset de Comercio (Trade Network): Para analizar la red de comercio mundial, debe existir una carpeta llamada exactamente Trade_Network en el mismo directorio que los archivos fuente, conteniendo los archivos .net correspondientes a los años (ej. 2000.net, 2005.net).
* Dataset de Actores (IMDb): Para analizar la red de actores, debe existir una carpeta llamada IMDb_actors_network (o IMDb_actors_ network) en el mismo directorio, conteniendo el archivo imdb_edgelist.csv.
# Sobre la compilación y ejecución
Para probar este proyecto, se debe abrir la terminal en el directorio de los archivos fuente y compilar con el siguiente comando:
```bash
g++ -std=c++17 Load_Graph.cpp Experimento.cpp Graph.cpp Metricas.cpp main.cpp -o main
```
Para ejecutar el programa, se debe indicar el dataset a analizar y (opcionalmente) la cantidad de repeticiones para el cálculo o tambien las aristas maximas.

Para la ejecucion normal con imdb:
```bash
./main imdb
```
Para la ejecucion normal con Trade Network:
```bash
./main trade
```
Para la ejecucion con parametros opcionales
```bash
./main imdb repeticiones aristas
```
```bash
./main trade repeticiones aristas
```
