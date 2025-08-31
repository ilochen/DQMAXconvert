#include "Converter.h"
#include <set>
#include <map>
#include <algorithm>

DSSATModel Converter::convert(const Parser& parser) {
    DSSATModel model;
    model.comments = parser.getComments();

    int V = parser.getVarCount();
    int C = parser.getClauseCount();
    std::set<int> Y = parser.getASet();
    std::map<int, std::set<int>> dDeps = parser.getDDeps();

    // Save original counts for stats
    model.oldV = V;
    model.oldC = C;

    // X = LHS(d)
    std::set<int> X;
    for (auto &kv : dDeps) X.insert(kv.first);

    // All variables 1..V
    std::set<int> all;
    for (int i = 1; i <= V; ++i) all.insert(i);

    // Z = existentials = all \ (X ∪ Y)
    std::set<int> Z;
    std::set_difference(all.begin(), all.end(),
                        Y.begin(), Y.end(),
                        std::inserter(Z, Z.begin()));
    // remove X
    for (int v : X) Z.erase(v);

    model.numZAll = (int)Z.size();

    // rhs_union = ⋃ deps(X)
    std::set<int> rhs;
    for (auto &kv : dDeps)
        rhs.insert(kv.second.begin(), kv.second.end());

    // hatZ = Z ∩ rhs_union
    std::set<int> hatZ;
    for (int z : Z) if (rhs.count(z)) hatZ.insert(z);

    model.numHatZ = (int)hatZ.size();
    model.hatZList.assign(hatZ.begin(), hatZ.end());

    // Z \ hatZ (dependency-free existentials)
    std::set<int> Zfree;
    std::set_difference(Z.begin(), Z.end(),
                        hatZ.begin(), hatZ.end(),
                        std::inserter(Zfree, Zfree.begin()));
    model.numZFree = (int)Zfree.size();

    // Create Z' for each z ∈ hatZ
    std::map<int,int> zprime;
    int nextVar = V + 1;
    for (int z : hatZ) {
        zprime[z] = nextVar++;
    }
    model.zprime = zprime;
    model.addedVars = (int)zprime.size();

    // new aSet = Y ∪ Z'
    model.aSet = Y;
    for (auto &kv : zprime) model.aSet.insert(kv.second);

    // Rewire X deps: replace z∈hatZ by z'
    for (auto &kv : dDeps) {
        std::vector<int> deps;
        for (int u : kv.second) {
            auto it = zprime.find(u);
            deps.push_back(it == zprime.end() ? u : it->second);
        }
        std::sort(deps.begin(), deps.end());
        deps.erase(std::unique(deps.begin(), deps.end()), deps.end());
        model.dDeps[kv.first] = deps;
    }

    // Promote ALL Z to max with deps = all Y
    std::vector<int> ydeps(Y.begin(), Y.end());
    for (int z : Z) {
        model.dDeps[z] = ydeps;  // empty if Y empty
    }

    // Copy original clauses
    model.clauses = parser.getClauses();

    // Add equivalences (z ↔ z') : two binary clauses per pair
    for (auto &kv : zprime) {
        int z = kv.first, zp = kv.second;
        Clause c1; c1.lits = { -zp,  z};
        Clause c2; c2.lits = { -z,  zp};
        model.clauses.push_back(c1);
        model.clauses.push_back(c2);
    }

    model.V = nextVar - 1;
    model.addedClauses = 2 * (int)zprime.size();
    model.C = C + model.addedClauses;

    return model;
}
