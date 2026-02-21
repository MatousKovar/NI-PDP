#include <iostream>
#include <exception>
#include "common/Board.h"
// TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
int main() {
    std::cout << "=== StartedAlgorithm ===" << std::endl;

    try {
        Board board;

        board.loadFromFile("data/mapb6_6c.txt");

        std::cout << "Mapa uspesne nactena!" << std::endl;
        std::cout << "Rozmery: " << board.getWidth() << "x" << board.getHeight() << std::endl;
        std::cout << "Trivialni horni mez: " << board.getTrivialUpperBound() << "\n" << std::endl;

        std::cout << "Pocatecni stav desky:" << std::endl;
        board.printSolution();

    } catch (const std::exception& e) {
        std::cerr << "Kriticka chyba: " << e.what() << std::endl;
        return 1; // Ukončíme s chybovým kódem
    }


    return 0;
    // TIP See CLion help at <a href="https://www.jetbrains.com/help/clion/">jetbrains.com/help/clion/</a>. Also, you can try interactive lessons for CLion by selecting 'Help | Learn IDE Features' from the main menu.
}