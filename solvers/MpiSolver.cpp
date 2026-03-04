//
// Created by Matouš Kovář on 04.03.2026.
//

#include "MpiSolver.h"


double MpiSolver::solve(const Board &initialBoard)
{
    best_cost = -1;
    best_board = initialBoard;
    calls_counter = 0;

    //buffer pro posilani dat slave procesum
    int buffer_size = 4 + initialBoard.getSize();
    int *buffer = new int[buffer_size];

    auto start_time = std::chrono::high_resolution_clock::now();

    // MASTER PROCESS
    if (world_rank == 0)
    {
        std::vector<SearchState> queue = generateStartingBoards(initialBoard);

        int active_slaves = 0;
        int next_task = 0;

        // prvotni rozdani prace
        for (int i = 1; i < world_size && next_task < queue.size(); ++i)
        {
            packState(queue[next_task], buffer);
            MPI_Send(buffer, buffer_size, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
            next_task++;
            active_slaves++;
        }


        while (active_slaves > 0)
        {
            MPI_Status status;

            // cekame na odpoved
            MPI_Recv(buffer, buffer_size, MPI_INT, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);

            int sender = status.MPI_SOURCE;

            // update nejlepsiho reseni logika
            calls_counter += buffer[1];
            if (buffer[0] > best_cost)
            {
                best_cost = buffer[0];

            }


            if (next_task < queue.size())
            {
                packState(queue[next_task], buffer);
                MPI_Send(buffer, buffer_size, MPI_INT, sender, TAG_WORK, MPI_COMM_WORLD);
                next_task++;
            }
            else
            {
                MPI_Send(buffer, buffer_size, MPI_INT, sender, TAG_END, MPI_COMM_WORLD);
                active_slaves--;
            }
        }
    }
    // SLAVE PROCESS
    else
    {
    }

    return 0.0;
}


/**
 * Spousteno pouze v slave procesech
 * @param board - deska kterou ma vyresit
 * @param start_idx index na kterem zacina resit
 * @param piece_id - piece_id, ktere muze pouzit jako nejnizsi
 * @param local_calls - pocet volani, ktere agreguje v rekurzi kolikrat se funkce vola
 * @param global_best - global best neni globalni promenna, kvuli distribuovane pameti
 */
void MpiSolver::solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls, int global_best) {
}

/**
 * stejne jako v Omp solveru
 * @param original_board deska ze zadani
 * @return
 */
std::vector<SearchState> MpiSolver::generateStartingBoards(const Board &original_board) const {
    std::vector<SearchState> queue;
    int limit = n_threads * z;

    queue.push_back({original_board, 0, 1});

    // fronta je realizována jako vektor
    // head je aktuální vrchol fronty
    // ve fronte za indexem head uz jsou tedy jen listy
    int head = 0;

    while ((queue.size() - head) < limit && head < queue.size())
    {
        SearchState current_state = queue[head];
        head++;

        Board currentBoard = current_state.board;
        int start_idx = current_state.start_idx;
        int piece_id = current_state.start_piece_id;

        int cell = currentBoard.getNextFreeCell(start_idx);

        // deska je uz plna
        if (cell == -1)
        {
            queue.push_back(current_state);
            continue;
        }

        // prazdna deska do fronty
        currentBoard.markAsEmpty(cell);
        queue.push_back({currentBoard, cell + 1, piece_id});
        currentBoard.unmarkAsEmpty(cell);

        // dilky do fronty
        for (const auto &item: Pieces::VARIANTS)
        {
            if (currentBoard.canPlacePiece(cell, item))
            {
                currentBoard.placePiece(cell, item, piece_id);
                queue.push_back({currentBoard, cell + 1, piece_id + 1});
                currentBoard.removePiece(cell, item);
            }
        }
    }

    std::vector<SearchState> startingBoards(queue.begin() + head, queue.end());

    return startingBoards;
}

/**
 * buffer packs start_idx,
 * @param state
 * @param buffer
 */
void MpiSolver::packState(const SearchState &state, int *buffer) {
    buffer[0] = state.start_idx;
    buffer[1] = state.start_piece_id;
    buffer[2] = state.board.getCurrentCost();
    buffer[3] = state.board.getRemainingPosSum();

    // Překopírování stavu desky (toho, co už je reálně položené)
    int size = state.board.getSize();
    for (int i = 0; i < size; ++i)
    {
        buffer[4 + i] = state.board.getStateAt(i);
    }
}

SearchState MpiSolver::unpackState(const int *buffer, const Board &original_board) {
    SearchState s;
    // Vytvoříme kopii původní prázdné desky (tím získáme správné values, width, height)
    s.board = original_board;

    s.start_idx = buffer[0];
    s.start_piece_id = buffer[1];
    s.board.setCurrentCost(buffer[2]);
    s.board.setRemainingPosSum(buffer[3]);

    // Přepíšeme stav desky z přijatého bufferu
    int size = original_board.getSize();
    for (int i = 0; i < size; ++i)
    {
        s.board.setStateAt(i, buffer[4 + i]);
    }

    return s;
}


