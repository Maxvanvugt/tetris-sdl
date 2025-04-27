#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_ttf.h>

#include <sys/types.h>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <utility>
#include <vector>

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 600;
constexpr int BOARD_HEIGHT = 16;
constexpr int BOARD_WIDTH = 10;

constexpr int BLOCK_SIZE_PX = 30;
constexpr int PART_SIZE = 4;
constexpr int SHIFT_SIZE = 16;

const std::bitset<16> I_TETROID[4] = {
    0b0000000011110000,
    0b0100010001000100,
    0b0000111100000000,
    0b0010001000100010,
};

const std::bitset<16> J_TETROID[4] = {
    0b0000001100100010,
    0b0000000001110001,
    0b0000001000100110,
    0b0000010001110000,
};

const std::bitset<16> L_TETROID[4] = {
    0b0000000001110100,
    0b0000011000100010,
    0b0000000101110000,
    0b0000001000100011,
};

const std::bitset<16> O_TETROID[4] = {
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110,
};

const std::bitset<16> S_TETROID[4] = {
    0b0000000000110110,
    0b0000010001100010,
    0b0000001101100000,
    0b0000001000110001,
};

const std::bitset<16> T_TETROID[4] = {
    0b0000000001110010,
    0b0000001001100010,
    0b0000001001110000,
    0b0000001000110010,
};

const std::bitset<16> Z_TETROID[4] = {
    0b0000000001100011,
    0b0000001001100100,
    0b0000011000110000,
    0b0000000100110010,
};

const std::bitset<16>* availableBlocks[] = {
    I_TETROID, J_TETROID, L_TETROID, O_TETROID, S_TETROID, T_TETROID, Z_TETROID,
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

using ShapeBits = std::bitset<16>;
using MatrixBits = std::bitset<160>;

const std::bitset<16>* currentBlockBitmap[4];
const std::bitset<16>* nextBlockBitmap[4];

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
    .w = 150,
    .h = 150,
};

struct TextureState {
    SDL_Texture* textureX = NULL;
    SDL_Texture* textureO = NULL;
    SDL_Texture* textureEmpty = NULL;
    SDL_Texture* currenTexture = NULL;
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
    MatrixBits IMatrixBits;
    MatrixBits JMatrixBits;
    MatrixBits LMatrixBits;
    MatrixBits OMatrixBits;
    MatrixBits SMatrixBits;
    MatrixBits TMatrixBits;
    MatrixBits ZMatrixBits;
};

struct GameState {
    bool running = true;
    bool gameOver = false;
    int score = 0;
    int nextBlockIndex = 0;
    uint8_t rotationIndex = 0;
    ShapeBits shapeBits;
    MatrixBits matrixBits;
    MatrixBits playingFieldMatrixBits;
    TetroidMatrixBits tetroidMatrixBits;

    bool isCollisionDown = false;
    bool placeBlock = false;
    int xPos = 0;
    int yPos = 0;
    float fallSpeed = 0.02;
    float fallSpeedAcc = 0.0;

    float rotationSpeed = 0.2;
    float rotationSpeedAcc = 0.0;
    bool canRotate = true;
};

void clearInputs(InputState& inputState) {
    inputState.rightArrowDown = false;
    inputState.leftArrowDown = false;
    inputState.downArrowDown = false;
    inputState.upArrowDown = false;
}

MatrixBits convertShapeToMatrixBits(std::bitset<16> block, int xPos, int yPos) {
    std::bitset<160> board;
    std::bitset<PART_SIZE> parts[4];

    parts[0] = (block >> (SHIFT_SIZE - PART_SIZE)).to_ulong();
    parts[1] = (block >> (SHIFT_SIZE - PART_SIZE * 2)).to_ulong();
    parts[2] = (block >> (SHIFT_SIZE - PART_SIZE * 3)).to_ulong();
    parts[3] = (block >> (SHIFT_SIZE - PART_SIZE * 4)).to_ulong();

    board |= std::bitset<160>(parts[0].to_ullong()) << (30);
    board |= std::bitset<160>(parts[1].to_ullong()) << (20);
    board |= std::bitset<160>(parts[2].to_ullong()) << (10);
    board |= std::bitset<160>(parts[3].to_ullong());

    int shiftSize = (xPos + (yPos * 10));
    if (shiftSize <= 0) {
        return (board >> abs(shiftSize));
    }

    return (board << (xPos + (yPos * 10)));
}

