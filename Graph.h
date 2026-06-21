#pragma once
#include <vector>
#include <unordered_map>
#include <string>
#include <limits>

// Representa una arista con destino y peso opcional
struct Edge {
    int dest;
    double weight;
    Edge(int d, double w = 1.0) : dest(d), weight(w) {}
};

// ADT Grafo dirigido con lista de adyacencia
// Soporta vértices con IDs enteros y etiquetas string opcionales
class Graph {
public:
    // Constructor: weighted indica si el grafo es ponderado y directed si es dirigido o no.
    explicit Graph(bool directed = true, bool weighted = false);

    // Agrega un vértice con ID entero. Retorna false si ya existe.
    bool addVertex(int id);

    // Agrega un vértice con etiqueta string. Asigna un ID interno automáticamente.
    int addVertex(const std::string& label);

    // Agrega una arista entre dos vértices dados por sus IDs o etiquetas. Retorna false si alguno no existe.
    bool addEdge(int src, int dest, double w = 1.0);
    bool addEdge(const std::string& srcLabel, const std::string& destLabel, double w = 1.0);

    // Elimina una arista dirigida de src a dest
    bool removeEdge(int src, int dest);

    // Retorna lista de vecinos (aristas salientes) de un vértice
    const std::vector<Edge>& neighbors(int v) const;

    // Número de vértices y aristas
    int numVertices() const;
    int numEdges() const;

    // Verifica si un vértice o arista existe
    bool hasVertex(int v) const;
    bool hasEdge(int src, int dest) const;

    // Convierte etiqueta a ID interno y viceversa
    int labelToId(const std::string& label) const;
    std::string idToLabel(int id) const;

    // Retorna todos los IDs de vértices
    std::vector<int> vertices() const;

    // Indica si el grafo es dirigido
    bool isDirected() const;

    // Indica si el grafo es ponderado
    bool isWeighted() const;

    // Memoria aproximada en bytes
    size_t memoryBytes() const;

private:
    bool directed_;
    bool weighted_;
    int nextId_;
    int edgeCount_;

    // Lista de adyacencia: id -> vector de aristas
    std::unordered_map<int, std::vector<Edge>> adjList_;

    // Mapeo bidireccional label <-> id
    std::unordered_map<std::string, int> labelToId_;
    std::unordered_map<int, std::string> idToLabel_;
};
