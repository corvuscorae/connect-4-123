#include <algorithm>
#include "Connect4.h"
#include "Logger.h"

Logger *logger = Logger::GetInstance();

const int SQUARE_SIZE = 80;

//---------- (non-class) HELPER FUNCTIONS ----------//
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


//---------------- CLASS DEFINITIONS ---------------//
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

bool Connect4::updateBitboard(int column){
    uint64_t &PLAYER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? RED_BOARD : YELLOW_BOARD;
    uint64_t &OTHER_BOARD = (getCurrentPlayer()->playerNumber() == RED_PLAYER) ? YELLOW_BOARD : RED_BOARD;

    return updateBitboard(column, PLAYER_BOARD, OTHER_BOARD);
}

// for the bit shifting, i mostly followed this article
// https://jorrid.com/posts/the-wondrous-world-of-connect-four-bit-boards/
bool Connect4::updateBitboard(int column, uint64_t &PLAYER_BOARD, uint64_t &OTHER_BOARD){
    // set up masks
    uint64_t filled = PLAYER_BOARD | OTHER_BOARD;           // all occupied spaces

    // the most important mask, where can the piece go next?
    uint64_t VALID = (filled + ROW_0) & ALL_SPACES;         // the lowest available space of each column
    uint64_t to_column = COL_0 << (column * 9);             // the column where we're moving
    uint64_t move = VALID & to_column;                      // our move

    if(move == 0){
        return false;   // no valid moves in this column
    }

    // update player bitboard
    PLAYER_BOARD |= move;
    return true;
}

bool Connect4::placeBit(Bit *bit, ImVec2 pos, int dir){
    if(!updateBitboard((int)pos.x)){ // pass column being dropped into
        return false;
    } 

    // find lowest empty neighbor in this column
    if(dir != 0){
        while(inRange((int)pos.y + dir, 0, _gameOptions.rowY - 1) && getHolderAt((int)pos.x, (int)pos.y + 1).empty()){
            pos.y += dir;
        }
    }
    

    // update player bitboard
    BitHolder &neighbor = getHolderAt((int)pos.x, (int)pos.y);
    bit->setPosition(convertPixelCoords(pos));
    neighbor.setBit(bit);

    return true;
}

bool Connect4::actionForEmptyHolder(BitHolder &holder)
{
    int dir = (holder.empty()) ? 1 : -1;    // look down if holder is empty and up if not

    Bit *bit = createPiece(getCurrentPlayer()->playerNumber() == RED_PLAYER ? RED_PIECE : YELLOW_PIECE);
    if (bit) {
        ImVec2 pos = convertToGridCoords(holder.getPosition());

        if(!placeBit(bit, pos, dir)){   // try to place bit in position
            return false;
        }
        
        endTurn();
        return true;
    }   

    return false;
}

bool Connect4::canBitMoveFrom(Bit &bit, BitHolder &src) {   // this capability isn't in connect 4
    return false;
}

bool Connect4::canBitMoveFromTo(Bit& bit, BitHolder& src, BitHolder& dst) { // this capability isn't in connect 4
    return false;
}

// checks for bits in {stride} of passed {length}
bool Connect4::bitRow(uint64_t board, uint64_t stride, int length){
    if(length < 2) return true;

    return getRowMask(board, stride, length) != 0;
}

// checks for any stride of length {length}
bool Connect4::bitRow(uint64_t board, int length){
    if(length < 2) return true;

    for(size_t i = 0; i < 4; i++){
        if(getRowMask(board, ALL_STRIDES[i], length) != 0){ return true; }
    }
    return false;
}

// once again, i got the math here from this article:
// https://jorrid.com/posts/the-wondrous-world-of-connect-four-bit-boards/
uint64_t Connect4::getRowMask(uint64_t board, uint64_t stride, int length){
    uint64_t and2 = board & (board >> stride);          // get neighbors
    return and2 & (and2 >> ((length - 2) * stride));    // shift neighbors along {stride} by {length}
}

