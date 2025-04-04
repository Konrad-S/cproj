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
    memset(result, 0, data_size);
    return result;
}

u32 arena_remaining(Arena arena) {
    return arena.capacity - arena.current;
}

//
// Game
//

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

const f32 JUMP_VELOCITY = .2f;
const f32 GRAVITY = .005f;
const f32 TERMINAL_VELOCITY = 1.5f;
Vec2f get_player_pos_delta(Player* player, Input* input, Collision_Info last_info) {
    Entity* entity = player->e;
    Vec2f pos = {};
    if (input[INPUT_RIGHT].down) {
        pos.x += .1f;
        entity->facing = DIR_RIGHT;
    }
    if (input[INPUT_LEFT].down) {
        pos.x -= .1f;
        entity->facing = DIR_LEFT;
    }
    entity->grounded = last_info.sides_touched & DIR_DOWN;

    if (entity->grounded) {
        entity->velocity.y = 0;
        if (input[INPUT_UP].down) {
            entity->velocity.y = JUMP_VELOCITY;
            entity->grounded = false;
        }
    }

    entity->velocity.y -= GRAVITY;
    if (entity->velocity.y < -TERMINAL_VELOCITY) entity->velocity.y = -TERMINAL_VELOCITY;
    pos.y += entity->velocity.y;
    
    return pos;
}

bool rectf_overlap(Rectf a, Rectf b, Rectf* result) {
    f32 half_overlap_x  = (a.radiusx + b.radiusx - fabsf(a.posx - b.posx)) / 2;
    if (half_overlap_x <= 0) return false;
    f32 half_overlap_y = (a.radiusy + b.radiusy - fabsf(a.posy - b.posy)) / 2;
    if (half_overlap_y <= 0) return false;

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

    *result = Rectf{ cposx, cposy, half_overlap_x, half_overlap_y };
    return true;
}

u32 first_overlap_index(Rectf rect, Entity* others, u32 others_count, Entity_Type_Flag types_to_check) {
    for (int i = 0; i < others_count; ++i) {
        Entity other = others[i];
        if (!(other.type & types_to_check)) continue;
        Rectf result;
        if (rectf_overlap(rect, other.rect, &result))
        {
            return i;
        }
    }
    return others_count;
}

// u32 first_overlap_index_against_index(u32 rect_index, Entity* others, u32 others_count, Entity_Type_Flag types_to_check) {
//     assert(rect_index < others_count);
//     for (int i = 0; i < others_count; ++i) {
//         if (i == rect_index) continue;
//         Entity other = others[i];
//         if (!(other.type & types_to_check)) continue;
//         Rectf result = rectf_overlap(others[rect_index].rect, other.rect);
//         if (result.radiusx > 0 && result.radiusy > 0)
//         {
//             return i;
//         }
//     }
//     return others_count;
// }

u32 get_overlaps(Rectf rect, Rectf* others, u32 others_count, Rectf* results) {
    u32 results_count = 0;
    for (int i = 0; i < others_count; ++i) {
        Rectf result;
        if (rectf_overlap(rect, others[i], &result))
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
Rectf try_move_axis(Rectf player, f32 move_axis, Axis axis_offset, Entity* others, u32 others_count, Collision_Info* info) {
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
                if (sign > 0) info->sides_touched |= (DIR_RIGHT >> axis_offset);
                else          info->sides_touched |= (DIR_LEFT  >> axis_offset);
                info->other_index = i;
            }
        }
        player.pos.a[axis_offset] = most_extreme_edge - (player.radius.a[axis_offset] + COLLISION_EPSILON) * sign;
    }
    return player;
}

s8 direction_to_int(Direction dir) {
    switch(dir) {
        case DIR_RIGHT:
            return 1;
        case DIR_LEFT:
            return -1;
        case DIR_DOWN:
        case DIR_UP:
        default:
            return 0;
    }
}

Entity* create_entity(Frame_Info* frame) {
    u16 found_index;
    if (frame->empty_entities_count) {
        found_index = frame->empty_entities[--frame->empty_entities_count];
    } else {
        assert(frame->entities_count < ENTITIES_CAPACITY);
        found_index = frame->entities_count++;
    }
    Entity* result = frame->entities + found_index;
    *result = {};
    return result;
}

