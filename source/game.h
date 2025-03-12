#ifndef GAME_H

#include <stdint.h>

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

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
struct Arena {
    u8* data;
    u64 current;
    u64 capacity;
};

struct Vec2f {
    f32 x;
    f32 y;
};
#define POS_F32    union { Vec2f pos;    struct { f32 posx;    f32 posy;    }; }
#define RADIUS_F32 union { Vec2f radius; struct { f32 radiusx; f32 radiusy; }; }

#define ANONYMOUS
#define RECTF_DEFINITION(name) struct name { POS_F32; RADIUS_F32; }
RECTF_DEFINITION(Rectf);

enum Entity_Type {
    NONE = 0,
    PLAYER,
    STATIC,
    MONSTER,
};

struct Entity {
    Entity_Type type;
    bool is_an_existing_entity;
    union {
        Rectf rect;
        RECTF_DEFINITION(ANONYMOUS);
    };
    Entity* standing_on;
    bool    grounded;
    Vec2f   velocity;
    f32     move_speed;
    s8      facing;
};
typedef Entity Player;

struct Direction {
    bool up;
    bool right;
    bool down;
    bool left;
};

enum InputAction {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    CAM_UP,
    CAM_RIGHT,
    CAM_DOWN,
    CAM_LEFT,
    EDITOR_SAVE,
    ENUM_COUNT,
};
#define INPUT_COUNT (InputAction::ENUM_COUNT)
#define INPUT_SIZE (INPUT_COUNT * sizeof(Input))

struct Input {
    u32  presses;
    u32  releases;
    bool down;
};

struct Mouse {
    POS_F32;
    Input left;
    Input right;
};

struct Collision_Info {
    Direction sides_touched;
};

struct Drawing_Obstacle {
    bool active;
    POS_F32;
};

struct Camera {
    POS_F32;
    f32 scale;
};

struct Frame_Info {
    Player  player;
    Input   input[INPUT_COUNT];
    Camera  camera;
    Drawing_Obstacle drawing;
    Mouse*  mouse;
    Entity* objects;
    u32     objects_count;
    Rectf*  collisions;
    u32     collisions_count;
    Collision_Info collision_info;
};

typedef u32  (*Platform_Read_Entire_File)(Arena, const char*);
typedef bool (*Platform_Write_Entire_File)(const char*, const char*, u32);

struct Game_Info {
    Platform_Read_Entire_File platform_read_entire_file;
    Platform_Write_Entire_File platform_write_entire_file;
    // Stuff in Frame_Info that we always copy over without modification
    // and we don't care what the previous frames value was, should go here instead.
};

//Rectf get_updated_player(Rectf last_player, Input input);
extern "C" {
    __declspec(dllexport) bool update_game(Arena* frame_state, Frame_Info* last_frame, Arena* persistent_state);
}

#define GAME_H
#endif