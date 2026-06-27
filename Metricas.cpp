#include "Metricas.h"
#include <queue>
#include <stack>
#include <cmath>
#include <algorithm>
#include <limits>
#include <numeric>

// valor de infinito para distancias no alcanzables
static const double INF = std::numeric_limits<double>::infinity();

// ============================================================
// BFS: distancias no ponderadas desde un vertice fuente (para grafos no ponderados)
// Complejidad: O(V + E)
// ============================================================
std::unordered_map<int, double> Metricas::bfs(const Graph& g, int src) {
    std::unordered_map<int, double> dist;
    for (int v : g.vertices()) dist[v] = INF;
    dist[src] = 0.0;

    std::queue<int> q;
    q.push(src);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (const Edge& e : g.neighbors(u)) {
            if (dist[e.dest] == INF) {
                dist[e.dest] = dist[u] + 1.0;
                q.push(e.dest);
            }
        }
    }
    return dist;
}

// ============================================================
// Dijkstra: distancias ponderadas desde un vertice fuente (para grafos ponderados)
// Complejidad: O((V + E) log V)
// ============================================================
std::unordered_map<int, double> Metricas::dijkstra(const Graph& g, int src) {
    std::unordered_map<int, double> dist;
    for (int v : g.vertices()) dist[v] = INF;
    dist[src] = 0.0;

    // min-heap: (distancia, vertice)
    using P = std::pair<double, int>;
    std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto topPair = pq.top(); pq.pop();
        double d = topPair.first;
        int u = topPair.second;
        
        if (d > dist[u]) continue;
        for (const Edge& e : g.neighbors(u)) {
            double nd = dist[u] + e.weight;
            if (nd < dist[e.dest]) {
                dist[e.dest] = nd;
                pq.push({nd, e.dest});
            }
        }
    }
    return dist;
}

// Selecciona BFS o Dijkstra según tipo de grafo
std::unordered_map<int, double> Metricas::shortestPaths(const Graph& g, int src) {
    return g.isWeighted() ? dijkstra(g, src) : bfs(g, src);
}

// ============================================================
// 1. Degree Centrality
// DC(v) = grado(v) / (N - 1)
// Para grafos dirigidos: in-degree o out-degree según parámetro
// Complejidad: O(V + E)
// ============================================================
MetricMap Metricas::degreeCentrality(const Graph& g, bool inDegree) {
    MetricMap result;
    int N = g.numVertices();
    if (N <= 1) return result;
    // inicializar con 0.0 para todos los vértices
    for (int v : g.vertices()) result[v] = 0.0;

    if (!inDegree) {
        // Out-degree: contar aristas salientes
        for (int v : g.vertices())
            result[v] = static_cast<double>(g.neighbors(v).size()) / (N - 1);
    } else {
        // In-degree: contar aristas entrantes
        for (int v : g.vertices())
            for (const Edge& e : g.neighbors(v))
                // con arista v -> u incrementamos in-degree de u
                result[e.dest] += 1.0;
        // normalizamos con (N - 1)        
        for (int v : g.vertices())
            result[v] /= (N - 1);
    }
    return result;
}

