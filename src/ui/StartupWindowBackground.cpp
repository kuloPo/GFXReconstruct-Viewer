/********************************************************************************
 * MIT License
 *
 * Copyright (c) 2025-2026 kuloPo
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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <random>

const GLfloat fTriangleSize = 45;
const GLfloat fTriangleHeight = (sqrtf(3.0f) * fTriangleSize) / 2.0f;

const char* strVertexShaderSource = R"(
    #version 330 core

    layout(location = 0) in vec2 aPosition;
    layout(location = 1) in vec2 aData;

    uniform int i32Width;
    uniform int i32Height;

    out vec2 vData;

    void main() {
        float x = aPosition.x;
        float y = i32Height - aPosition.y;

        gl_Position = vec4(
            (x / i32Width) * 2.0 - 1.0,
            (y / i32Height) * 2.0 - 1.0,
            0.0, 1.0
        );

        vData = aData;
    }
    )";

const char* strFragmentShaderSource = R"(
    #version 330 core

    in vec2 vData;

    out vec4 FragColor;

    vec3 hsv2rgb(vec3 c) {
        vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    }

    void main() {
        FragColor = vec4(hsv2rgb(vec3(293.0 / 360.0, 0.18, vData.x)), vData.y);
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

GLuint Background::createShaderProgram(GLuint vertexShader = 0, GLuint fragmentShader = 0) {
    GLuint program = glCreateProgram();

    if (vertexShader) glAttachShader(program, vertexShader);
    if (fragmentShader) glAttachShader(program, fragmentShader);

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
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

Background::~Background() {
    makeCurrent();
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &dataBuffer);
    glDeleteProgram(renderProgram);
    doneCurrent();
}

void Background::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(77.0f / 255, 77.0f / 255, 138.0f / 255, 1.0f);

    i32TrianglePerRow = this->size().height() / fTriangleSize + 1;
    i32TrianglePerCol = this->size().width() / fTriangleHeight + 1;
    i32TriangleCount = i32TrianglePerRow * i32TrianglePerCol * 2;
    i32VertexCount = i32TriangleCount * 3;

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, strVertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, strFragmentShaderSource);

    renderProgram = createShaderProgram(vertexShader, fragmentShader);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    std::vector<GLfloat> positions;
    std::vector<GLfloat> data;
    generateGeometry(positions, data, static_cast<int>(std::time(0)));

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(positions.size() * sizeof(GLfloat)), positions.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &dataBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data.size() * sizeof(GLfloat)), data.data(), GL_STATIC_DRAW);

    glDeleteVertexArrays(1, &vao);
    vao = 0;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, dataBuffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Background::generateGeometry(std::vector<GLfloat>& positions, std::vector<GLfloat>& data, int i32Seed) const {
    positions.resize(i32VertexCount * 2);
    data.resize(i32VertexCount * 2);

    const float center_x = static_cast<float>(i32TrianglePerRow / 2) * fTriangleSize;
    const float radius = (static_cast<float>(i32TrianglePerCol) / 2.5f) * fTriangleHeight;

    std::mt19937 rng(static_cast<uint32_t>(i32Seed));
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);

    for (uint32_t idx = 0; idx < static_cast<uint32_t>(i32TriangleCount); ++idx) {
        const int col = static_cast<int>(idx) / i32TrianglePerRow;
        const int row = static_cast<int>(idx) % i32TrianglePerRow;

        uint32_t vertex[3];
        if (col % 2 == 0) {
            vertex[0] = static_cast<uint32_t>((col / 2) * (i32TrianglePerRow + 1) + row);
            vertex[1] = vertex[0] + 1;
            vertex[2] = vertex[1] + static_cast<uint32_t>(i32TrianglePerRow + 1 - ((col / 2) % 2));
        } else {
            vertex[0] = static_cast<uint32_t>((col / 2) * (i32TrianglePerRow + 1) + row + ((col / 2) % 2));
            vertex[1] = vertex[0] + static_cast<uint32_t>(i32TrianglePerRow + 1 - ((col / 2) % 2));
            vertex[2] = vertex[1] + 1;
        }

        const float v = dist(rng);

        const float dx = static_cast<float>(row) * fTriangleSize - center_x;
        const float dy = static_cast<float>(col / 2) * fTriangleHeight;
        const float d = std::sqrtf(dx * dx + dy * dy);
        const float alpha = (1.0f - std::min(1.0f, std::max(0.0f, d / radius))) * v;

        for (uint32_t i = 0; i < 3; ++i) {
            const uint32_t slot = idx * 3 + i;

            const int vertex_col = static_cast<int>(vertex[i]) / (i32TrianglePerRow + 1);
            const int vertex_row = static_cast<int>(vertex[i]) % (i32TrianglePerRow + 1);

            float x_offset = static_cast<float>(vertex_col) * fTriangleHeight;
            float y_offset = static_cast<float>(vertex_row) * fTriangleSize;
            if (vertex_col % 2 == 1) {
                y_offset -= fTriangleSize / 2.0f;
            }

            positions[slot * 2] = x_offset;
            positions[slot * 2 + 1] = y_offset;

            data[slot * 2] = v;
            data[slot * 2 + 1] = alpha;
        }
    }
}

void Background::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    glBindVertexArray(vao);
    glUseProgram(renderProgram);

    glUniform1i(glGetUniformLocation(renderProgram, "i32Width"), static_cast<GLint>(this->size().width()));
    glUniform1i(glGetUniformLocation(renderProgram, "i32Height"), static_cast<GLint>(this->size().height()));

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(i32VertexCount));
}