void attack(Player* player, Frame_Info* frame) {
    Entity* attack = create_entity(frame);
    *attack = {};
    attack->type = ENTITY_PLAYER_ATTACK;
    attack->rect = { player->e->posx + (player->e->radiusx * direction_to_int(player->e->facing)), player->e->posy, .8f, .4f };
    attack->facing = player->e->facing;
    player->attack = attack;
}

void throw_projectile(Rectf player, Frame_Info* frame) {
    Entity* proj = create_entity(frame);
    *proj = {};
    proj->type = ENTITY_PROJECTILE;
    proj->rect = { player.pos, .3f, .3f };
    proj->velocity = { .3, .30 };
}

void update_launched(Entity* object, Entity* others, u32 others_count) {
    object->velocity.y -= GRAVITY;
    Collision_Info collision = {};
    object->rect = try_move_axis(object->rect, object->velocity.y, AXIS_Y, others, others_count, &collision);
    object->rect = try_move_axis(object->rect, object->velocity.x, AXIS_X, others, others_count, &collision);
    if (collision.sides_touched & (DIR_UP | DIR_DOWN)) {
        object->velocity.y = 0;
        if (object->velocity.x) {
            #define STOP_VELOCITY .01f
            if (fabs(object->velocity.x) < STOP_VELOCITY) {
                object->velocity.x = 0;
            } else {
                f32 vel = object->velocity.x;
                object->velocity.x -= SIGN(vel) * ((fabs(vel) + .3f) / 200);
            }
        }
    }
    if (collision.sides_touched & (DIR_DOWN)) {
        object->grounded = true;
        object->standing_on = others + collision.other_index;
        if (object->facing = DIR_DOWN) {
            object->facing = DIR_LEFT;
        }
    }
    if (collision.sides_touched & (DIR_RIGHT | DIR_LEFT)) {
        object->velocity.x *= -1;
    }
}

void update_grounded(Entity* object) {
    object->velocity.x = 0;
    object->velocity.y = 0;
    switch (object->type) {
        case ENTITY_MONSTER:
            {
                Entity* other = object->standing_on;
                bool outside_right = object->posx + object->radiusx > other->posx + other->radiusx;
                bool outside_left  = object->posx - object->radiusx < other->posx - other->radiusx;
                if (outside_right && outside_left) {
                    object->facing = DIR_DOWN;
                } else if (outside_right) {
                    object->facing = DIR_LEFT;
                } else if (outside_left) {
                    object->facing = DIR_RIGHT;
                } else if (object->facing == DIR_DOWN) {
                    object->facing = DIR_RIGHT;
                }
                object->posx += object->move_speed * direction_to_int(object->facing);
            }
            break;
        case ENTITY_PROJECTILE:
            break;
    }
}

u32 flag_to_int(u32 type) {
    switch (type) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 4:
        return 3;
    case 8:
        return 4;
    case 16:
        return 5;
    case 32:
        return 6;
    case 64:
        return 7;
    case 128:
        return 8;
    case 256:
        return 9;
    case 512:
        return 10;
    case 1024:
        return 11;
    default:
        assert(false);
        return 0;
    }
}

#define COLLIDABLE_ENTITIES_COUNT 8
u32 overlap_mapping[COLLIDABLE_ENTITIES_COUNT] = { 
    ENTITY_NONE, // None
    ENTITY_DOOR, // Player
    0,           // Static
    ENTITY_PROJECTILE | ENTITY_PLAYER_ATTACK | ENTITY_SPIKE, // Monster
    0, // Projectile
    0, // Player attack
    0, // Spike
    0, // Door 
};

void push_to_list(Overlap_Info_List* list, Overlap_Info_Node node, Arena* perm) {
    Overlap_Info_Node* new_node = (Overlap_Info_Node*)arena_append(perm, sizeof(Overlap_Info_Node));
    *new_node = node;
    if (!list->first || !list->last) {
        assert(!list->first && !list->last);
        list->first = list->last = new_node;
    } else {
        list->last->next = new_node;
        list->last = new_node;
    }
}