// ============================================================
// 2. Betweenness Centrality
// Algoritmo de Brandes (2001)
// Para grafos no ponderados usa BFS; ponderados usa Dijkstra modificado
// Complejidad: O(V*E) no ponderado, O(V*E*log V) ponderado
// ============================================================
MetricMap Metricas::betweennessCentrality(const Graph& g, bool normalized) {
    MetricMap bc;
    for (int v : g.vertices()) bc[v] = 0.0;

    std::vector<int> verts = g.vertices();
    int N = static_cast<int>(verts.size());

    for (int s : verts) {
        // Stack de vértices en orden de finalización (inverso para acumulación)
        std::stack<int> S;
        // Predecesores de cada vértice en el camino más corto desde s
        std::unordered_map<int, std::vector<int>> pred;
        // Número de caminos más cortos desde s hasta cada vértice
        std::unordered_map<int, double> sigma;
        // Distancia desde s
        std::unordered_map<int, double> dist;

        // Inicialización para cada vértice con distancia infinita 
        for (int v : verts) {
            sigma[v] = 0.0;
            dist[v] = INF;
        }
        sigma[s] = 1.0;
        dist[s] = 0.0;
        // paso 1: encontrar camino más corto 
        if (g.isWeighted()) {
        // grafo ponderado: Dijkstra version Brandes ()
            using P = std::pair<double, int>;
            std::priority_queue<P, std::vector<P>, std::greater<P>> pq;
            pq.push({0.0, s});

            while (!pq.empty()) {
                auto topPair = pq.top();
                pq.pop();
                double dv = topPair.first;
                int v = topPair.second;
                
                if (dv > dist[v]) continue;

                S.push(v);
                for (const Edge& e : g.neighbors(v)) {
                    int w = e.dest;
                    double vwDist = dist[v] + e.weight;
                    
                    // si encontramos un camino más corto a w
                    if (vwDist < dist[w]) {
                        dist[w] = vwDist;
                        pq.push({vwDist, w});
                        pred[w].clear();
                        pred[w].push_back(v);
                        sigma[w] = sigma[v];
                    } else if (std::abs(vwDist - dist[w]) < 1e-12) {
                        // si encontramos otro camino más corto a w
                        pred[w].push_back(v);
                        sigma[w] += sigma[v];
                    }
                }
            }
        } else {
        // grafos no ponderados: BFS estándar    
            std::queue<int> Q;
            Q.push(s);

            while (!Q.empty()) {
                int v = Q.front(); Q.pop();
                S.push(v);
                for (const Edge& e : g.neighbors(v)) {
                    int w = e.dest;
                    // Primer descubrimiento de w
                    if (dist[w] == INF) {
                        Q.push(w);
                        dist[w] = dist[v] + 1.0;
                    }
                    // Camino más corto hasta w pasa por v
                    if (dist[w] == dist[v] + 1.0) {
                        sigma[w] += sigma[v];
                        pred[w].push_back(v);
                    }
                }
            }
        }

        // Acumulación de dependencias (backwards pass)
        std::unordered_map<int, double> delta;
        for (int v : verts) delta[v] = 0.0;

        while (!S.empty()) {
            int w = S.top(); S.pop();
            for (int v : pred[w]) {
                if (sigma[w] > 0.0) {
                    // acumulamos la dependencia de w en v
                    delta[v] += (sigma[v] / sigma[w]) * (1.0 + delta[w]);
                }
            }
            if (w != s) bc[w] += delta[w]; // no acumulamos para el vértice fuente
        }
    }

    // Normalización: dividir por (N-1)*(N-2) para grafos dirigidos
    if (normalized && N > 2) {
        double norm = 1.0 / ((N - 1.0) * (N - 2.0));
        for (auto& kv : bc) kv.second *= norm;
    }
    return bc;
}

// ============================================================
// 3. Closeness Centrality
// C(u) = (N-1) / sum_y d(u,y)
// Para grafos desconectados usa solo los vértices alcanzables
// Complejidad: O(V*(V+E)) con BFS, O(V*(V+E)*log V) con Dijkstra
// ============================================================
MetricMap Metricas::closenessCentrality(const Graph& g) {
    MetricMap result;
    int N = g.numVertices();

    // Calculamos distancias desde u a todos los demás
    for (int u : g.vertices()) {
        auto dist = shortestPaths(g, u);
        double sum = 0.0;
        int reachable = 0;

        // Sumamos solo distancias a vértices alcanzables 
        for (auto& kv : dist) {
            if (kv.second != INF && kv.first != u) {
                sum += kv.second;
                reachable++;
            }
        }
        if (sum > 0.0 && reachable > 0)
            // Normalización de Wasserman & Faust para grafos desconectados
            result[u] = (static_cast<double>(reachable) / (N - 1)) *
                        (static_cast<double>(reachable) / sum);
        else
            result[u] = 0.0; // vértice aislado o sin alcanzables tiene centralidad 0
    }
    return result;
}

