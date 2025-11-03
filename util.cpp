#include "util.h"
#include <algorithm>

std::string recortar(const std::string& s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);
}

std::vector<std::string> dividir(const std::string& s, char sep){
    std::vector<std::string> partes; std::string cur;
    for(char c: s){ if(c==sep){ if(!cur.empty()){ partes.push_back(recortar(cur)); cur.clear(); } } else cur.push_back(c); }
    if(!cur.empty()) partes.push_back(recortar(cur));
    for(auto &x: partes) x = recortar(x);
    return partes;
}

std::string empaquetar_clave(const std::vector<std::pair<std::string,std::string>>& asignaciones){
    std::vector<std::string> tmp; tmp.reserve(asignaciones.size());
    for(const auto &p: asignaciones) tmp.push_back(p.first+"="+p.second);
    std::sort(tmp.begin(), tmp.end());
    std::string res; for(size_t i=0;i<tmp.size();++i){ if(i) res+=","; res+=tmp[i]; }
    return res;
}