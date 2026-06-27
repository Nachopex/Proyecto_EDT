#pragma once
#include "Graph.h"
#include <string>

class LoadGraph {
public:
    // Carga la red de actores de IMDb
    // Grafo no dirigido, sin pesos. Formato esperado: archivo CSV.
    static Graph loadIMDb(const std::string& filepath, int maxEdges = -1);

    // Carga la red de Comercio Mundial (World Trade Network)
    // Grafo dirigido y ponderado. Formato esperado: archivo Pajek (.net).
    static Graph loadTrade(const std::string& filepath, int maxEdges = -1);
};