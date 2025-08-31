#ifndef CONVERTER_H
#define CONVERTER_H

#include "Parser.h"

struct DSSATModel {
    int V, C;
    std::set<int> aSet;
    std::map<int, std::vector<int>> dDeps;
    std::vector<Clause> clauses;
    std::vector<std::string> comments;
    std::map<int,int> zprime;

    int oldV = 0;
    int oldC = 0;
    int addedVars = 0;     // |Z'|
    int addedClauses = 0;  // 2 * |Z'|
    int numZAll = 0;       // |Z|
    int numHatZ = 0;       // |hatZ|
    int numZFree = 0;      // |Z \ hatZ|
    std::vector<int> hatZList; // explicit IDs of Z in dependency sets
};

class Converter {
public:
    DSSATModel convert(const Parser& parser);
};

#endif