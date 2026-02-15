#include <algorithm>
#include "Connect4.h"
#include "Logger.h"

Logger *logger = Logger::GetInstance();

const int SQUARE_SIZE = 80;

// trying a bitboard
uint64_t RED_BOARD;
uint64_t YELLOW_BOARD;

Connect4::Connect4() : Game() {
    _grid = new Grid(7, 6);
    setNumberOfPlayers(2);
}

Connect4::~Connect4() {
    stopGame();
    delete _grid;
}

void Connect4::setUpBoard() {
    _gameOptions.rowX = 7;
    _gameOptions.rowY = 6;

    // Initialize all squares
    _grid->initializeSquares(SQUARE_SIZE, "square.png");

    // init bitboard representations 
    RED_BOARD = 0;
    YELLOW_BOARD = 0;

    // TEMP
    // TODO: let play choose
    if (gameHasAI()) {
        AI_COLOR = (_gameOptions.AIPlayer == 0) ? RED_PIECE : YELLOW_PIECE;
        AI_BOARD = (_gameOptions.AIPlayer == 0) ? &RED_BOARD : &YELLOW_BOARD;
        HUMAN_COLOR = (_gameOptions.AIPlayer == 1) ? RED_PIECE : YELLOW_PIECE;
        HUMAN_BOARD = (_gameOptions.AIPlayer == 1) ? &RED_BOARD : &YELLOW_BOARD;
    }

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
bool Connect4::updateBitboard(int column){
    uint64_t &PLAYER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? RED_BOARD : YELLOW_BOARD;
    uint64_t &OTHER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? YELLOW_BOARD : RED_BOARD;

    return updateBitboard(column, PLAYER_BOARD, OTHER_BOARD);
}

bool Connect4::updateBitboard(int column, uint64_t &PLAYER_BOARD, uint64_t &OTHER_BOARD){
    // set up masks
    uint64_t filled = PLAYER_BOARD | OTHER_BOARD;   // all occupied spaces
    uint64_t col0 = 0x3f;                           // first col (0, 1, 2, 3, 4, 5)
    uint64_t row0 = 0x40201008040201;               // first row (0, 7, 14, 21, 28, 35, 42)
    uint64_t open = col0 * row0;                    // all open spaces

    // the most important mask, where can the piece go next?
    uint64_t VALID = (filled + row0) & open;        // the lowest available space of each column
    uint64_t to_column = col0 << (column * 9);      // the column where we're moving
    uint64_t move = VALID & to_column;              // our move

    if(move == 0){
        return false;   // no valid moves in this column
    }

    // update player bitboard
    PLAYER_BOARD |= move;
    return true;
}

bool Connect4::actionForEmptyHolder(BitHolder &holder)
{
    // TODO: currently, this only works if the player clicks on an empty holder, but i'd like
    //       for it to work as long as player is hovered over a column with an empty holder

    int dir = (holder.empty()) ? 1 : -1;    // look down if holder is empty and up if not

    Bit *bit = createPiece(getCurrentPlayer()->playerNumber() == RED_PLAYER ? RED_PIECE : YELLOW_PIECE);
    if (bit) {
        ImVec2 pos = convertToGridCoords(holder.getPosition());
        if(!updateBitboard((int)pos.x)){ // pass column being dropped into
            return false;
        } 

        // find lowest empty neighbor in this column
        while(inRange((int)pos.y + dir, 0, _gameOptions.rowY - 1) && getHolderAt((int)pos.x, (int)pos.y + 1).empty()){
            pos.y += dir;
        }

        // update player bitboard
        BitHolder &neighbor = getHolderAt((int)pos.x, (int)pos.y);
        bit->setPosition(convertPixelCoords(pos));
        neighbor.setBit(bit);

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
bool Connect4::bitRow(uint64_t board, uint64_t stride, int length){
    if(length < 2) return true;

    uint64_t and2 = board & (board >> stride);
    uint64_t inRow = and2 & (and2 >> ((length - 2) * stride));
    return (inRow != 0);
}

// checks for any stride of length {length}
bool Connect4::bitRow(uint64_t board, int length){
    if(length < 2) return true;

    for(size_t i = 0; i < 4; i++){
        uint64_t stride = ALL_STRIDES[i];
        uint64_t and2 = board & (board >> stride);
        uint64_t inRow = and2 & (and2 >> ((length - 2) * stride));
        if (inRow != 0) return true;
    }
    return false;
}

// using bit operations
// https://jorrid.com/posts/the-wondrous-world-of-connect-four-bit-boards/
bool Connect4::bitWin(uint64_t board){
    uint64_t h4 = bitRow(board, HORIZONTAL_STRIDE, 4);
    uint64_t v4 = bitRow(board, VERTICAL_STRIDE, 4);
    uint64_t dd4 = bitRow(board, DOWNDIAG_STRIDE, 4);
    uint64_t ud4 = bitRow(board, UPDIAG_STRIDE, 4);

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
    return bitCheckForFullBoard(RED_BOARD | YELLOW_BOARD);
}

void Connect4::stopGame() {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

std::string Connect4::initialStateString() {
    return "000000000000000000000000000000000000000000";
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

//
// this is the function that will be called by the AI
//
void Connect4::updateAI()
{
    // don't try to play if game over
    if (checkForDraw() || checkForWinner())
    {
        return;
    }
    
    // get current board state
    std::string state = stateString();

    // find next move
    int move = getNextMove(state);

    if(move != -1){
        actionForEmptyHolder(getHolderAt(move, 0));
    }
    else {
        logger->Log("AI turn failed: move not found", logger->ERROR, logger->GAME);
        // moving randomly in this case...
        move = (int)(std::rand() % _gameOptions.rowX);
        actionForEmptyHolder(getHolderAt(move, 0));
    }
}

int Connect4::getNextMove(std::string &state){
    int bestMove = -1000;
    int bestColumn = -1;

    uint64_t red_backup = RED_BOARD;
    uint64_t yellow_backup = YELLOW_BOARD;
    int currentPlayer = (getCurrentPlayer()->playerNumber() == _gameOptions.AIPlayer) ? AI_PLAYER : HUMAN_PLAYER;

    for(int i = 0; i < _gameOptions.rowX; i++){
        int col = MOVE_ORDER[i];
        if(!updateBitboard(col)){ // no available spaces in this column, move on
            continue;
        }

        int score = -negamax(0, -WINNING_SCORE, WINNING_SCORE, -currentPlayer);

        if(score > bestMove){
            bestMove = score;
            bestColumn = col;
        }

        RED_BOARD = red_backup;
        YELLOW_BOARD = yellow_backup;
    }

    return bestColumn;
}

bool Connect4::bitCheckForFullBoard(uint64_t state){
    uint64_t col0 = 0x3f;                           // first col (0, 1, 2, 3, 4, 5)
    uint64_t row0 = 0x40201008040201;               // first row (0, 7, 14, 21, 28, 35, 42)
    uint64_t all = col0 * row0;                    // all spaces

    if(state == all){ 
        return true;
    }
    return false;
}

int countBits(uint64_t board){
    int count = 0;
    while (board) {
        board &= (board - 1);
        count++;
    }
    return count;
}

// TODO: replace with eval fucntion that score different states
int Connect4::eval(uint64_t myBoard, uint64_t oppBoard){
    int score = 0;
    
    // my advantage
    // score center bits
    uint64_t center = 0x3f << (2 * 9);
    score += countBits(center & myBoard) * 3;
    center = 0x3f << (3 * 9);   
    score += countBits(center & myBoard) * 5;    // true center
    center = 0x3f << (4 * 9);
    score += countBits(center & myBoard) * 3;

    // if(bitRow(myBoard, 4)){
        // score += WINNING_SCORE * 10; // 4 in a row = win!
    // } 
    if(bitRow(myBoard, 3)){
        score += 1000;   // 3 in a row = strong advantage
    }
    else if(bitRow(myBoard, 2)){
        score += 10;    
    }
    else {
        score -= 100;   // punish isolated pieces
    }

    // opp advantage
    // if(bitRow(oppBoard, 4)){
        // score -= WINNING_SCORE * 10;
    // } 
    if(bitRow(oppBoard, 3)){
        score -= 2000;
    }
    else if(bitRow(oppBoard, 2)){
        score -= 100;
    }
    else {
        score += 10;
    }

    return score;
}

int Connect4::negamax(int depth, int alpha, int beta, int player){
    uint64_t &myBoard = player == HUMAN_PLAYER ? *HUMAN_BOARD : *AI_BOARD;
    uint64_t &oppBoard = player == HUMAN_PLAYER? *AI_BOARD : *HUMAN_BOARD;

    // check terminals
    if(bitWin(myBoard)) return WINNING_SCORE / (1 + depth);
    if(bitWin(oppBoard)) return -(WINNING_SCORE / (1 + depth));
    if(depth >= MAX_DEPTH) return eval(myBoard, oppBoard);

    // check for draw
    if (bitCheckForFullBoard(myBoard | oppBoard)) { 
        return 0;
    }

    int bestValue = -WINNING_SCORE * 100;

    uint64_t &PLAYER_BOARD = (player == AI_PLAYER) ? *AI_BOARD : *HUMAN_BOARD;
    uint64_t &OTHER_BOARD = (player == AI_PLAYER) ? *HUMAN_BOARD : *AI_BOARD;

    uint64_t red_backup = RED_BOARD;
    uint64_t yellow_backup = YELLOW_BOARD;

    for(int i = 0; i < _gameOptions.rowX; i++){
        int col = MOVE_ORDER[i];
        if(!updateBitboard(col, PLAYER_BOARD, OTHER_BOARD)){ // no available spaces in this column, move on
            continue;
        }

        int newValue = -negamax(depth + 1, -beta, -alpha, -player);
    
        RED_BOARD = red_backup;
        YELLOW_BOARD = yellow_backup;

        bestValue = std::max(bestValue, newValue);
        alpha = std::max(alpha, newValue);

        if(alpha >= beta) return bestValue;    // prune
    }

    return bestValue;
}

// // legacy AI (very bad, only picks at random from available holders)
// int Connect4::randomAI(std::string &state){
//     // find all empty spaces
//     std::vector<int> empty;
//     for (int i = 0; i < state.length(); i++)
//     {
//         if (state[i] == NULL_PLAYER)
//         {
//             empty.push_back(i);
//         }
//     }

//     // pick one randomly
//     std::srand(std::time(0)); // seed rand()
//     int max = empty.size();
//     return empty[std::rand() % max];
// }