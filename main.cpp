#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include "common/Board.h"

// Zahrnutí všech solverů (předpokládané názvy tvých hlavičkových souborů)
#include "solvers/SequentialSolver.h"
#include "solvers/OmpSolver.h"
// #include "solvers/MpiSolver.h"

int main(int argc, char *argv[]) {
    //kontrola poctu argumentu
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Chyba: Spatny pocet argumentu!\n";
        std::cerr << "Pouziti (sekvencni): " << argv[0] << " <cesta_k_mape> sequential\n";
        std::cerr << "Pouziti (paralelni): " << argv[0] << " <cesta_k_mape> <omp/mpi> <z_konstanta>\n";
        return 1;
    }


    // ------------------ Z parametr -------------------
    // je nutny pro omp a mpi solver
    std::string filepath = argv[1];
    std::string solver_type = argv[2];
    int z_constant = 0;

    if (solver_type == "omp" || solver_type == "mpi")
    {
        if (argc != 4)
        {
            std::cerr << "Chyba: Pro solver typu '" << solver_type <<
                    "' musite zadat konstantu 'z' jako 4. argument!\n";
            return 1;
        }
        try
        {
            z_constant = std::stoi(argv[3]);
            if (z_constant <= 0) throw std::invalid_argument("z musi byt kladne");
        }
        catch (...)
        {
            std::cerr << "Chyba: Konstanta 'z' musi byt kladne cele cislo!\n";
            return 1;
        }
    }
    else
    {
        std::cerr << "Chyba: Neznamy typ solveru. Povolene hodnoty: sequential, omp, mpi.\n";
        return 1;
    }

    //------------------------------  MAPA --------------------
    std::cout << "======\n";
    std::cout << "Zpracovavam mapu: " << filepath << " se solverem: " << solver_type << " ..." << std::flush;

    std::ofstream outfile("../results.txt", std::ios::app);
    if (!outfile.is_open())
    {
        std::cerr << "\nKriticka chyba: Nelze otevrit/vytvorit soubor results.txt!\n";
        return 1;
    }


    outfile << "========================================\n";
    outfile << "Mapa: " << filepath << " | Solver: " << solver_type << "\n";
    outfile << "========================================\n";

    try
    {
        Board board;
        board.loadFromFile(filepath);

        // Proměnné pro uložení výsledků ze solverů
        double time_taken = 0.0;
        int best_cost = -1;
        long long calls_counter = 0;
        Board best_board;


        if (solver_type == "sequential")
        {
            SequentialSolver solver;
            time_taken = solver.solve(board);
            best_cost = solver.getBestCost();
            best_board = solver.getBestBoard();
            calls_counter = solver.calls_counter;
        }
        else if (solver_type == "omp")
        {
            OmpSolver solver(4); // Ujisti se, že máš takovou třídu vytvořenou
            time_taken = solver.solve(board);
            best_cost = solver.best_cost;
            best_board = solver.best_board;
            calls_counter = solver.calls_counter;
        }
        // else if (solver_type == "mpi" || solver_type == "mvi")
        // {
        //     MpiSolver solver; // Ujisti se, že máš takovou třídu vytvořenou
        //     time_taken = solver.solve(board);
        //     best_cost = solver.getBestCost();
        //     best_board = solver.getBestBoard();
        //     calls_counter = solver.calls_counter;
        // }




        // ---------------------_VYSLEDKY---------------------
        outfile << "Cas behu: " << time_taken << " s\n";
        outfile << "Nejlepsi cena (cost): " << best_cost << "\n";
        outfile << "Pocet volani: " << calls_counter << "\n";
        outfile << "Vysledna deska:\n";

        if (best_cost != -1)
        {
            best_board.printSolution(outfile);
        }
        else
        {
            outfile << "Zadne platne reseni nebylo nalezeno.\n";
        }
        outfile << "\n";

        std::cout << " HOTOVO (" << time_taken << " s)\n";
    }
    catch (const std::exception &e)
    {
        std::cout << " CHYBA\n";
        outfile << "Doslo k chybe pri zpracovani: " << e.what() << "\n\n";
        outfile.close();
        return 1; // Ukončíme s chybou
    }

    outfile.close();
    return 0;
}
