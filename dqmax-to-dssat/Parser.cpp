#include "Parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

Parser::Parser(const std::string& fname) : filename(fname), V(0), C(0) {}

bool Parser::parse() {
    std::ifstream in(filename);
    if (!in.is_open()) return false;
    std::string line;

    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (line[0] == 'c') {
            comments.push_back(line);
            continue;
        }
        if (line[0] == 'p') {
            std::istringstream iss(line);
            std::string tmp; iss >> tmp; // p
            std::string cnf; iss >> cnf; // cnf
            iss >> V >> C;
            continue;
        }
        if (line[0] == 'a') {
            std::istringstream iss(line);
            char a; iss >> a;
            int var;
            while (iss >> var && var != 0) {
                aSet.insert(var);
            }
            continue;
        }
        if (line[0] == 'd') {
            std::istringstream iss(line);
            char d; iss >> d;
            int lhs; iss >> lhs;
            int dep;

            // Catch empty dependency sets
            dDeps.emplace(lhs, std::set<int>{});

            while (iss >> dep && dep != 0) {
                dDeps[lhs].insert(dep);
            }
            continue;
        }
        // Clause
        std::istringstream iss(line);
        int lit;
        Clause cl;
        while (iss >> lit && lit != 0) {
            cl.lits.push_back(lit);
        }
        if (!cl.lits.empty())
            clauses.push_back(cl);
    }
    return true;
}
