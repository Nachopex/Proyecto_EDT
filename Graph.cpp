#include "Graph.h"
#include <stdexcept> 

// Constructor: Configura si el grafo es dirigido y usa pesos en las aristas y arranca los contadores en cero.
Graph::Graph(bool directed, bool weighted) : directed_(directed), weighted_(weighted), nextId_(0), edgeCount_(0) {}

// Agrega un vértice usando directamente un ID numérico.
// Retorna falso si el vértice ya existía para evitar borrar sus conexiones actuales.
bool Graph::addVertex(int id) {
    if (adjList_.count(id)) return false;
    
    adjList_[id] = {};
    return true;
}

// Agrega un vértice usando texto (ej. el nombre de un país o actor).
// Si el país ya estaba registrado, simplemente devuelve el ID que ya tenía.
// Si es nuevo, le crea un ID automático y guarda la traducción en los mapas.
int Graph::addVertex(const std::string& label) {
    auto it = labelToId_.find(label);
    if (it != labelToId_.end()) return it->second;
    
    int id = nextId_++;
    adjList_[id] = {};
    labelToId_[label] = id;
    idToLabel_[id] = label;
    return id;
}

// Agrega una arista dirigida entre dos IDs numéricos.
bool Graph::addEdge(int src, int dest, double w) {
    // Si alguno de los vértices no existe en el grafo, no hace nada.
    if (!adjList_.count(src) || !adjList_.count(dest)) return false;
    
    // Si el grafo no es ponderado, fuerza el peso a 1.0.
    double weight;

    if (weighted_ == true) {
        weight = w;
    } 
    else {
        weight = 1.0;
    }
    // Agrega la arista al origen. Si el grafo no es dirigido, también agrega la arista inversa.
    adjList_[src].emplace_back(dest, weight);

    if (!directed_ && src != dest) { 
    adjList_[dest].emplace_back(src, weight);
}
    edgeCount_++;
    return true;
}

// Agrega una arista dirigida usando directamente los nombres de los vértices.
// Primero traduce los textos a números y luego reutiliza el addEdge anterior.
bool Graph::addEdge(const std::string& srcLabel, const std::string& destLabel, double w) {
    int s = labelToId(srcLabel);
    int d = labelToId(destLabel);
    if (s == -1 || d == -1) return false;
    return addEdge(s, d, w);
}

// Busca y elimina una arista específica que va del origen al destino.
bool Graph::removeEdge(int src, int dest) {
    auto it = adjList_.find(src);
    if (it == adjList_.end()) return false;
    
    auto& edges = it->second;
    for (auto e = edges.begin(); e != edges.end(); ++e) {
        if (e->dest == dest) {

            edges.erase(e);
            edgeCount_--;
            
            // Si el grafo no es dirigido, también elimina la arista inversa.
            if (!directed_ && src != dest) {
                auto it2 = adjList_.find(dest);
                if (it2 != adjList_.end()) {
                    auto& edges2 = it2->second;
                    for (auto e2 = edges2.begin(); e2 != edges2.end(); ++e2) {
                        if (e2->dest == src) {
                            edges2.erase(e2);
                            break;
                        }
                    }
                }
            }
            return true;
        }
    }
    return false;
}

// Devuelve todas las conexiones que salen de un vértice.
// Si el vértice no existe, lanza un error.
const std::vector<Edge>& Graph::neighbors(int v) const {
    auto it = adjList_.find(v);
    if (it == adjList_.end()) throw std::out_of_range("Vertex not found");
    return it->second;
}

// Retorna la cantidad total de vértices creados.
int Graph::numVertices() const {
    return static_cast<int>(adjList_.size());
}

// Retorna la cantidad total de aristas creadas.
int Graph::numEdges() const {
    return edgeCount_;
}

// Revisa rápidamente si un ID numérico ya está en el grafo.
bool Graph::hasVertex(int v) const {
    return adjList_.count(v) > 0;
}

// Revisa si existe una conexión directa entre el origen y el destino.
bool Graph::hasEdge(int src, int dest) const {
    auto it = adjList_.find(src);
    if (it == adjList_.end()) return false;
    
    for (const auto& e : it->second)
        if (e.dest == dest) return true;
    return false;
}

// Convierte un texto (ej. "Chile") a su ID interno. Devuelve -1 si no lo encuentra.
int Graph::labelToId(const std::string& label) const {
    auto it = labelToId_.find(label);
    return (it != labelToId_.end()) ? it->second : -1;
}

// Convierte un ID numérico a su texto original. Devuelve un texto vacío si no existe.
std::string Graph::idToLabel(int id) const {
    auto it = idToLabel_.find(id);
    return (it != idToLabel_.end()) ? it->second : "";
}

// Extrae una lista plana con todos los IDs numéricos que existen actualmente en el grafo.
std::vector<int> Graph::vertices() const {
    std::vector<int> verts;
    verts.reserve(adjList_.size());
    for (const auto& kv : adjList_)
        verts.push_back(kv.first);
    return verts;
}

// Nos dice si el grafo toma en cuenta los pesos o no.
bool Graph::isWeighted() const {
    return weighted_;
}

// Nos dice si el grafo es dirigido o no.
bool Graph::isDirected() const {
    return directed_;
}

// Calcula cuántos bytes de memoria RAM está consumiendo el grafo actualmente.
size_t Graph::memoryBytes() const {
    size_t bytes = 0;
    // Costo base de la tabla hash por cada vértice
    bytes += adjList_.size() * 64;
    // Costo de guardar la información de cada arista
    bytes += static_cast<size_t>(edgeCount_) * 12;
    // Costo de almacenar los textos de los labels
    for (const auto& kv : labelToId_)
        bytes += kv.first.size() + sizeof(int);
    return bytes;
}