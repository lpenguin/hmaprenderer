#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <fstream>
#include "shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

void error_callback(int error, const char* description)
{
    std::cout<<"Error: " << error << ", description: "<<description<<std::endl;
}

enum class Format {
    Image8Bit,
    ImageRGBA,
    ImageRGB,
};

class Image{
public:
    unsigned int width;
    unsigned int height;
    char* data;
    Format format;

    Image(unsigned int width, unsigned int height, Format format, char *data) : width(width), height(height), data(data), format(format) {}

    virtual ~Image() {
        delete data;
    }


};



std::unique_ptr<Image> read_image(const std::string& path, unsigned int width=0,unsigned int height=0, Format format=Format::Image8Bit) {
    auto input = std::ifstream(path, std::ios::binary);
    if(!input.is_open()){
        fprintf(stderr, "Error in open: %s\n", path.c_str());
        exit(1);
    }

    if(width == 0){
        input.read((char*)&width, sizeof(unsigned int));
        input.read((char*)&height, sizeof(unsigned int));
    }

    int size = 0;
    switch (format){
        case Format ::Image8Bit:
            size = 1;
            break;
        case Format ::ImageRGB:
            size = 3;
            break;
        case Format ::ImageRGBA:
            size = 4;
            break;
    }
    size = size * width * height;

    auto data = new char[size];
    input.read(data, size);
    printf("read image (%d, %d, size=%d*%d*%d=%d) from %s\n", width, height, width, height, size / (width * height), size, path.c_str());
    return std::unique_ptr<Image>(new Image(width, height, format, data));
}


GLuint texture_from_image(const std::unique_ptr<Image> &image){
    GLuint textureID;
    glGenTextures(1, &textureID);
    // Сделаем созданную текстуру текущий, таким образом все следующие функции будут работать именно с этой текстурой
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Передадим изображение OpenGL
    if(image->format == Format::Image8Bit){
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_R8UI,
                image->width,
                image->height,
                0,
                GL_RED_INTEGER,
                GL_UNSIGNED_BYTE,
                image->data
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    } else if (image->format == Format::ImageRGB){
        glTexImage1D(
                GL_TEXTURE_1D,
                0,
                GL_RGB,
                image->width,
                0,
                GL_RGB,
                GL_UNSIGNED_BYTE,
                image->data
        );
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    }


    auto err = glGetError();
    if ( err != GL_NO_ERROR ){
        printf( "glTexImage2D: %d\n",err );
        exit(1);
    }

    return textureID;
}


int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Ошибка при инициализации GLFWn");
        return -1;
    }
    glfwSetErrorCallback(error_callback);

//    glfwWindowHint(GLFW_SAMPLES, 4); // 4x Сглаживание
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Мы хотим использовать OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
//    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Мы не хотим старый OpenGL

    const auto width = 1024;
    const auto height = 768;

    GLFWwindow *window; // (В сопроводительном исходном коде эта переменная является глобальной)
    window = glfwCreateWindow(width, height, "Tutorial 01", nullptr, nullptr);
    if (window == nullptr) {

        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    printf("glfwMakeContextCurrent: %d\n", glGetError());
    // Инициализируем GLEW
    glewExperimental = GL_TRUE; // Флаг необходим в Core-режиме OpenGL
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Невозможно инициализировать GLEW\n");
        return -1;
    }
    printf("glewInit: %d\n", glGetError());
    printf("after : %d\n", glGetError());
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    auto heightMap = read_image("height.map");
    GLuint heightMapTexture = texture_from_image(heightMap);


    auto colorMap = read_image("color.map");
    GLuint colorMapTexture = texture_from_image(colorMap);

    auto palette = read_image("fostral.pal", 256 , 1, Format::ImageRGB);
    GLuint paletteTexture = texture_from_image(palette);


    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);


    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("heightmap.vert", "heightmap.frag");

    // Get a handle for our "MVP" uniform
    // Projection matrix : 45° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    glm::vec4 camPos = glm::vec4(0.5, -1, 1, 1.0);
    glm::vec4 screenSize = glm::vec4(heightMap->width/1024.0f, heightMap->height/1024.0f, 0.0f, 0.0f);
    float realHeight;
    if(heightMap->height > 4096){
        realHeight = 4096;
    }else {
        realHeight = heightMap->height;
    }
    float numLayers = heightMap->height / realHeight;

    glm::vec4 terrainScale = glm::vec4(heightMap->width, realHeight, 48,numLayers);

    glm::mat4 View       = glm::lookAt(
            glm::vec3(camPos), // Camera is at (4,3,3), in World Space
            glm::vec3(0.5,1,0), // and looks at the origin
            glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model      = glm::mat4(1.0f);
    glm::mat4 MVP        = Projection * View * Model; // Remember, matrix multiplication is the other way around
    glm::mat4 MVP_inv = glm::inverse(MVP);

//    GLint TextureID  = glGetUniformLocation(programID, "myTextureSampler");
    GLint colorMapTextureID  = glGetUniformLocation(programID, "t_Color");
    GLint heightMapTextureID  = glGetUniformLocation(programID, "t_Height");
    GLint paletteTextureID  = glGetUniformLocation(programID, "t_Palette");

    GLint u_CamPos_loc = glGetUniformLocation(programID, "u_CamPos");
    GLint u_ScreenSize_loc = glGetUniformLocation(programID, "u_ScreenSize");
    GLint u_TextureScale_loc = glGetUniformLocation(programID, "u_TextureScale");
    GLint u_ViewProj_loc = glGetUniformLocation(programID, "u_ViewProj");
    GLint u_InvViewProj_loc = glGetUniformLocation(programID, "u_InvViewProj");


    const auto unitw = 1.0f * colorMap->width / 1024;
    const auto unith = 1.0f * colorMap->height / 1024;
    const auto zero = 0.0f;
    static const GLfloat g_vertex_buffer_data[] = {
            zero, zero, 0.0f,
            unitw, unith, 0.0f,
            unitw, zero, 0.0f,
            zero, zero, 0.0f,
            zero, unith, 0.0f,
            unitw, unith, 0.0f,
    };
    static const GLfloat g_uv_buffer_data[] = {
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f,

        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f,
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);

    do{
        // Пока что ничего не выводим. Это будет в уроке 2.
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);
// Send our transformation to the currently bound shader,
        // in the "MVP" uniform
//        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniform4fv(u_CamPos_loc, 1, &camPos[0]);
        glUniform4fv(u_ScreenSize_loc, 1, &screenSize[0]);
        glUniform4fv(u_TextureScale_loc, 1, &terrainScale[0]);
        glUniformMatrix4fv(u_ViewProj_loc, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(u_InvViewProj_loc, 1, GL_FALSE, &MVP_inv[0][0]);

        //        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_1D, paletteTexture);
        glUniform1i(paletteTextureID, 0);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightMapTexture);
        glUniform1i(heightMapTextureID, 1);
        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, colorMapTexture);
        glUniform1i(colorMapTextureID, 2);


        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, 2 * 3); // 12*3 indices starting at 0 -> 12 triangles

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();

    } // Проверяем нажатие клавиши Escape или закрытие окна
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0 );
    // Cleanup VBO and shader
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteProgram(programID);
    glDeleteTextures(1, &colorMapTexture);
    glDeleteVertexArrays(1, &VertexArrayID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();
    return 0;
}