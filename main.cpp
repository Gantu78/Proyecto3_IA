#include <iostream>
#include <iomanip>
#include <string>
#include <unordered_map>
#include "red_bayesiana.h"
#include "inferencia.h"
#include "util.h"

// función auxiliar para imprimir la distribución de probabilidad resultante
// recibe un vector de pares donde cada par contiene (valor, probabilidad)
static void imprimir_distribucion(const std::vector<std::pair<std::string,double>>& d){
    // configuramos el formato de salida para números de punto flotante
    // setf con ios::fixed hace que se use notación decimal fija (no científica)
    std::cout.setf(std::ios::fixed); 
    // establecemos 6 decimales de precisión para todas las probabilidades
    std::cout<<std::setprecision(6);
    
    // iteramos sobre cada par (valor, probabilidad) en la distribución
    for(const auto &p: d) 
        // imprimimos el valor, dos puntos, espacio y la probabilidad
        std::cout << p.first << ": " << p.second << "\n";
}

// parsea una cadena con evidencias en formato "Var1=val1,Var2=val2,..."
// y retorna un mapa que asocia nombres de variables con sus valores observados
static std::unordered_map<std::string,std::string> parsear_evidencia(const std::string& s){
    // creamos el mapa que contendrá las asignaciones variable -> valor
    std::unordered_map<std::string,std::string> e; 
    // recortamos espacios en blanco al inicio y final de la cadena
    std::string t = recortar(s); 
    // si la cadena está vacía después de recortar, no hay evidencias
    if(t.empty()) return e; 
    
    // dividimos la cadena por comas para obtener cada par variable=valor
    // por ejemplo: "A=true,B=false" se divide en ["A=true", "B=false"]
    auto pares = dividir(t, ',');
    
    // procesamos cada par individualmente
    for(auto &kv: pares){ 
        // buscamos la posición del signo '=' en el par actual
        auto eq = kv.find('='); 
        // si no encontramos '=', este par está mal formado, lo saltamos
        if(eq==std::string::npos) continue; 
        
        // extraemos la parte antes del '=' (nombre de variable) y recortamos espacios
        // extraemos la parte después del '=' (valor) y recortamos espacios
        // guardamos el par en el mapa: variable -> valor
        e[ recortar(kv.substr(0,eq)) ] = recortar(kv.substr(eq+1)); 
    }
    // retornamos el mapa completo con todas las evidencias parseadas
    return e;
}

