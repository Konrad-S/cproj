#include <cstdio>
#include <math.h>
#include "game.h"

//
// Arena
//


// these should use void *
u8* arena_current(Arena arena) {
    return arena.data + arena.current;
}

u8* arena_append(Arena* arena, u32 data_size) {
    assert(arena->current + data_size <= arena->capacity);
    u8* result = arena->data + arena->current;
    arena->current += data_size;
    return result;
}

u32 arena_remaining(Arena arena) {
    return arena.capacity - arena.current;
}

u32 serialize_rectf(Rectf rect, Arena arena) {
    u32 chars_written = snprintf((char* const)arena_current(arena), arena_remaining(arena), "Rectf: posx=%g posy=%g radiusx=%g radiusy=%g\n", rect.posx, rect.posy, rect.radiusx, rect.radiusy);
    return chars_written;
}

Vec2f move_camera(Vec2f last_pos, Input* input) {
    Vec2f pos = last_pos;
    if (input[InputAction::CAM_UP].down) {
        pos.y += .1f;
    }
    if (input[InputAction::CAM_RIGHT].down) {
        pos.x += .1f;
    }
    if (input[InputAction::CAM_DOWN].down) {
        pos.y -= .1f;
    }
    if (input[InputAction::CAM_LEFT].down) {
        pos.x -= .1f;
    }
    return pos;
}

//
// Game
//

const f32 JUMP_VELOCITY = .2f;
const f32 GRAVITY = .005f;
const f32 TERMINAL_VELOCITY = 1.5f;
Vec2f get_player_pos_delta(Player* player, Input* input, Collision_Info last_info) {
    Vec2f pos = {};
    if (input[InputAction::RIGHT].down) {
        pos.x += .1f;
    }
    if (input[InputAction::LEFT].down) {
        pos.x -= .1f;
    }
    player->grounded = last_info.sides_touched.down;

    if (player->grounded) {
        player->velocity.y = 0;
        if (input[InputAction::UP].down) {
            player->velocity.y = JUMP_VELOCITY;
        }
    }

    player->velocity.y -= GRAVITY;
    if (player->velocity.y < -TERMINAL_VELOCITY) player->velocity.y = -TERMINAL_VELOCITY;
    pos.y += player->velocity.y;
    
    return pos;
}

Rectf rectf_overlap(Rectf a, Rectf b) {
    f32 half_overlap_x  = (a.radiusx + b.radiusx - fabsf(a.posx - b.posx)) / 2;
    f32 half_overlap_y = (a.radiusy + b.radiusy - fabsf(a.posy - b.posy)) / 2;
    
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

u32 first_overlap_index(Rectf rect, Rectf* others, u32 others_count) {
    for (int i = 0; i < others_count; ++i) {
        Rectf result = rectf_overlap(rect, others[i]);
        if (result.radiusx > 0 && result.radiusy > 0)
        {
            return i;
        }
    }
    return others_count;
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
    f32 overlap_x  = (a.radiusx + b.radiusx - fabsf(a.posx - b.posx));
    f32 overlap_y = (a.radiusy + b.radiusy - fabsf(a.posy - b.posy));
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

const f32 CULL_OBJECT_IF_SMALLER = .05;
u32 update_objects(Frame_Info* last_frame, Arena* this_frame_arena) {
    Rectf* this_frame = (Rectf*)(this_frame_arena->data + this_frame_arena->current);
    u32 added_count = 0;
    for (int i = 0; i < last_frame->objects_count; ++i) {
        Rectf rect = last_frame->objects[i];
        if (rect.radiusx < CULL_OBJECT_IF_SMALLER || rect.radiusy < CULL_OBJECT_IF_SMALLER) continue;
        this_frame[added_count] = rect;
        this_frame_arena->current += sizeof(rect);
        ++added_count;
    }
    return added_count;
}

Rectf points_to_rect(Vec2f a, Vec2f b) {
    f32 radiusx = fabs(a.x - b.x) / 2;
    f32 radiusy = fabs(a.y - b.y) / 2;

    f32 botx = MIN(a.x, b.x);
    f32 boty = MIN(a.y, b.y);
    return Rectf{ botx + radiusx, boty + radiusy, radiusx, radiusy };
}

Rectf scale_rect(Rectf r, f32 scale) {
    return Rectf{ r.posx * scale, r.posy * scale, r.radiusx * scale, r.radiusy * scale };
}
Rectf move_rect(Rectf r, Vec2f dist) {
    return Rectf{ r.posx + dist.x, r.posy + dist.y, r.radius };
}

u32 draw_obstacle(Drawing_Obstacle& drawing, Mouse* mouse, Camera camera, Rectf* data) {
    if (mouse->right.presses) {
        drawing.active = false;
        return 0;
    }
    if (!mouse->left.presses) return 0;
    if (!drawing.active) {
        drawing.active = true;
        drawing.pos = mouse->pos;
        return 0;
    } else {
        Rectf screen_rect = points_to_rect(drawing.pos, mouse->pos);
        Rectf scaled_rect = scale_rect(screen_rect, camera.scale);
        *data = move_rect(scaled_rect, camera.pos);
        drawing.active = false;
        return 1;
    }
    return 0;
}

Vec2f screen_to_world(Vec2f v, Camera camera) {
    return Vec2f { v.x * camera.scale + camera.posx, v.y * camera.scale + camera.posy };
}

Rectf screen_to_world(Rectf r, Camera camera) {
    return Rectf { screen_to_world(r.pos, camera), r.radiusx * camera.scale, r.radiusy * camera.scale };
}

void erase_obstacle(Mouse* mouse, Rectf* obstacles, u32 obstacles_count, Camera camera) {
    if (!mouse->right.down) return;
    u32 overlap_index = first_overlap_index(screen_to_world(Rectf {mouse->pos, 0, 0}, camera), obstacles, obstacles_count);
    if (overlap_index < obstacles_count) {
        obstacles[overlap_index].radius = { 0, 0 };
    }
}

bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state) {
    Frame_Info* this_frame = (Frame_Info*)frame_state->data;
    Game_Info* game_info = (Game_Info*)persistent_state->data;
    Player* player = &this_frame->player;
    *player = last_frame->player;

    this_frame->camera.pos = move_camera(last_frame->camera.pos, this_frame->input);
    

    this_frame->objects = (Rectf*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame, frame_state);
    
    Vec2f player_delta = get_player_pos_delta(player, this_frame->input, last_frame->collision_info);
    player->rect = try_move(player->rect, player_delta, this_frame->objects, this_frame->objects_count, &this_frame->collision_info);

    this_frame->objects_count += draw_obstacle(this_frame->drawing, this_frame->mouse, this_frame->camera, this_frame->objects + this_frame->objects_count);
    erase_obstacle(this_frame->mouse, this_frame->objects, this_frame->objects_count, this_frame->camera);


    if (this_frame->input[InputAction::EDITOR_SAVE].presses) {
        u32 persistent_reset = persistent_state->current;
        u32 total_length = 0;
        for (int i = 0; i < this_frame->objects_count; ++i) {
            u32 serialized_length = serialize_rectf(this_frame->objects[i], *persistent_state);
            total_length += serialized_length;
            persistent_state->current += serialized_length;
        }
        persistent_state->current = persistent_reset;
        game_info->platform_write_entire_file("test.txt", (const char*)arena_current(*persistent_state), total_length);
    }

    return true;
}