bool isCollision(ShapeBits shapeBits,
                 MatrixBits playingFieldMatrixBits,
                 int xPos,
                 int yPos) {
    MatrixBits matrixBits = convertShapeToMatrixBits(shapeBits, xPos, yPos);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (shapeBits.test(y * 4 + x)) {
                if ((xPos + x) >= 10) {
                    return true;
                }

                if ((xPos + x) < 0) {
                    return true;
                }

                if ((yPos + y) >= 16) {
                    return true;
                }
            }
        }
    }
    return (matrixBits & playingFieldMatrixBits).any();
}

std::vector<std::pair<int, int>> WALL_KICK_OFFSETS{{1, 0},   {2, 0},   {-1, 0},
                                                   {-2, 0},  {0, -1},  {0, -2},
                                                   {-1, -1}, {-1, -2}, {1, -2}};

void setRotationData(MatrixBits playingFieldMatrixBits,
                     int& xPos,
                     int& yPos,
                     uint8_t& rotationIndex) {
    uint8_t newRotationIndex = (rotationIndex + 1) & 0b11;
    ShapeBits shapeBits = *currentBlockBitmap[newRotationIndex];

    bool hasCollision =
        isCollision(shapeBits, playingFieldMatrixBits, xPos, yPos);

    if (!isCollision(shapeBits, playingFieldMatrixBits, xPos, yPos)) {
        rotationIndex = newRotationIndex;
        return;
    }

    for (const auto& offset : WALL_KICK_OFFSETS) {
        int newXPos = xPos + offset.first;
        int newYPos = yPos + offset.second;
        if (!isCollision(shapeBits, playingFieldMatrixBits, newXPos, newYPos)) {
            xPos = newXPos;
            yPos = newYPos;
            rotationIndex = newRotationIndex;
            return;
        }
    }

    return;
}

void checkForFullRows(MatrixBits playingFieldMatrixBits,
                      std::vector<int>& fullRows) {
    for (int y = 0; y < 16; y++) {
        bool hasFullRow = true;
        for (int x = 0; x < 10; x++) {
            int i = y * BOARD_WIDTH + x;
            if (!playingFieldMatrixBits.test(i)) {
                hasFullRow = false;
            }
        }

        if (hasFullRow) {
            fullRows.push_back(y);
        }
    }
}

void removeAndShiftRows(MatrixBits& playingFieldMatrixBits,
                        std::vector<int>& fullRows) {
    if (fullRows.size() <= 0) {
        return;
    }
    for (const auto& fullRow : fullRows) {
        for (int i = fullRow * 10; i < (fullRow * 10) + 10; i++) {
            playingFieldMatrixBits.reset(i);
        }

        MatrixBits tempPlayingFieldMatrixBits;
        tempPlayingFieldMatrixBits = playingFieldMatrixBits;

        tempPlayingFieldMatrixBits <<= (160 - fullRow * 10);
        tempPlayingFieldMatrixBits >>= (160 - fullRow * 10);
        tempPlayingFieldMatrixBits <<= 10;

        playingFieldMatrixBits >>= ((fullRow) * 10);
        playingFieldMatrixBits <<= ((fullRow) * 10);
        playingFieldMatrixBits |= tempPlayingFieldMatrixBits;
    }
}

void setCurrentBlockBitmap(int nextBlockIndex) {
    const std::bitset<16>* shapeBits = availableBlocks[nextBlockIndex];
    currentBlockBitmap[0] = &shapeBits[0];
    currentBlockBitmap[1] = &shapeBits[1];
    currentBlockBitmap[2] = &shapeBits[2];
    currentBlockBitmap[3] = &shapeBits[3];
}

void setNextBlockIndex(int& nextBlockIndex) {
    // Generate random number
    static std::mt19937 rng(time(nullptr));
    static std::uniform_int_distribution<int> dist(0, 6);  // 0 or 1
    nextBlockIndex = dist(rng);
}

