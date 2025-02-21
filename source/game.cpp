#include <stdint.h>
#include <math.h>

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



// Rectf rectf_overlap(Rectf a, Rectf b) {
//     f32 combine_r = a.r + b.r
//     f32 overlap_width  = combine_r - abs(a.posx - b.posx);
//     f32 overlap_height = combine_r - abs(a.posy - b.posy);
    
// }

// Use an epsilon to have rectfs touching eachother not constantly colliding?
// int32 collide_rectf(Rectf rectf, Rectf* others, int32 others_count, Rectf* results) {
    
//     for (int i = 0; i < others_count; ++i) {
//         Rectf result = 
//     }
// }

Rectf get_updated_player(Rectf last_player, Input input) {
    Rectf new_player;
    new_player.pos = move_pos(last_player.pos, input);
    new_player.radius = last_player.radius;
    return new_player;
}

u32 update_objects(Frame_Info* last_frame, Arena* this_frame_arena) {
    Rectf* this_frame = (Rectf*)(this_frame_arena->data + this_frame_arena->current);
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
    this_frame->player.rect = get_updated_player(last_frame->player.rect, this_frame->input);
    this_frame->camera_pos = move_pos(last_frame->camera_pos, this_frame->camera_input);
    this_frame->objects = (Rectf*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame, frame_state);
    return true;
}