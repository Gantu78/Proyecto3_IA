#include "nodo.h"
#include "tabla_probabilidad.h"

// Constructor simple: inicializa el nombre y deja el unique_ptr `cpt`
// en nullptr; la tabla de probabilidad se asigna más tarde al
// parsear los CPTs (si corresponde).
Nodo::Nodo(std::string n): nombre(std::move(n)), cpt(nullptr) {}