#include "inferencia.h"
#include "red_bayesiana.h"
#include "nodo.h"
#include "tabla_probabilidad.h"
#include <queue>
#include <stdexcept>
#include <sstream>

static std::vector<Nodo*> orden_topologico(const RedBayesiana& rb){
    std::unordered_map<Nodo*,int> indeg; std::queue<Nodo*> q; std::vector<Nodo*> topo;
    for(const auto &kv: rb.nodos){ auto* n = kv.second.get(); indeg[n]=(int)n->padres.size(); if(indeg[n]==0) q.push(n);} 
    while(!q.empty()){ auto* u=q.front(); q.pop(); topo.push_back(u); for(auto* v: u->hijos){ if(--indeg[v]==0) q.push(v);} }
    return topo;
}

InferenceEngine::InferenceEngine(const RedBayesiana& rb): rb_(rb), orden_(orden_topologico(rb)){}

double InferenceEngine::enumerar_todo(size_t i, std::unordered_map<std::string,std::string>& evidencia,
                                      std::ostream* trace, int depth) const{
    if(i==orden_.size()) return 1.0;
    Nodo* Y = orden_[i];
    auto it = evidencia.find(Y->nombre);
    std::string indent(depth*2, ' ');
    if(it!=evidencia.end()){
        double py = Y->cpt->condicionada(evidencia, it->second);
        if(trace){ (*trace) << indent << "Usando evidencia: "<<Y->nombre<<"="<<it->second<<" -> P="<<py<<"\n"; }
        return py * enumerar_todo(i+1, evidencia, trace, depth+1);
    }else{
        double suma=0.0;
        if(trace){ (*trace) << indent << "Enumerando "<<Y->nombre<<" sobre "<<Y->valores.size()<<" valores\n"; }
        for(const std::string& y: Y->valores){
            evidencia[Y->nombre]=y;
            double py = Y->cpt->condicionada(evidencia, y);
            if(trace){ (*trace) << indent << "  Probar "<<Y->nombre<<"="<<y<<" -> P="<<py<<"\n"; }
            double sub = enumerar_todo(i+1, evidencia, trace, depth+2);
            double contrib = py * sub;
            if(trace){ (*trace) << indent << "  Resultado recursivo: "<<sub<<" contrib="<<contrib<<"\n"; }
            suma += contrib;
            evidencia.erase(Y->nombre);
        }
        if(trace){ (*trace) << indent << "Suma para "<<Y->nombre<<" = "<<suma<<"\n"; }
        return suma;
    }
}

std::vector<std::pair<std::string,double>> InferenceEngine::consultar_enumeracion(
    const std::string& variable,
    const std::unordered_map<std::string,std::string>& evidencia,
    std::ostream* trace) const{

    auto it = rb_.nodos.find(variable);
    if(it==rb_.nodos.end()) throw std::runtime_error("Variable desconocida: "+variable);
    Nodo* Q = it->second.get();

    std::vector<std::pair<std::string,double>> dist; dist.reserve(Q->valores.size());
    for(const std::string& x: Q->valores){
        auto e = evidencia; e[variable]=x;
        if(trace){ (*trace) << "--- Calcular P("<<variable<<"="<<x<<" , evidencia) ---\n"; }
        double v = enumerar_todo(0, e, trace, 0);
        if(trace){ (*trace) << "  => P_unorm("<<variable<<"="<<x<<") = "<<v<<"\n\n"; }
        dist.push_back({x, v});
    }
    double Z=0; for(auto &p: dist) Z+=p.second; if(Z==0) throw std::runtime_error("Normalización 0");
    for(auto &p: dist) p.second/=Z;
    if(trace){ (*trace) << "Normalización Z="<<Z<<"\n"; (*trace) << "Distribución normalizada:\n"; for(auto &p: dist) (*trace) << p.first<<": "<<p.second<<"\n"; }
    return dist;
}