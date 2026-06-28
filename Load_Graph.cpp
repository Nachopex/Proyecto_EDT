#include "Load_Graph.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace {

// ============================================================
// Función auxiliar: splitCsv
// Separa una línea de texto en un vector de columnas basado 
// en las comas (formato CSV). Utilizado para procesar los 
// datos tabulares de los conjuntos de datos.
// ============================================================
std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> cols;
    cols.reserve(16);
    
    size_t start = 0;
    size_t end = 0;

    while (end != std::string::npos) {
        end = line.find(',', start);
        std::string cell;
        
        if (end == std::string::npos) {
            cell = line.substr(start);
        } else {
            cell = line.substr(start, end - start);
        }
        
        // Eliminación de espacios en blanco y retornos de carro
        size_t first = cell.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = cell.find_last_not_of(" \t\r\n");
            cell = cell.substr(first, last - first + 1);
        } else {
            cell.clear();
        }
        
        cols.push_back(std::move(cell));
        start = end + 1;
    }
    
    return cols;
}

} // fin del namespace anónimo

// ============================================================
// Carga de red de actores (IMDb)
// Grafo: No dirigido, ponderado.
// Formato esperado: Archivo CSV con columnas [From, To, Strength].
// ============================================================
Graph LoadGraph::loadIMDb(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();

    // Inicialización del grafo: no dirigido (false) y ponderado (true).
    Graph g(false, true); 

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
    }
    
    std::string line;
    
    // Omisión de la cabecera del archivo CSV
    if (!std::getline(file, line)){
        return g;
    }
    
    int edgeCount = 0;

    // Procesamiento secuencial del archivo
    while (std::getline(file, line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        // Limpieza de caracteres de escape en sistemas Windows
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Omisión de líneas vacías o comentarios
        if (line.empty() || line[0] == '#') continue; 

        auto cols = splitCsv(line);
        // Verificación de estructura mínima (Actor origen, Actor destino)
        if (cols.size() < 2) continue; 

        // Inserción de vértices
        int id1 = g.addVertex(cols[0]);
        int id2 = g.addVertex(cols[1]);
        
        // Extracción del peso de la arista (columna Strength)
        double weight = 1.0;
        if (cols.size() >= 3 && !cols[2].empty()) {
            try {
                weight = std::stod(cols[2]);
            } catch (...) {
                // Caída silenciosa a peso por defecto en caso de error de formato
                weight = 1.0;
            }
        }

        // Inserción de la arista en el grafo
        g.addEdge(id1, id2, weight);
        edgeCount++;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[IMDb] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo de carga: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
}

// ============================================================
// Carga de red de Comercio Mundial (World Trade Network)
// Formato esperado: Archivos Pajek (.net).
// Grafo: Dirigido, ponderado (el peso es el valor comercial).
// ============================================================
Graph LoadGraph::loadTrade(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Inicialización del grafo: dirigido (true) y ponderado (true).
    Graph g(true, true); 
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
    }
    
    std::string line;
    bool readingVertices = false;
    bool readingArcs = false;
    
    // Estructura temporal para traducir identificadores numéricos de Pajek a etiquetas de país
    std::unordered_map<int, std::string> pajekIdToLabel;
    
    int edgeCount = 0;

    // Procesamiento secuencial del archivo
    while (std::getline(file, line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        // Limpieza de caracteres de escape en sistemas Windows
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        // Omisión de líneas vacías o comentarios
        if (line.empty() || line[0] == '%') continue;

        // Detección de la sección de vértices (*Vertices)
        if (line.find("*Vertices") == 0 || line.find("*vertices") == 0) {
            readingVertices = true;
            readingArcs = false;
            continue;
        }
        // Detección de la sección de aristas (*Arcs / *Edges)
        if (line.find("*Arcs") == 0 || line.find("*arcs") == 0 || 
            line.find("*Edges") == 0 || line.find("*edges") == 0) {
            readingVertices = false;
            readingArcs = true;
            continue;
        }

        // Extracción y registro de vértices
        if (readingVertices) {
            std::istringstream ss(line);
            int id;
            if (ss >> id) { 
                // Extracción de la etiqueta del vértice encerrada en comillas
                size_t startQuote = line.find('"');
                size_t endQuote = line.rfind('"');
                std::string label = std::to_string(id); // Etiqueta por defecto
                
                if (startQuote != std::string::npos && endQuote != std::string::npos && startQuote < endQuote) {
                    label = line.substr(startQuote + 1, endQuote - startQuote - 1);
                }
                
                pajekIdToLabel[id] = label;
                g.addVertex(label); 
            }
        } 
        // Extracción y registro de aristas
        else if (readingArcs) {
            std::istringstream ss(line);
            int u, v;
            double w = 1.0;
            
            // Lectura de nodo origen, nodo destino y peso (opcional)
            if (ss >> u >> v) {
                if (!(ss >> w)) w = 1.0; 
                
                // Validación de existencia de ambos vértices antes de la inserción de la arista
                if (pajekIdToLabel.count(u) && pajekIdToLabel.count(v)) {
                    g.addEdge(pajekIdToLabel[u], pajekIdToLabel[v], w);
                    edgeCount++;
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[Trade] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo de carga: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
}