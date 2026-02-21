//
// Created by Matouš Kovář on 21.02.2026.
//

#ifndef NI_PDP_PIECES_H
#define NI_PDP_PIECES_H

#include <vector>

// Jednoduchá struktura bez virtuálních metod pro maximální rychlost
struct PieceVariant {
    char type;    // T or L
    int dx[3];    // X-ové offsety zbylých 3 bloků vůči kotevnímu bodu (0,0)
    int dy[3];    // Y-ové offsety zbylých 3 bloků vůči kotevnímu bodu (0,0)
};

class Pieces {
public:
    static const PieceVariant VARIANTS[12];
};


#endif //NI_PDP_PIECES_H