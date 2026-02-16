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

#pragma once

#include <QWidget>
#include <QStringListModel>
#include <QMouseEvent>

#include "ui_StartupWindow.h"
#include "StartupWindowBackground.hpp"
#include "adb.hpp"

class StartupWindow : public QWidget {
    Q_OBJECT

public:
    StartupWindow(QWidget* parent = nullptr);
    ~StartupWindow();

private:
    enum class Page {
        Startup,
        Record,
        Activity,
        Option,
        Replay,
        FileSelect,
    };

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void FlipPage(Page page);
    void OnRecordButtonClicked();
    void OnReplayButtonClicked();
    void OnNextButtonClicked();
    void OnBackButtonClicked();
    void OnFileSelectButtonClicked();
    void OnOpenButtonClicked();
    QString PopFileOpenWindow();

private:
    Ui::StartupWindow* ui;
    QPoint m_DragPos;
    Page m_eCurrentPage;
    ADB adb;
    QStringListModel m_ListModel;
    std::string m_strSelectedPackage;
    std::string m_strSelectedActivity;
};