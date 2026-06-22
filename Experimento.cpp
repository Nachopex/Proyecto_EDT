#include "Experimento.h"
#include <chrono>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>
#include <sstream>

using Clock = std::chrono::high_resolution_clock;

namespace {
// Función que formatea segundos a HH:MM:SS
std::string formatSeconds(double seconds) {
    if (seconds < 0) seconds = 0;
    long long total = static_cast<long long>(seconds + 0.5);
    long long h = total / 3600;
    long long m = (total % 3600) / 60;
    long long s = total % 60;

    std::ostringstream oss;
    oss << std::setfill('0');
    if (h > 0) oss << h << ':';
    oss << std::setw(2) << m << ':'
        << std::setw(2) << s;
    return oss.str();
}

//Esta función imprime progreso en tiempo real
void printProgressLine(const std::string& name, int done, int total,
                       double elapsedMs, double etaMs) {
    int width = 24;
    double ratio = (total > 0) ? static_cast<double>(done) / total : 1.0;
    int filled = static_cast<int>(ratio * width);
    if (filled > width) filled = width;

    std::cout << '\r' << std::left << std::setw(26) << name << " [";
    for (int i = 0; i < width; ++i) std::cout << (i < filled ? '#' : '-');
    std::cout << "] " << std::setw(3) << static_cast<int>(ratio * 100.0) << "% "
              << done << "/" << total
              << " transcurrido: " << formatSeconds(elapsedMs / 1000.0)
              << " restante aprox: " << formatSeconds(etaMs / 1000.0)
              << std::flush;
}

} // namespace

// ============================================================
// Benchmark: corre una función métrica con repeticiones y reporta
// tiempo promedio, varianza, min y max
// ============================================================
RunResult Experimento::benchmark(
    const std::string& name,
    const Graph& g,
    std::function<void(const Graph&)> metricFn,
    int reps)
{
    std::vector<double> times(reps);
    auto tBenchmarkStart = Clock::now();

    for (int i = 0; i < reps; i++) {
        auto t0 = Clock::now();
        metricFn(g); // Ejecuta la métrica
        auto t1 = Clock::now();
        times[i] = std::chrono::duration<double, std::milli>(t1 - t0).count();

        // mostramos progreso 
        auto now = Clock::now();
        double elapsedMs = std::chrono::duration<double, std::milli>(now - tBenchmarkStart).count();
        double avgMs = elapsedMs / (i + 1);
        double etaMs = avgMs * (reps - (i + 1));
        printProgressLine(name, i + 1, reps, elapsedMs, etaMs);
    }

    // linea final del progreso 
    std::cout << "\r" << std::left << std::setw(26) << name
              << " [########################] 100% "
              << reps << "/" << reps
              << " transcurrido: " << formatSeconds(
                     std::chrono::duration<double, std::milli>(Clock::now() - tBenchmarkStart).count() / 1000.0)
              << " restante aprox: 00:00" << "        \n";

    //calculo de estadísticas
    double mean = std::accumulate(times.begin(), times.end(), 0.0) / reps;
    double var = 0.0;
    for (double t : times) var += (t - mean) * (t - mean);
    var /= reps;

    return RunResult{
        name,
        mean,
        var,
        std::sqrt(var),
        *std::min_element(times.begin(), times.end()),
        *std::max_element(times.begin(), times.end()),
        reps
    };
}

// ============================================================
// Corre las 7 métricas sobre el grafo dado
// ============================================================
std::vector<RunResult> Experimento::runAll(const Graph& g, int reps) {
    std::vector<RunResult> results;

    std::cout << "\nProgreso de metricas:\n";
    // Degree Centrality (out)
    std::cout << "  1/8 Degree Centrality (out)\n";
    results.push_back(benchmark("Degree Centrality (out)", g,
        [](const Graph& g) { Metricas::degreeCentrality(g, false); }, reps));

    // Degree Centrality (in)
    std::cout << "  2/8 Degree Centrality (in)\n";
    results.push_back(benchmark("Degree Centrality (in)", g,
        [](const Graph& g) { Metricas::degreeCentrality(g, true); }, reps));
    
    // Betweenness Centrality
    std::cout << "  3/8 Betweenness Centrality\n";
    results.push_back(benchmark("Betweenness Centrality", g,
        [](const Graph& g) { Metricas::betweennessCentrality(g); }, reps));

    // Closeness Centrality
    std::cout << "  4/8 Closeness Centrality\n";
    results.push_back(benchmark("Closeness Centrality", g,
        [](const Graph& g) { Metricas::closenessCentrality(g); }, reps));

    // PageRank
    std::cout << "  5/8 PageRank\n";
    results.push_back(benchmark("PageRank", g,
        [](const Graph& g) { Metricas::pageRank(g); }, reps));

    // Average Shortest Path
    std::cout << "  6/8 Average Shortest Path\n";
    results.push_back(benchmark("Average Shortest Path", g,
        [](const Graph& g) { Metricas::averageShortestPath(g); }, reps));

    // Harmonic Centrality
    std::cout << "  7/8 Harmonic Centrality\n";
    results.push_back(benchmark("Harmonic Centrality", g,
        [](const Graph& g) { Metricas::harmonicCentrality(g); }, reps));

    // Eigenvector Centrality
    std::cout << "  8/8 Eigenvector Centrality\n";
    results.push_back(benchmark("Eigenvector Centrality", g,
        [](const Graph& g) { Metricas::eigenvectorCentrality(g); }, reps));

    std::cout << "Procesamiento de metricas completado.\n";
    return results;
}

