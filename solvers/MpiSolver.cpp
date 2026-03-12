//
// Created by Matouš Kovář na moderním C++ (bez manuálního delete)
//

#include "MpiSolver.h"
#include <vector>

double MpiSolver::solve(const Board &initialBoard) {
    best_cost = -1;
    best_board = initialBoard;
    calls_counter = 0;

    // =========================================================
    // ALOKACE BUFFERŮ POMOCÍ STD::VECTOR (RAII)
    // =========================================================
    int work_buffer_size = 5 + initialBoard.getSize();
    std::vector<int> work_buffer(work_buffer_size);

    int result_buffer_size = 3 + initialBoard.getSize();
    std::vector<long long> result_buffer(result_buffer_size);

    auto start_time = std::chrono::high_resolution_clock::now();

    // =========================================================
    // MASTER PROCESS
    // =========================================================
    if (world_rank == 0)
    {
        std::vector<SearchState> queue = generateStartingBoards(initialBoard);

        int active_slaves = 0;
        int next_task = 0;

        // Prvotní rozdání práce
        for (int i = 1; i < world_size && next_task < queue.size(); ++i)
        {
            packState(queue[next_task], work_buffer.data());
            MPI_Send(work_buffer.data(), work_buffer_size, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
            next_task++;
            active_slaves++;
        }

        while (active_slaves > 0)
        {
            MPI_Status status;

            // Čekáme na VÝSLEDEK od kteréhokoliv Slave procesu
            MPI_Recv(result_buffer.data(), result_buffer_size, MPI_LONG_LONG, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
            int sender = status.MPI_SOURCE;

            long long received_cost = result_buffer[0];
            long long received_calls = result_buffer[1];

            calls_counter += received_calls;

            // Update nejlepšího řešení a desky
            if (received_cost > best_cost)
            {
                best_cost = received_cost;

                // Slave nám poslal i tu desku, tak si ji uložme
                for (int i = 0; i < initialBoard.getSize(); ++i)
                {
                    best_board.setStateAt(i, (int)result_buffer[2 + i]);
                }
                best_board.setCurrentCost(best_cost);
            }

            // Pokud máme další práci, pošleme ji
            if (next_task < (int)queue.size())
            {
                packState(queue[next_task], work_buffer.data());
                MPI_Send(work_buffer.data(), work_buffer_size, MPI_INT, sender, TAG_WORK, MPI_COMM_WORLD);
                next_task++;
            }
            else
            {
                MPI_Send(work_buffer.data(), work_buffer_size, MPI_INT, sender, TAG_END, MPI_COMM_WORLD);
                active_slaves--;
            }
        }
    }
    // =========================================================
    // SLAVE PROCESS
    // =========================================================
    else
    {
        while (true)
        {
            MPI_Status status;

            // Čekáme na zadání práce od Mastera
            MPI_Recv(work_buffer.data(), work_buffer_size, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_END)
                break;

            if (status.MPI_TAG == TAG_WORK)
            {
                SearchState state = unpackState(work_buffer.data(), initialBoard);
                long long local_calls = 0;

                best_board = state.board;

                // Spuštění sekvenční rekurze pro tento podstrom
                solveDFS(state.board, state.start_idx, state.start_piece_id, local_calls);

                // Balení a odeslání výsledku ZPĚT Masterovi
                result_buffer[0] = best_cost;
                result_buffer[1] = local_calls;
                for (int i = 0; i < initialBoard.getSize(); ++i)
                {
                    result_buffer[2 + i] = best_board.getStateAt(i);
                }

                // Odesíláme výsledek
                MPI_Send(result_buffer.data(), result_buffer_size, MPI_LONG_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);
            }
        }
    }

    // Všichni na sebe slušně počkají, než se procesy rozeběhnou do mainu
    MPI_Barrier(MPI_COMM_WORLD);

    // ŽÁDNÉ DELETE[]! Vector si po sobě uklidí sám při vyskočení z této metody.

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    return elapsed.count();
}

void MpiSolver::solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls) {
    local_calls += 1;
    if (best_cost == board.getTrivialUpperBound())
        return;

    if (board.getTheoreticalMaxPossibleCost() <= best_cost)
        return;

    int cell = board.getNextFreeCell(start_idx);

    if (cell == -1)
    {
        if (board.getCurrentCost() > best_cost)
        {
            best_board = board;
            best_cost = board.getCurrentCost();
        }
        return;
    }

    int cell_val = board.getCellValue(cell);

    if (cell_val > 0)
    {
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);

        if (best_cost == board.getTrivialUpperBound()) return;

        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id, local_calls);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }
    }
    else
    {
        for (int i = 0; i < 12; ++i)
        {
            if (board.canPlacePiece(cell, Pieces::VARIANTS[i]))
            {
                board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                solveDFS(board, cell + 1, piece_id, local_calls);
                board.removePiece(cell, Pieces::VARIANTS[i]);
            }
        }

        if (best_cost == board.getTrivialUpperBound()) return;

        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);
    }
}

std::vector<SearchState> MpiSolver::generateStartingBoards(const Board &original_board) const {
    std::vector<SearchState> queue;
    int limit = n_threads * z;

    queue.push_back({original_board, 0, 1});

    int head = 0;

    while ((queue.size() - head) < limit && head < queue.size())
    {
        SearchState current_state = queue[head];
        head++;

        Board currentBoard = current_state.board;
        int start_idx = current_state.start_idx;
        int piece_id = current_state.start_piece_id;

        int cell = currentBoard.getNextFreeCell(start_idx);

        if (cell == -1)
        {
            queue.push_back(current_state);
            continue;
        }

        currentBoard.markAsEmpty(cell);
        queue.push_back({currentBoard, cell + 1, piece_id});
        currentBoard.unmarkAsEmpty(cell);

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

void MpiSolver::packState(const SearchState &state, int *buffer) {
    buffer[0] = state.start_idx;
    buffer[1] = state.start_piece_id;
    buffer[2] = state.board.getCurrentCost();
    buffer[3] = state.board.getRemainingPosSum();
    buffer[4] = best_cost;

    int size = state.board.getSize();
    for (int i = 0; i < size; ++i)
        buffer[5 + i] = state.board.getStateAt(i);
}

SearchState MpiSolver::unpackState(const int *buffer, const Board &original_board) {
    SearchState s;
    s.board = original_board;

    s.start_idx = buffer[0];
    s.start_piece_id = buffer[1];
    s.board.setCurrentCost(buffer[2]);
    s.board.setRemainingPosSum(buffer[3]);
    best_cost = buffer[4];

    int size = original_board.getSize();
    for (int i = 0; i < size; ++i)
        s.board.setStateAt(i, buffer[5 + i]);

    return s;
}