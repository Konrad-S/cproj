#include <cstdio>
#include <math.h>
#include "game.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//
// Arena
u8* arena_current(Arena arena) {
    return arena.data + arena.current;
}

u8* arena_current(Arena* arena) {
    return arena->data + arena->current;
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

u32 serialize_entity(Entity entity, Arena arena) {
    Rectf rect = entity.rect;
    u32 chars_written = snprintf((char* const)arena_current(arena), arena_remaining(arena), "Entity: type=%d posx=%g posy=%g radiusx=%g radiusy=%g move_speed=%g facing=%d\n", entity.type, rect.posx, rect.posy, rect.radiusx, rect.radiusy, entity.move_speed, entity.facing);
    return chars_written;
}

Vec2f move_camera(Vec2f last_pos, Input* input) {
    Vec2f pos = last_pos;
    if (input[INPUT_CAM_UP].down) {
        pos.y += .1f;
    }
    if (input[INPUT_CAM_RIGHT].down) {
        pos.x += .1f;
    }
    if (input[INPUT_CAM_DOWN].down) {
        pos.y -= .1f;
    }
    if (input[INPUT_CAM_LEFT].down) {
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
    if (input[INPUT_RIGHT].down) {
        pos.x += .1f;
    }
    if (input[INPUT_LEFT].down) {
        pos.x -= .1f;
    }
    player->grounded = last_info.sides_touched & DIR_DOWN;

    if (player->grounded) {
        player->velocity.y = 0;
        if (input[INPUT_UP].down) {
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

u32 first_overlap_index(Rectf rect, Entity* others, u32 others_count) {
    for (int i = 0; i < others_count; ++i) {
        Entity other = others[i];
        if (!other.type) continue;
        Rectf result = rectf_overlap(rect, other.rect);
        if (result.radiusx > 0 && result.radiusy > 0)
        {
            return i;
        }
    }
    return others_count;
}

u32 first_overlap_index(u32 rect_index, Entity* others, u32 others_count) {
    assert(rect_index < others_count);
    for (int i = 0; i < others_count; ++i) {
        if (i == rect_index) continue;
        Entity other = others[i];
        if (!other.type) continue;
        Rectf result = rectf_overlap(others[rect_index].rect, other.rect);
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
Rectf try_move_axis(Rectf player, Vec2f move, u8 axis_offset, Entity* others, u32 others_count, Collision_Info* info) {
    f32 move_axis = move.a[axis_offset];
    if (move_axis != 0) {
        f32 sign;
        if (move_axis > 0) {
            sign = 1.f;
        } else {
            sign = -1.f;
        }
        player.pos.a[axis_offset] += move_axis;
        f32 most_extreme_edge = player.pos.a[axis_offset] + player.radius.a[axis_offset] * sign;
        for (u32 i = 0; i < others_count; ++i) {
            Entity other = others[i];
            if (other.type != ENTITY_STATIC || !check_collided(player, other.rect)) continue;
            f32 edge = other.pos.a[axis_offset] - other.radius.a[axis_offset] * sign;
            if (edge * sign < most_extreme_edge * sign) {
                most_extreme_edge = edge;
                if (sign > 0) info->sides_touched |= (DIR_RIGHT - axis_offset);
                else          info->sides_touched |= (DIR_LEFT - axis_offset);
            }
        }
        player.pos.a[axis_offset] = most_extreme_edge - (player.radius.a[axis_offset] + COLLISION_EPSILON) * sign;
    }
    return player;
}

const f32 CULL_OBJECT_IF_SMALLER = .2;
u32 update_objects(Entity* last_objects, u32 last_objects_count, Entity* result) {
    u32 added_count = 0;
    for (int i = 0; i < last_objects_count; ++i) {
        Entity* object = result + added_count++;
        *object = last_objects[i];
        switch (object->type) {
        case ENTITY_MONSTER:
            if (object->standing_on && object->standing_on->type) {
                Entity* other = object->standing_on;
                bool outside_right = object->posx + object->radiusx > other->posx + other->radiusx;
                bool outside_left  = object->posx - object->radiusx < other->posx - other->radiusx;
                if (outside_right && outside_left) {
                    object->facing = 0;
                } else if (outside_right) {
                    object->facing = -1;
                } else if (outside_left) {
                    object->facing = 1;
                }
                object->posx += object->move_speed * object->facing;
            } else {
                //fall until colliding
                u32 overlap_index = first_overlap_index(i, last_objects, last_objects_count);
                if (overlap_index < last_objects_count) {
                    object->standing_on = last_objects + overlap_index;
                    Entity* other = object->standing_on;
                    object->posy = other->posy + other->radiusy + object->radiusy;
                    object->facing = -1;
                } else {
                    object->posy -= .1f;
                }

            }
        case ENTITY_STATIC:
            Rectf rect = object->rect;
            if (rect.radiusx < CULL_OBJECT_IF_SMALLER || rect.radiusy < CULL_OBJECT_IF_SMALLER) object->type = ENTITY_NONE;
            break;
        case ENTITY_NONE:
        default:
            object->is_an_existing_entity = false;
        }
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

u32 draw_obstacle(Drawing_Obstacle& drawing, Mouse* mouse, Camera camera, Entity* data) {
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
        *data = Entity{ ENTITY_STATIC, true, move_rect(scaled_rect, camera.pos)};
        drawing.active = false;

        *(data + 1) = Entity{ ENTITY_MONSTER, true, Rectf{ data->posx, data->posy + data->radiusy * 1.5f, data->radiusx * .3f, data->radiusy * .3f } };
        //(data + 1)->standing_on = data;
        (data + 1)->move_speed = .1f;
        return 2;
    }
    return 0;
}

Vec2f screen_to_world(Vec2f v, Camera camera) {
    return Vec2f { v.x * camera.scale + camera.posx, v.y * camera.scale + camera.posy };
}

Rectf screen_to_world(Rectf r, Camera camera) {
    return Rectf { screen_to_world(r.pos, camera), r.radiusx * camera.scale, r.radiusy * camera.scale };
}

void erase_obstacle(Mouse* mouse, Entity* obstacles, u32 obstacles_count, Camera camera) {
    if (!mouse->right.down) return;
    u32 overlap_index = first_overlap_index(screen_to_world(Rectf {mouse->pos, 0, 0}, camera), obstacles, obstacles_count);
    if (overlap_index < obstacles_count) {
        obstacles[overlap_index].type = ENTITY_NONE;
    }
}

Camera update_camera(Camera camera, Input* input) {
    camera.pos = move_camera(camera.pos, input);
    return camera;
}

void init_game_state(Game_Info* game_info, Frame_Info* this_frame, Frame_Info* last_frame) {
    f32 scale = .05f;
    this_frame->camera.scale = last_frame->camera.scale = scale * 2;
    this_frame->player.rect  = last_frame->player.rect  = Rectf{ 12.0f, 4.0f, 1.0f, 1.0f };

    game_info->display_text_count = 0;
    game_info->display_text_chars_to_draw_count = 0;
    game_info->input_text_count = 0;
    game_info->drawing.active = false;

    game_info->game_state_is_initialiezed = true;
}

// Idea: Loop over entire text and get list of indices of new lines
// Seems like a good way to deal with incorrect entries
// Entity: type=1 posx=123.4 posy=123.4 radiusx=123.4 radiusy=123.4 move_speed=123.4 facing=-1
#define ENTITY_START "Entity:"
#define ENTITY_TYPE " type="
#define ENTITY_POSX " posx="
#define ENTITY_POSY " posy="
#define ENTITY_RADIUSX " radiusx="
#define ENTITY_RADIUSY " radiusy="
#define ENTITY_MOVE_SPEED " move_speed="
#define ENTITY_FACING " facing="
#define LENGTH(s) (sizeof(s) - 1)
u32 parse_savefile(char* text_start, u32 text_size, Entity* result) {
    u32 count = 0;
    char* text = text_start;
    u32 start_size = LENGTH(ENTITY_START);
    while (true) {
        if (memcmp(text, ENTITY_START, start_size) == 0) {
            text += start_size;
            Entity_Type type = (Entity_Type)strtol(text + LENGTH(ENTITY_TYPE), &text, 10);
            f32 posx    = strtof(text + LENGTH(ENTITY_POSX), &text);
            f32 posy    = strtof(text + LENGTH(ENTITY_POSY), &text);
            f32 radiusx = strtof(text + LENGTH(ENTITY_RADIUSX), &text);
            f32 radiusy = strtof(text + LENGTH(ENTITY_RADIUSY), &text);
            f32 move_speed = strtof(text + LENGTH(ENTITY_MOVE_SPEED), &text);
            s8 facing = (s8)strtol(text + LENGTH(ENTITY_FACING), &text, 10);
            result[count++] = Entity{ type, true, Rectf{ posx, posy, radiusx, radiusy }, move_speed, facing};
        }
        while (true) {
            if (text - text_start + 1 >= text_size) return count;
            if (text[0] == '\n') break;
            ++text;
        }
        ++text;
    }
}

bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state) {
    Game_Info* game_info = (Game_Info*)persistent_state->data;
    Frame_Info* this_frame = (Frame_Info*)frame_state->data;
    if (!game_info->game_state_is_initialiezed) {
        init_game_state(game_info, this_frame, last_frame);
        //
        // Load save file
        // This is stupid since we need to load into last frame, so last frames objects are put into this frames arena.
        Arena temp_storage = *persistent_state;
        last_frame->objects = (Entity*)arena_current(frame_state);
        u32 file_size = game_info->platform_read_entire_file(temp_storage, "test.txt");
        last_frame->objects_count = parse_savefile((char*)arena_current(temp_storage), file_size, last_frame->objects);
        frame_state->current += sizeof(last_frame->objects[0]) * last_frame->objects_count;
    }
    Player* player = &this_frame->player;
    *player = last_frame->player;

    this_frame->camera = update_camera(last_frame->camera, this_frame->input);
    
    for (int i = 0; i < game_info->input_text_count; ++i) {
        u32 count = game_info->display_text_count++;
        count = MIN(count, DISPLAY_TEXT_CAPACITY);
        game_info->display_text[count] = game_info->input_text[i];
    }


    this_frame->objects = (Entity*)(frame_state->data + frame_state->current);
    this_frame->objects_count = update_objects(last_frame->objects, last_frame->objects_count, this_frame->objects);
    
    Vec2f player_delta = get_player_pos_delta(player, this_frame->input, last_frame->collision_info);
    player->rect = try_move_axis(player->rect, player_delta, /*axis_offset:*/ 0, this_frame->objects, this_frame->objects_count, &this_frame->collision_info);
    player->rect = try_move_axis(player->rect, player_delta, /*axis_offset:*/ 1, this_frame->objects, this_frame->objects_count, &this_frame->collision_info);

    this_frame->objects_count += draw_obstacle(game_info->drawing, this_frame->mouse, this_frame->camera, this_frame->objects + this_frame->objects_count);
    erase_obstacle(this_frame->mouse, this_frame->objects, this_frame->objects_count, this_frame->camera);


    if (this_frame->input[INPUT_EDITOR_SAVE].presses) {
        u32 persistent_reset = persistent_state->current;
        u32 total_length = 0;
        for (int i = 0; i < this_frame->objects_count; ++i) {
            Entity object = this_frame->objects[i];
            if (!object.type) continue;
            u32 serialized_length = serialize_entity(object, *persistent_state);
            total_length += serialized_length;
            persistent_state->current += serialized_length;
        }
        persistent_state->current = persistent_reset;
        game_info->platform_write_entire_file("test.txt", (const char*)arena_current(*persistent_state), total_length);
    }

    return true;
}