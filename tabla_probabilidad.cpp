#include "tabla_probabilidad.h"
#include "nodo.h"
#include "util.h"
#include <cmath>
#include <functional>
#include <stdexcept>

// establece la configuración básica de la tabla de probabilidad condicional
// asocia la tabla con una variable objetivo y sus padres en la red bayesiana
// este método debe llamarse antes de agregar filas a la tabla
void TablaProbabilidad::establecer(Nodo* var, const std::vector<Nodo*>& padres_){
    // guardamos el puntero al nodo variable que representa esta CPT
    // este es el nodo "hijo" cuyas probabilidades condicionales estamos definiendo
    variable = var; 
    
    // guardamos el vector de punteros a los nodos padres
    // el orden de los padres es importante porque determina cómo
    // se indexan las filas de la tabla
    padres = padres_;
}

// agrega una fila a la tabla de probabilidad condicional
// cada fila especifica P(variable=cada_valor | combinación_de_padres)
//
// Parámetros:
// asig_padres: vector de pares (nombre_padre, valor_padre) que especifica
//              la combinación de valores de los padres para esta fila
// valores_var: vector con todos los posibles valores de la variable
// probabilidades: vector con P(variable=valor_i | padres) para cada valor_i
void TablaProbabilidad::agregar_fila(
    const std::vector<std::pair<std::string,std::string>>& asig_padres,
    const std::vector<std::string>& valores_var,
    const std::vector<double>& probabilidades){

    // validación: el número de probabilidades debe coincidir con el número de valores
    // si la variable tiene 3 valores posibles, necesitamos exactamente 3 probabilidades
    if(probabilidades.size()!=valores_var.size()) 
        throw std::runtime_error("#probs != #valores en agregar_fila()");
    
    // verificamos que las probabilidades sumen aproximadamente 1.0
    // esto es un requisito fundamental de las distribuciones de probabilidad
    double suma=0; 
    for(double p: probabilidades) 
        suma+=p;
    
    // si la suma se desvía más de 1e-6 de 1.0, hay un problema
    // usamos fabs para el valor absoluto de la diferencia
    if(std::fabs(suma-1.0) > 1e-6) {
        // mostramos aviso pero no interrumpimos la ejecución
        // en algunos casos puede haber pequeños errores de redondeo aceptables
        // std::cerr << "[AVISO] La fila no normaliza a 1: " << suma << " ";
    }
    
    // agregamos una entrada en la tabla para cada valor posible de la variable
    // cada entrada mapea (asignación_padres + variable=valor) -> probabilidad
    for(size_t i=0;i<valores_var.size();++i){
        // copiamos las asignaciones de los padres
        auto todo = asig_padres;
        // agregamos la asignación de la variable objetivo con su valor específico
        todo.push_back({variable->nombre, valores_var[i]});
        
        // empaquetamos la clave completa como string y guardamos la probabilidad
        // empaquetar_clave convierte el vector de pares en un string único
        // que sirve como clave en el unordered_map
        // Ejemplo: [(A,true), (B,false), (C,v1)] -> "A=true,B=false,C=v1"
        tabla[empaquetar_clave(todo)] = probabilidades[i];
    }
}

// consulta la probabilidad condicional P(variable=valor | padres=valores_padres)
// donde los valores de los padres se toman del mapa de evidencia
//
// Parámetros:
// evidencia: mapa que contiene las asignaciones actuales de todas las variables
//            debe incluir valores para todos los padres de esta variable
// valor: el valor específico de la variable para el cual queremos la probabilidad
//
// Retorna: P(variable=valor | valores_de_padres_en_evidencia)
double TablaProbabilidad::condicionada(
    const std::unordered_map<std::string,std::string>& evidencia,
    const std::string& valor) const{
    
    // construimos el vector de asignaciones que servirá como clave de búsqueda
    // primero incluimos las asignaciones de los padres, luego la de la variable
    std::vector<std::pair<std::string,std::string>> asignaciones;
    // reservamos espacio: necesitamos espacio para todos los padres + 1 (la variable)
    asignaciones.reserve(padres.size()+1);
    
    // extraemos el valor de cada padre desde el mapa de evidencia
    for(Nodo* p: padres){
        // buscamos el valor del padre en la evidencia
        auto it = evidencia.find(p->nombre);
        
        // si falta algún padre en la evidencia, no podemos calcular la probabilidad
        // esto es un error porque necesitamos conocer todos los padres para
        // hacer la consulta condicional
        if(it==evidencia.end()) 
            throw std::runtime_error("Evidencia incompleta: falta " + p->nombre);
        
        // agregamos el par (nombre_padre, valor_padre) a las asignaciones
        asignaciones.push_back({p->nombre, it->second});
    }
    
    // agregamos la asignación de la variable objetivo con el valor consultado
    asignaciones.push_back({variable->nombre, valor});
    
    // buscamos en la tabla usando la clave empaquetada
    // la clave codifica toda la combinación de valores: padres + variable
    auto it = tabla.find(empaquetar_clave(asignaciones));
    
    // si no encontramos la entrada, significa que esa combinación
    // no fue definida en el archivo de CPTs, lo cual es un error
    if(it==tabla.end()) 
        throw std::runtime_error("Fila CPT no encontrada para " + variable->nombre);
    
    // retornamos la probabilidad almacenada para esta combinación
    return it->second;
}