// ============================================================
// 4. PageRank
// PR(u) = (1-d)/N + d * sum_{v->u} PR(v)/L(v)
// Power Iteration hasta convergencia o maxIter iteraciones
// Complejidad: O(k*(V+E)) donde k = iteraciones hasta convergencia
// ============================================================
MetricMap Metricas::pageRank(const Graph& g, double damping, int maxIter, double tol) {
    std::vector<int> verts = g.vertices();
    int N = static_cast<int>(verts.size());
    if (N == 0) return {};

    // Inicializar PR uniforme
    MetricMap pr, newPr;
    for (int v : verts) pr[v] = 1.0 / N;

    // Precalcular out-degree de cada vértice
    std::unordered_map<int, int> outDeg;
    for (int v : verts)
        outDeg[v] = static_cast<int>(g.neighbors(v).size());

    for (int iter = 0; iter < maxIter; iter++) {
        double dangling = 0.0; // masa de nodos sin aristas salientes (dangling nodes)
        for (int v : verts)
            if (outDeg[v] == 0) dangling += pr[v];

        // Inicializamos newPr con el término de teleportación
        for (int u : verts)
            newPr[u] = (1.0 - damping) / N + damping * dangling / N;

        // Distribución de PageRank de cada vértice a sus vecinos
        for (int v : verts) {
            if (outDeg[v] == 0) continue;
            double share = damping * pr[v] / outDeg[v];
            for (const Edge& e : g.neighbors(v))
                newPr[e.dest] += share;
        }

        // Verificar convergencia (usando norma L1)
        double diff = 0.0;
        for (int v : verts) diff += std::abs(newPr[v] - pr[v]);
        pr = newPr;
        if (diff < tol) break; //convergencia alcanzada
    }
    return pr;
}

// ============================================================
// 5. Average Shortest Path Length
// L = 1/(N*(N-1)) * sum_{i!=j} d(i,j)
// Solo considera pares alcanzables (grafos desconectados)
// Complejidad: O(V*(V+E)) con BFS, O(V*(V+E)*log V) con Dijkstra
// ============================================================
double Metricas::averageShortestPath(const Graph& g) {
    std::vector<int> verts = g.vertices();
    int N = static_cast<int>(verts.size());
    if (N <= 1) return 0.0;

    double total = 0.0;
    long long pairs = 0;

    // para cada vértice como fuente, calculamos distancias a todos los demás
    for (int src : verts) {
        auto dist = shortestPaths(g, src);
        // sumamos distancias a todos los vértices alcanzables
        for (int dst : verts) {
            if (dst != src && dist[dst] != INF) {
                total += dist[dst];
                pairs++;
            }
        }
    }
    return (pairs > 0) ? total / pairs : 0.0;
}

// ============================================================
// 6. Harmonic Centrality
// C_H(u) = sum_{j != u} 1/d(u,j)
// Maneja grafos desconectados: 1/inf = 0
// Complejidad: O(V*(V+E))
// ============================================================
MetricMap Metricas::harmonicCentrality(const Graph& g) {
    MetricMap result;
    for (int u : g.vertices()) {
        auto dist = shortestPaths(g, u);
        double sum = 0.0;
        // suma de los inersos de las distancias
        for (auto& kv : dist)
            if (kv.first != u && kv.second != INF && kv.second > 0.0)
                sum += 1.0 / kv.second; // 1/inf = 0 para vértices no alcanzables
        result[u] = sum;
    }
    return result;
}

// ============================================================
// 7. Eigenvector Centrality
// EC(u) = (1/lambda) * sum_{v in N(u)} EC(v)
// Power Iteration: multiplica iterativamente el vector por la matriz de adyacencia
// Complejidad: O(k*(V+E))
// ============================================================
MetricMap Metricas::eigenvectorCentrality(const Graph& g, int maxIter, double tol) {
    std::vector<int> verts = g.vertices();
    int N = static_cast<int>(verts.size());
    if (N == 0) return {};

    MetricMap ec, newEc;
    for (int v : verts) ec[v] = 1.0 / N; // Inicialización uniforme

    for (int iter = 0; iter < maxIter; iter++) {
        for (int v : verts) newEc[v] = 0.0;

        // Multiplicar: newEC[w] += EC[v] para cada arista v->w
        for (int v : verts)
            for (const Edge& e : g.neighbors(v))
                newEc[e.dest] += ec[v];

        // Normalizar por la norma L2 (aproxima el eigenvector dominante)
        double norm = 0.0;
        for (int v : verts) norm += newEc[v] * newEc[v];
        norm = std::sqrt(norm);
        if (norm == 0.0) break;
        for (int v : verts) newEc[v] /= norm;

        // Verificar convergencia
        double diff = 0.0;
        for (int v : verts) diff += std::abs(newEc[v] - ec[v]);
        ec = newEc;
        if (diff < tol) break; //convergencia alcanzada
    }
    return ec;
}