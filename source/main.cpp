#include <cstdio>
#include <math.h>
#include <windows.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

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

struct Square {
    union {
        Vec2f pos;
        struct {
            f32 posx;
            f32 posy;
        };
    };
    f32 r;
};

struct Input {
    bool up;
    bool right;
    bool down;
    bool left;
};

struct Player {
    Square square;
};

struct Frame_Info {
    Player player;
    Input input;

};

// Error callback function
void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

// Key callback function
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    }
    switch (key) {
    if (action != GLFW_PRESS) return;
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, 1);
    }
}

void clear_arena(Arena* arena) {
    memset(arena->data, 0, arena->capacity);
    arena->current = 0;
}

u32 read_entire_file_txt (Arena* arena, const char* file_path) {
    HANDLE handle = CreateFile(file_path,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    
    bool do_read_file = handle != INVALID_HANDLE_VALUE;
    if (!do_read_file) {
        printf("Failed getting a handle to: %s\nError code: %d\n", file_path, GetLastError());
    }
    DWORD bytes_read = 0;
    if (do_read_file) {
        bool success = ReadFile(handle, arena->data, arena->capacity - arena->current, &bytes_read, NULL);
        if (!success) {
            printf("Failed reading from: %s\nError code: %d\n", file_path, GetLastError());

        }
    }
    return bytes_read;
}

Input get_updated_input(GLFWwindow* window, Input last_input) {
    // Last input will be needed when input handling is more robust, e.g. held vs pressed.
    Input input = { false, false, false, false};

    int state = glfwGetKey(window, GLFW_KEY_E);
    if (state == GLFW_PRESS) {
        input.up = true;
    }
    state = glfwGetKey(window, GLFW_KEY_F);
    if (state == GLFW_PRESS) {
        input.right = true;
    }
    state = glfwGetKey(window, GLFW_KEY_D);
    if (state == GLFW_PRESS) {
        input.down = true;
    }
    state = glfwGetKey(window, GLFW_KEY_S);
    if (state == GLFW_PRESS) {
        input.left = true;
    }
    return input;
}

Player get_updated_player(Player last_player, Input input) {
    Player player = last_player;
    if (input.up) {
        player.square.posy += .1f;
    }
    if (input.right) {
        player.square.posx += .1f;
    }
    if (input.down) {
        player.square.posy -= .1f;
    }
    if (input.left) {
        player.square.posx -= .1f;
    }
    return player;
}

int main(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    // Set error callback
    glfwSetErrorCallback(error_callback);

    // OpenGL version: 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    u32 screen_width = 800;
    u32 screen_height = 600;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "CProj Game", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }

    // Make the window's context current
    glfwMakeContextCurrent(window);

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    // Set the viewport size
    glViewport(0, 0, 800, 600);

    // Set key callback
    glfwSetKeyCallback(window, key_callback);


    // Setting up glDrawArrays
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f
    };
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    Arena persistent;

    #define KILOBYTE 1024
    const int PERSISTENT_ARENA_SIZE = 1*KILOBYTE*KILOBYTE;
    const int FRAME_ARENA_SIZE = (KILOBYTE*KILOBYTE)/2;
    persistent.data = (u8*)malloc(PERSISTENT_ARENA_SIZE + FRAME_ARENA_SIZE*2); // 2 MB
    persistent.capacity = PERSISTENT_ARENA_SIZE;
    persistent.current = 0;
    Arena frame_arena_0;
    Arena frame_arena_1;
    frame_arena_0.data = persistent.data + persistent.capacity;
    frame_arena_0.capacity = FRAME_ARENA_SIZE;
    frame_arena_0.current = 0;
    frame_arena_1.data = frame_arena_0.data + frame_arena_0.capacity;
    frame_arena_1.capacity = FRAME_ARENA_SIZE;
    frame_arena_1.current = 0;


    u32 start_of_shader_text = persistent.current;

    u32 bytes_read = read_entire_file_txt(&persistent, "../assets/vertex.txt");
    if (bytes_read == 0) {
        printf("Failed to get vertex shader, exiting...");
        return -1;
    }

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    if (vertex_shader == 0) {
        printf("Failed to create vertex shader");
        return -1;
    }
    const GLchar* vertex_shader_source = (GLchar*) persistent.data + start_of_shader_text;
    glShaderSource(vertex_shader, 1, &vertex_shader_source, (GLint*)&bytes_read);
    glCompileShader(vertex_shader);
    persistent.current = start_of_shader_text;
    GLint success;
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_length;
        glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
        char* error_info = (char*)persistent.data + persistent.current;
        glGetShaderInfoLog(vertex_shader, log_length, nullptr, error_info);
        printf("Vertex shader compilation error:\n%s", error_info);
        return -1;
    }
    bytes_read = read_entire_file_txt(&persistent, "../assets/fragment.txt");
    const GLchar* fragment_shader_source = (GLchar*) persistent.data + start_of_shader_text;
    if (bytes_read == 0) {
        printf("Failed to get fragment shader, exiting...");
        return -1;
    }
    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, (GLint*)&bytes_read);
    glCompileShader(fragment_shader);
    persistent.current = start_of_shader_text;
    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_length;
        glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
        char* error_info = (char*)persistent.data + persistent.current;
        glGetShaderInfoLog(fragment_shader, log_length, nullptr, error_info);
        printf("Fragment shader compilation error:\n%s", error_info);
        return -1;
    }

    unsigned int shader_program = glCreateProgram();
    if (shader_program == 0) {
        printf("Failed to create shader program");
        return -1;
    }
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);

    // Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glUseProgram(shader_program);
    GLuint offset_location = glGetUniformLocation(shader_program, "offset");
    GLuint world_scale_location = glGetUniformLocation(shader_program, "world_scale");
    f32 scale = .01f;
    glUniform2f(world_scale_location, 1/(screen_width*scale), 1/(screen_height*scale));
    
    // Setup game state
    frame_arena_0.current += sizeof(Frame_Info);
    frame_arena_1.current += sizeof(Frame_Info);
    Frame_Info* this_frame = (Frame_Info*)frame_arena_0.data;
    this_frame->player.square.posx = 0.0f;
    this_frame->player.square.posy = 0.0f;

    bool even_frame = false;
    while (!glfwWindowShouldClose(window)) {
        Frame_Info* last_frame = this_frame;
        if (even_frame) {
            clear_arena(&frame_arena_0);
            frame_arena_0.current += sizeof(Frame_Info);
            this_frame = (Frame_Info*)frame_arena_0.data;
        }
        else {
            clear_arena(&frame_arena_1);
            frame_arena_1.current += sizeof(Frame_Info);
            this_frame = (Frame_Info*)frame_arena_1.data;
        }
        this_frame->input = get_updated_input(window, last_frame->input);
        this_frame->player = get_updated_player(last_frame->player, this_frame->input);

        float time = glfwGetTime();
        glClearColor(.2f, .5f, .5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glUniform2f(offset_location, this_frame->player.square.posx, this_frame->player.square.posy);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
        even_frame = !even_frame;
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}