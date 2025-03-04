#include <cstdio>
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

const f32 JUMP_VELOCITY = .2f;
const f32 GRAVITY = .005f;
const f32 TERMINAL_VELOCITY = 1.5f;
Vec2f get_player_pos_delta(Player* player, Input input, Collision_Info last_info) {
    Vec2f pos = {};
    if (input.right) {
        pos.x += .1f;
    }
    if (input.left) {
        pos.x -= .1f;
    }
    player->grounded = last_info.sides_touched.down;

    if (player->grounded) {
        player->velocity.y = 0;
        if (input.up) {
            player->velocity.y = JUMP_VELOCITY;
        }
    }

    player->velocity.y -= GRAVITY;
    if (player->velocity.y < -TERMINAL_VELOCITY) player->velocity.y = -TERMINAL_VELOCITY;
    pos.y += player->velocity.y;
    
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

u32 get_overlaps(Rectf rect, Rectf* others, u32 others_count, Rectf* results) {
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

bool check_collided(Rectf a, Rectf b) {
    f32 overlap_x  = (a.radiusx + b.radiusx - fabs(a.posx - b.posx));
    f32 overlap_y = (a.radiusy + b.radiusy - fabs(a.posy - b.posy));
    return (overlap_x > 0 && overlap_y > 0);
}

const f32 PLAYER_MOVE_SPEED = .1f;
const f32 COLLISION_EPSILON = .001f;
Rectf try_move(Rectf player, Vec2f move, Rectf* others, u32 others_count, Collision_Info* info) {
    if (move.x != 0) {
        f32 sign;
        if (move.x > 0) {
            sign = 1.f;
        } else {
            sign = -1.f;
        }
        player.posx += move.x;
        f32 most_extreme_edge = player.posx + player.radiusx * sign;
        for (u32 i = 0; i < others_count; ++i) {
            if (!check_collided(player, others[i])) continue;
            f32 edge = others[i].posx - others[i].radiusx * sign;
            if (edge * sign < most_extreme_edge * sign) {
                most_extreme_edge = edge;
                if (sign > 0) info->sides_touched.right = true;
                else          info->sides_touched.left  = true;
            }
        }
        player.posx = most_extreme_edge - (player.radiusx + COLLISION_EPSILON) * sign;
    }
    if (move.y != 0) {
        f32 sign;
        if (move.y > 0) {
            sign = 1.f;
        } else {
            sign = -1.f;
        }
        player.posy += move.y;
        f32 most_extreme_edge = player.posy + player.radiusy * sign;
        for (u32 i = 0; i < others_count; ++i) {
            if (!check_collided(player, others[i])) continue;
            f32 edge = others[i].posy - others[i].radiusy * sign;
            if (edge * sign < most_extreme_edge * sign) {
                most_extreme_edge = edge;
                if (sign > 0) info->sides_touched.up   = true;
                else          info->sides_touched.down = true;
            }
        }
        player.posy = most_extreme_edge - (player.radiusy + COLLISION_EPSILON) * sign;
    }
    return player;
}

u32 serialize_rectf(Rectf& rect, char* const result, u32 max_count) {
    u32 chars_written = snprintf(result, max_count, "Rectf: posx=%g posy=%g radiusx=%g radiusy=%g\n", rect.posx, rect.posy, rect.radiusx, rect.radiusy);
    return chars_written;
}

u32 update_objects(Frame_Info* last_frame, Arena* this_frame_arena) {
    Rectf* this_frame = (Rectf*)(this_frame_arena->data + this_frame_arena->current);
    u32 i = 0;
    for (i; i < last_frame->objects_count; ++i) {
        // We could copy the whole thing, but in 
        // the future we want to do things in here
        this_frame[i] = last_frame->objects[i];
        this_frame_arena->current += sizeof(this_frame[i]);
    }
    return i;
}

bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state) {
    assert(true);
    Frame_Info* this_frame = (Frame_Info*)frame_state->data;
    Player* player = &this_frame->player;
    *player = last_frame->player;
    this_frame->camera_pos = move_pos(last_frame->camera_pos, this_frame->camera_input);
    this_frame->objects = (Rectf*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame, frame_state);
    Vec2f player_delta = get_player_pos_delta(player, this_frame->input, last_frame->collision_info);
    player->rect = try_move(player->rect, player_delta, this_frame->objects, this_frame->objects_count, &this_frame->collision_info);
    return true;
}