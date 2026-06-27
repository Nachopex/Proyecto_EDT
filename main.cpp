// Comando de compilacion:
// g++ -Wall -std=c++17 Load_Graph.cpp Experimento.cpp Graph.cpp Metricas.cpp main.cpp -o main

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include "Graph.h"
#include "Load_Graph.h"
#include "Experimento.h"
#include "Metricas.h"

using Clock = std::chrono::high_resolution_clock;

// ============================================================
// Ejecuta las metricas e impacto de aristas sobre un grafo
// ============================================================
void runExperiments(Graph& g, const std::string& datasetName, int reps) {
    std::cout << "\nIniciando calculo de metricas para " << datasetName << "...\n";
    auto results = Experimento::runAll(g, reps);

    size_t mem = g.memoryBytes();
    Experimento::printResults(results, datasetName, mem, g.numVertices(), g.numEdges());

    std::string csvName = "resultados_" + datasetName + ".csv";
    Experimento::saveCSV(results, csvName);
}

// ============================================================
// Funcion Principal
// ============================================================
int main(int argc, char* argv[]) {
    std::cout << "=== Metricas de Centralidad en Redes - Proyecto Semestral ===\n";
    std::cout << "Estructura de Datos - Universidad de Concepcion\n\n";

    // Validacion de argumentos de entrada
    if (argc < 2) {
        std::cerr << "Uso general: ./main <comando> [repeticiones] [max_aristas]\n";
        std::cerr << "Comandos soportados:\n";
        std::cerr << "  trade  -> Procesa los archivos de la Red de Comercio (Trade_Network/)\n";
        std::cerr << "  imdb   -> Procesa la Red de Actores (IMDb_actors_network/)\n";
        std::cerr << "\nEjemplos de ejecucion:\n";
        std::cerr << "  ./main trade 10\n"; 
        std::cerr << "  ./main imdb 10\n";
        return 1;
    }

    std::string comando = argv[1];

    // --------------------------------------------------------
    // FLUJO 1: RED DE COMERCIO MUNDIAL (Multiples archivos)
    // --------------------------------------------------------
    if (comando == "trade") {
        int reps = (argc >= 3) ? std::atoi(argv[2]) : 10;
        int maxEdges = (argc >= 4) ? std::atoi(argv[3]) : -1;

        std::vector<std::string> archivosTrade = {
            "Trade_Network/2000.net",
            "Trade_Network/2005.net",
            "Trade_Network/2010.net",
            "Trade_Network/2015.net",
            "Trade_Network/2018.net"
        };
        
        for (const std::string& archivoActual : archivosTrade) {
            std::cout << "\n\n//////////////////////////////////////////////////";
            std::cout << "\n/// INICIANDO ANALISIS DE: " << archivoActual;
            std::cout << "\n//////////////////////////////////////////////////\n";

            try {
                Graph g_trade = LoadGraph::loadTrade(archivoActual, maxEdges);
                std::cout << "Memoria estimada: " << g_trade.memoryBytes() / 1024 << " KB\n";

                // Extraer el año desde la ruta (asume el prefijo "Trade_Network/" de 14 caracteres)
                std::string anio = archivoActual.substr(14, 4);
                std::string nombreReporte = "trade_" + anio;

                runExperiments(g_trade, nombreReporte, reps);
                Experimento::edgeImpactExperiment(g_trade, nombreReporte);

            } catch (const std::exception& e) {
                std::cerr << "Error al procesar " << archivoActual << ": " << e.what() << "\n";
                std::cerr << "Verifique la existencia del directorio 'Trade_Network' y sus archivos.\n";
            }
        }
        return 0;
    } 
    
    // --------------------------------------------------------
    // FLUJO 2: RED DE ACTORES IMDB (Archivo unico)
    // --------------------------------------------------------
    else if (comando == "imdb") {
        int reps = (argc >= 3) ? std::atoi(argv[2]) : 10;
        int maxEdges = (argc >= 4) ? std::atoi(argv[3]) : -1;
        
        std::string filepath = "IMDb_actors_network/imdb_edgelist.csv";

        try {
            std::cout << "\n\n//////////////////////////////////////////////////";
            std::cout << "\n/// INICIANDO ANALISIS DE: " << filepath;
            std::cout << "\n//////////////////////////////////////////////////\n";

            Graph g_imdb = LoadGraph::loadIMDb(filepath, maxEdges);
            std::cout << "Memoria estimada: " << g_imdb.memoryBytes() / 1024 << " KB\n";

            runExperiments(g_imdb, "imdb_actors", reps);
            Experimento::edgeImpactExperiment(g_imdb, "imdb_actors");

        } catch (const std::exception& e) {
            std::cerr << "Error al cargar el dataset IMDb: " << e.what() << "\n";
            std::cerr << "Verifique la existencia del directorio 'IMDb_actors_network' y el archivo .csv.\n";
            return 1;
        }
        return 0;
    } 
    else {
        std::cerr << "Tipo de dataset no reconocido. Comandos validos: 'imdb' o 'trade'.\n";
        return 1;
    }

    return 0;
}