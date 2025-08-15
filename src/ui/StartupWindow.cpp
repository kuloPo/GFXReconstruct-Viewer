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
    : QWidget(parent), ui(new Ui::StartupWindow), m_eCurrentPage(Page::Startup), m_ListModel(this)
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
    ui->SelectListView->raise();
    ui->InputLineEdit->raise();

    ui->NextButton->hide();
    ui->BackButton->hide();
    ui->SelectListView->hide();
    ui->InputLineEdit->hide();

    connect(ui->CloseButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->RecordButton, &QPushButton::clicked, this, &StartupWindow::OnRecordButtonClicked);
    connect(ui->NextButton, &QPushButton::clicked, this, &StartupWindow::OnNextButtonClicked);
    connect(ui->BackButton, &QPushButton::clicked, this, &StartupWindow::OnBackButtonClicked);
    connect(ui->SelectListView, &QListView::doubleClicked, this, &StartupWindow::OnNextButtonClicked);

    ui->SelectListView->setModel(&m_ListModel);

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
            ui->RecordButton->show();
            ui->ReplayButton->show();
            ui->OpenButton->show();
            ui->NextButton->hide();
            ui->BackButton->hide();
            ui->SelectListView->hide();
            ui->InputLineEdit->hide();
            break;
        }
        case StartupWindow::Page::Record:
        {
            ui->RecordButton->hide();
            ui->ReplayButton->hide();
            ui->OpenButton->hide();
            ui->NextButton->show();
            ui->BackButton->show();
            ui->SelectListView->show();
            ui->InputLineEdit->show();

            ui->InputLineEdit->setPlaceholderText("Connect to new device");

            QStringList rows;
            m_ListModel.setStringList(rows);

            std::vector<std::string> devices = adb.GetDevices();
            LOGD("ADB device num %d", devices.size());
            for (std::string device : devices)
                LOGD("%s", device.c_str());

            for (std::string device : devices) {
                rows << QString(QString::fromStdString(device));
            }
            m_ListModel.setStringList(rows);

            break;
        }
        case StartupWindow::Page::Activity:
        {
            ui->InputLineEdit->show();

            QStringList rows;
            m_ListModel.setStringList(rows);

            std::vector<std::string> packages = adb.GetPackages();
            LOGD("package num %d", packages.size());
            for (std::string package : packages) {
                LOGD("%s", package.c_str());
                rows << QString(QString::fromStdString(package));
            }
            m_ListModel.setStringList(rows);

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

void StartupWindow::OnNextButtonClicked() {
    LOGD("Next button clicked");
    switch (m_eCurrentPage) {
        case StartupWindow::Page::Record:
        {
            QModelIndex idx = ui->SelectListView->currentIndex();
            std::string serial;

            if (!ui->InputLineEdit->text().isEmpty()) {
                serial = ui->InputLineEdit->text().toStdString();
                LOGD("Input new device %s", serial.c_str());
            }
            else if (idx.isValid()) {
                serial = idx.data(Qt::DisplayRole).toString().toStdString();
                LOGD("Selected device is %s", serial.c_str());
            }
            else {
                LOGD("No device is selected");
                break;
            }

            if (adb.ConnectDevice(serial)) {
                LOGD("Connected with %s", serial.c_str());
                FlipPage(Page::Activity);
            }
            else {
                LOGW("Failed to connect with %s", serial.c_str());
            }
            break;
        }
        case StartupWindow::Page::Activity:
        {
            QModelIndex idx = ui->SelectListView->currentIndex();
            if (idx.isValid()) {
                std::string package = idx.data(Qt::DisplayRole).toString().toStdString();
                LOGD("Selected package is %s", package.c_str());
                std::string abi = adb.GetAppAbi(package);
                if (abi.size()) {
                    LOGD("ABI of %s is %s", package.c_str(), abi.c_str());
                }
                else {
                    LOGW("Failed to get ABI of %s", package.c_str());
                }
            }
            else {
                LOGD("No package is selected");
            }
            break;
        }
    }
}

void StartupWindow::OnBackButtonClicked() {
    LOGD("Back button clicked");
    switch (m_eCurrentPage) {
        case StartupWindow::Page::Record:
        {
            FlipPage(Page::Startup);
            break;
        }
        case StartupWindow::Page::Activity:
        {
            FlipPage(Page::Record);
            break;
        }
    }
}