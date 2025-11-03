#include "nodo.h"
#include "tabla_probabilidad.h"

Nodo::Nodo(std::string n): nombre(std::move(n)), cpt(nullptr) {}