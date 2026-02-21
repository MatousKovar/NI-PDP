#include <iostream>
#include <exception>
#include "common/Board.h"
#include "tests/TestPiecesLogic.cpp"

int main() {
    std::cout << "=== NI-PDP: SQX (Skladani quatromin s maximem) ===\n";

    try {
        // Založíme instanci testovací třídy a spustíme testy
        TestPiecesLogic tests;
        tests.runAllTests();

    } catch (const std::exception& e) {
        std::cerr << "Kriticka chyba: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.