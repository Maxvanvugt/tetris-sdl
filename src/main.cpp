#include <SDL2/SDL.h>
#include <SDL2/SDL_blendmode.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_surface.h>
#include <SDL2/SDL_timer.h>

#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <random>
#include <sys/types.h>
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
    0b0010001000100010,
    0b0000000011110000,
    0b0010001000100010
};

const std::bitset<16> J_TETROID[4] = {
    0b0000000001110010,
    0b0000001000100110,
    0b0000010001110000,
    0b0000011001000100
};

const std::bitset<16> L_TETROID[4] = {
    0b0000000001110100,
    0b0000001000100011,
    0b0000000101110000,
    0b0000110001000100
};

const std::bitset<16> O_TETROID[4] = {
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110,
    0b0000000001100110
};

const std::bitset<16> S_TETROID[4] = {
    0b0000000000110110,
    0b0000001000110001,
    0b0000000000110110,
    0b0000001000110001
};

const std::bitset<16> T_TETROID[4] = {
    0b0000000001110001,
    0b0000001000110001,
    0b0000000101110000,
    0b0000001000110010
};

const std::bitset<16> Z_TETROID[4] = {
    0b0000000001100011,
    0b0000000100110001,
    0b0000000001100011,
    0b0000000100110001
};

const std::bitset<16>* availableBlocks[] = {
    I_TETROID,
    J_TETROID,
    L_TETROID,
    O_TETROID,
    S_TETROID,
    T_TETROID,
    Z_TETROID
};

enum BlockType {
    I,
    J,
    L
};

using ShapeBits = std::bitset<16>;
using MatrixBits = std::bitset<160>;

const std::bitset<16> *currentBlockBitmap[4];

struct Rectangle
{
    int x, y;
    int w, h;
};

const Rectangle playfield = {
    .x = 25,
    .y = 25,
    .w = 300,
    .h = 480
};

struct TextureState {
    SDL_Texture *textureX = NULL;
    SDL_Texture *textureO = NULL;
    SDL_Texture *textureEmpty = NULL;
    SDL_Texture *currenTexture = NULL;
};

struct InputState {
    bool keyDown = false;
    bool running = true;

    bool leftArrowDown = false;
    bool rightArrowDown = false;
    bool upArrowDown = false;
    bool downArrowDown = false;
};

struct GameState {
    bool running = true;
    uint8_t rotationIndex = 1;
    ShapeBits shapeBits;
    MatrixBits matrixBits;
    MatrixBits playingFieldMatrixBits;

    MatrixBits IMatrixBits;
    MatrixBits JMatrixBits;
    MatrixBits LMatrixBits;
    MatrixBits OMatrixBits;
    MatrixBits SMatrixBits;
    MatrixBits TMatrixBits;
    MatrixBits ZMatrixBits;

    BlockType blockType = BlockType::L;
    int xPos = 0;
    int yPos = 0;
    float fallSpeed = 0.02;
    float fallSpeecAcc = 0.0;
};

void clearInputs(InputState &inputState) {
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

    return (board << (xPos + (yPos * 10)));
}

