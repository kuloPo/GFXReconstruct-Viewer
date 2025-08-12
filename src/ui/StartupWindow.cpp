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

#include "StartupWindow.hpp"

#include "common.hpp"

StartupWindow::StartupWindow(QWidget* parent)
    : QWidget(parent), ui(new Ui::StartupWindow), m_eCurrentPage(Page::Startup)
{
    ui->setupUi(this);
    ui->background = new Background(ui->centralwidget);
    ui->background->resize(this->size());

    ui->CloseButton->raise();

    ui->RecordButton->raise();
    ui->ReplayButton->raise();
    ui->OpenButton->raise();

    ui->NextButton->raise();
    ui->BackButton->raise();

    ui->NextButton->hide();
    ui->BackButton->hide();

    connect(ui->CloseButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->RecordButton, &QPushButton::clicked, this, &StartupWindow::OnRecordButtonClicked);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
}

StartupWindow::~StartupWindow() {
    delete ui->background;
    delete ui;
}

void StartupWindow::FlipPage(Page page) {
    switch (page)
    {
        case StartupWindow::Page::Startup:
        {
            break;
        }
        case StartupWindow::Page::Record:
        {
            ui->RecordButton->hide();
            ui->ReplayButton->hide();
            ui->OpenButton->hide();
            ui->NextButton->show();
            ui->BackButton->hide();

            std::vector<std::string> devices = adb.GetDevices();
            LOGD("ADB device num %d", devices.size());
            for (std::string device : devices)
                LOGD("%s", device.c_str());

            break;
        }
        default:
        {
            LOGE("Unknown enum page %d", page);
        }
    }
    LOGD("Flip from page %d to %d", m_eCurrentPage, page);
    m_eCurrentPage = page;
}

void StartupWindow::OnRecordButtonClicked() {
    LOGD("Record button clicked");
    FlipPage(Page::Record);
}