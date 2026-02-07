#pragma once
#include "Game.h"

class Connect4 : public Game
{
public:
    Connect4();
    ~Connect4();

    // Required virtual methods from Game base class
    void        setUpBoard() override;
    Player*     checkForWinner() override;
    bool        checkForDraw() override;
    std::string initialStateString() override;
    std::string stateString() override;
    void        setStateString(const std::string &s) override;
    bool        actionForEmptyHolder(BitHolder &holder) override;
    bool        canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool        canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    void        stopGame() override;

    // AI methods
    void        updateAI() override;
    bool        gameHasAI() override { return false; } // Set to true when AI is implemented
    Grid* getGrid() override { return _grid; }

private:
    // Constants for piece types
    static const int EMPTY = 0;
    static const int RED_PIECE = 1;
    static const int YELLOW_PIECE = 3;

    // Player constants
    static const int RED_PLAYER = 0;
    static const int YELLOW_PLAYER = 1;

    // Helper methods
    Bit*        createPiece(int pieceType);

    // Board representation
    Grid*        _grid;
};