#include "Connect4.h"
#include "Logger.h"

Logger *logger = Logger::GetInstance();

const int SQUARE_SIZE = 80;

// trying a bitboard
uint64_t RED_BOARD;
uint64_t YELLOW_BOARD;

Connect4::Connect4() : Game() {
    _grid = new Grid(7, 6);
}

Connect4::~Connect4() {
    delete _grid;
}

void Connect4::setUpBoard() {
    setNumberOfPlayers(2);
    _gameOptions.rowX = 7;
    _gameOptions.rowY = 6;

    // Initialize all squares
    _grid->initializeSquares(SQUARE_SIZE, "square.png");

    // init bitboard representations 
    RED_BOARD = 0;
    YELLOW_BOARD = 0;

    startGame();
}

Bit* Connect4::createPiece(int pieceType) {
    Bit* bit = new Bit();
    bool isRed = pieceType == RED_PIECE;
    bit->LoadTextureFromFile(isRed ? "red.png" : "yellow.png");
    bit->setOwner(getPlayerAt(isRed ? RED_PLAYER : YELLOW_PLAYER));
    bit->setGameTag(pieceType);
    return bit;
}

// helper
ImVec2 convertToGridCoords(ImVec2 pixel_pos){
    return ImVec2(
        (pixel_pos.x - SQUARE_SIZE / 2) / SQUARE_SIZE, 
        (pixel_pos.y - SQUARE_SIZE / 2) / SQUARE_SIZE
    );
}

ImVec2 convertPixelCoords(ImVec2 grid_pos){
    return ImVec2(
        (grid_pos.x * SQUARE_SIZE) + SQUARE_SIZE / 2, 
        (grid_pos.y * SQUARE_SIZE) + SQUARE_SIZE / 2
    );
}

bool inRange(int num, int min, int max){
    return (num >= min && num <= max);
}

// https://jorrid.com/posts/the-wondrous-world-of-connect-four-bit-boards/
void Connect4::updateBitboard(int column){
    /*
    filled = bitboardPlayerToMove | bitboardOtherPlayer
    COL0 = 0x3f
    ROW0 = 0x40201008040201
    VALID_PLACES = COL0 * ROW0
    moves = (filled + ROW0) & VALID_PLACES // main part
    column_mask = COL0 << (column * 9)
    move = moves & column_mask
    bitboardPlayerToMove |= move
    */

    uint64_t &PLAYER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? RED_BOARD : YELLOW_BOARD;
    uint64_t &OTHER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? YELLOW_BOARD : RED_BOARD;

    // set up masks
    uint64_t filled = PLAYER_BOARD | OTHER_BOARD;   // all occupied spaces
    uint64_t col0 = 0x3f;                           // first col
    uint64_t row0 = 0x40201008040201;               // first row (0, 7, 14, 21, 28, 35, 42)
    uint64_t open = col0 * row0;                    // all open spaces

    // the most important mask, where can the piece go next?
    uint64_t VALID = (filled + row0) & open;        // the lowest available space of each column
    uint64_t to_column = col0 << (column * 9);      // the column where we're moving
    uint64_t move = VALID & to_column;              // our move

    // update player bitboard
    PLAYER_BOARD |= move;
}

bool Connect4::actionForEmptyHolder(BitHolder &holder)
{
    // TODO: currently, this only works if the player clicks on an empty holder, but i'd like
    //       for it to work as long as player is hovered over a column with an empty holder

    int dir = (holder.empty()) ? 1 : -1;    // look down if holder is empty and up if not

    Bit *bit = createPiece(getCurrentPlayer()->playerNumber() == RED_PLAYER ? RED_PIECE : YELLOW_PIECE);
    if (bit) {
        ImVec2 pos = convertToGridCoords(holder.getPosition());
        
        // find lowest empty neighbor in this column
        while(inRange(pos.y + dir, 0, _gameOptions.rowY - 1) && getHolderAt((int)pos.x, (int)pos.y + 1).empty()){
            pos.y += dir;
        }

        BitHolder &neighbor = getHolderAt((int)pos.x, (int)pos.y);
        bit->setPosition(convertPixelCoords(pos));
        neighbor.setBit(bit);

        // update player bitboard
        updateBitboard((int)pos.x); // pass column being dropped into

        endTurn();
        return true;
    }   

    return false;
}

bool Connect4::canBitMoveFrom(Bit &bit, BitHolder &src) {
    return false;
}

bool Connect4::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) {
    return false;
}

// checks for bits in a line of passed {length}
bool bitRow(uint64_t board, uint64_t stride, int length){
    if(length < 2) return true;

    uint64_t and2 = board & (board >> stride);
    uint64_t inRow = and2 & (and2 >> ((length - 2) * stride));
    return (inRow != 0);
}

// using bit operations
// https://jorrid.com/posts/the-wondrous-world-of-connect-four-bit-boards/
bool bitWin(uint64_t board){
    uint64_t hStride = 9;
    uint64_t vStride = 1;
    uint64_t downDiagStride = hStride - 1;
    uint64_t upDiagStride = hStride + 1;

    uint64_t h4 = bitRow(board, hStride, 4);
    uint64_t v4 = bitRow(board, vStride, 4);
    uint64_t dd4 = bitRow(board, downDiagStride, 4);
    uint64_t ud4 = bitRow(board, upDiagStride, 4);

    return (h4 || v4 || dd4 || ud4);
}

Player* Connect4::checkForWinner() {
    if(bitWin(RED_BOARD)){
        return getPlayerAt(RED_PLAYER);
    }
    if(bitWin(YELLOW_BOARD)){
        return getPlayerAt(YELLOW_PLAYER);
    }

    return nullptr;
}

bool Connect4::checkForDraw() {
    return false;
}

void Connect4::stopGame() {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

std::string Connect4::initialStateString() {
    return "------------------------------------------";
}

std::string Connect4::stateString() {
    return _grid->getStateString();
}

void Connect4::setStateString(const std::string &s) {
    if (s.length() != 32) return;

    _grid->setStateString(s);

    // Recreate pieces from state
    // TODO
}

void Connect4::updateAI() {}

