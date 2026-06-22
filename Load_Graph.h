#pragma once
#include "Graph.h"
#include <string>

// Carga los datasets en el formato esperado
class LoadGraph {
public:
    // IMDb Actors Network
    // Formato CSV: actor1,actor2,num_peliculas_compartidas
    // Grafo no dirigido -> se cargan ambas direcciones
    static Graph loadIMDb(const std::string& filepath, int maxEdges = -1);

    // Train/Test Network
    // Formato CSV: src_ip,...,dst_ip,...
    // Se construye un grafo dirigido entre IP origen y destino
    static Graph loadTrainTestNetwork(const std::string& filepath, int maxEdges = -1);

    // World Trade Network
    // Formato CSV: origen,destino,ano,valor
    // Grafo dirigido y ponderado (valor de importacion)
    static Graph loadTrade(const std::string& filepath, int year = -1, int maxEdges = -1);

    // Formato generico edge-list: src dst [weight]
    static Graph loadEdgeList(const std::string& filepath, bool weighted = false,
                               bool directed = true, int maxEdges = -1);
};
