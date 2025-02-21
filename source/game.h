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

struct Input {
    bool up;
    bool right;
    bool down;
    bool left;
};

struct Player {
    Rectf rect;
};

struct Frame_Info {
    Player  player;
    Input   input;
    Vec2f   camera_pos;
    Input   camera_input;
    Rectf* objects;
    u32     objects_count;
    Rectf* collisions;
    u32     collisions_count;
};

//Rectf get_updated_player(Rectf last_player, Input input);
extern "C" {
    __declspec(dllexport) bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state);
}

#endif