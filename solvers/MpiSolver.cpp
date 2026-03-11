//
// Created by Matouš Kovář on 04.03.2026.
//

#include "MpiSolver.h"


double MpiSolver::solve(const Board &initialBoard) {
    best_cost = -1;
    best_board = initialBoard;
    calls_counter = 0;

    // ALOKACE BUFFERU
    // work_buffer: Master->Slave --- zadani ukolu
    int work_buffer_size = 4 + initialBoard.getSize(); // prvni 4 policka jsou viz packState
    int *work_buffer = new int[work_buffer_size];

    // result_buffer: Slave->Master --- nalezene reseni
    int result_buffer_size = 2 + initialBoard.getSize(); // prvni dve policka jsou calls, best_score - viz send_state
    long long *result_buffer = new long long[result_buffer_size]; // Použijeme long long kvůli calls

    auto start_time = std::chrono::high_resolution_clock::now();

    // =========================================================
    // MASTER PROCESS
    // =========================================================
    if (world_rank == 0)
    {
        std::vector<SearchState> queue = generateStartingBoards(initialBoard);

        int active_slaves = 0;
        int next_task = 0;

        // Prvotní rozdání práce (Pokud je úkolů méně než procesů, nezaměstnáme všechny!)
        for (int i = 1; i < world_size && next_task < queue.size(); ++i)
        {
            packState(queue[next_task], work_buffer);
            MPI_Send(work_buffer, work_buffer_size, MPI_INT, i, TAG_WORK, MPI_COMM_WORLD);
            next_task++;
            active_slaves++;
        }

        while (active_slaves > 0)
        {
            MPI_Status status;

            // Čekáme na VÝSLEDEK od kteréhokoliv Slave procesu (Nyní přijímáme správný typ result_buffer!)
            MPI_Recv(result_buffer, result_buffer_size, MPI_LONG_LONG, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD,
                     &status);
            int sender = status.MPI_SOURCE;

            long long received_cost = result_buffer[0];
            long long received_calls = result_buffer[1];

            calls_counter += received_calls;

            // Update nejlepšího řešení a desky
            if (received_cost > best_cost)
            {
                best_cost = received_cost;

                // Slave nám poslal i tu desku, tak si ji uložme!
                for (int i = 0; i < initialBoard.getSize(); ++i)
                {
                    best_board.setStateAt(i, (int) result_buffer[2 + i]);
                }
                best_board.setCurrentCost(best_cost);
            }

            // Pokud máme další práci, pošleme ji tomu samému Slave procesu, který se právě uvolnil
            if (next_task < (int) queue.size())
            {
                packState(queue[next_task], work_buffer);
                MPI_Send(work_buffer, work_buffer_size, MPI_INT, sender, TAG_WORK, MPI_COMM_WORLD);
                next_task++;
            }
            else
            {
                MPI_Send(work_buffer, work_buffer_size, MPI_INT, sender, TAG_END, MPI_COMM_WORLD);
                active_slaves--;
            }
        }
    }
    // SLAVE
    else
    {
        while (true)
        {
            MPI_Status status;

            // Čekáme na zadání práce od Mastera
            MPI_Recv(work_buffer, work_buffer_size, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            if (status.MPI_TAG == TAG_END)
                break;

            if (status.MPI_TAG == TAG_WORK)
            {
                SearchState state = unpackState(work_buffer, initialBoard);
                long long local_calls = 0;

                best_cost = -1;
                best_board = state.board;

                // Spuštění sekvenční rekurze pro tento podstrom!
                // Zde nejspíš chybí implementace samotné rekurze uvnitř solveDFS,
                // ale zavoláme ji s tím lokálním best_cost.
                solveDFS(state.board, state.start_idx, state.start_piece_id, local_calls);

                // 4. Balení a odeslání výsledku ZPĚT Masterovi
                result_buffer[0] = best_cost;
                result_buffer[1] = local_calls;
                for (int i = 0; i < initialBoard.getSize(); ++i)
                {
                    result_buffer[2 + i] = best_board.getStateAt(i);
                }

                // Odesíláme best_cost local_calls a nejlepsi nalezenou vypracovanou desku
                MPI_Send(result_buffer, result_buffer_size, MPI_LONG_LONG, 0, TAG_RESULT, MPI_COMM_WORLD);
            }
        }
    }

    // Úklid paměti!
    delete[] work_buffer;
    delete[] result_buffer;

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    return elapsed.count();
}

/**
 * Spousteno pouze v slave procesech
 * @param board - deska kterou ma vyresit
 * @param start_idx index na kterem zacina resit
 * @param piece_id - piece_id, ktere muze pouzit jako nejnizsi
 * @param local_calls - pocet volani, ktere agreguje v rekurzi kolikrat se funkce vola
 * @param global_best - global best neni globalni promenna, kvuli distribuovane pameti
 */
void MpiSolver::solveDFS(Board &board, int start_idx, int piece_id, long long &local_calls) {
    local_calls += 1;
    if (best_cost == board.getTrivialUpperBound())
        return;

    // Ořezávání neperspektivních větví - pokud bych zakryl vsechny zaporne, tak stejne nedostanu lepsi reseni nez jsem uz nasel
    if (board.getTheoreticalMaxPossibleCost() <= best_cost)
        return;

    // Nalezení dalšího volného políčka k rozhodnutí - je volne a nema byt preskoceno
    int cell = board.getNextFreeCell(start_idx);

    // Konec desky - zkontrolujeme, zda máme nový rekord
    if (cell == -1)
    {
        if (board.getCurrentCost() > global_best)
        {
            global_best = board.getCurrentCost();
            best_board = board; // Uložíme kopii nejlepšího stavu
            best_cost = board.getCurrentCost();
        }
        return;
    }

    // Branching
    int cell_val = board.getCellValue(cell);

    // Hodnota na volnem policku je kladna - chceme idealne nezakryt -> ten stav navstivime prvni
    if (cell_val > 0)
    {
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);

        // Pokud jsme vynecháním už dosáhli maxima, nemusíme zkoušet pokládat dílky
        if (best_cost == board.getTrivialUpperBound()) return;

        // Následně zkusíme všechny varianty dilku
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
        // v pripade ze je policko se zapornou hodnotou, tak je nejlepsi moznost ze bude zakryto -> to zkousime prvni
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

        // Až jako poslední zoufalou možnost zkusíme záporné políčko úmyslně vynechat
        board.markAsEmpty(cell);
        solveDFS(board, cell + 1, piece_id, local_calls);
        board.unmarkAsEmpty(cell);
    }
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