void overlap_entities(Entity* entities, u32 entities_count, Overlap_Info_List* overlap_lists, Arena* perm) {
    u32 result_count = 0;
    for (int i = 0; i < entities_count - 1; ++i) {
        for (int j = i + 1; j < entities_count; ++j) {
            Entity* a = entities + i;
            Entity* b = entities + j;
            bool a_cares_b = overlap_mapping[flag_to_int(a->type)] & b->type;
            bool b_cares_a = overlap_mapping[flag_to_int(b->type)] & a->type;
            if (!a_cares_b && !b_cares_a) {
                continue;
            }
            Rectf overlap;
            bool collided = rectf_overlap(a->rect, b->rect, &overlap);
            if (!collided) continue;
            if (a_cares_b) {
                Overlap_Info_Node node;
                node.data = { b->type, j, overlap };
                push_to_list(overlap_lists + i, node, perm);
            }
            if (b_cares_a) {
                Overlap_Info_Node node;
                node.data = { a->type, i, overlap };
                push_to_list(overlap_lists + j, node, perm);
            }
        }
    }
}

void launch(Entity* entity, Vec2f velocity) {
    entity->grounded = false;
    entity->standing_on = NULL;
    entity->velocity = velocity;
}

void delete_entity(Entity* entity) {
    entity->type = ENTITY_NONE;
}

void player_door_overlap(Player* player, Entity* door) {
    if (door->flags & DOOR_OPEN) {
        player->transition_level_in_direction = door->facing;
    }
}



