#ifndef UTIL_H
#define UTIL_H
#include <string>
#include <vector>
#include <utility>

std::string recortar(const std::string& s);
std::vector<std::string> dividir(const std::string& s, char sep);
std::string empaquetar_clave(const std::vector<std::pair<std::string,std::string>>& asignaciones);

#endif // UTIL_H