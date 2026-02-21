//
// Created by Matouš Kovář on 21.02.2026.
//

#include "Pieces.h"

// Kotevní bod (0,0) je vždy ten nejvíce vlevo-nahoře.
const PieceVariant Pieces::VARIANTS[12] = {

    // # # #
    //   #
    { 'T', { {1, 0}, {2, 0}, {1, 1} } },

    // T2: Tvar 'T' otočený doprava (stříška doprava)
    // #
    // # #
    // #
    { 'T', { {0, 1}, {1, 1}, {0, 2} } },

    // T3: Tvar 'T' vzhůru nohama (stříška dolů)
    //   #
    // # # #
    // Kotevní bod je horní špička, zbytek je o řádek níž
    { 'T', { {-1, 1}, {0, 1}, {1, 1} } },

    // T4: Tvar 'T' otočený doleva (stříška doleva)
    //   #
    // # #
    //   #
    // Kotevní bod je horní špička.
    { 'T', { {-1, 1}, {0, 1}, {-1, 2} } },

    // === L-QUATROMINA (8 variant: 4 rotace * 2 překlopení) ===
    // L1: Klasické L
    // #
    // #
    // # #
    { 'L', { {0, 1}, {0, 2}, {1, 2} } },

    // L2: Klasické L otočené o 90 stupňů doprava
    // # # #
    // #
    { 'L', { {1, 0}, {2, 0}, {0, 1} } },

    // L3: Klasické L otočené o 180 stupňů
    // # #
    //   #
    //   #
    { 'L', { {1, 0}, {1, 1}, {1, 2} } },

    // L4: Klasické L otočené o 270 stupňů
    //     #
    // # # #
    { 'L', { {-2, 1}, {-1, 1}, {0, 1} } },

    // L5: Zrcadlové L
    //   #
    //   #
    // # #
    { 'L', { {0, 1}, {-1, 2}, {0, 2} } },

    // L6: Zrcadlové L otočené o 90 stupňů
    // #
    // # # #
    { 'L', { {0, 1}, {1, 1}, {2, 1} } },

    // L7: Zrcadlové L otočené o 180 stupňů
    // # #
    // #
    // #
    { 'L', { {1, 0}, {0, 1}, {0, 2} } },

    // L8: Zrcadlové L otočené o 270 stupňů
    // # # #
    //     #
    { 'L', { {1, 0}, {2, 0}, {2, 1} } }
};