const f32 CULL_OBJECT_IF_SMALLER = .2;
u32 update_objects(Entity* last_objects, u32 last_objects_count, Entity* result, Arena scratch, u16* empty_entities_result, u16* empty_entities_result_count, Player* player) {
    Overlap_Info_List* overlaps = (Overlap_Info_List*)arena_append(&scratch, sizeof(Overlap_Info_List) * last_objects_count);
    overlap_entities(last_objects, last_objects_count, overlaps, &scratch);

    u32 monsters_alive = 0;
    for (int i = 0; i < last_objects_count; ++i) {
        Entity* object = result + i;
        *object = last_objects[i];
        if (object->radiusx < CULL_OBJECT_IF_SMALLER || object->radiusy < CULL_OBJECT_IF_SMALLER) {
            object->type = ENTITY_NONE;
        }
        Overlap_Info_Node* overlap = overlaps[i].first;
        switch (object->type) {
            case ENTITY_PLAYER:
                while (overlap) {
                    switch (overlap->data.type) {
                        case ENTITY_DOOR:
                            player_door_overlap(player, last_objects + overlap->data.other_index);
                            break;
                    }
                    overlap = overlap->next;
                }        
                break;
            case ENTITY_PLAYER_ATTACK:
                break;
            case ENTITY_MONSTER:
                ++monsters_alive;
                while (overlap) {
                    switch (overlap->data.type) {
                        case ENTITY_PLAYER_ATTACK:
                            launch(object, { .1f * direction_to_int(last_objects[overlap->data.other_index].facing), .8f });
                            break;
                        case ENTITY_SPIKE:
                            delete_entity(object);
                            break;
                    }
                    overlap = overlap->next;
                }
            case ENTITY_PROJECTILE:
                {
                    #define COUNT_AS_LAUNCHED_VELOCITY .2f
                    if (object->standing_on && object->standing_on->type &&
                        fabs(object->velocity.x) < COUNT_AS_LAUNCHED_VELOCITY)
                    {
                        update_grounded(object);
                    } else {
                        update_launched(object, last_objects, last_objects_count);
                    }
                }
                break;
            case ENTITY_STATIC:
                break;
            case ENTITY_SPIKE:
                break;
            case ENTITY_DOOR:
                break;
            case ENTITY_NONE:
                empty_entities_result[(*empty_entities_result_count)++] = i;
                break;
            default:
                assert(false);
                break;
        }
    }
    s64 frames_pointer_offset = (u8*)last_objects - (u8*)result;
    for (int i = 0; i < last_objects_count; ++i) {
        Entity* object = result + i;

        if (object->type == ENTITY_DOOR) {
            if (monsters_alive == 0) object->flags |=  DOOR_OPEN;
            else                     object->flags &= ~DOOR_OPEN;
        }

        if (!object->standing_on) continue;
        if (!object->standing_on->type) {
            object->standing_on = NULL;
            continue;
        }
        // Point to this frames objects instead of last. The pointers relative position is the same.
        object->standing_on = (Entity*)((u8*)object->standing_on - frames_pointer_offset);
    }
    return last_objects_count;
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

void draw_obstacle(Drawing_Obstacle& drawing, Mouse* mouse, Camera camera, Frame_Info* frame, Entity_Type type) {
    if (mouse->right.presses) {
        drawing.active = false;
        return;
    }
    if (!mouse->left.presses) return;
    if (!drawing.active) {
        drawing.active = true;
        drawing.pos = mouse->pos;
    } else {
        Rectf screen_rect = points_to_rect(drawing.pos, mouse->pos);
        Rectf scaled_rect = scale_rect(screen_rect, camera.scale);
        Entity* obstacle = create_entity(frame);
        *obstacle = Entity{ type, move_rect(scaled_rect, camera.pos)};
        obstacle->facing = DIR_RIGHT;
        obstacle->move_speed = .02f;
        drawing.active = false;
    }
}

Vec2f screen_to_world(Vec2f v, Camera camera) {
    return Vec2f { v.x * camera.scale + camera.posx, v.y * camera.scale + camera.posy };
}

Rectf screen_to_world(Rectf r, Camera camera) {
    return Rectf { screen_to_world(r.pos, camera), r.radiusx * camera.scale, r.radiusy * camera.scale };
}

void erase_obstacle(Mouse* mouse, Entity* obstacles, u32 obstacles_count, Camera camera) {
    if (!mouse->right.presses) return;
    u32 overlap_index = first_overlap_index(screen_to_world(Rectf {mouse->pos, 0, 0}, camera), obstacles, obstacles_count, 0XFFFFFFFF & ~ENTITY_PLAYER);
    if (overlap_index < obstacles_count) {
        delete_entity(obstacles + overlap_index);
    }
}

Camera update_camera(Camera camera, Input* input) {
    camera.pos = move_camera(camera.pos, input);
    return camera;
}

void init_game_state(Game_Info* game_info, Frame_Info* last_frame) {
    f32 scale = .05f;
    last_frame->player.e = last_frame->entities;
    last_frame->camera.scale = scale * 2;

    game_info->display_text_count = 0;
    game_info->display_text_chars_to_draw_count = 0;
    game_info->input_text_count = 0;
    game_info->drawing.active = false;
    game_info->game_state_is_initialiezed = true;
    game_info->currently_drawing = ENTITY_STATIC;
}

// Idea: Loop over entire text and get list of indices of new lines
// Seems like a good way to deal with incorrect entries
// Entity: type=1 posx=123.4 posy=123.4 radiusx=123.4 radiusy=123.4 move_speed=123.4 facing=2
#define ENTITY_START "Entity:"
#define ENTITY_TYPE " type="
#define ENTITY_POSX " posx="
#define ENTITY_POSY " posy="
#define ENTITY_RADIUSX " radiusx="
#define ENTITY_RADIUSY " radiusy="
#define ENTITY_MOVE_SPEED " move_speed="
#define ENTITY_FACING " facing="
#define LENGTH(s) (sizeof(s) - 1)
void parse_savefile(char* text_start, u32 text_size, Frame_Info* frame) {
    frame->entities_count = 0;
    frame->empty_entities_count = 0;
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
            Direction facing = (Direction)strtol(text + LENGTH(ENTITY_FACING), &text, 10);
            Entity* result = create_entity(frame);
            *result = Entity{ type, Rectf{ posx, posy, radiusx, radiusy }, move_speed, facing};
        }
        while (true) {
            if (text - text_start + 1 >= text_size) return;
            if (text[0] == '\n') break;
            ++text;
        }
        ++text;
    }
}

void load_level(char* level_path, Game_Info* game_info, Frame_Info* frame, Arena scratch) {
    u32 file_size = game_info->platform_read_entire_file(scratch, level_path);
    char* file_content = (char*)arena_current(scratch);
    parse_savefile(file_content, file_size, frame);

    frame->player.transition_level_in_direction = DIR_NONE;
}

