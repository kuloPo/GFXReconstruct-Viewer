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

#include "MainWindow.hpp"

#include "simdjson.h"
#include "common.hpp"

MainWindow::MainWindow(const QString& filePath, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    showMaximized();

    std::string path = filePath.toStdString();
    LOGD("MainWindow created, file: %s", path.c_str());

    simdjson::ondemand::parser parser;
    simdjson::padded_string json;
    auto error = simdjson::padded_string::load(path).get(json);
    if (error) {
        LOGW("Failed to load \"%s\": %s.\n\n"
             "The file may have been moved, deleted, or is locked by another process.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    simdjson::ondemand::document doc;
    error = parser.iterate(json).get(doc);
    if (error) {
        LOGW("Failed to parse \"%s\": %s.\n\n"
             "The file may be corrupted or is not a valid GFXReconstruct capture.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    uint64_t frameCount = 1;

    simdjson::ondemand::array arr;
    error = doc.get_array().get(arr);
    if (error) {
        LOGW("Unexpected JSON structure in \"%s\": %s.\n\n"
             "The file does not contain a valid GFXReconstruct capture array.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    for (auto elem : arr) {
        simdjson::ondemand::object obj;
        error = elem.get_object().get(obj);
        if (error) continue;

        if (obj.find_field("function").error() == simdjson::SUCCESS) {
            std::string_view name;
            if (obj["function"]["name"].get(name) == simdjson::SUCCESS && name == "vkQueuePresentKHR") {
                frameCount++;
            }
        }
    }

    LOGD("Total frames: %llu", static_cast<unsigned long long>(frameCount));
}

MainWindow::~MainWindow() {
    delete ui;
    LOGD("MainWindow destroyed");
}
