#ifndef GAME_H
#define GAME_H

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

#if CPROJ_SLOW
#define assert(expression) if (!(expression)) { *(int*)0 = 0; } 
#else
#define assert(expression) printf("not slow");
#endif

struct Arena {
    u8* data;
    u64 current;
    u64 capacity;
};

struct Vec2f {
    f32 x;
    f32 y;
};

struct Rectf {
    union {
        Vec2f pos;
        struct {
            f32 posx;
            f32 posy;
        };
    };
    union {
        Vec2f radius;
        struct {
            f32 radiusx;
            f32 radiusy;
        };
    };
};

struct Direction {
    bool up;
    bool right;
    bool down;
    bool left;
};
typedef Direction Input;

struct Player {
    Rectf rect;
    bool  grounded;
    Vec2f velocity;
};

struct Collision_Info {
    Direction sides_touched;
};

struct Frame_Info {
    Player  player;
    Input   input;
    Vec2f   camera_pos;
    Input   camera_input;
    Rectf*  objects;
    u32     objects_count;
    Rectf*  collisions;
    u32     collisions_count;
    Collision_Info collision_info;
};

u8* arena_current(Arena& arena) {
    return arena.data + arena.current;
}

u8* arena_append(Arena& arena, u32 data_size) {
    assert(arena.current + data_size <= arena.capacity);
    u8* result = arena.data + arena.current;
    arena.current += data_size;
    return result;
}

u32 arena_remaining(Arena& arena) {
    return arena.capacity - arena.current;
}

u32 serialize_rectf(Rectf& rect, char* const result, u32 max_count) {
    u32 chars_written = snprintf(result, max_count, "Rectf: posx=%g posy=%g radiusx=%g radiusy=%g\n", rect.posx, rect.posy, rect.radiusx, rect.radiusy);
    return chars_written;
}

//Rectf get_updated_player(Rectf last_player, Input input);
extern "C" {
    __declspec(dllexport) bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state);
}

#endif