// ============================================================
// Experimentoo: impacto de añadir/quitar aristas en las métricas
// Prueba 3 tipos de aristas:
//   1. Entre nodos de alto grado (hubs)
//   2. Entre nodos de bajo grado (periféricos)  
//   3. Aleatoria
// ============================================================
void Experimento::edgeImpactExperiment(Graph& g, const std::string& datasetName) {
    std::cout << "\n=== Experimentoo de Impacto de Aristas: " << datasetName << " ===\n";
    bool undirectedMode = (datasetName == "imdb" || datasetName == "imdb_edgelist");

    // Calcular métricas base
    auto bcBase = Metricas::betweennessCentrality(g);
    auto ccBase = Metricas::closenessCentrality(g);
    auto prBase = Metricas::pageRank(g);
    double aspBase = Metricas::averageShortestPath(g);

    auto printDelta = [](const std::string& name, double before, double after) {
        double delta = after - before;
        std::cout << "  " << std::setw(30) << std::left << name
                  << " antes=" << std::fixed << std::setprecision(6) << before
                  << " despues=" << after
                  << " delta=" << (delta >= 0 ? "+" : "") << delta << "\n";
    };

    std::vector<int> verts = g.vertices();
    if (verts.size() < 4) {
        std::cout << "Grafo muy pequeño para el experimentoo\n";
        return;
    }

    // Ordenar vértices por grado saliente para elegir hubs y periféricos
    std::sort(verts.begin(), verts.end(), [&](int a, int b) {
        return g.neighbors(a).size() > g.neighbors(b).size();
    });

    int hub1 = verts[0], hub2 = verts[1];
    int peri1 = verts[verts.size()-2], peri2 = verts[verts.size()-1];

    // Arista aleatoria
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, static_cast<int>(verts.size())-1);
    int rand1, rand2;
    do {
        rand1 = verts[dist(rng)];
        rand2 = verts[dist(rng)];
    } while (rand1 == rand2 || g.hasEdge(rand1, rand2));

    auto getEdgeWeight = [&](int src, int dst) {
        double weight = 1.0;
        for (const Edge& e : g.neighbors(src)) {
            if (e.dest == dst) {
                weight = e.weight;
                break;
            }
        }
        return weight;
    };

    // Función auxiliar para medir impacto al añadir una arista
    auto testAddEdge = [&](const std::string& tipo, int src, int dst, double w = 1.0) {
        if (g.hasEdge(src, dst) || (undirectedMode && g.hasEdge(dst, src))) {
            std::cout << "  [" << tipo << "] La arista (" << src << "->" << dst << ") ya existe\n";
            return;
        }
        std::cout << "\n  [AÑADIR] " << tipo << " (" << src << " -> " << dst << ")\n";
        g.addEdge(src, dst, w);
        if (undirectedMode) g.addEdge(dst, src, w);

        auto bcNew = Metricas::betweennessCentrality(g);
        auto ccNew = Metricas::closenessCentrality(g);
        auto prNew = Metricas::pageRank(g);
        double aspNew = Metricas::averageShortestPath(g);

        // Reportar cambio en Average Shortest Path (medida global)
        printDelta("Avg Shortest Path (global)", aspBase, aspNew);

        // Reportar cambio en los nodos afectados
        double bcSrc = bcBase.count(src) ? bcBase.at(src) : 0;
        double bcDst = bcBase.count(dst) ? bcBase.at(dst) : 0;
        printDelta("Betweenness(src)", bcSrc, bcNew.at(src));
        printDelta("Betweenness(dst)", bcDst, bcNew.at(dst));
        printDelta("Closeness(src)", ccBase.at(src), ccNew.at(src));
        printDelta("Closeness(dst)", ccBase.at(dst), ccNew.at(dst));
        printDelta("PageRank(src)", prBase.at(src), prNew.at(src));
        printDelta("PageRank(dst)", prBase.at(dst), prNew.at(dst));

        // Deshacer el cambio
        g.removeEdge(src, dst);
        if (undirectedMode) g.removeEdge(dst, src);
    };

    // Función auxiliar para medir impacto al quitar una arista existente
    auto testRemoveEdge = [&](const std::string& tipo, int src, int dst) {
        if (!g.hasEdge(src, dst)) {
            std::cout << "  [" << tipo << "] La arista (" << src << "->" << dst << ") no existe\n";
            return;
        }
        // Guardar peso antes de quitar
        double savedWeight = getEdgeWeight(src, dst);
        double savedReverseWeight = undirectedMode ? getEdgeWeight(dst, src) : 1.0;

        std::cout << "\n  [QUITAR] " << tipo << " (" << src << " -> " << dst << ")\n";
        g.removeEdge(src, dst);
        if (undirectedMode) g.removeEdge(dst, src);

        auto bcNew = Metricas::betweennessCentrality(g);
        auto ccNew = Metricas::closenessCentrality(g);
        auto prNew = Metricas::pageRank(g);
        double aspNew = Metricas::averageShortestPath(g);

        printDelta("Avg Shortest Path (global)", aspBase, aspNew);
        printDelta("Betweenness(src)", bcBase.at(src), bcNew.at(src));
        printDelta("Betweenness(dst)", bcBase.at(dst), bcNew.at(dst));
        printDelta("Closeness(src)", ccBase.at(src), ccNew.at(src));
        printDelta("Closeness(dst)", ccBase.at(dst), ccNew.at(dst));
        printDelta("PageRank(src)", prBase.at(src), prNew.at(src));
        printDelta("PageRank(dst)", prBase.at(dst), prNew.at(dst));

        // Restaurar arista
        g.addEdge(src, dst, savedWeight);
        if (undirectedMode) g.addEdge(dst, src, savedReverseWeight);
    };

    // --- AÑADIR aristas ---
    std::cout << "\n--- Pruebas de ADICION de aristas ---\n";
    testAddEdge("Entre hubs (alto grado)", hub1, hub2);
    testAddEdge("Entre perifericos (bajo grado)", peri1, peri2);
    testAddEdge("Aleatoria", rand1, rand2);

    // --- QUITAR aristas ---
    std::cout << "\n--- Pruebas de ELIMINACION de aristas ---\n";
    // Para quitar, necesitamos aristas existentes. se eliminan desde hub y desde periférico
    if (!g.neighbors(hub1).empty())
        testRemoveEdge("Desde hub de alto grado", hub1, g.neighbors(hub1)[0].dest);
    
    if (!g.neighbors(peri1).empty())
        testRemoveEdge("Desde nodo periferico", peri1, g.neighbors(peri1)[0].dest);
    
    // quitar arista aleatoria existente
    std::uniform_int_distribution<int> dist2(0, static_cast<int>(verts.size())-1);
    for (int attempt = 0; attempt < 20; attempt++) {
        int rv = verts[dist2(rng)];
        if (!g.neighbors(rv).empty()) {
            testRemoveEdge("Aleatoria existente", rv, g.neighbors(rv)[0].dest);
            break;
        }
    }
}

