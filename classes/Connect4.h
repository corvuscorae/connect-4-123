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
    bool        gameHasAI() override { return _gameOptions.AIPlayer; } // Set to true when AI is implemented
    Grid*       getGrid() override { return _grid; }
    int         getNextMove(std::string &state);
    int         negamax(int depth, int alpha, int beta, int player);
    bool        bitCheckForFullBoard(uint64_t state);
    int         eval(uint64_t myBoard, uint64_t oppBoard);

private:
    bool hasAI = false;

    // Constants for piece types
    static const int EMPTY = 0;
    static const int RED_PIECE = 1;
    static const int YELLOW_PIECE = 3;

    // Player constants
    static const int RED_PLAYER = 0;
    static const int YELLOW_PLAYER = 1;
    static const char NULL_PLAYER = '0';
    // define these in class so player can choose which is AI
    int AI_COLOR;
    uint64_t *AI_BOARD;
    int HUMAN_COLOR;
    uint64_t *HUMAN_BOARD;

    // Constants for stride types (for checking n-in-a-row)
    const uint64_t HORIZONTAL_STRIDE = 9;
    const uint64_t VERTICAL_STRIDE = 1;
    const uint64_t DOWNDIAG_STRIDE = HORIZONTAL_STRIDE - 1;
    const uint64_t UPDIAG_STRIDE = HORIZONTAL_STRIDE + 1;
    const uint64_t ALL_STRIDES[4] = {HORIZONTAL_STRIDE, VERTICAL_STRIDE, DOWNDIAG_STRIDE, UPDIAG_STRIDE};
  
    // consts for eval function stuff
    const int MAX_DEPTH = 10; // max search depth
    const int WINNING_SCORE = 10000;
    const int MOVE_ORDER[7] = {3, 2, 4, 1, 5, 0, 6};

    // Helper methods
    Bit*        createPiece(int pieceType);

    // Board representation
    Grid*        _grid;

    // helpers
    bool updateBitboard(int column, uint64_t &PLAYER_BOARD, uint64_t &OTHER_BOARD);
    bool updateBitboard(int column);
    bool bitRow(uint64_t board, uint64_t stride, int length);
    bool bitRow(uint64_t board, int length);    // checks for a {length} row in any dir
    bool bitWin(uint64_t board);
};