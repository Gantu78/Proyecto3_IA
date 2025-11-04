#include "red_bayesiana.h"
#include "nodo.h"
#include "tabla_probabilidad.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <stdexcept>

// método auxiliar que obtiene un nodo existente o crea uno nuevo si no existe
// esto permite construcción incremental de la red durante la carga de archivos
// garantiza que cada nombre de nodo tenga exactamente un objeto Nodo asociado
Nodo* RedBayesiana::obtener_o_crear(const std::string& nombre){
    // buscamos el nodo en el mapa de nodos existentes
    auto it = nodos.find(nombre);
    
    // Si no existe el nodo con este nombre, lo creamos y lo almacenamos
    // en el mapa `nodos`. Usamos std::make_unique para garantizar
    // la correcta construcción y propiedad exclusiva del puntero.
    // make_unique crea un unique_ptr que gestiona automáticamente
    // la memoria y evita fugas
    if(it==nodos.end()) 
        nodos[nombre] = std::make_unique<Nodo>(nombre);
    
    // Devolvemos el puntero crudo gestionado por el unique_ptr.
    // Es seguro usar el puntero crudo mientras el mapa `nodos`
    // mantenga la entrada (lo cual ocurre durante toda la vida del objeto).
    // El unique_ptr mantiene la propiedad de la memoria.
    return nodos[nombre].get(); 
}

// método que obtiene un nodo existente o retorna nullptr si no existe
// versión const que no modifica la red, útil para consultas
Nodo* RedBayesiana::obtener(const std::string& nombre) const{
    // buscamos el nodo en el mapa
    auto it = nodos.find(nombre);
    // si no lo encontramos, retornamos nullptr
    if(it==nodos.end()) return nullptr;
    // si existe, retornamos el puntero crudo del unique_ptr
    return it->second.get();
}

// carga la estructura de la red (grafo dirigido) desde un archivo de texto
// el archivo contiene líneas con el formato: "padre -> hijo"
// cada línea representa una arista dirigida en el grafo de la red bayesiana
void RedBayesiana::cargar_estructura(const std::string& ruta){
    // intentamos abrir el archivo en modo lectura
    std::ifstream in(ruta); 
    // si no se puede abrir, lanzamos excepción con mensaje descriptivo
    if(!in) 
        throw std::runtime_error("No se puede abrir estructura: "+ruta);
    
    std::string linea; // buffer para leer cada línea
    int ln=0; // contador de líneas para mensajes de error informativos
    
    // leemos el archivo línea por línea hasta el final
    while(std::getline(in, linea)){
        ++ln; // incrementamos contador de línea
        linea = recortar(linea); // eliminamos espacios al inicio y final
        
        // ignoramos líneas vacías y líneas de comentario (que empiezan con #)
        // esto permite documentar el archivo de estructura
        if(linea.empty()||linea[0]=='#') continue;
        
        // dividimos la línea usando '-' como delimitador
        // esperamos obtener dos partes: "padre" y "> hijo"
        auto partes = dividir(linea, '-');
        
        // validamos el formato: debe tener exactamente 2 partes
        // la segunda parte debe empezar con '>' (formando "->")
        if(partes.size()!=2 || partes[1].empty() || partes[1][0] != '>')
            throw std::runtime_error("Formato inválido en estructura línea "+
                                   std::to_string(ln)+": "+linea);
        
        // extraemos el nombre del padre (primera parte, antes del '-')
        std::string padre = recortar(partes[0]);
        // extraemos el nombre del hijo (segunda parte, después del '>')
        // usamos substr(1) para saltar el carácter '>'
        std::string hijo  = recortar(partes[1].substr(1));
        
        // Construimos la relación dirigida padre -> hijo en el grafo
        // obtener_o_crear garantiza que ambos nodos existan
        Nodo* u = obtener_o_crear(padre);
        Nodo* v = obtener_o_crear(hijo);
        
        // Agregamos punteros crudos para establecer la relación bidireccional
        // el hijo conoce a sus padres (necesario para inferencia)
        v->padres.push_back(u);
        // el padre conoce a sus hijos (útil para orden topológico)
        u->hijos.push_back(v);
        
        // Nota: la propiedad de memoria está en el mapa `nodos` con unique_ptr,
        // por lo que no hay fugas de memoria aunque usemos punteros crudos aquí
    }
}