void updateGameState(GameState& gameState, InputState& inputState) {
    if(gameState.gameOver) {
        if(inputState.rightArrowDown) {
            gameState = *new GameState();
        }
        return;
    }

    gameState.shapeBits = *currentBlockBitmap[gameState.rotationIndex];
    gameState.matrixBits = convertShapeToMatrixBits(
        gameState.shapeBits, gameState.xPos, gameState.yPos);

    gameState.isCollisionDown =
        isCollision(gameState.shapeBits, gameState.playingFieldMatrixBits,
                    gameState.xPos, gameState.yPos + 1);

    if (inputState.leftArrowDown) {
        if (!isCollision(gameState.shapeBits, gameState.playingFieldMatrixBits,
                         gameState.xPos - 1, gameState.yPos)) {
            gameState.xPos -= 1;
        }
    }

    if (inputState.rightArrowDown) {
        if (!isCollision(gameState.shapeBits, gameState.playingFieldMatrixBits,
                         gameState.xPos + 1, gameState.yPos)) {
            gameState.xPos += 1;
        }
    }

    if (inputState.upArrowDown && gameState.canRotate) {
        setRotationData(gameState.playingFieldMatrixBits, gameState.xPos,
                        gameState.yPos, gameState.rotationIndex);
        gameState.canRotate = false;
    }

    if (gameState.isCollisionDown && gameState.placeBlock) {
        if (currentBlockBitmap[0] == &I_TETROID[0]) {
            gameState.tetroidMatrixBits.IMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &J_TETROID[0]) {
            gameState.tetroidMatrixBits.JMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &L_TETROID[0]) {
            gameState.tetroidMatrixBits.LMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &O_TETROID[0]) {
            gameState.tetroidMatrixBits.OMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &S_TETROID[0]) {
            gameState.tetroidMatrixBits.SMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &T_TETROID[0]) {
            gameState.tetroidMatrixBits.TMatrixBits |= gameState.matrixBits;
        }

        if (currentBlockBitmap[0] == &Z_TETROID[0]) {
            gameState.tetroidMatrixBits.ZMatrixBits |= gameState.matrixBits;
        }

        gameState.playingFieldMatrixBits |=
            gameState.tetroidMatrixBits.IMatrixBits |
            gameState.tetroidMatrixBits.JMatrixBits |
            gameState.tetroidMatrixBits.LMatrixBits |
            gameState.tetroidMatrixBits.OMatrixBits |
            gameState.tetroidMatrixBits.SMatrixBits |
            gameState.tetroidMatrixBits.TMatrixBits |
            gameState.tetroidMatrixBits.ZMatrixBits;

        gameState.isCollisionDown = false;
        gameState.placeBlock = false;
        gameState.yPos = 0;
        gameState.xPos = 5;
        gameState.rotationIndex = 0;

        setCurrentBlockBitmap(gameState.nextBlockIndex);

        setNextBlockIndex(gameState.nextBlockIndex);

        ShapeBits shapeBits = *currentBlockBitmap[gameState.rotationIndex];
        MatrixBits matrixBits = convertShapeToMatrixBits(
            shapeBits, gameState.xPos, gameState.yPos);

        if((matrixBits & gameState.playingFieldMatrixBits).any()) {
            gameState.gameOver = true;
        }

        std::vector<int> fullRows;
        checkForFullRows(gameState.playingFieldMatrixBits, fullRows);
        if(fullRows.size() > 0) {
            removeAndShiftRows(gameState.tetroidMatrixBits.IMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.JMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.LMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.OMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.SMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.TMatrixBits, fullRows);
            removeAndShiftRows(gameState.tetroidMatrixBits.ZMatrixBits, fullRows);

            gameState.playingFieldMatrixBits =
                gameState.tetroidMatrixBits.IMatrixBits |
                gameState.tetroidMatrixBits.JMatrixBits |
                gameState.tetroidMatrixBits.LMatrixBits |
                gameState.tetroidMatrixBits.OMatrixBits |
                gameState.tetroidMatrixBits.SMatrixBits |
                gameState.tetroidMatrixBits.TMatrixBits |
                gameState.tetroidMatrixBits.ZMatrixBits;

            // Update score
            gameState.score += (fullRows.size() * 100);
            gameState.fallSpeed += 0.01;

        }

        return;
    }

    if (inputState.downArrowDown) {
        if (!gameState.isCollisionDown) {
            gameState.yPos += 1;
        }
        if (gameState.isCollisionDown) {
            gameState.placeBlock = true;
        }
    }

    gameState.fallSpeedAcc += gameState.fallSpeed;
    gameState.rotationSpeedAcc += gameState.rotationSpeed;

    if (gameState.rotationSpeedAcc >= 1.0) {
        gameState.rotationSpeedAcc = 0;
        gameState.canRotate = true;
    }

    if (gameState.fallSpeedAcc >= 1.0) {
        if (gameState.isCollisionDown) {
            gameState.placeBlock = true;
        } else {
            gameState.yPos += 1;
            gameState.fallSpeedAcc = 0;
        }
    }
}

void SDLInitialiseGame(SDL_Window*& window,
                       SDL_Renderer*& renderer,
                       TTF_Font*& font) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    window = SDL_CreateWindow("Title", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, 0, 0);

    font = TTF_OpenFont("../AdwaitaSans-Regular.ttf", 20);

    if (!font) {
        SDL_Log("Failed to load font: %s", TTF_GetError());
    }
}

