//
// Created by Matouš Kovář on 21.02.2026.
//


#ifndef NI_PDP_BOARD_H
#define NI_PDP_BOARD_H

#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include "Pieces.h"


class Board {
private:
    int width;
    int height;
    int size;

    // Hodnoty políček ze vstupního souboru (1D pole pro rychlost)
    int* values;

    // Stav desky: 0 = volné, >0 = ID quatromina, -1 = úmyslně nepokryté
    int* state;

    // Rychlé výpočty pro Branch & Bound
    int current_cost;     // Aktuální cena (součet hodnot úmyslně nepokrytých políček)
    int positive_sum;     // Součet všech KLADNÝCH hodnot políček na začátku (pro triviální mez)
    int remaining_pos_sum;// Součet kladných hodnot zbývajících volných políček (pro ořezávání)

    std::vector<char> piece_types; // L, T
    // Převod 2D na 1D
    inline int getIndex(int x, int y) const { return y * width + x; }

public:
    Board();
    ~Board();

    // Zabráníme zbytečnému kopírování objektu (Rule of Three/Five)

    // Správně implementovaný Rule of Three pro hlubokou kopii dynamických polí
    Board(const Board& other);
    Board& operator=(Board other);

    void loadFromFile(const std::string& filename);

    // Gettery pro základní rozměry a limity
    int getWidth() const { return width; }
    // Vrátí bodovou hodnotu políčka na daném indexu
    int getCellValue(int idx) const { return values[idx]; }
    int getHeight() const { return height; }
    int getSize() const { return size; }
    int getCurrentCost() const { return current_cost; }
    int getTrivialUpperBound() const { return positive_sum; }

    // Pro ořezávání: current_cost + remaining_pos_sum, kdyby se podarilo zakryt vsechny zaporne a zadne kladne
    int getTheoreticalMaxPossibleCost() const;

    // Najde nejbližší nerozhodnuté políčko (index 0 až size-1)
    int getNextFreeCell(int start_idx = 0) const;

    // Metody pro DFS (aplikace a vrácení tahu)
    bool canPlacePiece(int idx, const PieceVariant& piece) const;
    void placePiece(int idx, const PieceVariant& piece, int piece_id);
    void removePiece(int idx, const PieceVariant& piece);

    void markAsEmpty(int idx);
    void unmarkAsEmpty(int idx);

    // Pro tisk výsledku
    void printSolution() const;
};


#endif //NI_PDP_BOARD_H