// carga las tablas de probabilidad condicional (CPTs) desde un archivo de texto
// el archivo sigue un formato estructurado con secciones NODE, VALUES, PARENTS, TABLE
// cada nodo tiene su CPT que especifica P(nodo | padres)
void RedBayesiana::cargar_cpts(const std::string& ruta){
    // intentamos abrir el archivo en modo lectura
    std::ifstream in(ruta); 
    // si no se puede abrir, lanzamos excepción
    if(!in) 
        throw std::runtime_error("No se puede abrir CPTs: "+ruta);
    
    std::string linea; // buffer para cada línea
    int ln=0; // contador de líneas
    Nodo* actual=nullptr; // puntero al nodo que estamos procesando actualmente
    std::vector<Nodo*> padres; // vector de punteros a los padres del nodo actual
    
    // leemos el archivo línea por línea
    while(std::getline(in, linea)){
        ++ln; // incrementamos el contador de línea
        std::string t = recortar(linea); // recortamos espacios
        
        // ignoramos líneas vacías y comentarios
        if(t.empty()||t[0]=='#') continue;
        
        // procesamos según el tipo de línea que encontremos
        
        // --- Línea NODE: inicio de definición de un nodo ---
        if(t.rfind("NODE ",0)==0){
            // extraemos el nombre del nodo (todo después de "NODE ")
            std::string nombre = recortar(t.substr(5));
            // obtenemos o creamos el nodo
            actual = obtener_o_crear(nombre); 
            // limpiamos el vector de padres para este nuevo nodo
            padres.clear();
            // creamos la tabla de probabilidad si aún no existe
            if(!actual->cpt) 
                actual->cpt = std::make_unique<TablaProbabilidad>();
        }
        
        // --- Línea VALUES: define los posibles valores del nodo ---
        else if(t.rfind("VALUES:",0)==0){
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("VALUES sin NODE en línea "+std::to_string(ln));
            // extraemos la parte después de "VALUES:"
            std::string rest = recortar(t.substr(7)); 
            // dividimos los valores por espacios y los guardamos
            // por ejemplo: "VALUES: true false" -> ["true", "false"]
            actual->valores = dividir(rest, ' ');
        }
        
        // --- Línea PARENTS: define los padres del nodo ---
        else if(t.rfind("PARENTS:",0)==0){
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("PARENTS sin NODE en línea "+std::to_string(ln));
            // extraemos los nombres de los padres
            std::string rest = recortar(t.substr(8)); 
            auto nombres = dividir(rest,' '); // dividimos por espacios
            padres.clear(); // limpiamos el vector de padres
            // obtenemos o creamos cada nodo padre
            for(auto &pn: nombres){ 
                if(pn.empty()) continue; // saltamos strings vacíos
                padres.push_back(obtener_o_crear(pn)); 
            }
        }
        
        // --- Línea TABLE: inicio de la tabla de probabilidades ---
        else if(t=="TABLE"){
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("TABLE sin NODE en línea "+std::to_string(ln));
            
            // Llegamos a la sección TABLE: aquí sabemos que las filas
            // siguientes dependen de los padres declarados anteriormente.
            // Inicializamos la TablaProbabilidad con la variable objetivo (`actual`)
            // y el vector `padres` en el orden que fueron declarados.
            // Esto prepara la estructura interna de la tabla para recibir filas.
            actual->cpt->establecer(actual, padres);
        }
        
        // --- Línea END: fin de definición del nodo ---
        else if(t=="END"){
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("END sin NODE en línea "+std::to_string(ln));
            
            // finalizamos la configuración de la CPT del nodo actual
            // esto asegura que todos los datos estén correctamente establecidos
            actual->cpt->establecer(actual, padres);
            
            // limpiamos las variables para procesar el siguiente nodo
            actual=nullptr; 
            padres.clear();
        }
        
        // --- Línea p:: probabilidad prior para nodos sin padres ---
        else if(t.rfind("p:",0)==0){
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("p: sin NODE en línea "+std::to_string(ln));
            
            // extraemos los números después de "p:"
            auto toks = dividir(recortar(t.substr(2)), ' ');
            
            // convertimos cada token string a double y lo guardamos
            std::vector<double> probs; 
            probs.reserve(toks.size());
            for(auto &x: toks) 
                probs.push_back(std::stod(x));
            
            // Caso especial: fila `p:` para nodos sin padres (distribución prior)
            // estos nodos raíz tienen probabilidades incondicionales
            // establecemos con vector de padres vacío
            actual->cpt->establecer(actual, {});
            
            // creamos un vector vacío de asignaciones de padres
            std::vector<std::pair<std::string,std::string>> vacio;
            
            // agregamos la fila con las probabilidades para cada valor del nodo
            // como no hay padres, solo hay una fila en la CPT
            actual->cpt->agregar_fila(vacio, actual->valores, probs);
        }
        
        // --- Línea de probabilidad condicional: contiene condiciones y probabilidades ---
        else{
            // verificamos que estemos procesando un nodo
            if(!actual) 
                throw std::runtime_error("Fila sin NODE en línea "+std::to_string(ln));
            
            // buscamos el separador ':' que divide condiciones de probabilidades
            // formato: "Padre1=valor1,Padre2=valor2: prob1 prob2 prob3"
            auto col = t.find(':'); 
            if(col==std::string::npos) 
                throw std::runtime_error("Falta ':' en línea "+std::to_string(ln));
            
            // izq contiene las condiciones de los padres (antes del ':')
            std::string izq = recortar(t.substr(0,col));
            // der contiene las probabilidades (después del ':')
            std::string der = recortar(t.substr(col+1));
            
            // vector que contendrá las asignaciones de los padres
            // cada par es (nombre_padre, valor)
            std::vector<std::pair<std::string,std::string>> asign;
            
            // procesamos las condiciones de los padres si existen
            if(!izq.empty()){
                // dividimos las asignaciones por comas
                // ej: "A=true,B=false" -> ["A=true", "B=false"]
                auto pares = dividir(izq, ',');
                
                // procesamos cada par variable=valor
                for(auto &kv: pares){
                    // buscamos el '=' en cada par
                    auto eq = kv.find('='); 
                    if(eq==std::string::npos) 
                        throw std::runtime_error("Falta '=' en línea "+std::to_string(ln));
                    
                    // extraemos nombre de variable (antes del '=')
                    std::string k = recortar(kv.substr(0,eq)); 
                    // extraemos valor (después del '=')
                    std::string v = recortar(kv.substr(eq+1));
                    
                    // guardamos el par (variable, valor)
                    asign.push_back({k,v});
                }
            }
            
            // parseamos las probabilidades desde la parte derecha
            auto toks = dividir(der, ' ');
            std::vector<double> probs; 
            for(auto &x: toks) 
                if(!x.empty()) // ignoramos tokens vacíos
                    probs.push_back(std::stod(x));
            
            // Añadimos la fila a la tabla de probabilidad condicional
            // asign especifica la combinación de valores de los padres
            // probs contiene las probabilidades para cada valor del nodo actual
            // dada esa combinación de valores de padres
            // Ejemplo: si asign = [(A,true), (B,false)] y el nodo tiene valores [v1,v2,v3]
            // entonces probs = [P(v1|A=true,B=false), P(v2|A=true,B=false), P(v3|A=true,B=false)]
            actual->cpt->agregar_fila(asign, actual->valores, probs);
        }
    }
    // al terminar de leer el archivo, todas las CPTs están cargadas
}

