#include "Load_Graph.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <future>
#include <thread>

namespace {

inline void trim(std::string s) {
    size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        s.clear();
        return;
    }
    size_t last = s.find_last_not_of(" \t\r\n");
    s = s.substr(first, last - first + 1);
}

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> cols;
    cols.reserve(16);  // Reserva capacidad típica para columnas
    
    size_t start = 0;
    size_t end = 0;

    while (end != std::string::npos) {
        end = line.find(',', start);
        std::string cell;
        // Si no se encuentra una coma, se toma el resto de la línea
        if (end == std::string::npos) {
            cell = line.substr(start);
        } else {
            cell = line.substr(start, end - start);
        }
        
        // Eliminar espacios en blanco alrededor de la celda
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

struct findColumn {
    std::unordered_map<std::string, int> indices;
    
    findColumn(const std::vector<std::string>& header) {
        for (size_t i = 0; i < header.size(); ++i) {
            indices[header[i]] = static_cast<int>(i);
        }
    }
    
    int get(const std::string& name) const {
        auto it = indices.find(name);
        return (it != indices.end()) ? it->second : -1;
    }
};

// ============================================================
// Lectura por bloques para evitar cargar todo el archivo en memoria
// ============================================================
class FileReader {
private:
    std::ifstream file_;
    static constexpr size_t BUFFER_SIZE = 1 << 20;  // 1MB buffer
    std::vector<char> buffer_;
    size_t bufferPos_;
    size_t bufferSize_;
    bool eof_;
    
public:
    FileReader(const std::string& filename) 
        : buffer_(BUFFER_SIZE), bufferPos_(0), bufferSize_(0), eof_(false) {
        file_.open(filename, std::ios::in | std::ios::binary);
        if (!file_.is_open()) {
            throw std::runtime_error("No se pudo abrir el archivo: " + filename);
        }
    }
    
    ~FileReader() {
        if (file_.is_open()) file_.close();
    }
    
    // Lee una línea completa usando buffer interno
    bool getline(std::string& line) {
        line.clear();
        
        while (true) {
            // Si el buffer está vacío, cargar más datos
            if (bufferPos_ >= bufferSize_) {
                file_.read(buffer_.data(), BUFFER_SIZE);
                bufferSize_ = file_.gcount();
                bufferPos_ = 0;
                
                if (bufferSize_ == 0) {
                    eof_ = true;
                    return !line.empty();  // Retornar última línea si existe
                }
            }
            
            // Buscar nueva línea en el buffer
            char* start = buffer_.data() + bufferPos_;
            char* end = buffer_.data() + bufferSize_;
            char* newline = std::find(start, end, '\n');
            
            if (newline != end) {
                // Encontramos nueva línea
                line.append(start, newline - start);
                bufferPos_ = (newline - buffer_.data()) + 1;
                
                // Remover '\r' si existe (Windows CRLF)
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                return true;
            } else {
                // No encontramos nueva línea, agregar todo el buffer
                line.append(start, end - start);
                bufferPos_ = bufferSize_;
            }
        }
    }
};

} // namespace

// ============================================================
// IMDb Actors Network
// Formato esperado: actor1,actor2,peso(opcional)
// Solo se usan las dos primeras columnas para construir el grafo.
// Grafo no dirigido: se agregan ambas direcciones
// ============================================================
Graph LoadGraph::loadIMDb(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();

    Graph g(false);

    std::string line;
    FileReader file(filepath);
    // lectura del encabezado, primera linea
    if (!file.getline(line)){
        return g;
    }
    
    int edgeCount = 0;

    while (file.getline(line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        if (line.empty() || line[0] == '#') continue;

        auto cols = splitCsv(line);
        if (cols.size() < 2) continue;

        int id1 = g.addVertex(cols[0]);
        int id2 = g.addVertex(cols[1]);
        g.addEdge(id1, id2);
        g.addEdge(id2, id1);
        edgeCount++;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[IMDb] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
} 

// ============================================================
// Train/Test Network
// Formato esperado: src_ip,...,dst_ip,...
// Se usa como grafo dirigido entre IP origen y destino
// ============================================================
Graph LoadGraph::loadTrainTestNetwork(const std::string& filepath, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Graph g(false);
    FileReader file(filepath);
    std::string line;
    if (!file.getline(line))
        return g;

    auto header = splitCsv(line);
    findColumn findCol(header);
    
    int srcIdx = findCol.get("src_ip");
    int dstIdx = findCol.get("dst_ip");

    if (srcIdx < 0 || dstIdx < 0) {
        throw std::runtime_error("El archivo no contiene las columnas src_ip y dst_ip: " + filepath);
    }

    int edgeCount = 0;
    while (file.getline(line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        if (line.empty() || line[0] == '#') continue;

        auto cols = splitCsv(line);
        if (static_cast<int>(cols.size()) <= std::max(srcIdx, dstIdx)) continue;

        const std::string& src = cols[srcIdx];
        const std::string& dst = cols[dstIdx];
        if (src.empty() || dst.empty()) continue;

        int id1 = g.addVertex(src);
        int id2 = g.addVertex(dst);
        g.addEdge(id1, id2);
        edgeCount++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[Train/Test] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
}

// ============================================================
// World Trade Network
// Formato esperado: origen,destino,ano,valor
// Solo se cargan los datos del ano indicado (o todos si year == -1)
// ============================================================
Graph LoadGraph::loadTrade(const std::string& filepath, int year, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Graph g(true);
    
    FileReader file(filepath);
    std::string line;
    int edgeCount = 0;

    if (!file.getline(line))
        return g;

    while (file.getline(line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        if (line.empty() || line[0] == '#') continue;

        auto cols = splitCsv(line);
        if (cols.size() < 4) continue;

        if (year >= 0) {
            int y = 0;
            try { y = std::stoi(cols[2]); }
            catch (...) { continue; }
            if (y != year) continue;
        }

        double weight = 1.0;
        try { weight = std::stod(cols[3]); }
        catch (...) { weight = 1.0; }

        int id1 = g.addVertex(cols[0]);
        int id2 = g.addVertex(cols[1]);
        g.addEdge(id1, id2, weight);
        edgeCount++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[Trade] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
}

// ============================================================
// Edge List generico
// Formato: src dst [weight]  (separado por espacio o coma)
// ============================================================
Graph LoadGraph::loadEdgeList(const std::string& filepath, bool weighted,
                                bool directed, int maxEdges) {
    auto startTime = std::chrono::high_resolution_clock::now();
    Graph g(weighted);
    FileReader file(filepath);
    
    std::string line;
    int edgeCount = 0;

    while (file.getline(line) && (maxEdges < 0 || edgeCount < maxEdges)) {
        if (line.empty() || line[0] == '#' || line[0] == '%') continue;
        
        // reemplazar comas por espacios para facilitar el parsing
        for (char& c : line) if (c == ',') c = ' ';

        std::istringstream ss(line);
        std::string s, d;
        double w = 1.0;

        if (!(ss >> s >> d)) continue;
        if (weighted) ss >> w;

        int id1 = g.addVertex(s);
        int id2 = g.addVertex(d);
        g.addEdge(id1, id2, w);
        if (!directed) g.addEdge(id2, id1, w);
        edgeCount++;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    std::cout << "[EdgeList] Vertices: " << g.numVertices()
              << ", Aristas: " << g.numEdges()
              << ", Tiempo: " << std::fixed << std::setprecision(2) << elapsed << " ms\n";
    return g;
}
