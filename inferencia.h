#ifndef INFERENCIA_H
#define INFERENCIA_H
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <ostream>

struct RedBayesiana; struct Nodo;

// Clase orientada a objetos para realizar inferencia por enumeración.
// Permite habilitar una traza paso a paso enviando un std::ostream* (por ejemplo &std::cout).
class InferenceEngine{
public:
    explicit InferenceEngine(const RedBayesiana& rb);

    // Realiza la consulta para la variable dada con la evidencia provista.
    // Si 'trace' != nullptr, se emitirá una traza paso a paso en ese stream.
    std::vector<std::pair<std::string,double>> consultar_enumeracion(
        const std::string& variable,
        const std::unordered_map<std::string,std::string>& evidencia,
        std::ostream* trace = nullptr) const;

private:
    const RedBayesiana& rb_;
    // Orden topológico precalculado de nodos de la red. Se usa para
    // recorrer las variables en un orden consistente durante la
    // enumeración (Kahn). Guardar este vector evita recalcularlo
    // en cada llamada a la función de enumeración recursiva.
    std::vector<Nodo*> orden_;

    double enumerar_todo(size_t i, std::unordered_map<std::string,std::string>& evidencia,
                         std::ostream* trace, int depth) const;
};

#endif // INFERENCIA_H