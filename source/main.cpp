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

Input global_inputs[INPUT_ENUM_COUNT] = {};
#define INPUT_TEXT_CAPACITY 32
char global_input_text[INPUT_TEXT_CAPACITY] = {};
u8 global_input_text_count = 0;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key < 0) return;
    InputAction input_key;

    if (action == GLFW_PRESS && key >= GLFW_KEY_A && key <= GLFW_KEY_Z && global_input_text_count < INPUT_TEXT_CAPACITY) {
        global_input_text[global_input_text_count++] = (char)key;
    }

    switch(key) {
        case GLFW_KEY_E:
            input_key = INPUT_UP;
            break;
        case GLFW_KEY_F:
            input_key = INPUT_RIGHT;
            break;
        case GLFW_KEY_D:
            input_key = INPUT_DOWN;
            break;
        case GLFW_KEY_S:
            input_key = INPUT_LEFT;
            break;
        case GLFW_KEY_I:
            input_key = INPUT_CAM_UP;
            break;
        case GLFW_KEY_L:
            input_key = INPUT_CAM_RIGHT;
            break;
        case GLFW_KEY_K:
            input_key = INPUT_CAM_DOWN;
            break;
        case GLFW_KEY_J:
            input_key = INPUT_CAM_LEFT;
            break;
        case GLFW_KEY_F6:
            input_key = INPUT_EDITOR_SAVE;
            break;
        case GLFW_KEY_T:
            input_key = INPUT_THROW;
            break;
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, 1);
            return;
        default:
            return;
    }
    Input* input = global_inputs + input_key;
    if (action == GLFW_PRESS)
    {
        input->presses++;
        input->down = true;
    }
    else if (action == GLFW_RELEASE) {
        input->releases++;
        input->down = false;
    }
}

Mouse global_mouse = {};
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button < 0) return;
    Input* input;
    switch(button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            input = &global_mouse.left;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            input = &global_mouse.right;
            break;
        default:
            return;
    }
    if (action == GLFW_PRESS)
    {
        input->presses++;
        input->down = true;
    }
    else if (action == GLFW_RELEASE) {
        input->releases++;
        input->down = false;
    }
}

