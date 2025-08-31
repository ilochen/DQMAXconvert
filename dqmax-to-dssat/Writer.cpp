#include "Writer.h"
#include <fstream>
#include <iostream>

bool Writer::write(const DSSATModel& model, const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) return false;

    for (auto &c : model.comments)
        out << c << "\n";

    // z' mapping comment block
    if (!model.zprime.empty()) {
        out << "c zprime mapping (z -> z'):\n";
        for (const auto& kv : model.zprime) {
            out << "c   " << kv.first << " -> " << kv.second << "\n";
        }
    }

    out << "p cnf " << model.V << " " << model.C << "\n";

    if (!model.aSet.empty()) {
        out << "a ";
        for (int v : model.aSet) out << v << " ";
        out << "0\n";
    }

    for (auto &kv : model.dDeps) {
        out << "d " << kv.first << " ";
        for (int u : kv.second) out << u << " ";
        out << "0\n";
    }

    for (auto &cl : model.clauses) {
        for (int lit : cl.lits) out << lit << " ";
        out << "0\n";
    }
    return true;
}
