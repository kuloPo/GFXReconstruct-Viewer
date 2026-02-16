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

#include <iostream>
#include <filesystem>
#include <cstdio>
#include <cstdarg>
#include <QMessageBox>
#include "log.hpp"

Logger::Logger() {
}

Logger::~Logger() {
}

void Logger::log(const char* file, int line, const char* func, Logger::Level level, const char* format, ...) {
    std::time_t now = std::time(nullptr);
    std::cout << std::put_time(std::localtime(&now), "%c ");

    const char* infoStr = nullptr;
    switch (level) {
    case Debug:
        infoStr = "\x1b[39;1mDEBUG: \x1b[30;1m(%s:%d) \x1b[0m";
        break;
    case Warn:
        infoStr = "\x1b[33;1mWARN: \x1b[30;1m(%s:%d) \x1b[0m";
        break;
    case Error:
        infoStr = "\x1b[31;1mERROR: \x1b[30;1m(%s:%d) \x1b[0m";
        break;
    default:
        LOGE("Unknown log level!");
    }

    printf(infoStr, file, line, func);

    va_list argptr;
    va_start(argptr, format);
    vprintf(format, argptr);
    if (level == Warn)
        QMessageBox::warning(nullptr, "", QString::vasprintf(format, argptr));
    else if (level == Error)
        QMessageBox::critical(nullptr, "", QString::vasprintf(format, argptr));
    va_end(argptr);

    std::cout << std::endl;

    if (level == Error)
        abort();
}