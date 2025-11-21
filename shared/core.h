#ifndef CORE_H
#define CORE_H
#include <stdint.h>
#include <string.h>

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

#define ASSERT_THROWS // todo : this should depend on how we build
#ifdef ASSERT_THROWS
#define assert(expression) if (!(expression)) { *(int*)0 = 0; } 
#else
#define assert(expression) printf("not slow");
#endif

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define SIGN(x) (((x) > 0) - ((x) < 0))

#define KILOBYTE 1024
#define PERSISTENT_ARENA_SIZE (1*KILOBYTE*KILOBYTE)
#define FRAME_ARENA_SIZE ((KILOBYTE*KILOBYTE)/2)

struct Vec2f {
    union {
        struct {
            f32 x;
            f32 y;
        };
        f32 a[2];
    };
};

//
// Dynamic Array
struct Pointers {
    void** data;
    u32 count;
    u32 capacity;
};

#define LIST_APPEND(list, item)\
    do{\
        if (list.count >= capacity){\
            if (list.capacity == 0) list.capacity = 8;\
            else list.capacity *= 2;\
            list.data = realloc(list.data, list.capacity*sizeof(*list.data));\
        }\
        list.data[list.count] = item;\
    } while(0)\


#define LIST_PUSH(list, item)\
    do{\
        if (list.count >= list.capacity){\
            assert(list.capacity != 0);\
            list.count = list.capacity--;\
        }\
        list.data[list.count++] = item;\
    } while(0)\

//
// Arena

struct Arena {
    u8* data;
    u64 current;
    u64 capacity;
};

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

#endif // CORE_H