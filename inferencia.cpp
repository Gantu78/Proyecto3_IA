#include "inferencia.h"
#include "red_bayesiana.h"
#include "nodo.h"
#include "tabla_probabilidad.h"
#include <queue>
#include <stdexcept>
#include <sstream>

// función estática que calcula el orden topológico de los nodos
// usando el algoritmo de Kahn para grafos dirigidos acíclicos
// el orden topológico es fundamental para la enumeración porque
// garantiza que procesamos padres antes que hijos
static std::vector<Nodo*> orden_topologico(const RedBayesiana& rb){
    // algoritmo de Kahn para orden topologico.
    // Utilizamos un mapa de grados de entrada y una cola de nodos de grado de entrada cero.
    
    // mapa que almacena el grado de entrada (número de padres) de cada nodo
    std::unordered_map<Nodo*,int> indeg; 
    // cola para procesar nodos que tienen indegree 0 (sin padres pendientes)
    std::queue<Nodo*> q; 
    // vector que almacenará el resultado del orden topológico
    std::vector<Nodo*> topo;
    
    // Primera pasada: inicializar indegrees de todos los nodos
    // contamos cuántos padres tiene cada nodo y encolamos las raíces
    for(const auto &kv: rb.nodos){
        // obtenemos el puntero al nodo desde el unique_ptr
        auto* n = kv.second.get();
        // el indegree es igual al número de padres del nodo
        indeg[n] = (int)n->padres.size();
        // si el nodo no tiene padres (indegree 0), es una raíz
        // las raíces se pueden procesar inmediatamente
        if(indeg[n] == 0) q.push(n);
    }
    
    // Segunda pasada: procesamos nodos en orden topológico
    // cada vez que procesamos un nodo, "eliminamos" sus aristas salientes
    // decrementando el indegree de sus hijos
    while(!q.empty()){
        // extraemos el siguiente nodo con indegree 0 de la cola
        auto* u = q.front(); 
        q.pop();
        // agregamos este nodo al orden topológico
        topo.push_back(u);
        
        // decrementamos el indegree de cada hijo del nodo actual
        // esto simula "eliminar" la arista u -> v
        for(auto* v: u->hijos){
            // decrementamos el indegree y verificamos si llega a 0
            // si llega a 0, significa que ya procesamos todos sus padres
            if(--indeg[v] == 0) 
                // encolamos el hijo para procesarlo en la siguiente iteración
                q.push(v);
        }
    }
    // retornamos el vector con el orden topológico completo
    return topo;
}

// constructor del motor de inferencia
// recibe la red bayesiana y calcula su orden topológico
// el orden topológico se guarda para reutilizarlo en múltiples consultas
InferenceEngine::InferenceEngine(const RedBayesiana& rb)
    : rb_(rb), // guardamos referencia a la red bayesiana
      orden_(orden_topologico(rb)) // calculamos y guardamos el orden topológico
{}

// función recursiva que implementa la enumeración completa
// esta es la función core del algoritmo de inferencia por enumeración
// calcula P(X1,...,Xn | evidencia) donde X1,...,Xn son las variables no observadas
// 
// Parámetros:
// i: índice actual en el orden topológico (qué variable estamos procesando)
// evidencia: mapa mutable con las asignaciones de variables (observadas y temporales)
// trace: stream opcional para imprimir traza de ejecución (debugging)
// depth: profundidad actual de recursión (solo para indentación en traza)
double InferenceEngine::enumerar_todo(size_t i, 
                                      std::unordered_map<std::string,std::string>& evidencia,
                                      std::ostream* trace, 
                                      int depth) const{
    // caso base de la recursión: si ya procesamos todas las variables
    // retornamos 1.0 porque no quedan más factores que multiplicar
    if(i==orden_.size()) return 1.0;
    
    // obtenemos la variable Y que corresponde al índice i en el orden topológico
    Nodo* Y = orden_[i];
    
    // buscamos si Y está en la evidencia (observada o temporalmente asignada)
    auto it = evidencia.find(Y->nombre);
    
    // creamos string de indentación para hacer la traza más legible
    // cada nivel de profundidad añade 2 espacios
    std::string indent(depth*2, ' ');
    
    // verificamos si Y está en la evidencia
    if(it!=evidencia.end()){
        // Caso 1: la variable Y está fijada por la evidencia
        // esto significa que Y es una variable observada o fue asignada
        // temporalmente en una iteración anterior de la enumeración
        // No debemos sumar sobre sus valores: usamos directamente la
        // probabilidad condicional P(Y = y | padres) y seguimos.
        
        // obtenemos P(Y = valor_observado | valores_de_padres)
        // la CPT consulta los valores de los padres en el mapa evidencia
        double py = Y->cpt->condicionada(evidencia, it->second);
        
        // si hay traza activa, imprimimos que usamos evidencia
        if(trace){ 
            (*trace) << indent << "Usando evidencia: "
                    << Y->nombre << "=" << it->second 
                    << " -> P=" << py << "\n"; 
        }
        
        // multiplicamos por la probabilidad condicional y continuamos
        // con la siguiente variable (i+1) en el orden topológico
        // incrementamos depth para la indentación en la siguiente llamada
        return py * enumerar_todo(i+1, evidencia, trace, depth+1);
    }else{
        // Caso 2: la variable Y no está en la evidencia
        // debemos marginalizar (sumar) sobre todos los posibles valores de Y
        // esto implementa: Σ_y P(Y=y | padres) * P(resto | Y=y, evidencia)
        
        // acumulador para la suma sobre todos los valores de Y
        double suma=0.0;
        
        // si hay traza, indicamos que vamos a enumerar sobre Y
        if(trace){ 
            (*trace) << indent << "Enumerando " << Y->nombre 
                    << " sobre " << Y->valores.size() << " valores\n"; 
        }
        
        // iteramos sobre cada posible valor que puede tomar Y
        for(const std::string& y: Y->valores){
            // asignamos temporalmente Y=y en el mapa de evidencia
            // esto permite que las llamadas recursivas vean este valor
            evidencia[Y->nombre]=y;
            
            // obtenemos P(Y=y | padres) dado el contexto actual
            double py = Y->cpt->condicionada(evidencia, y);
            
            // si hay traza, mostramos el valor que estamos probando
            if(trace){ 
                (*trace) << indent << "  Probar " << Y->nombre 
                        << "=" << y << " -> P=" << py << "\n"; 
            }
            
            // llamada recursiva para procesar las variables restantes
            // con Y fijado temporalmente a este valor
            // el resultado es P(resto | Y=y, evidencia)
            double sub = enumerar_todo(i+1, evidencia, trace, depth+2);
            
            // calculamos la contribución de este valor específico de Y
            // que es: P(Y=y | padres) * P(resto | Y=y, evidencia)
            double contrib = py * sub;
            
            // Cada valor de Y contribuye con P(Y=y | padres) * (suma
            // recursiva sobre las variables posteriores).
            if(trace){ 
                (*trace) << indent << "  Resultado recursivo: " << sub 
                        << " contrib=" << contrib << "\n"; 
            }
            
            // acumulamos la contribución de este valor a la suma total
            suma += contrib;
            
            // IMPORTANTE: removemos Y del mapa de evidencia antes de
            // probar el siguiente valor, para no contaminar iteraciones
            evidencia.erase(Y->nombre);
        }
        
        // si hay traza, mostramos el resultado final de la suma
        if(trace){ 
            (*trace) << indent << "Suma para " << Y->nombre 
                    << " = " << suma << "\n"; 
        }
        
        // retornamos la suma sobre todos los valores de Y
        return suma;
    }
}

