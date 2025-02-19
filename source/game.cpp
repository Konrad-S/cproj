#include <stdint.h>

#include "game.h"

Vec2f move_pos(Vec2f last_pos, Input input) {
    Vec2f pos = last_pos;
    if (input.up) {
        pos.y += .1f;
    }
    if (input.right) {
        pos.x += .1f;
    }
    if (input.down) {
        pos.y -= .1f;
    }
    if (input.left) {
        pos.x -= .1f;
    }
    return pos;
}

Square get_updated_player(Square last_player, Input input) {
    Square new_player;
    new_player.pos = move_pos(last_player.pos, input);
    new_player.r = last_player.r;
    return new_player;
}