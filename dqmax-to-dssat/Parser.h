#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include <set>
#include <map>

struct Clause {
    std::vector<int> lits;
};

class Parser {
public:
    Parser(const std::string& filename);
    bool parse();

    int getVarCount() const { return V; }
    int getClauseCount() const { return C; }
    const std::set<int>& getASet() const { return aSet; }
    const std::map<int, std::set<int>>& getDDeps() const { return dDeps; }
    const std::vector<Clause>& getClauses() const { return clauses; }
    const std::vector<std::string>& getComments() const { return comments; }

private:
    std::string filename;
    int V, C;
    std::set<int> aSet;
    std::map<int, std::set<int>> dDeps;
    std::vector<Clause> clauses;
    std::vector<std::string> comments;
};

#endif