int main(int argc, char** argv){
    // verificamos que se pasen al menos los dos archivos requeridos como argumentos
    // argc incluye el nombre del programa, por eso necesitamos al menos 3
    if(argc<3){
        // mostramos mensaje de uso explicando los parámetros requeridos
        std::cerr << "Uso: ./bn <estructura.txt> <cpts.txt> [COMANDOS]\n\n";
        // explicamos los comandos disponibles con ejemplos
        std::cerr << "Comandos:\n  MOSTRAR:ESTRUCT\n  MOSTRAR:CPTS\n  CONSULTAR: Var | evidencias  (ej. CONSULTAR: Cita | Tren=tiempo)\n";
        // retornamos código de error 1 indicando uso incorrecto
        return 1;
    }
    
    // extraemos el nombre del archivo de estructura desde el primer argumento
    std::string f_estructura = argv[1];
    // extraemos el nombre del archivo de CPTs desde el segundo argumento
    std::string f_cpts = argv[2];

    // creamos una instancia de la red bayesiana vacía
    RedBayesiana rb;
    
    // intentamos cargar los archivos, envolvemos en try-catch para manejar errores
    try{ 
        // primero cargamos la estructura (grafo dirigido con las conexiones)
        rb.cargar_estructura(f_estructura); 
        // luego cargamos las tablas de probabilidad condicional para cada nodo
        rb.cargar_cpts(f_cpts); 
    }
    catch(const std::exception& ex){ 
        // si ocurre cualquier error durante la carga, capturamos la excepción
        // mostramos el mensaje de error descriptivo
        std::cerr << "Error al cargar: "<<ex.what()<<"\n"; 
        // retornamos código de error 2 indicando fallo en carga de datos
        return 2; 
    }

    // procesamos cada comando adicional pasado como argumento
    // comenzamos desde el índice 3 (después de nombre_programa, estructura, cpts)
    for(int i=3;i<argc;++i){
        // obtenemos el comando actual como string
        std::string cmd = argv[i];
        
        // verificamos si el comando comienza con "MOSTRAR:ESTRUCT"
        // rfind con posición 0 verifica que empiece desde el inicio
        if(cmd.rfind("MOSTRAR:ESTRUCT",0)==0){ 
            // imprimimos la estructura de la red (nodos y sus conexiones)
            rb.imprimir_estructura(std::cout); 
        }
        // verificamos si el comando comienza con "MOSTRAR:CPTS"
        else if(cmd.rfind("MOSTRAR:CPTS",0)==0){ 
            // imprimimos todas las tablas de probabilidad condicional
            rb.imprimir_cpts(std::cout); 
        }
        // verificamos si es un comando de consulta (con o sin traza)
        // puede ser "CONSULTAR:" o "CONSULTAR_TRACE:"
        else if(cmd.rfind("CONSULTAR:",0)==0 || cmd.rfind("CONSULTAR_TRACE:",0)==0){
            // determinamos si queremos ver la traza detallada de ejecución
            // CONSULTAR_TRACE muestra paso a paso cómo se realiza la inferencia
            bool trace = (cmd.rfind("CONSULTAR_TRACE:",0)==0);
            
            // extraemos la parte después del prefijo CONSULTAR: o CONSULTAR_TRACE:
            // si tiene trace, el prefijo mide 16 caracteres, sino mide 10
            // esto nos da la parte "Variable | evidencias"
            std::string resto = recortar(cmd.substr(trace?16:10));
            
            // buscamos el separador '|' que divide variable de evidencias
            auto barra = resto.find('|');
            
            // la variable a consultar está antes del '|' (o es todo si no hay '|')
            // si no hay barra, toda la cadena es la variable
            std::string var = recortar(barra==std::string::npos? resto : resto.substr(0,barra));
            
            // las evidencias están después del '|' (o string vacío si no hay '|')
            // si no hay barra, no hay evidencias
            std::string evs = barra==std::string::npos? std::string("") : recortar(resto.substr(barra+1));
            
            // envolvemos la inferencia en try-catch para manejar errores
            try{
                // parseamos el string de evidencias a un mapa variable->valor
                auto e = parsear_evidencia(evs);
                
                // creamos el motor de inferencia pasándole la red bayesiana
                // el constructor calcula el orden topológico de los nodos
                InferenceEngine engine(rb);
                
                // verificamos si queremos traza de ejecución
                if(trace){
                    // llamamos a consultar_enumeracion pasando &std::cout
                    // esto hace que se imprima paso a paso el proceso
                    auto d = engine.consultar_enumeracion(var, e, &std::cout);
                    
                    // imprimimos el encabezado de la consulta con formato P(var | evidencias)
                    std::cout << "P("<<var<<" | "<<(evs.empty()?"":evs)<<")\n";
                    
                    // imprimimos la distribución de probabilidad resultante
                    imprimir_distribucion(d);
                }else{
                    // sin traza, pasamos nullptr como tercer argumento
                    // esto hace que no se imprima información de debug
                    auto d = engine.consultar_enumeracion(var, e, nullptr);
                    
                    // imprimimos el encabezado de la consulta
                    std::cout << "P("<<var<<" | "<<(evs.empty()?"":evs)<<")\n";
                    
                    // imprimimos la distribución de probabilidad resultante
                    imprimir_distribucion(d);
                }
            }catch(const std::exception& ex){ 
                // capturamos cualquier error durante la inferencia
                // puede ser variable inexistente, evidencia inconsistente, etc.
                std::cerr << "Error en CONSULTAR: "<<ex.what()<<"\n"; 
            }
        }else{
            // si el comando no coincide con ninguno de los anteriores
            // mostramos mensaje indicando que no se reconoce
            std::cerr << "Comando desconocido: "<<cmd<<"\n";
        }
    }
    
    // finalizamos el programa exitosamente
    return 0;
}