void cursor_position_callback(GLFWwindow* window, double posx, double posy) {
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    global_mouse.posx = (f32)posx;
    global_mouse.posy = height - (f32)posy;
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
        bool success = ReadFile(handle, arena_current(arena), arena.capacity - arena.current, &bytes_read, NULL);
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

bool write_entire_file(const char* file_path, const char* content, u32 content_length) {
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

void transfer_input(Input* input) {
    memcpy(input, global_inputs, INPUT_SIZE);
    for (int i = 0; i < INPUT_ENUM_COUNT; ++i) {
        global_inputs[i].presses = 0;
        global_inputs[i].releases = 0;
    }
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

u32 load_shader(const char* file_name, Arena arena, GLenum shader_type)
{
    u32 start_of_shader_text = arena.current;
    u32 bytes_read = read_entire_file(arena, file_name);
    if (bytes_read == 0) {
        printf("Failed to get shader, exiting...");
        assert(false);
    }
    u32 shader = glCreateShader(shader_type);
    if (shader == 0) {
        printf("Failed to create shader");
        assert(false);
    }
    const GLchar* shader_source = (GLchar*) arena.data + start_of_shader_text;
    glShaderSource(shader, 1, &shader_source, (GLint*)&bytes_read);
    glCompileShader(shader);
    arena.current = start_of_shader_text;
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        char* error_info = (char*)arena.data + arena.current;
        glGetShaderInfoLog(shader, log_length, nullptr, error_info);
        printf("Shader compilation error in '%s':\n%s", file_name, error_info);
        assert(false);
    }
    return shader;
}


u32 text_to_char_coords(const char* text, u32 text_count, Vec2f starting_offset, Arena arena) {
    int i = 0;
    f32 x_pos = starting_offset.x;
    f32 y_pos = starting_offset.y;
    while (i < text_count && text[i] != '\0') {
        char c = text[i++];
        if (c == '\n') {
            x_pos = starting_offset.x;
            --y_pos;
            continue;
        }
        //40 * 7
        f32 x_offset = c % 40;
        f32 y_offset = c / 40;
        Pos_Offset* result = (Pos_Offset*)arena_append(&arena, sizeof(Pos_Offset));
        *result = Pos_Offset{ x_pos, y_pos, x_offset, y_offset };
        ++x_pos;
    }
    return i;
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
    //
    // Load OpenGL functions using GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }
    glViewport(0, 0, screen_width, screen_height);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);
    //glfwSwapInterval(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //
    // Setup openGL buffers 
    unsigned int rect_VAO;
    glGenVertexArrays(1, &rect_VAO);
    glBindVertexArray(rect_VAO);
    const u8 POS_COUNT = 2;
    const u8 COLOR_COUNT = 3;
    const u8 UV_COUNT = 2;
    const u8 NUMBER_OF_VERTICES = 6;
    const u8 VERTEX_COUNT = (POS_COUNT + COLOR_COUNT + UV_COUNT);
    const u8 VERTICES_COUNT = (NUMBER_OF_VERTICES * VERTEX_COUNT); 
    float rect_vertices[VERTICES_COUNT] = {
        -1.f, -1.f,   1.f, 1.f, 1.f,    0.f, 1.f,   //left bot
         1.f, -1.f,   0.f, 0.f, 1.f,    1.f, 1.f,   //right bot
         1.f,  1.f,   0.f, 1.f, 0.f,    1.f, 0.f,   //right top
        -1.f, -1.f,   1.f, 1.f, 1.f,    0.f, 1.f,   //left bot
         1.f,  1.f,   0.f, 1.f, 0.f,    1.f, 0.f,   //right top
        -1.f,  1.f,   1.f, 1.f, 0.f,    0.f, 0.f,   //left top
    };
    unsigned int rect_VBO;
    glGenBuffers(1, &rect_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, rect_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rect_vertices), rect_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, POS_COUNT,   GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, COLOR_COUNT, GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)(POS_COUNT * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, UV_COUNT,    GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)((POS_COUNT + COLOR_COUNT) * sizeof(float)));
    glEnableVertexAttribArray(2);

    //
    // Setup text VAO
    unsigned int text_VAO;
    glGenVertexArrays(1, &text_VAO);
    glBindVertexArray(text_VAO);

    // Use the same VBO as we used for rects,
    glBindBuffer(GL_ARRAY_BUFFER, rect_VBO);
    glVertexAttribPointer(0, POS_COUNT,   GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, COLOR_COUNT, GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)(POS_COUNT * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, UV_COUNT,    GL_FLOAT, GL_FALSE, VERTEX_COUNT * sizeof(float), (void*)((POS_COUNT + COLOR_COUNT) * sizeof(float)));
    glEnableVertexAttribArray(2);

    const u8 TEXT_POS_COUNT = 2;
    const u8 TEXT_OFFSET_COUNT = 2;
    const u8 NUMBER_OF_CHARS = 2;
    const u8 TEXT_VERTEX_COUNT = TEXT_POS_COUNT + TEXT_OFFSET_COUNT;
    const u8 TEXT_VERTICES_COUNT = TEXT_VERTEX_COUNT * NUMBER_OF_CHARS;
    float text_vertices[TEXT_VERTICES_COUNT] = {
        0.f, -0.f,   1.f, 0.f,
        1.f, -0.f,   2.f, 0.f,
    };
    unsigned int text_VBO;
    glGenBuffers(1, &text_VBO);
    glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(text_vertices), text_vertices, GL_STATIC_DRAW); // todo : Update this with an array created from a string, 
                                                                                         // whenever the text content needs to be updated
                                                                                         // aka not every frame
    glVertexAttribPointer(3, TEXT_POS_COUNT,    GL_FLOAT, GL_FALSE, (TEXT_VERTEX_COUNT) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);
    glVertexAttribPointer(4, TEXT_OFFSET_COUNT, GL_FLOAT, GL_FALSE, (TEXT_VERTEX_COUNT) * sizeof(float), (void*)((TEXT_POS_COUNT) * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);


    //
    // Create memory arenas
    Arena persistent;
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
    u32 shader_program;
    u32 text_shader_program;
    {
        u32 vertex_shader =        load_shader("../assets/vertex.txt",   persistent, GL_VERTEX_SHADER);
        u32 fragment_shader =      load_shader("../assets/fragment.txt", persistent, GL_FRAGMENT_SHADER);
        u32 text_fragment_shader = load_shader("../assets/text_fragment.txt", persistent, GL_FRAGMENT_SHADER);
        u32 text_vertex_shader =   load_shader("../assets/text_vertex.txt", persistent, GL_VERTEX_SHADER);

        shader_program = glCreateProgram();
        if (shader_program == 0) {
            printf("Failed to create shader program");
            return -1;
        }
        text_shader_program = glCreateProgram();
        if (text_shader_program == 0) {
            printf("Failed to create shader program");
            return -1;
        }
        glAttachShader(shader_program, vertex_shader);
        glAttachShader(shader_program, fragment_shader);
        glLinkProgram(shader_program);
        glAttachShader(text_shader_program, text_vertex_shader);
        glAttachShader(text_shader_program, text_fragment_shader);
        glLinkProgram(text_shader_program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
    }
    //
    // Texture to shader program
    GLuint texture;
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        int width;
        int height;
        int pixel_depth;
        u8 *data = stbi_load("../assets/ASCII.bmp", &width, &height, &pixel_depth, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
        glUseProgram(text_shader_program);
        glUniform1i(glGetUniformLocation(text_shader_program, "ourTexture"), 0);
    }
    //
    // Get uniforms locations
    GLuint offset_location = glGetUniformLocation(shader_program, "offset");
    GLuint world_scale_location = glGetUniformLocation(shader_program, "world_scale");
    GLuint scale_location = glGetUniformLocation(shader_program, "scale");
    GLuint camera_location = glGetUniformLocation(shader_program, "camera");
    GLuint color_location = glGetUniformLocation(shader_program, "color");
    GLuint rect_color_location = glGetUniformLocation(shader_program, "rect_color");
    //
    // Setup game state
    frame_arena_0.current += sizeof(Frame_Info);
    frame_arena_1.current += sizeof(Frame_Info);
    Frame_Info* this_frame = (Frame_Info*)frame_arena_0.data;
    this_frame->frame_pointer_delta = FRAME_ARENA_SIZE;
    // Game_Info is stored at start of the persistent arena.
    Game_Info* game_info = (Game_Info*)arena_append(&persistent, sizeof(Game_Info));
    game_info->platform_read_entire_file = read_entire_file;
    game_info->platform_write_entire_file = write_entire_file;
    game_info->input_text = global_input_text;
    game_info->game_state_is_initialiezed = false;
    //
    // Load game .dll
    char* game_dll_filename = "game.dll";
    char* temp_game_dll_filename = "TEMP_game.dll";
    Game_Code game_code = load_game_code(game_dll_filename, temp_game_dll_filename);

    bool even_frame = false;
    bool game_wants_to_keep_running = true;
    f32 last_time = 0;
    {
        u32 result = timeBeginPeriod(1);
        assert(result == TIMERR_NOERROR);
    }
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
#define MIN_FRAME_TIME .0016f

        f32 current_time = glfwGetTime() - last_time;
        while ((current_time - last_time) < MIN_FRAME_TIME) {
            Sleep(1);
            current_time = glfwGetTime();
        }
        last_time = current_time;


        transfer_input(this_frame->input);

        game_info->input_text_count = global_input_text_count;
        this_frame->mouse        = &global_mouse;
        game_wants_to_keep_running = game_code.update_function(frame_arena, last_frame, &persistent);

        float time = glfwGetTime();
        glClearColor(.2f, .5f, .5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        //
        // draw text
        glUseProgram(text_shader_program);
        glBindTexture(GL_TEXTURE_2D, texture);
        glBindVertexArray(text_VAO);

        if (game_info->input_text_count) {
            game_info->display_text_chars_to_draw_count = text_to_char_coords(game_info->display_text, game_info->display_text_count, Vec2f{0, 0}, persistent);
            glBindBuffer(GL_ARRAY_BUFFER, text_VBO);
            glBufferData(GL_ARRAY_BUFFER, game_info->display_text_chars_to_draw_count * sizeof(Pos_Offset), arena_current(persistent), GL_STATIC_DRAW);
        }

        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, game_info->display_text_chars_to_draw_count);
        //
        // draw rects
        glUseProgram(shader_program);
        glUniform2f(camera_location, this_frame->camera.posx, this_frame->camera.posy);
        glUniform2f(world_scale_location, 2/(screen_width*this_frame->camera.scale), 2/(screen_height*this_frame->camera.scale));
        glBindVertexArray(rect_VAO);
        
        for (int i = 0; i < this_frame->entities_count; i++) {
            Entity object = this_frame->entities[i];
            if (!object.type) continue;
            Rectf rect = object.rect;
            glUniform2f(offset_location, rect.posx, rect.posy);
            glUniform2f(scale_location, rect.radiusx, rect.radiusy);
            glUniform3f(rect_color_location, 0, 1, 0);
            switch (object.type) {
                case ENTITY_PLAYER:
                case ENTITY_PLAYER_ATTACK:
                    glUniform3f(rect_color_location, .1, 0, .8);
                    break;
                case ENTITY_STATIC:
                    glUniform3f(rect_color_location, .3, .3, .3);
                    break;
                case ENTITY_MONSTER:
                    glUniform3f(rect_color_location, 1, 0, 0);
                    break;
            }
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        global_mouse.left.presses = 0;
        global_mouse.left.releases = 0;
        global_mouse.right.presses = 0;
        global_mouse.right.releases = 0;
        global_input_text_count = 0;
        glfwPollEvents();
        even_frame = !even_frame;
    }
    timeEndPeriod(1);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}