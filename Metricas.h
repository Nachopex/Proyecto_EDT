#pragma once
#include "Graph.h"
#include <unordered_map>
#include <vector>
#include <string>

// Resultado de una métrica por vértice
using MetricMap = std::unordered_map<int, double>;

// ----------------------------------------------------------
// Métricas de centralidad para grafos dirigidos
// ----------------------------------------------------------

class Metricas {
public:
    // ---------------------------------------------------------
    // 1. Degree Centrality
    // DC(v) = deg(v) / (N-1)
    // in_degree: cuenta aristas entrantes; out_degree: salientes
    // Complejidad: O(V + E)
    // ---------------------------------------------------------
    static MetricMap degreeCentrality(const Graph& g, bool inDegree = false);

    // ---------------------------------------------------------
    // 2. Betweenness Centrality
    // B(u) = sum_{s!=u!=t} sigma_st(u) / sigma_st
    // Algoritmo de Brandes: O(V * E) no ponderado, O(V * E * log V) ponderado
    // ---------------------------------------------------------
    static MetricMap betweennessCentrality(const Graph& g, bool normalized = true);

    // ---------------------------------------------------------
    // 3. Closeness Centrality
    // C(u) = (N-1) / sum_y d(u,y)
    // Complejidad: O(V * (V + E)) con BFS / O(V * E * log V) con Dijkstra
    // ---------------------------------------------------------
    static MetricMap closenessCentrality(const Graph& g);

    // ---------------------------------------------------------
    // 4. PageRank
    // PR(u) = (1-d)/N + d * sum_{v->u} PR(v)/L(v)
    // d = damping factor (0.85 por defecto)
    // Complejidad: O(k * (V + E)) donde k = iteraciones
    // ---------------------------------------------------------
    static MetricMap pageRank(const Graph& g, double damping = 0.85, int maxIter = 100, double tol = 1e-6);

    // ---------------------------------------------------------
    // 5. Average Shortest Path Length (medida global)
    // L = 1/(N*(N-1)) * sum_{i!=j} d(i,j)
    // Complejidad: O(V * (V + E)) con BFS / O(V * E * log V) con Dijkstra
    // ---------------------------------------------------------
    static double averageShortestPath(const Graph& g);

    // ---------------------------------------------------------
    // 6. Harmonic Centrality (medida adicional 1)
    // C_H(u) = sum_{j != u} 1/d(u,j)
    // Ventaja sobre closeness: maneja grafos desconectados
    // Complejidad: O(V * (V + E))
    // ---------------------------------------------------------
    static MetricMap harmonicCentrality(const Graph& g);

    // ---------------------------------------------------------
    // 7. Eigenvector Centrality (medida adicional 2)
    // EC(u) = (1/lambda) * sum_{v in N(u)} EC(v)
    // Implementado con Power Iteration
    // Complejidad: O(k * (V + E))
    // ---------------------------------------------------------
    static MetricMap eigenvectorCentrality(const Graph& g, int maxIter = 100, double tol = 1e-6);

private:
    // BFS desde src, retorna distancias (inf si no alcanzable)
    static std::unordered_map<int, double> bfs(const Graph& g, int src);

    // Dijkstra desde src (para grafos ponderados)
    static std::unordered_map<int, double> dijkstra(const Graph& g, int src);

    // Elige BFS o Dijkstra según si el grafo es ponderado
    static std::unordered_map<int, double> shortestPaths(const Graph& g, int src);
};