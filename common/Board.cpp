//
// Created by Matouš Kovář on 21.02.2026.
//

#include "Board.h"

Board::Board() : width(0), height(0), size(0), values(nullptr), state(nullptr),
                 current_cost(0), positive_sum(0), remaining_pos_sum(0) {}

Board::~Board() {
    delete[] values;
    delete[] state;
}

Board::Board(const Board& other) : width(other.width), height(other.height), size(other.size),
                                   current_cost(other.current_cost), positive_sum(other.positive_sum),
                                   remaining_pos_sum(other.remaining_pos_sum) {
    values = new int[size];
    state = new int[size];
    for (int i = 0; i < size; ++i) {
        values[i] = other.values[i];
        state[i] = other.state[i];
    }
}

Board& Board::operator=(Board other) {
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(size, other.size);
    std::swap(current_cost, other.current_cost);
    std::swap(positive_sum, other.positive_sum);
    std::swap(remaining_pos_sum, other.remaining_pos_sum);

    std::swap(values, other.values);
    std::swap(state, other.state);

    return *this;
}

// Inicializace desky (představte si zde logiku načtení ze souboru)
// Při načítání nezapomeňte sečíst všechny kladné hodnoty do positive_sum a remaining_pos_sum!
int Board::getNextFreeCell(int start_idx) const {
    for (int i = start_idx; i < size; ++i) {
        if (state[i] == 0) return i;
    }
    return -1; // Deska je plná
}

bool Board::canPlacePiece(int idx, const PieceVariant& piece) const {
    int x = idx % width;
    int y = idx / width;

    for (int i = 0; i < 3; ++i) {
        int nx = x + piece.dx[i];
        int ny = y + piece.dy[i];

        // 1. Je to vůbec na desce?
        if (nx < 0 || nx >= width || ny >= height) return false;

        // 2. Je políčko volné?
        int n_idx = getIndex(nx, ny);
        if (state[n_idx] != 0) return false;
    }
    return true;
}

void Board::placePiece(int idx, const PieceVariant& piece, int piece_id) {
    int x = idx % width;
    int y = idx / width;

    // Umístění kotevního bodu
    state[idx] = piece_id;
    if (values[idx] > 0) remaining_pos_sum -= values[idx];

    // Umístění zbývajících bloků
    for (int i = 0; i < 3; ++i) {
        int nx = x + piece.dx[i];
        int ny = y + piece.dy[i];
        int n_idx = getIndex(nx, ny);

        state[n_idx] = piece_id;
        if (values[n_idx] > 0) remaining_pos_sum -= values[n_idx];
    }
}

void Board::removePiece(int idx, const PieceVariant& piece) {
    int x = idx % width;
    int y = idx / width;

    // Odebrání kotevního bodu
    state[idx] = 0;
    if (values[idx] > 0) remaining_pos_sum += values[idx];

    // Odebrání zbývajících bloků
    for (int i = 0; i < 3; ++i) {
        int nx = x + piece.dx[i];
        int ny = y + piece.dy[i];
        int n_idx = getIndex(nx, ny);

        state[n_idx] = 0;
        if (values[n_idx] > 0) remaining_pos_sum += values[n_idx];
    }
}

// Logika pro "úmyslné vynechání" políčka
void Board::markAsEmpty(int idx) {
    state[idx] = -1; // -1 znamená úmyslně prázdné
    current_cost += values[idx]; // Přičteme cenu za nepokrytí
    if (values[idx] > 0) remaining_pos_sum -= values[idx];
}

void Board::unmarkAsEmpty(int idx) {
    state[idx] = 0;
    current_cost -= values[idx];
    if (values[idx] > 0) remaining_pos_sum += values[idx];
}

int Board::getTheoreticalMaxPossibleCost() const {
    return current_cost + remaining_pos_sum;
}

void Board::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Nelze otevrit vstupni soubor: " + filename);
    }

    // Načtení rozměrů desky
    file >> width >> height;
    size = width * height;

    // Bezpečnostní dealokace (pokud by se metoda volala vícekrát na stejném objektu)
    delete[] values;
    delete[] state;

    // Alokace 1D polí pro maximální výkon
    values = new int[size];
    state = new int[size];

    positive_sum = 0;
    current_cost = 0;

    // Načtení hodnot a inicializace stavu
    for (int i = 0; i < size; ++i) {
        file >> values[i];
        state[i] = 0; // 0 znamená nerozhodnuté políčko

        // Výpočet triviální horní meze (součet všech kladných ohodnocení)
        if (values[i] > 0) {
            positive_sum += values[i];
        }
    }

    // Na začátku jsou všechna kladná políčka zbývající
    remaining_pos_sum = positive_sum;

    file.close();
}

void Board::printSolution() const {
    // Procházíme desku po řádcích a tiskneme maticový popis
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = getIndex(x, y);
            int cell_state = state[idx];

            if (cell_state == -1) {
                // Políčko je úmyslně nepokryté, vypíšeme jeho ohodnocení
                std::cout << std::setw(4) << values[idx] << " ";
            } else if (cell_state > 0) {
                // Políčko je pokryté quatrominem.
                // Zde vypíšeme ID quatromina.
                std::cout << std::setw(3) << "Q" << cell_state << " ";
            } else {
                // Nerozhodnutá políčka by ve finálním řešení neměla být
                std::cout << std::setw(4) << "0" << " ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "Cena pokryti: " << current_cost << "\n";
}

