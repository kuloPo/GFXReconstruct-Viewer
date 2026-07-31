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

#include <QTimer>
#include <QWheelEvent>
#include <QResizeEvent>

#include "common.hpp"

MainWindow::MainWindow(const QString& filePath, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    showMaximized();
    connect(ui->frameSlider, &QSlider::valueChanged, this, &MainWindow::OnFrameChanged);
    ui->frameSlider->installEventFilter(this);
    connect(ui->prevFrameButton, &QPushButton::clicked, this, [this]() {
        ui->frameSlider->setValue(ui->frameSlider->value() - 1);
    });
    connect(ui->nextFrameButton, &QPushButton::clicked, this, [this]() {
        ui->frameSlider->setValue(ui->frameSlider->value() + 1);
    });

    LOGD("MainWindow created, file: %s", filePath.toStdString().c_str());

    QTimer::singleShot(0, this, [this, filePath]() {
        LoadFile(filePath);
    });
}

MainWindow::~MainWindow() {
    delete ui;
    LOGD("MainWindow destroyed");
}

void MainWindow::LoadFile(const QString& filePath) {
    std::string path = filePath.toStdString();

    auto error = simdjson::padded_string::load(path).get(m_Json);
    if (error) {
        LOGW("Failed to load \"%s\": %s.\n\n"
             "The file may have been moved, deleted, or is locked by another process.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    error = m_Parser.parse(m_Json).get(m_Doc);
    if (error) {
        LOGW("Failed to parse \"%s\": %s.\n\n"
             "The file may be corrupted or is not a valid GFXReconstruct capture.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    simdjson::dom::array arr;
    error = m_Doc.get_array().get(arr);
    if (error) {
        LOGW("Unexpected JSON structure in \"%s\": %s.\n\n"
             "The file does not contain a valid GFXReconstruct capture array.",
             path.c_str(), simdjson::error_message(error));
        return;
    }

    size_t idx = 0, lastApiIdx = 0;
    m_FrameBoundaries.push_back(0);
    for (auto elem : arr) {
        simdjson::dom::object obj;
        if (elem.get_object().get(obj)) { idx++; continue; }

        try {
            std::string_view name;
            if (!obj["function"]["name"].get(name)) {
                lastApiIdx = idx;
                if (name == "vkQueuePresentKHR") {
                    m_FrameBoundaries.push_back(idx + 1);
                }
            }
        } catch (const simdjson::simdjson_error&) {
        }
        idx++;
    }
    // The trace file is not ended with vkQueuePresentKHR
    // Use lastApiIdx to prevent non-API block like EndMarker
    if (m_FrameBoundaries.back() != lastApiIdx + 1) {
        m_FrameBoundaries.push_back(lastApiIdx + 1);
    }

    LOGD("Total frames: %zu", GetFrameCount());

    m_Entries.reserve(idx);
    for (auto elem : arr) {
        m_Entries.push_back(elem);
    }

    ui->frameSlider->setRange(0, static_cast<int>(GetFrameCount() - 1));
    OnFrameChanged(ui->frameSlider->value());
}

size_t MainWindow::GetFrameCount() const {
    if (m_FrameBoundaries.empty()) return 0;
    return m_FrameBoundaries.size() - 1;
}

void MainWindow::OnFrameChanged(int frame) {
    LOGD("Jump to frame %d", frame);
    m_CurrentFrame = frame;
    ui->frameLabel->setText(
        QString("%1/%2").arg(frame).arg(GetFrameCount() - 1));
    UpdateApiList();
}

void MainWindow::UpdateApiList() {
    ui->apiListView->clear();
    size_t start = m_FrameBoundaries[m_CurrentFrame];
    size_t end = m_FrameBoundaries[m_CurrentFrame + 1];

    for (size_t i = start; i < end; i++) {
        simdjson::dom::object obj;
        if (m_Entries[i].get_object().get(obj)) continue;

        try {
            std::string_view name;
            if (!obj["function"]["name"].get(name)) {
                ui->apiListView->addItem(QString::fromUtf8(
                    name.data(), static_cast<int>(name.size())));
            }
        } catch (const simdjson::simdjson_error&) {
            ui->apiListView->addItem("(non-API entry)");
        }
    }
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui->frameSlider) {
        if (event->type() == QEvent::Resize) {
            // Keep the step buttons 1.5x the slider height.
            int h = static_cast<QResizeEvent*>(event)->size().height() * 3 / 2;
            ui->prevFrameButton->setFixedSize(h, h);
            ui->nextFrameButton->setFixedSize(h, h);
            return false;
        }
        if (event->type() == QEvent::Wheel) {
            // Always step exactly one frame per wheel notch,
            // regardless of the system mouse wheel settings.
            // Wheel up: frame - 1, wheel down: frame + 1.
            auto* wheel = static_cast<QWheelEvent*>(event);
            int delta = wheel->angleDelta().y();
            if (delta == 0)
                delta = wheel->pixelDelta().y();
            if (delta == 0)
                return true;

            ui->frameSlider->setValue(ui->frameSlider->value() + (delta > 0 ? -1 : 1));
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
