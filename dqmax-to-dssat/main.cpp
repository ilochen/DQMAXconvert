#include "Parser.h"
#include "Converter.h"
#include "Writer.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <string>
#include <vector>

void print_help() {
    std::cout << "Usage: dqmax2dssat [options] <input.qdimacs> <output.qdimacs>\n\n"
              << "Convert a DQDIMACS (DQMax#SAT) formula into a DSSAT formula\n"
              << "according to Theorem-1 transformation.\n\n"
              << "Options:\n"
              << "  --help       Show this help message and exit\n"
              << "  --verbose    Print detailed statistics about the conversion\n\n"
              << "Example:\n"
              << "  ./dqmax2dssat --verbose int_001_dqmax.qdimacs int_001_dssat.qdimacs\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    bool verbose = false;
    std::vector<std::string> positional;

    // Parse flags
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_help();
            return 0;
        } else if (arg == "--verbose" || arg == "-v") {
            verbose = true;
        } else {
            positional.push_back(arg);
        }
    }

    if (positional.size() != 2) {
        std::cerr << "[ERROR] Expected <input.qdimacs> <output.qdimacs>\n";
        print_help();
        return 1;
    }

    std::string infile = positional[0];
    std::string outfile = positional[1];

    Parser parser(infile);
    if (!parser.parse()) {
        std::cerr << "[ERROR] Failed to parse input file: " << infile << "\n";
        return 1;
    }

    // Convert
    auto t0 = std::chrono::high_resolution_clock::now();
    Converter conv;
    DSSATModel model = conv.convert(parser);
    auto t1 = std::chrono::high_resolution_clock::now();

    // Write result
    if (!Writer::write(model, outfile)) {
        std::cerr << "[ERROR] Failed to write output file: " << outfile << "\n";
        return 1;
    }

    // Print stats if verbose
    if (verbose) {
        std::cout << "detected existential variables in dependency sets (hatZ): ";
        if (model.hatZList.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < model.hatZList.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << model.hatZList[i];
            }
            std::cout << "\n";
        }
        std::cout << "Converted " << model.numZFree
                  << " dependency-free existential variables\n";
        std::cout << "Converted " << model.numHatZ
                  << " existential variables in dependency sets\n";
        std::cout << "Added number of variables: " << model.addedVars << "\n";
        std::cout << "Added number of clauses: "  << model.addedClauses << "\n";
        std::cout << "Old (V, C): (" << model.oldV << ", " << model.oldC << ")\n";
        std::cout << "New (V, C): (" << model.V    << ", " << model.C    << ")\n";
    }

    // Conversion timing + confirmation
    double elapsed = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Converted " << infile << " -> " << outfile
              << " in " << std::fixed << std::setprecision(6)
              << elapsed << " seconds\n";

    return 0;
}
