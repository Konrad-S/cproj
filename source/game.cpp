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



Rectf rectf_overlap(Rectf a, Rectf b) {
    f32 half_overlap_x  = (a.radiusx + b.radiusx - fabs(a.posx - b.posx)) / 2;
    f32 half_overlap_y = (a.radiusy + b.radiusy - fabs(a.posy - b.posy)) / 2;
    
    f32 cposx;
    if (a.posx > b.posx) {
        cposx = b.posx + b.radiusx - half_overlap_x;
    } else {
        cposx = b.posx - b.radiusx + half_overlap_x;
    }
    f32 cposy;
    if (a.posy > b.posy) {
        cposy = b.posy + b.radiusy - half_overlap_y;
    } else {
        cposy = b.posy - b.radiusy + half_overlap_y;
    }

    return Rectf{ cposx, cposy, half_overlap_x, half_overlap_y };
}

// Use an epsilon to have rectfs touching eachother not constantly colliding?
u32 collide_rectf(Rectf rect, Rectf* others, u32 others_count, Rectf* results) {
    u32 results_count = 0;
    for (int i = 0; i < others_count; ++i) {
        Rectf result = rectf_overlap(rect, others[i]);
        if (result.radiusx > 0 && result.radiusy > 0)
        {
            results[results_count++] = result;
        }
    }
    return results_count;
}

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
    Player* player = &this_frame->player;
    player->rect = get_updated_player(last_frame->player.rect, this_frame->input);
    this_frame->camera_pos = move_pos(last_frame->camera_pos, this_frame->camera_input);
    this_frame->objects = (Rectf*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame, frame_state);

    this_frame->collisions = (Rectf*)frame_state->data + frame_state->current;
    this_frame->collisions_count = collide_rectf(player->rect, this_frame->objects, this_frame->objects_count, this_frame->collisions);
    frame_state->current += sizeof(Rectf) * this_frame->collisions_count;



    return true;
}