// PDSC task 3 - Wojciech Walczak 250280
#include "primlib.h"
#include <stdlib.h>
#include <unistd.h>
#include "pieces.inl"

#define SCREEN_WIDTH gfx_screenWidth()
#define SCREEN_HEIGHT gfx_screenHeight()

#define GAME_MATRIX_WIDTH 10
#define GAME_MATRIX_HEIGHT 30

#define GAME_OVER 0
#define PLAY 1
#define EXIT 2

struct point {
    int x;
    int y;
};

const struct point ZERO = {0, 0};
const struct point DOWN = {0, 1};
const struct point LEFT = {-1, 0};
const struct point RIGHT = {1, 0};

const int DELAY = 100;
const int RANDOM_SEED = 123456789;
const int N_OF_TYPES = 7;
const int N_OF_ORIENTATIONS = 4;
const int SIZE_OF_PIECE = 4;
const float BOARD_WIDTH_SCALE = 0.75; // amount of available space the matrix will occupy
const float BOARD_HEIGHT_SCALE = 0.90; // amount of available space the matrix will occupy
const float GAME_OVER_X_OFFSET_SCALE = 0.3;
const float GAME_OVER_Y_OFFSET_SCALE = 0.45;

struct point ORIGIN;
int BOARD_WIDTH, BOARD_HEIGHT;
int SQUARE_WIDTH, SQUARE_HEIGHT;

int game_state = PLAY;
int game_matrix[GAME_MATRIX_WIDTH][GAME_MATRIX_HEIGHT] = {0};
struct point piece;
int piece_type, piece_orientation;

void tetris_init() {
    float screen_aspect_ratio = SCREEN_WIDTH / (float) SCREEN_HEIGHT;
    float matrix_aspect_ratio = GAME_MATRIX_WIDTH / (float) GAME_MATRIX_HEIGHT;
    if (screen_aspect_ratio > matrix_aspect_ratio) { // screen is wider than game matrix
        BOARD_HEIGHT = SCREEN_HEIGHT * BOARD_HEIGHT_SCALE;
        BOARD_WIDTH = BOARD_HEIGHT * matrix_aspect_ratio;
    } else { //screen is narrower or equal to game matrix
        BOARD_WIDTH = SCREEN_WIDTH * BOARD_WIDTH_SCALE;
        BOARD_HEIGHT = BOARD_WIDTH / matrix_aspect_ratio;
    }
    ORIGIN.x = (SCREEN_WIDTH - BOARD_WIDTH) / 2;
    ORIGIN.y = (SCREEN_HEIGHT - BOARD_HEIGHT) / 2;
    SQUARE_WIDTH = BOARD_WIDTH / GAME_MATRIX_WIDTH;
    SQUARE_HEIGHT = BOARD_HEIGHT / GAME_MATRIX_HEIGHT;
    srand(RANDOM_SEED);
}

void draw_square_in_matrix(int x, int y, enum color col, int filled) {
    if (filled) {
        gfx_filledRect(ORIGIN.x + x * SQUARE_WIDTH, ORIGIN.y + y * SQUARE_HEIGHT,
                       ORIGIN.x + (x + 1) * SQUARE_WIDTH, ORIGIN.y + (y + 1) * SQUARE_HEIGHT,
                       col);
    } else {
        gfx_rect(ORIGIN.x + x * SQUARE_WIDTH, ORIGIN.y + y * SQUARE_HEIGHT,
                 ORIGIN.x + (x + 1) * SQUARE_WIDTH, ORIGIN.y + (y + 1) * SQUARE_HEIGHT,
                 col);
    }
}

void draw_game_matrix() {
    for (int i = 0; i < GAME_MATRIX_WIDTH; i++) {
        for (int j = 0; j < GAME_MATRIX_HEIGHT; j++) {
            if (game_matrix[i][j] != 0) {
                draw_square_in_matrix(i, j, YELLOW, 1);
            } else {
                draw_square_in_matrix(i, j, WHITE, 0);
            }
        }
    }
}

void draw_falling_piece() {
    for (int i = 0; i < SIZE_OF_PIECE; i++) {
        for (int j = 0; j < SIZE_OF_PIECE; j++) {
            if (pieces[piece_type][piece_orientation][i][j] == 1) {
                draw_square_in_matrix(piece.x + i, piece.y + j, RED, 1);
            } else if (pieces[piece_type][piece_orientation][i][j] == 2) {
                draw_square_in_matrix(piece.x + i, piece.y + j, MAGENTA, 1);
            }
        }
    }
}

