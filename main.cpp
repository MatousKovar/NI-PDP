#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include "common/Board.h"

// Zahrnutí všech solverů (předpokládané názvy tvých hlavičkových souborů)
#include "solvers/SequentialSolver.h"
#include "solvers/OmpSolver.h"
#include "solvers/MpiSolver.h"
#include "solvers/OmpTaskSolver.h"
#include <mpi.h>

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    //kontrola poctu argumentu
    if (argc < 3 || argc > 4)
    {
        std::cerr << "Chyba: Spatny pocet argumentu!\n";
        std::cerr << "Pouziti (sekvencni): " << argv[0] << " <cesta_k_mape> sequential\n";
        std::cerr << "Pouziti (paralelni): " << argv[0] << " <cesta_k_mape> <omp/mpi> <z_konstanta>\n";
        MPI_Finalize();
        return 1;
    }

    // ------------------ Z parametr -------------------
    // je nutny pro omp a mpi solver
    std::string filepath = argv[1];
    std::string solver_type = argv[2];
    int z_constant = 0;

    if (solver_type == "omp" || solver_type == "mpi" || solver_type == "omptask")
    {
        if (argc != 4)
        {
            std::cerr << "Chyba: Pro solver typu '" << solver_type <<
                    "' musite zadat konstantu 'z' jako 4. argument!\n";
            MPI_Finalize();
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
            MPI_Finalize();
            return 1;
        }
    }
    else if (solver_type != "sequential")
    {
        std::cerr << "Chyba: Neznamy typ solveru. Povolene hodnoty: sequential, omp, mpi.\n";
        MPI_Finalize();
        return 1;
    }

    //------------------------------  MAPA --------------------
    // nacitani provadi pouze master process
    std::ofstream outfile;

    if (world_rank == 0)
    {
        std::cout << "======\n";
        std::cout << "Zpracovavam mapu: " << filepath << " se solverem: " << solver_type << " ..." << std::flush;

        outfile.open("../results.txt", std::ios::app); // <-- Takhle je to správně, jen volání metody
        if (!outfile.is_open())
        {
            std::cerr << "\nKriticka chyba: Nelze otevrit/vytvorit soubor results.txt!\n";
            MPI_Finalize();
            return 1;
        }
        outfile << "========================================\n";
        outfile << "Mapa: " << filepath << " | Solver: " << solver_type << "\n";
        outfile << "========================================\n";
    }

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
            if (world_rank == 0)
            {
                SequentialSolver solver;
                time_taken = solver.solve(board);
                best_cost = solver.getBestCost();
                best_board = solver.getBestBoard();
                calls_counter = solver.calls_counter;
            }
        }
        else if (solver_type == "omp")
        {
            if (world_rank == 0)
            {
                OmpSolver solver(4, z_constant);
                time_taken = solver.solve(board);
                best_cost = solver.best_cost;
                best_board = solver.best_board;
                // calls_counter = solver.calls_counter;
            }
        }
        else if (solver_type == "omptask")
        {
            if (world_rank == 0)
            {
                OmpTaskSolver solver(z_constant);
                time_taken = solver.solve(board);
                best_cost = solver.best_cost;
                best_board = solver.best_board;
                // calls_counter = solver.calls_counter;
            }
        }
        else if (solver_type == "mpi")
        {
            MpiSolver solver(4, z_constant, world_rank, world_size);
            time_taken = solver.solve(board);

            if (world_rank == 0)
            {
                best_cost = solver.best_cost;
                best_board = solver.best_board;
                // calls_counter = solver.calls_counter;
            }
        }


        // ---------------------_VYSLEDKY---------------------
        if (world_rank == 0)
        {
            outfile << "Cas behu: " << time_taken << " s\n";
            outfile << "Nejlepsi cena (cost): " << best_cost << "\n";
            outfile << "Pocet volani: " << calls_counter << "\n";
            outfile << "Vysledna deska:\n";

            std::cout << "Cas behu: " << time_taken << " s\n";
            std::cout << "Nejlepsi cena (cost): " << best_cost << "\n";
            std::cout << "Pocet volani: " << calls_counter << "\n";
            std::cout << "Vysledna deska:\n";
            if (best_cost != -1)
            {
                best_board.printSolution(outfile);
                best_board.printSolution(std::cout);
            }
            else
            {
                outfile << "Zadne platne reseni nebylo nalezeno.\n";
                std::cout << "Zadne platne reseni nebylo nalezeno.\n";
            }
            outfile << "\n";
            std::cout << "\n";

            std::cout << " HOTOVO (" << time_taken << " s)\n";
        }


    }
    catch (const std::exception &e)
    {
        std::cout << " CHYBA\n";
        outfile << "Doslo k chybe pri zpracovani: " << e.what() << "\n\n";
        outfile.close();
        MPI_Finalize();
        return 1; // Ukončíme s chybou
    }

    outfile.close();
    MPI_Finalize();
    return 0;
}
