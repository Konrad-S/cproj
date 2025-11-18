#ifndef GAME_H
#define GAME_H

#include "core.h"

#define POS_F32    union { Vec2f pos;    struct { f32 posx;    f32 posy;    }; }
#define OFFSET_F32 union { Vec2f offset; struct { f32 offsetx; f32 offsety; }; }
#define RADIUS_F32 union { Vec2f radius; struct { f32 radiusx; f32 radiusy; }; }

#define ANONYMOUS
#define RECTF_DEFINITION(name) struct name { POS_F32; RADIUS_F32; }
RECTF_DEFINITION(Rectf);

struct Pos_Offset {
    POS_F32;
    OFFSET_F32;
};

enum Entity_Type {
    ENTITY_NONE          = 0,
    ENTITY_PLAYER        = 1,
    ENTITY_STATIC        = 2,
    ENTITY_MONSTER       = 4,
    ENTITY_PROJECTILE    = 8,
    ENTITY_PLAYER_ATTACK = 16,
    ENTITY_SPIKE         = 32,
    ENTITY_DOOR          = 64,
};
typedef u32 Entity_Type_Flag;

enum Door_Flags {
    DOOR_OPEN = 1,
};

enum Direction {
    DIR_NONE  = 0,
    DIR_UP    = 1,
    DIR_RIGHT = 2,
    DIR_DOWN  = 4,
    DIR_LEFT  = 8,
};
typedef u8 Direction_Flag;

struct Entity {
    Entity_Type type = {};
    union {
        Rectf rect = {};
        RECTF_DEFINITION(ANONYMOUS);
    };
    f32     move_speed = {};
    Direction facing = {};
    bool    grounded = {};
    Entity* standing_on = {};
    Vec2f   velocity = {};
    u16     flags = {};
};

struct Player {
    Entity*   e;
    Entity*   attack;
    u32       attack_frames;
    Direction transition_level_in_direction;
};

enum Axis {
    AXIS_X = 0,
    AXIS_Y = 1,
};

enum InputAction {
    INPUT_UP,
    INPUT_RIGHT,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_CAM_UP,
    INPUT_CAM_RIGHT,
    INPUT_CAM_DOWN,
    INPUT_CAM_LEFT,
    INPUT_THROW,
    INPUT_EDITOR_SAVE,
    INPUT_EDITOR_LOAD,
    INPUT_EDITOR_CYCLE_DRAW,
    INPUT_ENUM_COUNT,
};
#define INPUT_SIZE (INPUT_ENUM_COUNT * sizeof(Input))

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

struct Overlap_Info {
    Entity_Type type;
    s32 other_index = -1;
    Rectf rect = {};
    Direction_Flag flags = 0;
};

struct Overlap_Info_Node {
    Overlap_Info data = {};
    Overlap_Info_Node* next = NULL;
};

struct Overlap_Info_List {
    Overlap_Info_Node* first = NULL;
    Overlap_Info_Node* last  = NULL;
};

struct Collision_Info {
    s32 other_index = -1;
    Direction_Flag sides_touched = 0;
};

struct Drawing_Obstacle {
    bool active;
    POS_F32;
};

struct Camera {
    POS_F32;
    f32 scale;
};

typedef u32  (*Platform_Read_Entire_File)(Arena, const char*);
typedef bool (*Platform_Write_Entire_File)(const char*, const char*, u32);

struct Game_Info {
    Platform_Read_Entire_File platform_read_entire_file;
    Platform_Write_Entire_File platform_write_entire_file;
    bool    game_state_is_initialiezed;
#define DISPLAY_TEXT_CAPACITY 255
    char    display_text[DISPLAY_TEXT_CAPACITY];
    u32     display_text_count;
    u32     display_text_chars_to_draw_count;
    char*   input_text;
    u8      input_text_count;
    Drawing_Obstacle drawing;
    Entity_Type currently_drawing;

    Player  player;
    Input   input[INPUT_ENUM_COUNT];
    Camera  camera;
    Mouse*  mouse;
    Collision_Info collision_info;
#define ENTITIES_CAPACITY 2000
    Entity  entities[ENTITIES_CAPACITY];
    u32     entities_count;
    s32     frame_pointer_delta;
    u16     empty_entities[ENTITIES_CAPACITY];
    u16     empty_entities_count;
};

//Rectf get_updated_player(Rectf last_player, Input input);
extern "C" {
    __declspec(dllexport) bool update_game(Arena* frame_state, Arena* persistent_state);
}

#endif // GAME_H