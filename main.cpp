#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <mpi.h>
#include <omp.h>

#include "common/Board.h"
#include "solvers/SequentialSolver.h"
#include "solvers/OmpSolver.h"
#include "solvers/MpiSolver.h"
#include "solvers/OmpTaskSolver.h"

struct ProgramConfig {
    std::string filepath;
    std::string solver_type;
    int z_constant = 0;
    int num_threads = 1;
    int max_depth = 1;
};


bool parseArguments(int argc, char *argv[], ProgramConfig &config, int world_rank) {
    if (argc < 3 || argc > 6) {
        if (world_rank == 0) {
            std::cerr << "Chyba: Spatny pocet argumentu!\n";
            std::cerr << "Pouziti (sekvencni): " << argv[0] << " <cesta> sequential\n";
            std::cerr << "Pouziti (MPI):       " << argv[0] << " <cesta> mpi <z_konstanta> <pocet_vlaken> <maximalni_zanoreni>\n";
            std::cerr << "Pouziti (OpenMP):    " << argv[0] << " <cesta> <omp/omptask> <z_konstanta> <pocet_vlaken>\n";
        }
        return false;
    }

    config.filepath = argv[1];
    config.solver_type = argv[2];

    try {
        if (config.solver_type == "mpi") {
            if (argc != 6) throw std::invalid_argument("Pro MPI zadejte presne 4 argumenty <cesta> mpi <z_konstanta> <pocet_vlaken> <maximalni_zanoreni>\n");
            config.z_constant = std::stoi(argv[3]);
            config.num_threads = std::stoi(argv[4]);
            config.max_depth = std::stoi(argv[5]);
            if (config.z_constant <= 0) throw std::invalid_argument("Konstanta 'z' musi byt kladna.");
        }
        else if (config.solver_type == "omp" || config.solver_type == "omptask") {
            if (argc != 5) throw std::invalid_argument("Pro OpenMP zadejte presne 5 argumentu.");
            config.z_constant = std::stoi(argv[3]);
            config.num_threads = std::stoi(argv[4]);
            if (config.z_constant <= 0 || config.num_threads <= 0)
                throw std::invalid_argument("Konstanta 'z' a pocet vlaken musi byt kladne hodnoty.");
        }
        else if (config.solver_type != "sequential") {
            throw std::invalid_argument("Neznamy typ solveru. Povolene: sequential, omp, omptask, mpi.");
        }
    }
    catch (const std::exception& e) {
        if (world_rank == 0) std::cerr << "Chyba parametru: " << e.what() << "\n";
        return false;
    }

    return true;
}


double runSolver(const ProgramConfig& config, const Board& board, int world_rank, int world_size,
                 int& best_cost, Board& best_board, long long& calls_counter) {

    double time_taken = 0.0;

    if (config.solver_type == "sequential") {
        if (world_rank == 0) {
            SequentialSolver solver;
            time_taken = solver.solve(board);
            best_cost = solver.getBestCost();
            best_board = solver.getBestBoard();
            calls_counter = solver.calls_counter;
        }
    }
    else if (config.solver_type == "omp") {
        if (world_rank == 0) {
            OmpSolver solver(config.num_threads, config.z_constant);
            time_taken = solver.solve(board);
            best_cost = solver.best_cost;
            best_board = solver.best_board;
        }
    }
    else if (config.solver_type == "omptask") {
        if (world_rank == 0) {
            OmpTaskSolver solver(config.z_constant);
            time_taken = solver.solve(board);
            best_cost = solver.best_cost;
            best_board = solver.best_board;
            calls_counter = solver.calls_counter;
        }
    }
    else if (config.solver_type == "mpi") {
        // MPI solver spouštějí všechny procesy
        MpiSolver solver(config.num_threads, world_rank,world_size, config.max_depth, config.z_constant);
        time_taken = solver.solve(board);

        if (world_rank == 0) {
            best_cost = solver.best_cost;
            best_board = solver.best_board;
        }
    }

    return time_taken;
}


void printResults(const ProgramConfig& config, double time_taken, int best_cost,
                  long long calls_counter, const Board& best_board, std::ofstream& outfile) {

    auto print_to_both = [&](auto& stream) {
        stream << "Cas behu: " << time_taken << " s\n";
        stream << "Nejlepsi cena (cost): " << best_cost << "\n";
        stream << "Pocet volani: " << calls_counter << "\n";
        stream << "Vysledna deska:\n";

        if (best_cost != -1) {
            best_board.printSolution(stream);
        } else {
            stream << "Zadne platne reseni nebylo nalezeno.\n";
        }
        stream << "\n";
    };

    print_to_both(outfile);
    print_to_both(std::cout);
    std::cout << " HOTOVO (" << time_taken << " s)\n";
}

// ==============================================================================
// MAIN
// ==============================================================================
int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    ProgramConfig config;
    if (!parseArguments(argc, argv, config, world_rank)) {
        MPI_Finalize();
        return 1;
    }

    if (config.solver_type == "omp" || config.solver_type == "omptask") {
        omp_set_num_threads(config.num_threads);
    }

    std::ofstream outfile;
    if (world_rank == 0) {
        std::cout << "======\nZpracovavam mapu: " << config.filepath << " se solverem: " << config.solver_type << " ...\n" << std::flush;

        outfile.open("../results.txt", std::ios::app);
        if (!outfile.is_open()) {
            std::cerr << "Kriticka chyba: Nelze otevrit soubor results.txt!\n";
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        outfile << "========================================\n"
                << "Mapa: " << config.filepath << " | Solver: " << config.solver_type << "\n"
                << "========================================\n";
    }

    try {
        Board board;
        // Načítáme desku (případně jen na ranku 0, ale to závisí na vaší implementaci)
        board.loadFromFile(config.filepath);

        int best_cost = -1;
        long long calls_counter = 0;
        Board best_board;

        // Spuštění logiky solveru
        double time_taken = runSolver(config, board, world_rank, world_size, best_cost, best_board, calls_counter);

        // Výpis dat (pouze Master)
        if (world_rank == 0) {
            printResults(config, time_taken, best_cost, calls_counter, best_board, outfile);
        }
    }
    catch (const std::exception &e) {
        if (world_rank == 0) {
            std::cerr << " CHYBA\n";
            if (outfile.is_open()) outfile << "Doslo k chybe: " << e.what() << "\n\n";
        }
        MPI_Finalize();
        return 1;
    }

    if (world_rank == 0 && outfile.is_open()) {
        outfile.close();
    }

    MPI_Finalize();
    return 0;
}