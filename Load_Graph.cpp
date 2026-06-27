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

// Separa una línea de texto en un vector de columnas basado en las comas (formato CSV).
// Lo usamos exclusivamente para limpiar las columnas del dataset de IMDb.
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
        
        // Elimina espacios en blanco y retornos de carro
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
// Grafo: No dirigido, sin peso.
// ============================================================
Graph LoadGraph::loadIMDb(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();

    Graph g(false); // false = Inicializa un grafo no dirigido

    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
    }
    
    std::string line;
    
    // Se salta el encabezado (primera línea con los nombres de las columnas)
    if (!std::getline(file, line)){
        return g;
    }
    
    int edgeCount = 0;

    // Lee línea por línea
    while (std::getline(file, line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        // Limpiamos el posible \r al final de la línea para sistemas Windows
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Ignora comentarios o lineas vacias
        if (line.empty() || line[0] == '#') continue; 

        auto cols = splitCsv(line);
        if (cols.size() < 2) continue; // Requiere al menos dos columnas (Actor 1, Actor 2)

        // Agrega los vértices y la arista
        int id1 = g.addVertex(cols[0]);
        int id2 = g.addVertex(cols[1]);
        
        g.addEdge(id1, id2);
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
// Formato: Archivos Pajek (.net).
// Grafo: Dirigido y Ponderado (el peso es el valor comercial).
// ============================================================
Graph LoadGraph::loadTrade(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    Graph g(true, true); // true, true = Grafo dirigido y ponderado
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);
    }
    
    std::string line;
    bool readingVertices = false;
    bool readingArcs = false;
    
    // Diccionario temporal para traducir el ID numérico del Pajek al nombre del país
    std::unordered_map<int, std::string> pajekIdToLabel;
    
    int edgeCount = 0;

    // Lee línea por línea
    while (std::getline(file, line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        // Limpiamos el salto de línea de Windows (\r)
        if (!line.empty() && line.back() == '\r') line.pop_back();
        
        // Ignorar líneas vacías o comentarios
        if (line.empty() || line[0] == '%') continue;

        // Detectar en qué sección del archivo estamos
        if (line.find("*Vertices") == 0 || line.find("*vertices") == 0) {
            readingVertices = true;
            readingArcs = false;
            continue;
        }
        if (line.find("*Arcs") == 0 || line.find("*arcs") == 0 || 
            line.find("*Edges") == 0 || line.find("*edges") == 0) {
            readingVertices = false;
            readingArcs = true;
            continue;
        }

        // Si estamos leyendo países
        if (readingVertices) {
            std::istringstream ss(line);
            int id;
            if (ss >> id) { 
                // Extraemos el nombre del país que está entre comillas dobles
                size_t startQuote = line.find('"');
                size_t endQuote = line.rfind('"');
                std::string label = std::to_string(id); // Valor por defecto si no hay comillas
                
                if (startQuote != std::string::npos && endQuote != std::string::npos && startQuote < endQuote) {
                    label = line.substr(startQuote + 1, endQuote - startQuote - 1);
                }
                
                pajekIdToLabel[id] = label;
                g.addVertex(label); 
            }
        } 
        // Si estamos leyendo las transacciones comerciales (aristas)
        else if (readingArcs) {
            std::istringstream ss(line);
            int u, v;
            double w = 1.0;
            
            // Extrae el origen (u), destino (v) y peso (w) separados por espacios
            if (ss >> u >> v) {
                if (!(ss >> w)) w = 1.0; 
                
                // Aseguramos que ambos países existan antes de conectarlos
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