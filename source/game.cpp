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

u32 update_objects(Frame_Info* last_frame, Arena* this_frame_arena) {
    Square* this_frame = (Square*)(this_frame_arena->data + this_frame_arena->current);
    u32 i = 0;
    for (i; i < last_frame->objects_count; ++i) {
        // We could copy the whole thing, but in 
        // the future we want to do things in here
        this_frame[i] = last_frame->objects[i];
        this_frame_arena->current++;
    }
    return i;
}

bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state) {
    Frame_Info* this_frame = (Frame_Info*)frame_state->data;
    this_frame->player.square = get_updated_player(last_frame->player.square, this_frame->input);
    this_frame->camera_pos = move_pos(last_frame->camera_pos, this_frame->camera_input);
    this_frame->objects = (Square*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame, frame_state);
    return true;
}