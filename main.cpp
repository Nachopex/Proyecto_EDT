#include "Graph.h"
#include "Metricas.h"
#include "Load_Graph.h"
#include "Experimento.h"
#include <iostream>
#include <chrono>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

// ============================================================
// Imprime las top-K vertices para una metrica dada
// ============================================================
template<typename MapType>
void printTopK(const MapType& metricMap, const Graph& g,
               const std::string& metricName, int k = 5) {
    std::vector<std::pair<double, int>> ranked;
    for (const auto& kv : metricMap)
        ranked.push_back({kv.second, kv.first});
    std::sort(ranked.rbegin(), ranked.rend());

    std::cout << "\nTop-" << k << " por " << metricName << ":\n";
    for (int i = 0; i < k && i < static_cast<int>(ranked.size()); i++) {
        std::string label = g.idToLabel(ranked[i].second);
        if (label.empty()) label = std::to_string(ranked[i].second);
        std::cout << "  " << std::setw(3) << (i + 1) << ". "
                  << std::setw(30) << std::left << label
                  << std::fixed << std::setprecision(6) << ranked[i].first << "\n";
    }
}

// ============================================================
// Carga un grafo y ejecuta todos los experimentos
// ============================================================
void runExperiments(const Graph& g, const std::string& datasetName, int reps = 10) {
    std::cout << "\n\n======================================\n";
    std::cout << "DATASET: " << datasetName << "\n";
    std::cout << "======================================\n";

    auto results = Experimento::runAll(g, reps);
    Experimento::printResults(results, datasetName,
                             g.memoryBytes(), g.numVertices(), g.numEdges());

    Experimento::saveCSV(results, "results_" + datasetName + ".csv");

    std::cout << "\n--- Top-5 Vertices por Metrica ---\n";
    printTopK(Metricas::degreeCentrality(g, false), g, "Degree Centrality (out)");
    printTopK(Metricas::degreeCentrality(g, true),  g, "Degree Centrality (in)");
    printTopK(Metricas::betweennessCentrality(g),   g, "Betweenness Centrality");
    printTopK(Metricas::closenessCentrality(g),     g, "Closeness Centrality");
    printTopK(Metricas::pageRank(g),                g, "PageRank");
    printTopK(Metricas::harmonicCentrality(g),      g, "Harmonic Centrality");
    printTopK(Metricas::eigenvectorCentrality(g),   g, "Eigenvector Centrality");

    double asp = Metricas::averageShortestPath(g);
    std::cout << "\nAverage Shortest Path Length: " << std::fixed
              << std::setprecision(6) << asp << "\n";
}

// ============================================================
// Grafo de prueba pequeno para verificar correctitud
// ============================================================
Graph buildTestGraph() {
    Graph g(false);
    for (const std::string v : {"A", "B", "C", "D", "E"}) g.addVertex(v);
    g.addEdge(g.labelToId("A"), g.labelToId("B"));
    g.addEdge(g.labelToId("A"), g.labelToId("C"));
    g.addEdge(g.labelToId("B"), g.labelToId("C"));
    g.addEdge(g.labelToId("B"), g.labelToId("D"));
    g.addEdge(g.labelToId("C"), g.labelToId("D"));
    g.addEdge(g.labelToId("C"), g.labelToId("E"));
    g.addEdge(g.labelToId("D"), g.labelToId("E"));
    g.addEdge(g.labelToId("E"), g.labelToId("A"));
    return g;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Metricas de Centralidad en redes - Proyecto Semestral ===\n";
    std::cout << "Estructura de Datos - Universidad de Concepcion\n\n";

    // --------------------------------------------------------
    // 1. Modo demo: solo si no se pasa dataset por linea de comandos
    // --------------------------------------------------------
    if (argc < 3) {
        std::cout << "--- Prueba de correctitud (grafo pequeno) ---\n";
        Graph testG = buildTestGraph();
        std::cout << "Vertices: " << testG.numVertices()
                  << ", Aristas: " << testG.numEdges() << "\n";

        printTopK(Metricas::degreeCentrality(testG, false), testG, "Degree (out)");
        printTopK(Metricas::betweennessCentrality(testG),   testG, "Betweenness");
        printTopK(Metricas::closenessCentrality(testG),     testG, "Closeness");
        printTopK(Metricas::pageRank(testG),                testG, "PageRank");
        printTopK(Metricas::harmonicCentrality(testG),      testG, "Harmonic");
        printTopK(Metricas::eigenvectorCentrality(testG),   testG, "Eigenvector");
        std::cout << "ASP: " << Metricas::averageShortestPath(testG) << "\n";

        Experimento::edgeImpactExperiment(testG, "TestGraph");
    }

    // --------------------------------------------------------
    // 2. Datasets reales
    // --------------------------------------------------------
    // Ejemplos con tus archivos:
    //   ./graph_analysis imdb_edgelist.csv imdb
    //   ./graph_analysis train_test_network.csv traffic

    if (argc >= 3) {
        std::string filepath = argv[1];
        std::string dtype    = argv[2];
        int reps = (argc >= 4) ? std::atoi(argv[3]) : 10;
        int maxEdges = (argc >= 5) ? std::atoi(argv[4]) : -1;

        auto t0 = Clock::now();
        Graph g(false);
        try {
            if (dtype == "imdb" || dtype == "imdb_edgelist") {
                g = LoadGraph::loadIMDb(filepath, maxEdges);
            } else if (dtype == "traffic" || dtype == "train_test" || dtype == "train_test_network") {
                g = LoadGraph::loadTrainTestNetwork(filepath, maxEdges);
            } else if (dtype == "trade") {
                int year = maxEdges;
                int tradeMaxEdges = (argc >= 6) ? std::atoi(argv[5]) : -1;
                g = LoadGraph::loadTrade(filepath, year, tradeMaxEdges);
            } else {
                bool weighted = (dtype == "weighted");
                bool directed = (dtype != "undirected");
                g = LoadGraph::loadEdgeList(filepath, weighted, directed, maxEdges);
            } 
        } catch (const std::exception& e) {
            std::cerr << "Error al cargar el dataset: " << e.what() << "\n";
            return 1;
        }

        auto t1 = Clock::now();
        double buildMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
        
        std::cout << "\nTiempo de construccion del grafo: "
                  << std::fixed << std::setprecision(3) << buildMs << " ms\n";
        std::cout << "Memoria estimada: " << g.memoryBytes() / 1024 << " KB\n";

        runExperiments(g, dtype, reps);
        Experimento::edgeImpactExperiment(g, dtype);
    } else {
        std::cout << "\n[INFO] Para usar con datasets reales:\n";
        std::cout << "  ./graph_analysis <archivo> <tipo> [reps] [maxEdges|ano] [maxEdgesTrade]\n";
        std::cout << "  Tipos: imdb | traffic | train_test | trade | edgelist | weighted\n";
        std::cout << "  Ejemplo: ./graph_analysis imdb_edgelist.csv imdb 10\n";
        std::cout << "  Ejemplo: ./graph_analysis imdb_edgelist.csv imdb 10 50000\n";
        std::cout << "  Ejemplo: ./graph_analysis train_test_network.csv traffic 10 50000\n";
    }

    return 0;
}