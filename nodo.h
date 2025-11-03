#ifndef NODO_H
#define NODO_H
#include <string>
#include <vector>
#include <memory>

// Hacer visible la definición de TablaProbabilidad antes de usar std::unique_ptr<TablaProbabilidad>
#include "tabla_probabilidad.h"

struct Nodo{
    std::string nombre;
    std::vector<std::string> valores;      // dominio
    std::vector<Nodo*> padres;
    std::vector<Nodo*> hijos;
    std::unique_ptr<TablaProbabilidad> cpt; // tabla de probabilidad condicional

    // Nodo representa una variable aleatoria en la red. Mantiene
    // - `valores`: dominio discreto de la variable
    // - listas de punteros a padres/hijos (las relaciones dirigidas)
    // - un unique_ptr a su CPT (TablaProbabilidad) para gestión RAII
    explicit Nodo(std::string n="");
};

#endif // NODO_H