bool isOutOfBounds(ShapeBits shapeBits, int xPos, int yPos) {
    for(int y = 0; y < 4; y++) {
        for(int x = 0; x < 4; x++) {
            if(shapeBits.test(y * 4 + x)) {
                if((xPos + x) >= 10) {
                    return true;
                }
                
                if((xPos + x) < 0) {
                    return true;
                }
                
                if((yPos + y) >= 16) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool isCollision(MatrixBits matrixBits, MatrixBits playingFieldMatrixBits) {
    return (matrixBits & playingFieldMatrixBits).any();
}

std::vector<std::pair<int, int>> WALL_KICK_OFFSETS {
    {1, 0},
    {-1, 0},
    {-2, 0},
    {0, -1},
    {0, -2},
    {-1, -1},
    {-1, -2},
    {1, -2},
};

MatrixBits getRotatedMatrix(ShapeBits shapeBits, int &xPos, int& yPos) {
    MatrixBits matrixBits = convertShapeToMatrixBits(shapeBits, xPos, yPos);

    if(!isOutOfBounds(shapeBits, xPos, yPos)) {
        return matrixBits;
    }

    for(const auto& offset : WALL_KICK_OFFSETS) {
        int newXPos = xPos + offset.first;
        int newYPos = yPos + offset.second;
        if(!isOutOfBounds(shapeBits, newXPos, newYPos)) {
            xPos = newXPos;
            yPos = newYPos;
            return convertShapeToMatrixBits(shapeBits, xPos, yPos);
        }
    }

    return false;
}

void setCurrentBlockBitmap() {
    static std::mt19937 rng(time(nullptr));
    static std::uniform_int_distribution<int> dist(0, 6); // 0 or 1
    const std::bitset<16>* shapeBits = availableBlocks[dist(rng)];
    currentBlockBitmap[0] = &shapeBits[0];
    currentBlockBitmap[1] = &shapeBits[1];
    currentBlockBitmap[2] = &shapeBits[2];
    currentBlockBitmap[3] = &shapeBits[3];
}

void updateGameState(GameState &gameState, InputState &inputState) {
    gameState.shapeBits = *currentBlockBitmap[gameState.rotationIndex];
    gameState.matrixBits = convertShapeToMatrixBits(gameState.shapeBits, gameState.xPos, gameState.yPos);

    if(inputState.leftArrowDown) {
        if(isCollision(convertShapeToMatrixBits(gameState.shapeBits, gameState.xPos - 1, gameState.yPos), gameState.playingFieldMatrixBits)) {
            return;
        }
        if(isOutOfBounds(gameState.shapeBits, gameState.xPos - 1, gameState.yPos)) {
            return;
        }
        gameState.xPos -= 1;
    }

    if(inputState.rightArrowDown) {
        if(isCollision(convertShapeToMatrixBits(gameState.shapeBits, gameState.xPos + 1, gameState.yPos), gameState.playingFieldMatrixBits)) {
            return;
        }
        if(isOutOfBounds(gameState.shapeBits, gameState.xPos + 1, gameState.yPos)) {
            return;
        }

        gameState.xPos += 1;
    }

    if(inputState.upArrowDown) {
        int rotationIndex = (gameState.rotationIndex + 1) & 0b11;
        ShapeBits shapeBits = *currentBlockBitmap[rotationIndex];
        getRotatedMatrix(shapeBits, gameState.xPos, gameState.yPos);
        gameState.rotationIndex = rotationIndex;
        return;
    }

    if(isOutOfBounds(gameState.shapeBits, gameState.xPos, gameState.yPos + 1) || isCollision(convertShapeToMatrixBits(gameState.shapeBits, gameState.xPos, gameState.yPos + 1), gameState.playingFieldMatrixBits)) {
        if(currentBlockBitmap[0] == &I_TETROID[0]) {
            gameState.IMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &J_TETROID[0]) {
            gameState.JMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &L_TETROID[0]) {
            gameState.LMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &O_TETROID[0]) {
            gameState.OMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &S_TETROID[0]) {
            gameState.SMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &T_TETROID[0]) {
            gameState.TMatrixBits |= gameState.matrixBits;
        }

        if(currentBlockBitmap[0] == &Z_TETROID[0]) {
            gameState.ZMatrixBits |= gameState.matrixBits;
        }

        gameState.playingFieldMatrixBits |= gameState.matrixBits;
        gameState.yPos = 0;
        gameState.xPos = 5;
        setCurrentBlockBitmap();
        return;
    }

    if(inputState.downArrowDown) {
        if(!isOutOfBounds(gameState.shapeBits, gameState.xPos, gameState.yPos + 1)) {
            gameState.yPos += 1;
        }
    }

    gameState.fallSpeecAcc += gameState.fallSpeed;

    if(gameState.fallSpeecAcc >= 1.0) {
        gameState.matrixBits = gameState.matrixBits << 10;
        gameState.yPos += 1;
        gameState.fallSpeecAcc = 0;
    }
}

void SDLInitialiseGame(SDL_Window *&window, SDL_Renderer *&renderer) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    IMG_Init(IMG_INIT_PNG);
    window = SDL_CreateWindow("Title", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, 0, 0);
}

void SDLHandleEvent(SDL_Event &event, InputState &inputState) {
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
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
        .x = rectangle.x,
        .y = rectangle.y,
        .w = rectangle.w,
        .h = rectangle.h
    };
}

void SDLRenderToScreen(SDL_Renderer *renderer, GameState &gameState, TextureState &textureState) {
    SDL_SetRenderDrawColor(renderer, 5, 54, 54, 255);
    SDL_RenderClear(renderer);

    SDL_Rect playfieldSDL = getSDLRect(playfield);

    SDL_SetRenderDrawColor(renderer, 5, 0, 5, 255);
    SDL_RenderFillRect(renderer, &playfieldSDL);

    int i = 0;
    for(int y = 0; y < BOARD_HEIGHT; y++) {
        for(int x = 0; x < BOARD_WIDTH; x++) {

            if(gameState.playingFieldMatrixBits.test(y * BOARD_WIDTH + x)) {
                SDL_Rect blockRect = {
                    .x = (BLOCK_SIZE_PX * x) + playfield.x,
                    .y = (BLOCK_SIZE_PX * y) + playfield.y,
                    .w = BLOCK_SIZE_PX,
                    .h = BLOCK_SIZE_PX
                };

                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

                if(gameState.IMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
                }

                if(gameState.JMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                }

                if(gameState.LMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                }

                if(gameState.OMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                }

                if(gameState.SMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                }

                if(gameState.TMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                }

                if(gameState.ZMatrixBits.test(y * BOARD_WIDTH + x)) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                }

                SDL_RenderFillRect(renderer, &blockRect);
            }

            if(gameState.matrixBits.test(y * BOARD_WIDTH + x)) {
                SDL_Rect blockRect = {
                    .x = (BLOCK_SIZE_PX * x) + playfield.x,
                    .y = (BLOCK_SIZE_PX * y) + playfield.y,
                    .w = BLOCK_SIZE_PX,
                    .h = BLOCK_SIZE_PX
                };

                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                if(currentBlockBitmap[0] == &I_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 255, 255);
                }

                if(currentBlockBitmap[0] == &J_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
                }

                if(currentBlockBitmap[0] == &L_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                }

                if(currentBlockBitmap[0] == &O_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                }

                if(currentBlockBitmap[0] == &S_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                }

                if(currentBlockBitmap[0] == &T_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
                }

                if(currentBlockBitmap[0] == &Z_TETROID[0]) {
                    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                }
                SDL_RenderFillRect(renderer, &blockRect);
            }
            i++;
        }
    }

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Window *window;
    SDL_Renderer *renderer;

    SDL_Event event;
    GameState gameState;
    InputState inputState;
    TextureState textureState;

    SDLInitialiseGame(window, renderer);
    setCurrentBlockBitmap();

    // gameState.shapeBits = *currentBlockBitmap[0];

    while (inputState.running) {
        SDLHandleEvent(event, inputState);
        updateGameState(gameState, inputState);

        SDLRenderToScreen(renderer, gameState, textureState);
        clearInputs(inputState);
        SDL_Delay(16);
    }

    SDL_DestroyWindow(window);
    return 0;
}