int check_collision(struct point dir_vector, int orientation) { // checks for collision in the direction of dir_vector
    int new_x, new_y;
    for (int i = 0; i < SIZE_OF_PIECE; i++) {
        for (int j = 0; j < SIZE_OF_PIECE; j++) {
            if (pieces[piece_type][orientation][i][j] != 0) {
                new_x = piece.x + i + dir_vector.x;
                new_y = piece.y + j + dir_vector.y;
                if (new_x < 0 || new_x >= GAME_MATRIX_WIDTH || new_y >= GAME_MATRIX_HEIGHT) {
                    return 1;
                }
                if (new_y >= 0 && game_matrix[new_x][new_y] != 0) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

void append_piece_to_matrix() {
    for (int i = 0; i < SIZE_OF_PIECE; i++) {
        for (int j = 0; j < SIZE_OF_PIECE; j++) {
            if (pieces[piece_type][piece_orientation][i][j] != 0) {
                game_matrix[piece.x + i][piece.y + j] = 1;
            }
        }
    }
}

struct point get_center_offset(int type, int orientation) {
    struct point offset = {0, 0};
    for (int i = 0; i < SIZE_OF_PIECE; i++) {
        for (int j = 0; j < SIZE_OF_PIECE; j++) {
            if (pieces[type][orientation][i][j] == 2) {
                offset.x = i;
                offset.y = j;
                return offset;
            }
        }
    }
    return offset;
}

int rotate_piece() {
    int new_orientation = (piece_orientation + 1) % N_OF_ORIENTATIONS;
    struct point offset = get_center_offset(piece_type, new_orientation);
    struct point old_offset = get_center_offset(piece_type, piece_orientation);
    struct point move = {old_offset.x - offset.x, old_offset.y - offset.y};
    if (!check_collision(move, new_orientation)) {
        piece_orientation = new_orientation;
        piece.x += move.x;
        piece.y += move.y;
        return 1;
    }
    return 0;
}

void spawn_piece() {
    piece.x = GAME_MATRIX_WIDTH / 2 - SIZE_OF_PIECE / 2;
    piece.y = 0;
    piece_type = random() % N_OF_TYPES;
    piece_orientation = random() % N_OF_ORIENTATIONS;
}

void drop_rows_above(int row) {
    for (int k = row; k > 0; k--) {
        for (int column = 0; column < GAME_MATRIX_WIDTH; column++) {
            game_matrix[column][k] = game_matrix[column][k - 1];
        }
    }
}

int check_for_full_lines() {
    for (int row = GAME_MATRIX_HEIGHT - 1; row >= 0; row--) {
        int full = 1;
        for (int column = 0; column < GAME_MATRIX_WIDTH; column++) {
            if (game_matrix[column][row] == 0) {
                full = 0;
                break;
            }
        }
        if (full) {
            drop_rows_above(row);
            return 1;
        }
    }
    return 0;
}

void draw_background() {
    gfx_filledRect(0, 0, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, BLACK);
}

void display_game_over() {
    gfx_textout(ORIGIN.x + BOARD_WIDTH * GAME_OVER_X_OFFSET_SCALE,
                ORIGIN.y + BOARD_HEIGHT * GAME_OVER_Y_OFFSET_SCALE,
                "GAME OVER", RED);
}

int handle_input() {
    switch (gfx_pollkey()) {
        case SDLK_RIGHT:
            if (!check_collision(RIGHT, piece_orientation))
                piece.x++;
            break;
        case SDLK_LEFT:
            if (!check_collision(LEFT, piece_orientation))
                piece.x--;
            break;
        case SDLK_DOWN:
            while (!check_collision(DOWN, piece_orientation))
                piece.y++;
            break;
        case SDLK_SPACE:
            rotate_piece();
            break;
        case SDLK_UP:
            rotate_piece();
            break;
        case SDLK_ESCAPE:
            return EXIT;
        default:
            if (!check_collision(DOWN, piece_orientation)) {
                piece.y++;
            } else {
                append_piece_to_matrix();
                while (check_for_full_lines()); // drop all full lines
                spawn_piece();
                if (check_collision(ZERO, piece_orientation)) {
                    return GAME_OVER;
                }
            }
    }
    return PLAY;
}

int main() {
    if (gfx_init()) {
        exit(3);
    }
    tetris_init();
    spawn_piece();
    while (game_state == PLAY) {
        draw_background();
        draw_game_matrix();
        draw_falling_piece();
        game_state = handle_input();
        if (game_state == GAME_OVER)
            display_game_over();
        gfx_updateScreen();
        SDL_Delay(DELAY);
    }
    while (gfx_getkey() != SDLK_ESCAPE);
}