// imprime la estructura de la red en orden topológico
// muestra cada nodo con sus predecesores (padres) de forma legible
void RedBayesiana::imprimir_estructura(std::ostream& os) const{
    // calculamos el orden topológico para imprimir en orden lógico
    // usamos el mismo algoritmo de Kahn que en la inferencia
    
    // mapa de grados de entrada (número de padres no procesados)
    std::unordered_map<Nodo*,int> indeg; 
    // cola de nodos listos para procesar (sin padres pendientes)
    std::queue<Nodo*> q; 
    // vector resultado con el orden topológico
    std::vector<Nodo*> topo;
    
    // inicializamos los grados de entrada de todos los nodos
    for(auto &kv: nodos){ 
        auto* n = kv.second.get(); 
        // el grado de entrada es el número de padres
        indeg[n]=(int)n->padres.size(); 
        // si no tiene padres, puede procesarse inmediatamente
        if(indeg[n]==0) q.push(n);
    } 
    
    // procesamos nodos en orden topológico
    while(!q.empty()){ 
        // extraemos el siguiente nodo sin padres pendientes
        auto* u=q.front(); 
        q.pop(); 
        // lo agregamos al resultado
        topo.push_back(u); 
        // decrementamos el grado de entrada de sus hijos
        for(auto* v: u->hijos){ 
            // si un hijo llega a grado 0, lo encolamos
            if(--indeg[v]==0) q.push(v);
        } 
    }
    
    // imprimimos encabezado
    os << "Estructura (predecesores):\n";
    
    // imprimimos cada nodo en orden topológico con sus padres
    for(auto* n: topo){
        os << "- "<<n->nombre<<" <- ";
        
        // si no tiene padres, es un nodo raíz
        if(n->padres.empty()) 
            os<<"(raíz)";
        else{ 
            // imprimimos la lista de padres separados por comas
            for(size_t i=0;i<n->padres.size();++i){ 
                // agregamos coma antes de cada padre excepto el primero
                if(i) os<<","; 
                // imprimimos el nombre del padre
                os<<n->padres[i]->nombre; 
            } 
        }
        os << "\n";
    }
}

// imprime las tablas de probabilidad condicional de todos los nodos
// las imprime en orden topológico para facilitar la lectura
void RedBayesiana::imprimir_cpts(std::ostream& os) const{
    // calculamos orden topológico (mismo algoritmo que en imprimir_estructura)
    // lo hacemos de nuevo porque este método es independiente
    
    // estructuras para el algoritmo de Kahn
    std::unordered_map<Nodo*,int> indeg; 
    std::queue<Nodo*> q; 
    std::vector<Nodo*> topo;
    
    // inicializamos grados de entrada
    for(auto &kv: nodos){ 
        auto* n = kv.second.get(); 
        indeg[n]=(int)n->padres.size(); 
        if(indeg[n]==0) q.push(n);
    } 
    
    // calculamos el orden topológico
    while(!q.empty()){ 
        auto* u=q.front(); 
        q.pop(); 
        topo.push_back(u); 
        for(auto* v: u->hijos){ 
            if(--indeg[v]==0) q.push(v);
        } 
    }
    
    // imprimimos la CPT de cada nodo en orden topológico
    for(auto* n: topo){ 
        // solo imprimimos si el nodo tiene una CPT definida
        if(n->cpt) 
            n->cpt->imprimir(os); 
        // agregamos línea en blanco para separar visualmente las CPTs
        os << "\n"; 
    }
}