#include "tabla_probabilidad.h"
#include "nodo.h"
#include "util.h"
#include <cmath>
#include <functional>
#include <stdexcept>

void TablaProbabilidad::establecer(Nodo* var, const std::vector<Nodo*>& padres_){
    variable = var; padres = padres_;
}

void TablaProbabilidad::agregar_fila(
    const std::vector<std::pair<std::string,std::string>>& asig_padres,
    const std::vector<std::string>& valores_var,
    const std::vector<double>& probabilidades){

    if(probabilidades.size()!=valores_var.size()) throw std::runtime_error("#probs != #valores en agregar_fila()");
    double suma=0; for(double p: probabilidades) suma+=p;
    if(std::fabs(suma-1.0) > 1e-6) {
        // aviso pero no interrumpe
        // std::cerr << "[AVISO] La fila no normaliza a 1: " << suma << " ";
    }
    for(size_t i=0;i<valores_var.size();++i){
        auto todo = asig_padres;
        todo.push_back({variable->nombre, valores_var[i]});
        tabla[empaquetar_clave(todo)] = probabilidades[i];
    }
}

double TablaProbabilidad::condicionada(const std::unordered_map<std::string,std::string>& evidencia,
                                        const std::string& valor) const{
    std::vector<std::pair<std::string,std::string>> asignaciones;
    asignaciones.reserve(padres.size()+1);
    for(Nodo* p: padres){
        auto it = evidencia.find(p->nombre);
        if(it==evidencia.end()) throw std::runtime_error("Evidencia incompleta: falta " + p->nombre);
        asignaciones.push_back({p->nombre, it->second});
    }
    asignaciones.push_back({variable->nombre, valor});
    auto it = tabla.find(empaquetar_clave(asignaciones));
    if(it==tabla.end()) throw std::runtime_error("Fila CPT no encontrada para " + variable->nombre);
    return it->second;
}

void TablaProbabilidad::imprimir(std::ostream& os) const{
    os << "P(" << variable->nombre;
    if(!padres.empty()){
        os << " | ";
        for(size_t i=0;i<padres.size();++i){ if(i) os<<","; os<<padres[i]->nombre; }
    }
    os << ")\nValores: ";
    for(size_t i=0;i<variable->valores.size();++i){ if(i) os<<", "; os<<variable->valores[i]; }
    os << "\n";

    // generar combinaciones de padres para imprimir ordenado
    std::vector<std::vector<std::string>> dominios; dominios.reserve(padres.size());
    for(Nodo* p: padres) dominios.push_back(p->valores);

    std::vector<std::pair<std::string,std::string>> asign; asign.reserve(padres.size());
    std::function<void(size_t)> bt = [&](size_t i){
        if(i==padres.size()){
            os << " ";
            if(asign.empty()) os << "<prior>"; else{
                for(size_t k=0;k<asign.size();++k){ if(k) os<<","; os<<asign[k].first<<"="<<asign[k].second; }
            }
            os << " : ";
            for(size_t j=0;j<variable->valores.size();++j){
                auto todo = asign; todo.push_back({variable->nombre, variable->valores[j]});
                std::string clave = empaquetar_clave(todo);
                double p = tabla.count(clave)? tabla.at(clave): NAN;
                if(j) os << " ";
                os << p;
            }
            os << "\n";
            return;
        }
        for(const std::string& v : dominios[i]){ asign.push_back({padres[i]->nombre, v}); bt(i+1); asign.pop_back(); }
    };
    bt(0);
}