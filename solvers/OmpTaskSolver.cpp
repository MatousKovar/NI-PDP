    //
    // Created by Matouš Kovář on 05.03.2026.
    //

    #include "OmpTaskSolver.h"
    #include <omp.h>
    #include <chrono>



    /**
     * pri spousteni tasku si posilaji promenne best_cost a best_board jako sdilene, protoze task si dela neco jako kopii kontextu standardne
     * @param board predavano hodnotou, musi byt kopie pro kazdy task
     * @param depth maximalni povolena uroven zanoreni, nez prechazim k sekvencnimu reseni, idealne mensi cislo - 2, nebo 3, tak aby vetsina vypoctu byla v sekvencnim reseni
     */
    void OmpTaskSolver::solveDFS(Board board, int start_idx, int piece_id, int depth)
    {
        // #pragma omp atomic
        // calls_counter++;
        // Donutíme vlákno načíst aktuální best_cost z paměti

        if (best_cost == board.getTrivialUpperBound()) return;
        if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

        int cell = board.getNextFreeCell(start_idx);

        if (cell == -1) {
            if (board.getCurrentCost() > best_cost) {
                #pragma omp critical
                {
                    if (board.getCurrentCost() > best_cost) {
                        best_cost = board.getCurrentCost();
                        best_board = board;
                    }
                }
            }
            return;
        }

        int cell_val = board.getCellValue(cell);

        if (cell_val > 0) {
            board.markAsEmpty(cell);
            // PROAKTIVNÍ KONTROLA PŘED ZANOŘENÍM
            if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                if (depth < z) {
                    #pragma omp task shared(best_cost, best_board)
                    solveDFS(board, cell + 1, piece_id, depth + 1);
                } else {
                    solveDFSSeq(board, cell + 1, piece_id);
                }
            }
            board.unmarkAsEmpty(cell);

            if (best_cost == board.getTrivialUpperBound()) return;

            for (int i = 0; i < 12; ++i) {
                if (best_cost == board.getTrivialUpperBound()) break; // Rychlý únik z cyklu
                if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                    board.placePiece(cell, Pieces::VARIANTS[i], piece_id);

                    // PROAKTIVNÍ KONTROLA PŘED ZANOŘENÍM
                    if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                        if (depth < z) {
                            #pragma omp task shared(best_cost, best_board)
                            solveDFS(board, cell + 1, piece_id + 1, depth + 1);
                        } else {
                            solveDFSSeq(board, cell + 1, piece_id + 1);
                        }
                    }
                    board.removePiece(cell, Pieces::VARIANTS[i]);
                }
            }
        } else {
            for (int i = 0; i < 12; ++i) {
                if (best_cost == board.getTrivialUpperBound()) break;
                if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                    board.placePiece(cell, Pieces::VARIANTS[i], piece_id);

                    if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                        if (depth < z) {
                            #pragma omp task shared(best_cost, best_board)
                            solveDFS(board, cell + 1, piece_id + 1, depth + 1);
                        } else {
                            solveDFSSeq(board, cell + 1, piece_id + 1);
                        }
                    }
                    board.removePiece(cell, Pieces::VARIANTS[i]);
                }
            }

            if (best_cost == board.getTrivialUpperBound()) return;

            board.markAsEmpty(cell);
            if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                if (depth < z) {
                    #pragma omp task shared(best_cost, best_board)
                    solveDFS(board, cell + 1, piece_id, depth + 1);
                } else {
                    solveDFSSeq(board, cell + 1, piece_id);
                }
            }
            board.unmarkAsEmpty(cell);
        }

        #pragma omp taskwait
    }

    /**
     * Klasicke sekvencni, jedine co, tak kdyz pristupuju k atributum oznacenym jako SHARED, tak musim pres kritisckou sekci
     * @param board - predava se referenci protoze tady uz se bezi sekvencne, nemusi se resit preisovani
     */
    void OmpTaskSolver::solveDFSSeq(Board &board, int start_idx, int piece_id) {

        // #pragma omp atomic
        // calls_counter++;

        if (best_cost == board.getTrivialUpperBound()) return;
        if (board.getTheoreticalMaxPossibleCost() <= best_cost) return;

        int cell = board.getNextFreeCell(start_idx);

        if (cell == -1) {
            if (board.getCurrentCost() > best_cost) {
                #pragma omp critical
                {
                    if (board.getCurrentCost() > best_cost) {
                        best_cost = board.getCurrentCost();
                        best_board = board;
                    }
                }
            }
            return;
        }

        int cell_val = board.getCellValue(cell);

        if (cell_val > 0) {
            board.markAsEmpty(cell);
            // PROAKTIVNÍ KONTROLA
            if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                solveDFSSeq(board, cell + 1, piece_id);
            }
            board.unmarkAsEmpty(cell);

            if (best_cost == board.getTrivialUpperBound()) return;

            for (int i = 0; i < 12; ++i) {
                if (best_cost == board.getTrivialUpperBound()) break;
                if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                    board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                    if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                        solveDFSSeq(board, cell + 1, piece_id + 1);
                    }
                    board.removePiece(cell, Pieces::VARIANTS[i]);
                }
            }
        } else {
            for (int i = 0; i < 12; ++i) {
                if (best_cost == board.getTrivialUpperBound()) break;
                if (board.canPlacePiece(cell, Pieces::VARIANTS[i])) {
                    board.placePiece(cell, Pieces::VARIANTS[i], piece_id);
                    if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                        solveDFSSeq(board, cell + 1, piece_id + 1);
                    }
                    board.removePiece(cell, Pieces::VARIANTS[i]);
                }
            }

            if (best_cost == board.getTrivialUpperBound()) return;

            board.markAsEmpty(cell);
            if (board.getTheoreticalMaxPossibleCost() > best_cost) {
                solveDFSSeq(board, cell + 1, piece_id);
            }
            board.unmarkAsEmpty(cell);
        }
    }

    double OmpTaskSolver::solve(const Board& initial_board) {
        best_cost = -1;
        best_board = initial_board;
        calls_counter = 0;
        int depth = 0;

        auto start_time = std::chrono::high_resolution_clock::now();
        #pragma omp parallel
        {
            #pragma omp single
            {
                solveDFS(initial_board, 0, 1, depth);
            }
        }
        auto end_time = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double> elapsed = end_time - start_time;
        return elapsed.count();
    }
