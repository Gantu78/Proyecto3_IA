#ifndef TABLA_PROBABILIDAD_H
#define TABLA_PROBABILIDAD_H
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <ostream>

struct Nodo;

struct TablaProbabilidad{
    Nodo* variable = nullptr;                 // variable objetivo
    std::vector<Nodo*> padres;                // orden de padres
    std::unordered_map<std::string,double> tabla; // clave compacta (padres+var)

    void establecer(Nodo* var, const std::vector<Nodo*>& padres_);
    void agregar_fila(const std::vector<std::pair<std::string,std::string>>& asig_padres,
                      const std::vector<std::string>& valores_var,
                      const std::vector<double>& probabilidades);
    double condicionada(const std::unordered_map<std::string,std::string>& evidencia,
                        const std::string& valor) const; // P(var=valor | padres)
    void imprimir(std::ostream& os) const;
};

#endif // TABLA_PROBABILIDAD_H