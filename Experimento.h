#pragma once
#include "Graph.h"
#include "Metricas.h"
#include <string>
#include <vector>
#include <functional>

// Resultado de una métrica corrida N veces
struct RunResult {
    std::string metricName;
    double meanMs;       // Tiempo promedio en ms
    double varianceMs;   // Varianza del tiempo
    int repetitions;
};

class Experimento {
public:
    // Corre una métrica `reps` veces y reporta estadísticas de tiempo
    static RunResult benchmark(
        const std::string& name,
        const Graph& g,
        std::function<void(const Graph&)> metricFn,
        int reps = 10
    );

    // Corre todas las métricas sobre un grafo y reporta resultados
    static std::vector<RunResult> runAll(const Graph& g, int reps = 10);

    // Experimento de añadir/quitar aristas y medir impacto en métricas
    // Prueba en distintas posiciones: arista entre nodos de alto grado,
    // nodos de bajo grado, y aleatoria
    static void edgeImpactExperiment(Graph& g, const std::string& datasetName);

    // Imprime tabla de resultados en consola
    static void printResults(const std::vector<RunResult>& results,
                              const std::string& datasetName,
                              size_t memBytes, int numVertices, int numEdges);

    // Guarda resultados en CSV
    static void saveCSV(const std::vector<RunResult>& results,
                        const std::string& filename);
};