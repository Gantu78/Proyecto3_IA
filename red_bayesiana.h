#ifndef RED_BAYESIANA_H
#define RED_BAYESIANA_H

// incluir nodo.h (ahora con tabla_probabilidad.h disponible)
#include "nodo.h"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <ostream>

struct RedBayesiana{
    std::unordered_map<std::string, std::unique_ptr<Nodo>> nodos;

    // Devuelve un puntero a un Nodo existente o crea uno nuevo si no
    // está presente en el mapa `nodos`.
    Nodo* obtener_o_crear(const std::string& nombre);
    // Devuelve nullptr si no existe.
    Nodo* obtener(const std::string& nombre) const;

    void cargar_estructura(const std::string& ruta);
    void cargar_cpts(const std::string& ruta);

    void imprimir_estructura(std::ostream& os) const;
    void imprimir_cpts(std::ostream& os) const;
};

#endif // RED_BAYESIANA_H