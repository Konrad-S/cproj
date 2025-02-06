#include <cstdio>
#include <math.h>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

struct {
    int posx;
    int posy;
} global_player;

const char* vertex_shader_source = 
"#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"uniform vec2 offset;"
"void main() {\n"
"   gl_Position = vec4(aPos + vec3(offset, 0), 1.0);\n"
"}\n";

const char* fragment_shader_source = 
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"   FragColor = vec4(1.0, 0.5, 0.2, 1.0);\n"
"}\n";

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
    case GLFW_KEY_W:
        global_player.posx += 1;

    }
}

int main(void) {
    // Initialize GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    global_player.posx = 0;
    global_player.posy = 0;

    // Set error callback
    glfwSetErrorCallback(error_callback);

    // OpenGL version: 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Create a windowed mode window and its OpenGL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "CProj Game", NULL, NULL);
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

    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertex_shader);
    glAttachShader(shaderProgram, fragment_shader);
    glLinkProgram(shaderProgram);

    // Cleanup
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    glUseProgram(shaderProgram);

    float i = 0.0f;
    GLuint offset_location = glGetUniformLocation(shaderProgram, "offset");
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the screen
        float time = glfwGetTime();
        glClearColor(.2f, .5f, (float)global_player.posx / 255, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glBindVertexArray(VAO);
        glUniform2f(offset_location, global_player.posx / 100.0f, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}