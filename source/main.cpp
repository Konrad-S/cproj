#include <cstdio>
#include <math.h>
#include <windows.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "game.cpp"

typedef bool (*Update_Game)(Arena*, Frame_Info*, Arena*);

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

FILETIME get_write_time(char* file) {
    FILETIME write_time = {};
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesEx(file, GetFileExInfoStandard, &data)) {
        write_time = data.ftLastWriteTime;
    }
    return (write_time);
}

u32 read_entire_file(Arena arena, const char* file_path) {
    HANDLE handle = CreateFileA(file_path,
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
        bool success = ReadFile(handle, arena.data, arena.capacity - arena.current, &bytes_read, NULL);
        if (!success) {
            printf("Failed reading from: %s\nError code: %d\n", file_path, GetLastError());

        }
    }
    CloseHandle(handle);
    return bytes_read;
}

bool append_file_txt(const char* file_path, const char* content, const u32 content_length) {
    HANDLE handle = CreateFileA(file_path,
                                FILE_GENERIC_READ | FILE_APPEND_DATA,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    bool success = handle != INVALID_HANDLE_VALUE;
    if (!success) {
        printf("Failed getting a handle to: %s\nError code: %d\n", file_path, GetLastError());
    }
    if (success) {
        DWORD bytes_written;
        success = SetFilePointer(handle, 0, NULL, FILE_END);
        if (!success) {
            printf("Failed setting file pointer of: %s\nError code:%d\n", file_path, GetLastError());
        }
        success = WriteFile(handle, content, content_length, &bytes_written, NULL);
        if (!success) {
            printf("Failed writing to: %s\nError code:%d\n", file_path, GetLastError());
        }
        if (bytes_written < content_length)
        {
            printf("Failed to write all content to: %s\nWrote %d bytes of %d bytes total.", file_path, bytes_written, content_length);
            success = false;
        }
    }
    CloseHandle(handle);
    return success;

    return false;
}

bool write_entire_file_txt(const char* file_path, const char* content, u32 content_length) {
    HANDLE handle = CreateFileA(file_path,
                                GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL,
                                CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
    bool success = handle != INVALID_HANDLE_VALUE;
    if (!success) {
        printf("Failed getting a handle to: %s\nError code: %d\n", file_path, GetLastError());
    }
    if (success) {
        DWORD bytes_written;
        success = WriteFile(handle, content, content_length, &bytes_written, NULL);
        if (!success) {
            printf("Failed writing to: %s\nError code:%d\n", file_path, GetLastError());
        }
        if (bytes_written < content_length)
        {
            printf("Failed to write all content to: %s\nWrote %d bytes of %dbytes total.", file_path, bytes_written, content_length);
            success = false;
        }
    }
    CloseHandle(handle);
    return success;
}

// Idea: Loop over entire text and get list of indices of new lines
// Seems like a good way to deal with incorrect entries
#define RECT_START "Rectf:"
#define RECT_POSX " posx="
#define RECT_POSY " posy="
#define RECT_RADIUSX " radiusx="
#define RECT_RADIUSY " radiusy="
#define LENGTH(s) (sizeof(s) - 1)
u32 parse_savefile(char* text_start, u32 text_size, Rectf* result) {
    u32 count = 0;
    char* text = text_start;
    u32 start_size = LENGTH(RECT_START);
    while (true) {
        if (memcmp(text, RECT_START, start_size) == 0) {
            text += start_size;
            f32 posx    = strtof(text + LENGTH(RECT_POSX), &text);
            f32 posy    = strtof(text + LENGTH(RECT_POSY), &text);
            f32 radiusx = strtof(text + LENGTH(RECT_RADIUSX), &text);
            f32 radiusy = strtof(text + LENGTH(RECT_RADIUSY), &text);
            result[count++] = Rectf{ posx, posy, radiusx, radiusy };
        }
        while (true) {
            if (text - text_start + 1 >= text_size) return count;
            if (text[0] == '\n') break;
            ++text;
        }
        ++text;
    }
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

Input get_updated_camera_input(GLFWwindow* window, Input last_input) {
    // Last input will be needed when input handling is more robust, e.g. held vs pressed.
    Input input = { false, false, false, false};

    int state = glfwGetKey(window, GLFW_KEY_I);
    if (state == GLFW_PRESS) {
        input.up = true;
    }
    state = glfwGetKey(window, GLFW_KEY_L);
    if (state == GLFW_PRESS) {
        input.right = true;
    }
    state = glfwGetKey(window, GLFW_KEY_K);
    if (state == GLFW_PRESS) {
        input.down = true;
    }
    state = glfwGetKey(window, GLFW_KEY_J);
    if (state == GLFW_PRESS) {
        input.left = true;
    }
    return input;
}

Mouse get_updated_mouse(GLFWwindow* window, Mouse last) {
    // Last input will be needed when input handling is more robust, e.g. held vs pressed.

    f64 posx, posy;
    bool left = false, right = false;
    glfwGetCursorPos(window, &posx, &posy);
    int sizex, sizey;
    glfwGetWindowSize(window, &sizex, &sizey);
    posy = sizey - posy;

    u32 state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS) {
        left = true;
    }
    state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (state == GLFW_PRESS) {
        right = true;
    }
    return Mouse{ (f32)posx, (f32)posy, left, right };
}

struct Game_Code {
    HMODULE dll_handle;
    Update_Game update_function;
    FILETIME dll_write_time;
    bool valid;
};

Game_Code load_game_code(char* file, char* temp_file) {
    Game_Code game_code;
    game_code.valid = false;
    CopyFile(file, temp_file, FALSE);
    game_code.dll_handle = LoadLibrary(temp_file);
    if (!game_code.dll_handle) {
        printf("Failed to load DLL '%s'", temp_file);
        return game_code;
    }
    const char* update_game_function_name = "update_game";
    game_code.update_function = (Update_Game)GetProcAddress(game_code.dll_handle, update_game_function_name); 
    if (!game_code.update_function) {
        printf("Failed to find function '%s'. Error code: %d", update_game_function_name, GetLastError());
        return game_code;
    }
    game_code.valid = true;
    game_code.dll_write_time = get_write_time(temp_file);
    return game_code;
}

void unload_game_code(Game_Code* game_code) {
    if (game_code->dll_handle) {
        FreeLibrary(game_code->dll_handle); //what if we free invalid?
        game_code->dll_handle = 0;
    }
    game_code->valid = false;
    game_code->update_function = 0;
}

int main(void) {
    //
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }
    glfwSetErrorCallback(error_callback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    u32 screen_width = 800;
    u32 screen_height = 600;
    GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "CProj Game", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, screen_width, screen_height);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //
    // Setup openGL buffers 
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    float vertices[] = {
        -1.f, -1.f, 0.0f,
         1.f, -1.f, 0.0f,
         1.f,  1.f, 0.0f,
        -1.f, -1.f, 0.0f,
         1.f,  1.f, 0.0f,
        -1.f,  1.f, 0.0f
    };
    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    //
    // Create memory arenas
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

    //
    // Load shaders
    u32 start_of_shader_text = persistent.current;
    u32 bytes_read = read_entire_file(persistent, "../assets/vertex.txt");
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
    bytes_read = read_entire_file(persistent, "../assets/fragment.txt");
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
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    glUseProgram(shader_program);

    GLuint offset_location = glGetUniformLocation(shader_program, "offset");
    GLuint world_scale_location = glGetUniformLocation(shader_program, "world_scale");
    GLuint scale_location = glGetUniformLocation(shader_program, "scale");
    GLuint camera_location = glGetUniformLocation(shader_program, "camera");
    GLuint color_location = glGetUniformLocation(shader_program, "color");
    f32 scale = .01f;
    glUniform2f(world_scale_location, 1/(screen_width*scale), 1/(screen_height*scale));
    
    //
    // Setup game state
    frame_arena_0.current += sizeof(Frame_Info);
    frame_arena_1.current += sizeof(Frame_Info);
    Frame_Info* this_frame = (Frame_Info*)frame_arena_0.data;
    this_frame->camera_scale = scale * 2;
    this_frame->player.rect = Rectf{ 12.0f, 4.0f, 1.0f, 1.0f };

    this_frame->objects = (Rectf*)arena_current(frame_arena_0);
    u32 file_size = read_entire_file(persistent, "test.txt");
    this_frame->objects_count = parse_savefile((char*)arena_current(persistent), file_size, this_frame->objects);
    frame_arena_0.current += sizeof(Rectf) * this_frame->objects_count;

    bool even_frame = false;
    bool game_wants_to_keep_running = true;
    char* game_dll_filename = "game.dll";
    char* temp_game_dll_filename = "TEMP_game.dll";
    Game_Code game_code = load_game_code(game_dll_filename, temp_game_dll_filename);

    //char* const serialized_buffer = (char* const)arena_current(persistent);
    //u32 serialized_count = serialize_rectf(this_frame->objects[1], serialized_buffer, arena_remaining(persistent));

    //append_file_txt("test.txt", serialized_buffer, serialized_count);

    while (!glfwWindowShouldClose(window) && game_wants_to_keep_running) {
        //
        // Get game DLL and game update procedure

        FILETIME dll_write_time = get_write_time(game_dll_filename);
        if(CompareFileTime(&dll_write_time, &game_code.dll_write_time) != 0) {
            unload_game_code(&game_code);
            game_code = load_game_code(game_dll_filename, temp_game_dll_filename);
            // todo: if we can't run game code, keep looping but don't advance frames
            if (!game_code.valid) return -1;
        }

        Frame_Info* last_frame = this_frame;
        Arena* frame_arena;
        if (even_frame) {
            frame_arena = &frame_arena_0;
        }
        else {
            frame_arena = &frame_arena_1;
        }
        clear_arena(frame_arena);
        frame_arena->current += sizeof(Frame_Info);
        this_frame = (Frame_Info*)frame_arena->data;

        this_frame->input        = get_updated_input(window, last_frame->input);
        this_frame->camera_input = get_updated_camera_input(window, last_frame->camera_input);
        this_frame->mouse        = get_updated_mouse(window, last_frame->mouse);
        this_frame->camera_scale = last_frame->camera_scale;
        this_frame->drawing      = last_frame->drawing;
        if (this_frame->mouse.left) printf("posx:%f posy:%f\n", this_frame->mouse.posx * this_frame->camera_scale, this_frame->mouse.posy * this_frame->camera_scale);
        game_wants_to_keep_running = game_code.update_function(frame_arena, last_frame, &persistent);

        float time = glfwGetTime();
        glClearColor(.2f, .5f, .5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUniform2f(camera_location, this_frame->camera_pos.x, this_frame->camera_pos.y);

        glBindVertexArray(VAO);
        for (int i = 0; i < this_frame->objects_count; i++) {
            Rectf rect = this_frame->objects[i];
            glUniform2f(offset_location, rect.posx, rect.posy);
            glUniform2f(scale_location, rect.radiusx, rect.radiusy);
            glUniform4f(color_location, 0, 1, 0, 1);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glUniform2f(offset_location, this_frame->player.rect.posx, this_frame->player.rect.posy);
        glUniform2f(scale_location, this_frame->player.rect.radiusx, this_frame->player.rect.radiusy);
        glUniform4f(color_location, 1, 1, 1, 1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        for (int i = 0; i < this_frame->collisions_count; i++) {
            Rectf rect = this_frame->collisions[i];
            glUniform2f(offset_location, rect.posx, rect.posy);
            glUniform2f(scale_location, rect.radiusx, rect.radiusy);
            glUniform4f(color_location, 0, 0, 1, 0.5f);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
        even_frame = !even_frame;
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}