void SDLHandleEvent(SDL_Event& event, InputState& inputState) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        inputState.upArrowDown = true;
                        break;
                    case SDLK_DOWN:
                        inputState.downArrowDown = true;
                        break;
                    case SDLK_LEFT:
                        inputState.leftArrowDown = true;
                        break;
                    case SDLK_RIGHT:
                        inputState.rightArrowDown = true;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_QUIT:
                inputState.running = false;
                break;
        }
    }
}

SDL_Rect getSDLRect(Rectangle rectangle) {
    return {
        .x = rectangle.x, .y = rectangle.y, .w = rectangle.w, .h = rectangle.h};
}

void SDLRenderBlock(BlockType blockType, SDL_Renderer* renderer) {
    switch (blockType) {
        case I:
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            break;

        case J:
            SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
            break;

        case L:
            SDL_SetRenderDrawColor(renderer, 255, 170, 0, 255);
            break;

        case O:
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            break;

        case S:
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            break;

        case T:
            SDL_SetRenderDrawColor(renderer, 153, 0, 255, 255);
            break;

        case Z:
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        default:
            break;
    }
}

void SDLRenderScore(SDL_Renderer* renderer, TTF_Font* font, int score) {
    SDL_Color textColor = {255, 255, 255, 255};
    std::string scoreStr = "Score: " + std::to_string(score);
    SDL_Surface* textSurface =
        TTF_RenderText_Solid(font, scoreStr.c_str(), textColor);

    SDL_Texture* textTexture =
        SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_FreeSurface(textSurface);

    SDL_Rect textRect;
    textRect.x = 360;
    textRect.y = 200;
    textRect.w = textSurface->w;
    textRect.h = textSurface->h;
    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
}