#define ATTACK_DURATION 20
void update_player(Player* player, Player* last_player) {
    // Point to this frames entities instead of last. The pointers relative position is the same.
    s64 frames_pointer_offset = (u8*)last_player - (u8*)player;
    if (player->e)      player->e      = (Entity*)((u8*)player->e      - frames_pointer_offset);
    if (player->attack) player->attack = (Entity*)((u8*)player->attack - frames_pointer_offset);

    if (player->attack) {
        u32 frames_active = last_player->attack_frames;
        if (frames_active > ATTACK_DURATION) {
            delete_entity(player->attack);
            player->attack = NULL;
            player->attack_frames = 0;
        } else {
            player->attack_frames = ++frames_active;
        }
    }
}

bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state) {
    Game_Info* game_info = (Game_Info*)persistent_state->data;
    Frame_Info* this_frame = (Frame_Info*)frame_state->data;
    if (!game_info->game_state_is_initialiezed) {
        init_game_state(game_info, last_frame);
        
        load_level("test.txt", game_info, last_frame, *persistent_state);
    }
    Player* player = &this_frame->player;
    *player = last_frame->player;

    this_frame->camera = update_camera(last_frame->camera, this_frame->input);
    
    for (int i = 0; i < game_info->input_text_count; ++i) {
        u32 count = game_info->display_text_count++;
        count = MIN(count, DISPLAY_TEXT_CAPACITY);
        game_info->display_text[count] = game_info->input_text[i];
    }

    if (player->transition_level_in_direction || this_frame->input[INPUT_EDITOR_LOAD].presses) {
        load_level("test.txt", game_info, last_frame, *persistent_state);
    }

    this_frame->entities_count = update_objects(last_frame->entities, last_frame->entities_count, this_frame->entities, *frame_state, this_frame->empty_entities, &this_frame->empty_entities_count, player);
    update_player(player, &last_frame->player);
    if (!player->attack && this_frame->input[INPUT_THROW].presses) {
        //throw_projectile(player->rect, game_info, this_frame);
        attack(player, this_frame);
    }
    
    Vec2f player_delta = get_player_pos_delta(player, this_frame->input, last_frame->collision_info);
    player->e->rect = try_move_axis(player->e->rect, player_delta.x, AXIS_X, this_frame->entities, this_frame->entities_count, &this_frame->collision_info);
    player->e->rect = try_move_axis(player->e->rect, player_delta.y, AXIS_Y, this_frame->entities, this_frame->entities_count, &this_frame->collision_info);

    if (this_frame->input[INPUT_EDITOR_CYCLE_DRAW].presses) {
        switch (game_info->currently_drawing) {
            case ENTITY_STATIC:
                game_info->currently_drawing = ENTITY_MONSTER;
                break;
            case ENTITY_MONSTER:
                game_info->currently_drawing = ENTITY_SPIKE;
                break;
            case ENTITY_SPIKE:
                game_info->currently_drawing = ENTITY_DOOR;
                break;
            case ENTITY_DOOR:
                game_info->currently_drawing = ENTITY_STATIC;
                break;
            default:
                game_info->currently_drawing = ENTITY_STATIC;
                break;
        }
    }
    draw_obstacle(game_info->drawing, this_frame->mouse, this_frame->camera, this_frame, game_info->currently_drawing);
    erase_obstacle(this_frame->mouse, this_frame->entities, this_frame->entities_count, this_frame->camera);

    if (this_frame->input[INPUT_EDITOR_SAVE].presses) {
        u32 persistent_reset = persistent_state->current;
        u32 total_length = 0;
        for (int i = 0; i < this_frame->entities_count; ++i) {
            Entity object = this_frame->entities[i];
            if (!object.type) continue;
            u32 serialized_length = serialize_entity(object, *persistent_state);
            total_length += serialized_length;
            persistent_state->current += serialized_length;
        }
        persistent_state->current = persistent_reset;
        game_info->platform_write_entire_file("test.txt", (const char*)arena_current(*persistent_state), total_length);
    }

    assert(this_frame->entities_count < ENTITIES_CAPACITY);
    return true;
}