// realiza una consulta de inferencia por enumeración exacta
// calcula P(variable | evidencia) para todos los valores de 'variable'
// implementa el algoritmo ENUMERATION-ASK del libro de Russell & Norvig
//
// Parámetros:
// variable: la variable sobre la que queremos calcular la distribución
// evidencia: las variables observadas (mapa variable -> valor)
// trace: stream opcional para ver el proceso detallado de inferencia
//
// Retorna: vector de pares (valor, probabilidad) con la distribución normalizada
std::vector<std::pair<std::string,double>> InferenceEngine::consultar_enumeracion(
    const std::string& variable,
    const std::unordered_map<std::string,std::string>& evidencia,
    std::ostream* trace) const{

    // verificamos que la variable de consulta exista en la red
    auto it = rb_.nodos.find(variable);
    if(it==rb_.nodos.end()) 
        throw std::runtime_error("Variable desconocida: "+variable);
    
    // obtenemos el puntero al nodo de la variable de consulta
    Nodo* Q = it->second.get();

    // vector que contendrá la distribución de probabilidad resultante
    // cada entrada es un par (valor, probabilidad)
    std::vector<std::pair<std::string,double>> dist; 
    // reservamos espacio para evitar realocaciones
    dist.reserve(Q->valores.size());
    
    // calculamos la probabilidad conjunta no normalizada para cada valor
    // iteramos sobre todos los posibles valores de la variable de consulta
    for(const std::string& x: Q->valores){
        // Para cada valor x de la variable de consulta, creamos una
        // evidencia extendida que incluye la asignación variable=x
        auto e = evidencia; // copiamos la evidencia original
        e[variable] = x; // agregamos la asignación variable=x
        
        // si hay traza, mostramos qué estamos calculando
        if(trace){ 
            (*trace) << "--- Calcular P(" << variable << "=" << x 
                    << " , evidencia) ---\n"; 
        }
        
        // llamamos a enumerar_todo empezando desde el primer nodo (índice 0)
        // esto calcula P(variable=x, evidencia) = P(variable=x ∧ evidencia)
        // que es la probabilidad conjunta no normalizada
        double v = enumerar_todo(0, e, trace, 0);
        
        // si hay traza, mostramos el valor calculado
        if(trace){ 
            (*trace) << "  => P_unorm(" << variable << "=" << x 
                    << ") = " << v << "\n\n"; 
        }
        
        // v es la probabilidad no normalizada; la normalización se
        // hace después de computar todas las entradas de la distribución
        // para normalizar necesitamos dividir por la suma de todas las probabilidades
        dist.push_back({x, v});
    }
    
    // Fase de normalización: necesitamos dividir cada probabilidad por Z
    // donde Z = Σ P(variable=x, evidencia) para todos los valores x
    // esto da P(variable=x | evidencia) = P(variable=x, evidencia) / Z
    
    // calculamos la constante de normalización Z
    double Z=0; 
    for(auto &p: dist) 
        Z+=p.second; 
    
    // verificamos que Z no sea 0 (evidencia inconsistente o error)
    if(Z==0) 
        throw std::runtime_error("Normalización 0");
    
    // normalizamos dividiendo cada probabilidad por Z
    for(auto &p: dist) 
        p.second/=Z;
    
    // si hay traza, mostramos información de la normalización
    if(trace){ 
        (*trace) << "Normalización Z=" << Z << "\n"; 
        (*trace) << "Distribución normalizada:\n"; 
        for(auto &p: dist) 
            (*trace) << p.first << ": " << p.second << "\n"; 
    }
    
    // retornamos la distribución de probabilidad normalizada
    return dist;
}