// ============================================================
// Imprime tabla de resultados en consola
// ============================================================
void Experimento::printResults(const std::vector<RunResult>& results,
                               const std::string& datasetName,
                               size_t memBytes, int numVertices, int numEdges) {
    std::cout << "\n=================================================\n";
    std::cout << "Dataset: " << datasetName << "\n";
    std::cout << "Vertices: " << numVertices << " | Aristas: " << numEdges
              << " | Memoria: " << memBytes / 1024 << " KB\n";
    std::cout << "-------------------------------------------------\n";
    std::cout << std::left
              << std::setw(30) << "Metrica"
              << std::setw(12) << "Media(ms)"
              << std::setw(12) << "Var(ms2)"
              << std::setw(12) << "StdDev(ms)"
              << std::setw(10) << "Min(ms)"
              << std::setw(10) << "Max(ms)"
              << "\n";
    std::cout << "-------------------------------------------------\n";
    for (const auto& r : results) {
        std::cout << std::fixed << std::setprecision(4)
                  << std::setw(30) << std::left  << r.metricName
                  << std::setw(12) << r.meanMs
                  << std::setw(12) << r.varianceMs
                  << std::setw(12) << r.stddevMs
                  << std::setw(10) << r.minMs
                  << std::setw(10) << r.maxMs
                  << "\n";
    }
    std::cout << "=================================================\n";
}

// ============================================================
// Guarda resultados en CSV para incluir en el informe
// ============================================================
void Experimento::saveCSV(const std::vector<RunResult>& results,
                          const std::string& filename) {
    std::ofstream f(filename);
    f << "metrica,media_ms,varianza_ms2,stddev_ms,min_ms,max_ms,repeticiones\n";
    for (const auto& r : results) {
        f << std::fixed << std::setprecision(6)
          << r.metricName << ","
          << r.meanMs << ","
          << r.varianceMs << ","
          << r.stddevMs << ","
          << r.minMs << ","
          << r.maxMs << ","
          << r.repetitions << "\n";
    }
    std::cout << "Resultados guardados en: " << filename << "\n";
}
