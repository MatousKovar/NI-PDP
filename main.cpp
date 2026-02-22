#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <dirent.h> // POSIX knihovna pro práci se složkami
#include "common/Board.h"
#include "solvers/SequentialSolver.h"

int main() {
    std::cout << "=== NI-PDP: SQX Davkove zpracovani (C++14) ===\n\n";

    std::ofstream outfile("results.txt");
    if (!outfile.is_open()) {
        std::cerr << "Kriticka chyba: Nelze vytvorit soubor results.txt!\n";
        return 1;
    }

    std::string data_dir = "../data/";
    DIR* dir = opendir(data_dir.c_str());


    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        std::string filename = entry->d_name;

        // Zajímají nás pouze soubory končící na .txt
        if (filename.length() >= 4 && filename.substr(filename.length() - 4) == ".txt") {
            std::string filepath = data_dir + filename;

            std::cout << "Zpracovavam mapu: " << filename << " ..." << std::flush;

            outfile << "========================================\n";
            outfile << "Mapa: " << filename << "\n";
            outfile << "========================================\n";

            try {
                Board board;
                board.loadFromFile(filepath);

                SequentialSolver solver;
                double time_taken = solver.solve(board);

                outfile << "Cas behu: " << time_taken << " s\n";
                outfile << "Nejlepsi cena (cost): " << solver.getBestCost() << "\n";
                outfile << "Vysledna deska:\n";

                if (solver.getBestCost() != -1) {
                    solver.getBestBoard().printSolution(outfile);
                } else {
                    outfile << "Zadne platne reseni nebylo nalezeno.\n";
                }
                outfile << "\n";

                std::cout << " HOTOVO (" << time_taken << " s)\n";

            } catch (const std::exception& e) {
                std::cout << " CHYBA\n";
                outfile << "Doslo k chybe pri zpracovani: " << e.what() << "\n\n";
            }
        }
    }
    closedir(dir);

    std::cout << "\nVsechny mapy zpracovany! Vysledky naleznete v souboru results.txt.\n";
    outfile.close();

    return 0;
}