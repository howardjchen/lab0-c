#pragma once

#include <stdint.h>
#include "game.h"
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

extern int move_record[N_GRIDS];
extern int move_count;
extern int game_stop;

int ttt();
int corutine_ai();
