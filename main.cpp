#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include "red_bayesiana.h"
#include "inferencia.h"
#include "util.h"

static void imprimir_distribucion(const std::vector<std::pair<std::string,double>>& d){
    std::cout.setf(std::ios::fixed); std::cout<<std::setprecision(6);
    for(const auto &p: d) std::cout << p.first << ": " << p.second << "\n";
}

static std::unordered_map<std::string,std::string> parsear_evidencia(const std::string& s){
    std::unordered_map<std::string,std::string> e; std::string t = recortar(s);
    if(t.empty()) return e;
    auto pares = dividir(t, ',');
    for(auto &kv: pares){ auto eq = kv.find('='); if(eq==std::string::npos) continue; e[ recortar(kv.substr(0,eq)) ] = recortar(kv.substr(eq+1)); }
    return e;
}

int main(int argc, char** argv){
    if(argc<3){
        std::cerr << "Uso: ./bn <estructura.txt> <cpts.txt> [COMANDOS]\n\n";
        std::cerr << "Comandos:\n  MOSTRAR:ESTRUCT\n  MOSTRAR:CPTS\n  CONSULTAR: Var | evidencias  (ej. CONSULTAR: Cita | Tren=tiempo)\n";
        return 1;
    }
    std::string f_estructura = argv[1];
    std::string f_cpts = argv[2];

    RedBayesiana rb;
    try{ rb.cargar_estructura(f_estructura); rb.cargar_cpts(f_cpts); }
    catch(const std::exception& ex){ std::cerr << "Error al cargar: "<<ex.what()<<"\n"; return 2; }

    for(int i=3;i<argc;++i){
        std::string cmd = argv[i];
        if(cmd.rfind("MOSTRAR:ESTRUCT",0)==0){ rb.imprimir_estructura(std::cout); }
        else if(cmd.rfind("MOSTRAR:CPTS",0)==0){ rb.imprimir_cpts(std::cout); }
        else if(cmd.rfind("CONSULTAR:",0)==0 || cmd.rfind("CONSULTAR_TRACE:",0)==0){
            bool trace = (cmd.rfind("CONSULTAR_TRACE:",0)==0);
            // extraer la parte después del prefijo CONSULTAR: o CONSULTAR_TRACE:
            // trace?16:10 calcula el desplazamiento del substring
            std::string resto = recortar(cmd.substr(trace?16:10));
            auto barra = resto.find('|');
            std::string var = recortar(barra==std::string::npos? resto : resto.substr(0,barra));
            std::string evs = barra==std::string::npos? std::string("") : recortar(resto.substr(barra+1));
            try{
                auto e = parsear_evidencia(evs);
                InferenceEngine engine(rb);
                if(trace){
                    auto d = engine.consultar_enumeracion(var, e, &std::cout);
                    // imprimir final con formato
                    std::cout << "P("<<var<<" | "<<(evs.empty()?"":evs)<<")\n";
                    imprimir_distribucion(d);
                }else{
                    auto d = engine.consultar_enumeracion(var, e, nullptr);
                    std::cout << "P("<<var<<" | "<<(evs.empty()?"":evs)<<")\n";
                    imprimir_distribucion(d);
                }
            }catch(const std::exception& ex){ std::cerr << "Error en CONSULTAR: "<<ex.what()<<"\n"; }
        }else{
            std::cerr << "Comando desconocido: "<<cmd<<"\n";
        }
    }
    return 0;
}