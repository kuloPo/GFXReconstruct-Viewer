/********************************************************************************
 * MIT License
 *
 * Copyright (c) 2025 kuloPo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#include "StartupWindowBackground.hpp"
#include "common.hpp"

const GLfloat fTriangleSize = 45;
const GLfloat fTriangleHeight = (sqrtf(3.0f) * fTriangleSize) / 2.0f;

const int WORKGROUP_SIZE = 128;

const char* strComputeShaderSource = R"(
    #version 430 core
    
    layout(local_size_x = 128) in;
    
    layout(std430, binding = 0) buffer VertexBuffer {
        float vertices[];
    };

    layout(std430, binding = 1) buffer IndexBuffer {
        uint indexes[];
    };

    layout(rgba32f, binding = 0) uniform imageBuffer imageData;
    
    uniform float fTriangleSize;
    uniform float fTriangleHeight;
    uniform int i32TrianglePerRow;
    uniform int i32TrianglePerCol;
    uniform int i32TriangleCount;
    uniform int i32VertexCount;
    uniform int i32Seed;
    
    vec3 hsv2rgb(vec3 c) {
        vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }

    float random(int i32Seed, uint idx) {
        uint combined = i32Seed ^ (idx * 0x9E3779B9u);
    
        uint state = combined * 747796405u + 2891336453u;
        uint word = (state >> ((state >> 28u) + 4u)) ^ state;
        uint multiplier = 277803737u;
    
        return float(word * multiplier) / 4294967295.0;
    }

    void main() {
        uint idx = gl_GlobalInvocationID.x;
        int row;
        int col;
        float x_offset;
        float y_offset;
        
        if (idx >= i32TriangleCount) return;
        
        col = int(idx) / i32TrianglePerRow;
        row = int(idx) % i32TrianglePerRow;

        if (col % 2 == 0) {
            indexes[idx * 3] = (col / 2) * (i32TrianglePerRow + 1) + row;
            indexes[idx * 3 + 1] = indexes[idx * 3] + 1;
            indexes[idx * 3 + 2] = indexes[idx * 3 + 1] + i32TrianglePerRow + 1 - (col / 2 % 2);
        }
        else {
            indexes[idx * 3] = (col / 2) * (i32TrianglePerRow + 1) + row + (col / 2 % 2);
            indexes[idx * 3 + 1] = indexes[idx * 3] + i32TrianglePerRow + 1 - (col / 2 % 2);
            indexes[idx * 3 + 2] = indexes[idx * 3 + 1] + 1;
        }

        float v = random(i32Seed, idx);
        vec3 color = hsv2rgb(vec3(293.0f / 360.0f, 0.18f, v));
        float d = distance(vec2(i32TrianglePerRow / 2 * fTriangleSize, 0), vec2(row * fTriangleSize, col / 2 * fTriangleHeight));
        float alpha =  1.0f - clamp(d / (i32TrianglePerCol / 2.5 * fTriangleHeight), 0.0f, 1.0f);

        imageStore(imageData, int(idx), vec4(color, alpha * v));

        if (idx >= i32VertexCount) return;

        col = int(idx) / (i32TrianglePerRow + 1);
        row = int(idx) % (i32TrianglePerRow + 1);
        
        x_offset = float(col) * fTriangleHeight;
        y_offset = float(row) * fTriangleSize;
        
        if (col % 2 == 1) {
            y_offset -= fTriangleSize / 2.0f;
        }
        
        vertices[idx * 2] = x_offset;
        vertices[idx * 2 + 1] = y_offset;
    }
    )";

const char* strVertexShaderSource = R"(
    #version 430 core

    layout(location = 0) in vec2 aPosition;
    
    uniform int i32Width;
    uniform int i32Height;
    
    void main() {
        float x = aPosition.x;
        float y = i32Height - aPosition.y;
        
        gl_Position = vec4(
            (x / i32Width) * 2.0 - 1.0,
            (y / i32Height) * 2.0 - 1.0,
            0.0, 1.0
        );
    }
    )";

const char* strFragmentShaderSource = R"(
    #version 430 core
    
    layout(rgba32f, binding = 0) uniform imageBuffer imageData;

    out vec4 FragColor;
    
    void main() {
        FragColor = imageLoad(imageData, gl_PrimitiveID);
    }
    )";

GLuint Background::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        LOGE("Shader compilation error type %d log: %s", type, infoLog);
    }

    return shader;
}

GLuint Background::createShaderProgram(GLuint vertexShader = 0, GLuint fragmentShader = 0, GLuint computeShader = 0) {
    GLuint program = glCreateProgram();

    if (vertexShader) glAttachShader(program, vertexShader);
    if (fragmentShader) glAttachShader(program, fragmentShader);
    if (computeShader) glAttachShader(program, computeShader);

    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        LOGE("Shader program linking error: %s", infoLog);
    }

    return program;
}

Background::Background(QWidget* parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setVersion(4, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
}

Background::~Background()
{
    makeCurrent();
    doneCurrent();
}

void Background::initializeGL()
{
    initializeOpenGLFunctions();
    
    i32TrianglePerRow = this->size().height() / fTriangleSize + 1;
    i32TrianglePerCol = this->size().width() / fTriangleHeight + 1;
    i32TriangleCount = i32TrianglePerRow * i32TrianglePerCol * 2;
    i32VertexCount = (i32TrianglePerRow + 1) * (i32TrianglePerCol + 1);

    GLuint computeShader = compileShader(GL_COMPUTE_SHADER, strComputeShaderSource);
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, strVertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, strFragmentShaderSource);

    computeProgram = createShaderProgram(0, 0, computeShader);
    renderProgram = createShaderProgram(vertexShader, fragmentShader);

    glDeleteShader(computeShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    GLuint ebo;
    glGenBuffers(1, &ebo);
    GLuint colorBuffer;
    glGenBuffers(1, &colorBuffer);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, i32VertexCount * 2 * sizeof(GLfloat), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, i32TriangleCount * 3 * sizeof(GLuint), nullptr, GL_STATIC_DRAW);

    glBindBuffer(GL_UNIFORM_BUFFER, colorBuffer);
    glBufferData(GL_UNIFORM_BUFFER, i32TriangleCount * 4 * sizeof(float), nullptr, GL_STATIC_DRAW);

    GLuint tbo;
    glGenTextures(1, &tbo);
    glBindTexture(GL_TEXTURE_BUFFER, tbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, colorBuffer);
    glBindImageTexture(0, tbo, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ebo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, colorBuffer);

    glUseProgram(computeProgram);

    glUniform1f(glGetUniformLocation(computeProgram, "fTriangleSize"), fTriangleSize);
    glUniform1f(glGetUniformLocation(computeProgram, "fTriangleHeight"), fTriangleHeight);
    glUniform1i(glGetUniformLocation(computeProgram, "i32TrianglePerRow"), i32TrianglePerRow);
    glUniform1i(glGetUniformLocation(computeProgram, "i32TrianglePerCol"), i32TrianglePerCol);
    glUniform1i(glGetUniformLocation(computeProgram, "i32TriangleCount"), i32TriangleCount);
    glUniform1i(glGetUniformLocation(computeProgram, "i32VertexCount"), i32VertexCount);
    glUniform1i(glGetUniformLocation(computeProgram, "i32Seed"), static_cast<GLint>(std::time(0)));

    int workgroupCount = (i32TriangleCount + WORKGROUP_SIZE - 1) / WORKGROUP_SIZE;
    glDispatchCompute(workgroupCount, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(77.0f / 255, 77.0f / 255, 138.0f / 255, 1.0f);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
}

void Background::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(renderProgram);

    glUniform1i(glGetUniformLocation(renderProgram, "i32Width"), static_cast<GLint>(this->size().width()));
    glUniform1i(glGetUniformLocation(renderProgram, "i32Height"), static_cast<GLint>(this->size().height()));

    glDrawElements(GL_TRIANGLES, i32TriangleCount * 3, GL_UNSIGNED_INT, 0);
}