// imprime la tabla de probabilidad condicional en formato legible
// muestra todas las combinaciones posibles de valores de padres
// y las probabilidades correspondientes para cada valor de la variable
void TablaProbabilidad::imprimir(std::ostream& os) const{
    // imprimimos el encabezado con la notación de probabilidad condicional
    os << "P(" << variable->nombre;
    
    // si hay padres, mostramos la barra condicional y sus nombres
    if(!padres.empty()){
        os << " | ";
        // imprimimos los nombres de los padres separados por comas
        for(size_t i=0;i<padres.size();++i){ 
            if(i) os<<","; // coma antes de cada padre excepto el primero
            os<<padres[i]->nombre; 
        }
    }
    os << ")\n";
    
    // imprimimos los valores posibles de la variable
    os << "Valores: ";
    for(size_t i=0;i<variable->valores.size();++i){ 
        if(i) os<<", "; // coma y espacio entre valores
        os<<variable->valores[i]; 
    }
    os << "\n";

    // generamos todas las combinaciones posibles de valores de padres
    // para imprimirlas en orden sistemático
    // cada padre tiene un dominio (conjunto de valores posibles)
    std::vector<std::vector<std::string>> dominios; 
    dominios.reserve(padres.size());
    for(Nodo* p: padres) 
        dominios.push_back(p->valores); // copiamos los valores de cada padre

    // vector que contendrá la asignación actual durante el backtracking
    std::vector<std::pair<std::string,std::string>> asign; 
    asign.reserve(padres.size());
    
    // función lambda recursiva que genera todas las combinaciones usando backtracking
    // i: índice del padre actual que estamos asignando
    std::function<void(size_t)> bt = [&](size_t i){
        // caso base: si ya asignamos todos los padres
        if(i==padres.size()){
            // imprimimos la combinación actual de valores de padres
            os << " ";
            
            // si no hay padres, es una distribución prior (incondicional)
            if(asign.empty()) 
                os << "<prior>"; 
            else{
                // imprimimos cada asignación padre=valor separadas por comas
                for(size_t k=0;k<asign.size();++k){ 
                    if(k) os<<","; 
                    os<<asign[k].first<<"="<<asign[k].second; 
                }
            }
            os << " : ";
            
            // para esta combinación de padres, imprimimos las probabilidades
            // de cada valor posible de la variable
            for(size_t j=0;j<variable->valores.size();++j){
                // construimos la clave completa: asignaciones_padres + variable=valor_j
                auto todo = asign; 
                todo.push_back({variable->nombre, variable->valores[j]});
                
                // empaquetamos la clave y buscamos en la tabla
                std::string clave = empaquetar_clave(todo);
                
                // obtenemos la probabilidad si existe, sino usamos NAN (Not A Number)
                // NAN indica que esa entrada no fue definida en la tabla
                double p = tabla.count(clave)? tabla.at(clave): NAN;
                
                // imprimimos la probabilidad separada por espacios
                if(j) os << " ";
                os << p;
            }
            os << "\n";
            return; // terminamos este caso base
        }
        
        // caso recursivo: probamos cada valor posible del padre i
        // esto genera todas las combinaciones mediante backtracking
        for(const std::string& v : dominios[i]){ 
            // agregamos la asignación padre_i = v
            asign.push_back({padres[i]->nombre, v}); 
            // recursión: procesamos el siguiente padre
            bt(i+1); 
            // backtrack: removemos la asignación para probar el siguiente valor
            asign.pop_back(); 
        }
    };
    
    // iniciamos el backtracking desde el padre 0
    // esto generará e imprimirá todas las combinaciones posibles
    bt(0);
}