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
    std::vector<Nodo*> orden_; // orden topológico precalculado

    double enumerar_todo(size_t i, std::unordered_map<std::string,std::string>& evidencia,
                         std::ostream* trace, int depth) const;
};

#endif // INFERENCIA_H