#include <bitset>

constexpr int MATRIX_BITS_SIZE = 160;
constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int BOARD_HEIGHT = 16;
constexpr int BOARD_WIDTH = 10;

constexpr int BLOCK_SIZE_PX = 30;
constexpr int PART_SIZE = 4;
constexpr int SHIFT_SIZE = 16;

const std::bitset<16> I_TETROID[4] = {
    0b0000111100000000,
    0b0010001000100010,
    0b0000000011110000,
    0b0100010001000100,
};

const std::bitset<16> J_TETROID[4] = {
    0b0000010001110000,
    0b0000001100100010,
    0b0000000001110001,
    0b0000001000100110,
};

const std::bitset<16> L_TETROID[4] = {
    0b0000000101110000,
    0b0000001000100011,
    0b0000000001110100,
    0b0000011000100010,
};

const std::bitset<16> O_TETROID[4] = {
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110,
};

const std::bitset<16> S_TETROID[4] = {
    0b0000001101100000,
    0b0000001000110001,
    0b0000000000110110,
    0b0000010001100010,
};

const std::bitset<16> T_TETROID[4] = {
    0b0000001001110000,
    0b0000001000110010,
    0b0000000001110010,
    0b0000001001100010,
};

const std::bitset<16> Z_TETROID[4] = {
    0b0000011000110000,
    0b0000000100110010,
    0b0000000001100011,
    0b0000001001100100,
};

struct Rectangle {
    int x, y;
    int w, h;
};

const Rectangle playfield = {
    .x = 25,
    .y = 25,
    .w = 300,
    .h = 480,
};

const Rectangle nextTetrominoField = {
    .x = 350,
    .y = 25,
    .w = 100,
    .h = 150,
};

struct InputState {
    bool keyDown = false;
    bool running = true;

    bool leftArrowDown = false;
    bool rightArrowDown = false;
    bool upArrowDown = false;
    bool downArrowDown = false;
};

struct TetroidMatrixBits {
    std::bitset<MATRIX_BITS_SIZE> IMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> JMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> LMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> OMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> SMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> TMatrixBits;
    std::bitset<MATRIX_BITS_SIZE> ZMatrixBits;
};

enum BlockType {
    I,
    J,
    L,
    O,
    S,
    T,
    Z,
};

