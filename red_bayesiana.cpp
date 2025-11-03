#include "red_bayesiana.h"
#include "nodo.h"
#include "tabla_probabilidad.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <stdexcept>

Nodo* RedBayesiana::obtener_o_crear(const std::string& nombre){
    auto it = nodos.find(nombre);
    if(it==nodos.end()) nodos[nombre] = std::make_unique<Nodo>(nombre);
        return nodos[nombre].get(); // Ensure we return a valid pointer
}

Nodo* RedBayesiana::obtener(const std::string& nombre) const{
    auto it = nodos.find(nombre);
    if(it==nodos.end()) return nullptr;
    return it->second.get();
}

void RedBayesiana::cargar_estructura(const std::string& ruta){
    std::ifstream in(ruta); if(!in) throw std::runtime_error("No se puede abrir estructura: "+ruta);
    std::string linea; int ln=0;
    while(std::getline(in, linea)){
        ++ln; linea = recortar(linea); if(linea.empty()||linea[0]=='#') continue;
        auto partes = dividir(linea, '-');
        if(partes.size()!=2 || partes[1].empty() || partes[1][0] != '>')
            throw std::runtime_error("Formato inválido en estructura línea "+std::to_string(ln)+": "+linea);
        std::string padre = recortar(partes[0]);
        std::string hijo  = recortar(partes[1].substr(1));
        Nodo* u = obtener_o_crear(padre);
        Nodo* v = obtener_o_crear(hijo);
        v->padres.push_back(u);
        u->hijos.push_back(v);
    }
}

void RedBayesiana::cargar_cpts(const std::string& ruta){
    std::ifstream in(ruta); if(!in) throw std::runtime_error("No se puede abrir CPTs: "+ruta);
    std::string linea; int ln=0; Nodo* actual=nullptr; std::vector<Nodo*> padres; 
    while(std::getline(in, linea)){
        ++ln; std::string t = recortar(linea); if(t.empty()||t[0]=='#') continue;
        if(t.rfind("NODE ",0)==0){
            std::string nombre = recortar(t.substr(5));
            actual = obtener_o_crear(nombre); padres.clear();
            if(!actual->cpt) actual->cpt = std::make_unique<TablaProbabilidad>();
        }else if(t.rfind("VALUES:",0)==0){
            if(!actual) throw std::runtime_error("VALUES sin NODE en línea "+std::to_string(ln));
            std::string rest = recortar(t.substr(7)); actual->valores = dividir(rest, ' ');
        }else if(t.rfind("PARENTS:",0)==0){
            if(!actual) throw std::runtime_error("PARENTS sin NODE en línea "+std::to_string(ln));
            std::string rest = recortar(t.substr(8)); auto nombres = dividir(rest,' ');
            padres.clear(); for(auto &pn: nombres){ if(pn.empty()) continue; padres.push_back(obtener_o_crear(pn)); }
        }else if(t=="TABLE"){
            if(!actual) throw std::runtime_error("TABLE sin NODE en línea "+std::to_string(ln));
            // establecer la tabla ahora que conocemos los padres
            actual->cpt->establecer(actual, padres);
        }else if(t=="END"){
            if(!actual) throw std::runtime_error("END sin NODE en línea "+std::to_string(ln));
            actual->cpt->establecer(actual, padres);
            actual=nullptr; padres.clear();
        }else if(t.rfind("p:",0)==0){
            if(!actual) throw std::runtime_error("p: sin NODE en línea "+std::to_string(ln));
            auto toks = dividir(recortar(t.substr(2)), ' ');
            std::vector<double> probs; probs.reserve(toks.size());
            for(auto &x: toks) probs.push_back(std::stod(x));
            actual->cpt->establecer(actual, {});
            std::vector<std::pair<std::string,std::string>> vacio;
            actual->cpt->agregar_fila(vacio, actual->valores, probs);
        }else{
            if(!actual) throw std::runtime_error("Fila sin NODE en línea "+std::to_string(ln));
            auto col = t.find(':'); if(col==std::string::npos) throw std::runtime_error("Falta ':' en línea "+std::to_string(ln));
            std::string izq = recortar(t.substr(0,col));
            std::string der = recortar(t.substr(col+1));
            std::vector<std::pair<std::string,std::string>> asign;
            if(!izq.empty()){
                auto pares = dividir(izq, ',');
                for(auto &kv: pares){ auto eq = kv.find('='); if(eq==std::string::npos) throw std::runtime_error("Falta '=' en línea "+std::to_string(ln));
                    std::string k = recortar(kv.substr(0,eq)); std::string v = recortar(kv.substr(eq+1));
                    asign.push_back({k,v});
                }
            }
            auto toks = dividir(der, ' ');
            std::vector<double> probs; for(auto &x: toks) if(!x.empty()) probs.push_back(std::stod(x));
            actual->cpt->agregar_fila(asign, actual->valores, probs);
        }
    }
}

void RedBayesiana::imprimir_estructura(std::ostream& os) const{
    // Kahn
    std::unordered_map<Nodo*,int> indeg; std::queue<Nodo*> q; std::vector<Nodo*> topo;
    for(auto &kv: nodos){ auto* n = kv.second.get(); indeg[n]=(int)n->padres.size(); if(indeg[n]==0) q.push(n);} 
    while(!q.empty()){ auto* u=q.front(); q.pop(); topo.push_back(u); for(auto* v: u->hijos){ if(--indeg[v]==0) q.push(v);} }
    os << "Estructura (predecesores):\n";
    for(auto* n: topo){
        os << "- "<<n->nombre<<" <- ";
        if(n->padres.empty()) os<<"(raíz)";
        else{ for(size_t i=0;i<n->padres.size();++i){ if(i) os<<","; os<<n->padres[i]->nombre; } }
        os << "\n";
    }
}

void RedBayesiana::imprimir_cpts(std::ostream& os) const{
    std::unordered_map<Nodo*,int> indeg; std::queue<Nodo*> q; std::vector<Nodo*> topo;
    for(auto &kv: nodos){ auto* n = kv.second.get(); indeg[n]=(int)n->padres.size(); if(indeg[n]==0) q.push(n);} 
    while(!q.empty()){ auto* u=q.front(); q.pop(); topo.push_back(u); for(auto* v: u->hijos){ if(--indeg[v]==0) q.push(v);} }
    for(auto* n: topo){ if(n->cpt) n->cpt->imprimir(os); os << "\n"; }
}