// looks for threats (bits where {length} in a row is possible)
int Connect4::countThreats(uint64_t me, uint64_t opp, int length){
    int count = 0;
    uint64_t empty = ~(me | opp) & ALL_SPACES;  // currently empty spaces

    for (int i = 0; i < 4; i++){
        uint64_t stride = ALL_STRIDES[i];

        // get positions with {length} in a row
        uint64_t threat = getRowMask(me, stride, length);

        // can threat actually complete the stride?
        // counts threats with PLAYABLE bit in extension (must be empty)
        uint64_t extendLeft = (threat >> (length * stride)) & empty;
        uint64_t extendRight = (threat << (length * stride)) & empty;
        count += countBits(extendLeft | extendRight);
    }

    return count;
}


bool Connect4::bitWin(uint64_t board){
    return bitRow(board, 4);    // checks for 4 in a row in any direction
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
    if (s.length() != 42) return;

    _grid->setStateString(s);

    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * _gameOptions.rowX + x;
        int playerNumber = s[index] - '0';
        if (playerNumber) {
            // actionForEmptyHolder(getHolderAt(x, y));
            Bit *bit = createPiece(playerNumber == RED_PIECE ? RED_PIECE : YELLOW_PIECE);
            placeBit(bit, ImVec2(x, y), 0);
            
        } else {
            square->setBit( nullptr );
        }
    });
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
    int move = getNextMove();

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

int Connect4::getNextMove(){
    std::srand((unsigned int)std::time(0));
    int bestMove = -WINNING_SCORE * 1000;
    int bestColumn = -1;

    uint64_t red_backup = RED_BOARD;
    uint64_t yellow_backup = YELLOW_BOARD;
    int currentPlayer = (getCurrentPlayer()->playerNumber() == _gameOptions.AIPlayer) ? AI_PLAYER : HUMAN_PLAYER;

    for(int i = 0; i < _gameOptions.rowX; i++){
        int col = MOVE_ORDER[i];
        if(!updateBitboard(col)){ // no available spaces in this column, move on
            RED_BOARD = red_backup;
            YELLOW_BOARD = yellow_backup;
            continue;
        }

        int score = -negamax(0, -WINNING_SCORE, WINNING_SCORE, -currentPlayer);

        if(score > bestMove){
            bestMove = score;
            bestColumn = col;
        } 
        // if equal to bestMove, do a coinflip to update best vals
        else if (score == bestMove && std::rand() % 10 > 4){
            bestMove = score;
            bestColumn = col;
        }

        RED_BOARD = red_backup;
        YELLOW_BOARD = yellow_backup;
    }

    return bestColumn;
}

bool Connect4::bitCheckForFullBoard(uint64_t state){
    if(state == ALL_SPACES){ 
        return true;
    }
    return false;
}

int Connect4::countBits(uint64_t bits){
    int count = 0;
    while (bits) {
        bits &= (bits - 1);
        count++;
    }
    return count;
}

int Connect4::eval(uint64_t myBoard, uint64_t oppBoard){
    int score = 0;
    
    // my advantage
    // score center bits
    uint64_t col2 = COL_0 << (2 * 9);
    uint64_t col3 = COL_0 << (3 * 9); // true center
    uint64_t col4 = COL_0 << (4 * 9);
    score += countBits(col2 & myBoard) * 3;
    score += countBits(col3 & myBoard) * 5;    
    score += countBits(col4 & myBoard) * 3;

    // my advantage
    score += countThreats(myBoard, oppBoard, 3) * 100;
    score += countThreats(myBoard, oppBoard, 2) * 10;
    
    // opp advantage
    score -= countThreats(oppBoard, myBoard, 3) * 500;  // score slightly higher to prefer blocking
    score -= countThreats(oppBoard, myBoard, 2) * 250;

    return score;
}

int Connect4::negamax(int depth, int alpha, int beta, int player){
    uint64_t &myBoard = player == HUMAN_PLAYER ? *HUMAN_BOARD : *AI_BOARD;
    uint64_t &oppBoard = player == HUMAN_PLAYER? *AI_BOARD : *HUMAN_BOARD;

    // check terminals
    if(bitWin(oppBoard)) return -(WINNING_SCORE / (1 + depth));
    if(bitWin(myBoard)) return WINNING_SCORE / (1 + depth);
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
            RED_BOARD = red_backup;
            YELLOW_BOARD = yellow_backup;
            continue;
        }

        if(bitWin(PLAYER_BOARD)){
            RED_BOARD = red_backup;
            YELLOW_BOARD = yellow_backup;
            return WINNING_SCORE / (1 + depth);  // immediate win, dont need to recurse
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