void SDLRenderToScreen(SDL_Renderer* renderer,
                       TTF_Font* font,
                       GameState& gameState,
                       TextureState& textureState) {
    SDL_SetRenderDrawColor(renderer, 5, 0, 5, 255);
    SDL_RenderClear(renderer);

    SDL_Rect playfieldSDL = getSDLRect(playfield);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &playfieldSDL);

    SDL_Rect nextTetrominoFieldSDL = getSDLRect(nextTetrominoField);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderDrawRect(renderer, &nextTetrominoFieldSDL);

    const std::bitset<16> *nextBlockBits =
        availableBlocks[gameState.nextBlockIndex];

    nextBlockBitmap[0] = &nextBlockBits[0];

    SDLRenderScore(renderer, font, gameState.score);

    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            SDL_Rect blockRect = {
                .x = (BLOCK_SIZE_PX * x) + nextTetrominoField.x + (25),
                .y = (BLOCK_SIZE_PX * y) + nextTetrominoField.y + (50),
                .w = BLOCK_SIZE_PX,
                .h = BLOCK_SIZE_PX};

            int i = y * 4 + x;

            if (nextBlockBitmap[0]->test(i)) {

                bool isIBlock = nextBlockBitmap[0] == &I_TETROID[0];
                bool isJBlock = nextBlockBitmap[0] == &J_TETROID[0];
                bool isLBlock = nextBlockBitmap[0] == &L_TETROID[0];
                bool isOBlock = nextBlockBitmap[0] == &O_TETROID[0];
                bool isSBlock = nextBlockBitmap[0] == &S_TETROID[0];
                bool isTBlock = nextBlockBitmap[0] == &T_TETROID[0];
                bool isZBlock = nextBlockBitmap[0] == &Z_TETROID[0];

                if (isIBlock) {
                    SDLRenderBlock(I, renderer);
                }
                if (isJBlock) {
                    SDLRenderBlock(J, renderer);
                }
                if (isLBlock) {
                    SDLRenderBlock(L, renderer);
                }
                if (isOBlock) {
                    SDLRenderBlock(O, renderer);
                }
                if (isSBlock) {
                    SDLRenderBlock(S, renderer);
                }
                if (isTBlock) {
                    SDLRenderBlock(T, renderer);
                }
                if (isZBlock) {
                    SDLRenderBlock(Z, renderer);
                }

                SDL_RenderFillRect(renderer, &blockRect);

                continue;
            }
        }
    }

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            SDL_Rect blockRect = {.x = (BLOCK_SIZE_PX * x) + playfield.x,
                                  .y = (BLOCK_SIZE_PX * y) + playfield.y,
                                  .w = BLOCK_SIZE_PX,
                                  .h = BLOCK_SIZE_PX};

            int i = y * BOARD_WIDTH + x;

            // Fill playing field
            if (gameState.playingFieldMatrixBits.test(i)) {
                bool isIBlock = gameState.tetroidMatrixBits.IMatrixBits.test(i);
                bool isJBlock = gameState.tetroidMatrixBits.JMatrixBits.test(i);
                bool isLBlock = gameState.tetroidMatrixBits.LMatrixBits.test(i);
                bool isOBlock = gameState.tetroidMatrixBits.OMatrixBits.test(i);
                bool isSBlock = gameState.tetroidMatrixBits.SMatrixBits.test(i);
                bool isTBlock = gameState.tetroidMatrixBits.TMatrixBits.test(i);
                bool isZBlock = gameState.tetroidMatrixBits.ZMatrixBits.test(i);

                if (isIBlock) {
                    SDLRenderBlock(I, renderer);
                }
                if (isJBlock) {
                    SDLRenderBlock(J, renderer);
                }
                if (isLBlock) {
                    SDLRenderBlock(L, renderer);
                }
                if (isOBlock) {
                    SDLRenderBlock(O, renderer);
                }
                if (isSBlock) {
                    SDLRenderBlock(S, renderer);
                }
                if (isTBlock) {
                    SDLRenderBlock(T, renderer);
                }
                if (isZBlock) {
                    SDLRenderBlock(Z, renderer);
                }

                // SDLRenderBlock(Z, renderer);
                SDL_RenderFillRect(renderer, &blockRect);

                continue;
            }

            // Fill current field
            if (gameState.matrixBits.test(i)) {
                bool isIBlock = currentBlockBitmap[0] == &I_TETROID[0];
                bool isJBlock = currentBlockBitmap[0] == &J_TETROID[0];
                bool isLBlock = currentBlockBitmap[0] == &L_TETROID[0];
                bool isOBlock = currentBlockBitmap[0] == &O_TETROID[0];
                bool isSBlock = currentBlockBitmap[0] == &S_TETROID[0];
                bool isTBlock = currentBlockBitmap[0] == &T_TETROID[0];
                bool isZBlock = currentBlockBitmap[0] == &Z_TETROID[0];

                if (isIBlock) {
                    SDLRenderBlock(I, renderer);
                }
                if (isJBlock) {
                    SDLRenderBlock(J, renderer);
                }
                if (isLBlock) {
                    SDLRenderBlock(L, renderer);
                }
                if (isOBlock) {
                    SDLRenderBlock(O, renderer);
                }
                if (isSBlock) {
                    SDLRenderBlock(S, renderer);
                }
                if (isTBlock) {
                    SDLRenderBlock(T, renderer);
                }
                if (isZBlock) {
                    SDLRenderBlock(Z, renderer);
                }

                SDL_RenderFillRect(renderer, &blockRect);
                continue;
            }

            // (DEBUG): show grid
            // SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
            // SDL_RenderDrawRect(renderer, &blockRect);
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;

    SDL_Event event;
    GameState gameState;
    InputState inputState;
    TextureState textureState;

    SDLInitialiseGame(window, renderer, font);

    setNextBlockIndex(gameState.nextBlockIndex);
    setCurrentBlockBitmap(gameState.nextBlockIndex);
    setNextBlockIndex(gameState.nextBlockIndex);
    // Debug for rotation;
    // gameState.playingFieldMatrixBits.set();
    // gameState.playingFieldMatrixBits <<= 130;

    // gameState.playingFieldMatrixBits.flip(135);
    // gameState.playingFieldMatrixBits.flip(136);
    // gameState.playingFieldMatrixBits.flip(137);
    // gameState.playingFieldMatrixBits.flip(138);
    // gameState.playingFieldMatrixBits.flip(139);

    // gameState.playingFieldMatrixBits.flip(145);
    // gameState.playingFieldMatrixBits.flip(146);
    // gameState.playingFieldMatrixBits.flip(147);
    // gameState.playingFieldMatrixBits.flip(148);
    // gameState.playingFieldMatrixBits.flip(149);

    // gameState.playingFieldMatrixBits.flip(155);
    // gameState.playingFieldMatrixBits.flip(156);
    // gameState.playingFieldMatrixBits.flip(157);
    // gameState.playingFieldMatrixBits.flip(158);
    // gameState.playingFieldMatrixBits.flip(159);

    while (inputState.running) {
        SDLHandleEvent(event, inputState);
        updateGameState(gameState, inputState);

        SDLRenderToScreen(renderer, font, gameState, textureState);
        clearInputs(inputState);
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    return 0;
}
