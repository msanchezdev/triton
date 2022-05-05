#include <triton/engine.hpp>
#include <triton/components/sprite/sprite.hpp>
#include <triton/components/mouse_listener/mouse_listener.hpp>
#define BOARD_SIZE 3

bool game_running = true;
char player = 'X';
char board_tile_value[BOARD_SIZE][BOARD_SIZE] = {
    {' ', ' ', ' '},
    {' ', ' ', ' '},
    {' ', ' ', ' '}
};

struct BoardData {
    int x;
    int y;
};

void BoardTile_MouseEnter(TRT_EventArgs<void>* event, TRT_MouseEnterEvent* mouse_event) {
    if (!game_running) return;

    BoardData* data = (BoardData*)event->sender->GetComponent<TRT_MouseListener2D>()->data;
    TRT_Sprite* sprite = event->sender->GetComponent<TRT_Sprite>();
    char occoupied_by = board_tile_value[data->x][data->y];
    info("Mouse Enter: %d, %d\n", data->x, data->y);

    if (occoupied_by == ' ') {
        sprite->SetImage(app.images[player == 'X' ? RES_IMG_CROSS : RES_IMG_CIRCLE]->surface);
        sprite->SetVisible(true);
    }
}

void BoardTile_MouseLeave(TRT_EventArgs<void>* event, TRT_MouseLeaveEvent* mouse_event) {
    if (!game_running) return;

    BoardData* data = (BoardData*)event->sender->GetComponent<TRT_MouseListener2D>()->data;
    TRT_Sprite* sprite = event->sender->GetComponent<TRT_Sprite>();
    char occoupied_by = board_tile_value[data->x][data->y];
    info("Mouse Leave: %d, %d\n", data->x, data->y);

    if (occoupied_by == ' ') {

        sprite->SetVisible(false);
    }
}

void BoardTile_MouseDown(TRT_EventArgs<void>* event, TRT_MouseDownEvent* mouse_event) {
    if (!game_running) return;

    BoardData* data = (BoardData*)event->sender->GetComponent<TRT_MouseListener2D>()->data;
    TRT_Sprite* sprite = event->sender->GetComponent<TRT_Sprite>();
    char* occoupied_by = &board_tile_value[data->x][data->y];
    info("Mouse Down: %d, %d\n", data->x, data->y);

    if (*occoupied_by == ' ') {
        *occoupied_by = player;
        sprite->SetImage(app.images[player == 'X' ? RES_IMG_CROSS : RES_IMG_CIRCLE]->surface);
        sprite->SetOpacity(1);
        player = player == 'X' ? 'O' : 'X';
    }
}
// tic tac toe win check logic
void WinCheck(TRT_EventArgs<>* event) {
    // check rows
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board_tile_value[i][0] == board_tile_value[i][1] && board_tile_value[i][1] == board_tile_value[i][2]) {
            if (board_tile_value[i][0] != ' ') {
                info("%c wins!\n", board_tile_value[i][0]);
                game_running = false;
                return;
            }
        }
    }

    // check columns
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (board_tile_value[0][i] == board_tile_value[1][i] && board_tile_value[1][i] == board_tile_value[2][i]) {
            if (board_tile_value[0][i] != ' ') {
                info("%c wins!\n", board_tile_value[0][i]);
                game_running = false;
                return;
            }
        }
    }

    // check diagonals
    if (board_tile_value[0][0] == board_tile_value[1][1] && board_tile_value[1][1] == board_tile_value[2][2]) {
        if (board_tile_value[0][0] != ' ') {
            info("%c wins!\n", board_tile_value[0][0]);
            game_running = false;
            return;
        }
    }

    if (board_tile_value[0][2] == board_tile_value[1][1] && board_tile_value[1][1] == board_tile_value[2][0]) {
        if (board_tile_value[0][2] != ' ') {
            info("%c wins!\n", board_tile_value[0][2]);
            game_running = false;
            return;
        }
    }
}