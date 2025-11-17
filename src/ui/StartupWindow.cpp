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

#include <QFileDialog>
#include <QStandardPaths>

#include <filesystem>
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
    ui->FileSelectButton->raise();
    ui->SelectListView->raise();
    ui->InputLineEdit->raise();
    ui->RemoveUnsupportedBox->raise();

    ui->NextButton->hide();
    ui->BackButton->hide();
    ui->FileSelectButton->hide();
    ui->SelectListView->hide();
    ui->InputLineEdit->hide();
    ui->RemoveUnsupportedBox->hide();

    connect(ui->CloseButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui->RecordButton, &QPushButton::clicked, this, &StartupWindow::OnRecordButtonClicked);
    connect(ui->ReplayButton, &QPushButton::clicked, this, &StartupWindow::OnReplayButtonClicked);
    connect(ui->OpenButton, &QPushButton::clicked, this, &StartupWindow::OnOpenButtonClicked);
    connect(ui->NextButton, &QPushButton::clicked, this, &StartupWindow::OnNextButtonClicked);
    connect(ui->BackButton, &QPushButton::clicked, this, &StartupWindow::OnBackButtonClicked);
    connect(ui->FileSelectButton, &QPushButton::clicked, this, &StartupWindow::OnFileSelectButtonClicked);
    connect(ui->SelectListView, &QListView::doubleClicked, this, &StartupWindow::OnNextButtonClicked);

    ui->SelectListView->setModel(&m_ListModel);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
}

StartupWindow::~StartupWindow() {
    delete ui->background;
    delete ui;
}

void StartupWindow::FlipPage(Page page) {
    ui->RecordButton->hide();
    ui->ReplayButton->hide();
    ui->OpenButton->hide();
    ui->NextButton->hide();
    ui->BackButton->hide();
    ui->FileSelectButton->hide();
    ui->SelectListView->hide();
    ui->InputLineEdit->hide();
    ui->RemoveUnsupportedBox->hide();

    ui->NextButton->setText("Next");
    ui->InputLineEdit->setText("");
    ui->InputLineEdit->setPlaceholderText("");
    ui->RemoveUnsupportedBox->setChecked(false);

    switch (page)
    {
        case StartupWindow::Page::Startup:
        {
            ui->RecordButton->show();
            ui->ReplayButton->show();
            ui->OpenButton->show();
            break;
        }
        case StartupWindow::Page::Record:
        case StartupWindow::Page::Replay:
        {
            ui->InputLineEdit->setPlaceholderText("Connect to new device");

            ui->NextButton->show();
            ui->BackButton->show();
            ui->SelectListView->show();
            ui->InputLineEdit->show();

            QStringList rows;
            m_ListModel.setStringList(rows);

            std::vector<std::string> devices = adb.GetDevices();
            LOGD("ADB device num %d", devices.size());
            for (std::string device : devices)
                LOGD("    %s", device.c_str());

            for (std::string device : devices) {
                rows << QString(QString::fromStdString(device));
            }
            m_ListModel.setStringList(rows);

            break;
        }
        case StartupWindow::Page::Activity:
        {
            ui->InputLineEdit->setPlaceholderText("DEFAULT ACTIVITY");

            ui->NextButton->show();
            ui->BackButton->show();
            ui->SelectListView->show();
            ui->InputLineEdit->show();

            QStringList rows;
            m_ListModel.setStringList(rows);

            std::vector<std::string> packages = adb.GetPackages();
            LOGD("package num %d", packages.size());
            for (std::string package : packages) {
                LOGD("    %s", package.c_str());
                rows << QString(QString::fromStdString(package));
            }
            m_ListModel.setStringList(rows);

            break;
        }
        case StartupWindow::Page::Option:
        {
            ui->InputLineEdit->setPlaceholderText("Input startup args");

            ui->NextButton->show();
            ui->BackButton->show();
            ui->InputLineEdit->show();

            break;
        }
        case StartupWindow::Page::FileSelect:
        {
            ui->NextButton->setText("Replay");
            ui->InputLineEdit->setPlaceholderText("Select replay file");

            ui->NextButton->show();
            ui->BackButton->show();
            ui->FileSelectButton->show();
            ui->InputLineEdit->show();
            ui->RemoveUnsupportedBox->show();

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

void StartupWindow::OnReplayButtonClicked() {
    LOGD("Replay button clicked");
    FlipPage(Page::Replay);
}

void StartupWindow::OnFileSelectButtonClicked() {
    ui->InputLineEdit->setText(PopFileOpenWindow());
}

void StartupWindow::OnNextButtonClicked() {
    LOGD("Next button clicked");
    switch (m_eCurrentPage) {
        case StartupWindow::Page::Record:
        case StartupWindow::Page::Replay:
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
                FlipPage(ENUM_NEXT(m_eCurrentPage));
            }
            else {
                LOGW("Failed to connected with %s", serial.c_str());
            }

            break;
        }
        case StartupWindow::Page::Activity:
        {
            QModelIndex idx = ui->SelectListView->currentIndex();
            if (!idx.isValid()) {
                LOGD("No package is selected");
                break;
            }

            std::string package = idx.data(Qt::DisplayRole).toString().toStdString();

            m_strSelectedPackage = package;
            m_strSelectedActivity = ui->InputLineEdit->text().toStdString();
            if (m_strSelectedActivity.empty()) {
                std::string cmd = std::format("cmd package resolve-activity "
                    "-a android.intent.action.MAIN "
                    "-c android.intent.category.LAUNCHER "
                    "--brief {} | tail -n 1 | cut -d '/' -f2", m_strSelectedPackage);
                m_strSelectedActivity = adb.ShellCommand(cmd);
            }

            LOGD("Selected package is %s", m_strSelectedPackage.c_str());
            LOGD("Selected activity is %s", m_strSelectedActivity.c_str());

            FlipPage(Page::Option);

            break;
        }
        case StartupWindow::Page::Option:
        {
            if (!adb.PushRecordLayer(m_strSelectedPackage))
                break;

            adb.SetRecordProp(m_strSelectedPackage);
            std::string args = ui->InputLineEdit->text().toStdString();
            adb.ShellCommand(std::format("am force-stop {}", m_strSelectedPackage));
            adb.ShellCommand(std::format("am start -n {}/{} {}", m_strSelectedPackage, m_strSelectedActivity, args));

            break;
        }
        case StartupWindow::Page::FileSelect:
        {
            QString localReplayFilePath = ui->InputLineEdit->text();
            QFileInfo localReplayFilePathInfo(localReplayFilePath);
            QString replayFileName = localReplayFilePathInfo.fileName();
            QString remoteReplayFilePath = "/data/user/0/com.lunarg.gfxreconstruct.replay/files/" + replayFileName;

            if (localReplayFilePath.isEmpty()) {
                LOGW("No replay file is selected");
                break;
            }

            if (!localReplayFilePathInfo.isFile()) {
                LOGW("Replay file does not exist");
                break;
            }

            if (!adb.InstallReplayApk()) {
                break;
            }

            if (!localReplayFilePathInfo.isFile()) {
                LOGW("Replay file %s not exists", localReplayFilePath.toStdString().c_str());
                break;
            }

            if (!adb.AlreadyUploaded(localReplayFilePathInfo, remoteReplayFilePath)) {
                if (!adb.PushFile(localReplayFilePathInfo, remoteReplayFilePath)) {
                    LOGW("Failed to push replay file");
                    break;
                }
            }
            else {
                LOGD("Replay file %s exists", replayFileName.toStdString().c_str());
            }

            adb.ShellCommand("am force-stop com.lunarg.gfxreconstruct.replay");

            QString args = "";
            if (ui->RemoveUnsupportedBox->isChecked())
                args += "--remove-unsupported ";

            QString cmd = QString(
                "am start -n \"com.lunarg.gfxreconstruct.replay/android.app.NativeActivity\""
                " -a android.intent.action.MAIN -c android.intent.category.LAUNCHER"
                " --es \"args\" \"%1%2\"").arg(args, remoteReplayFilePath);
            adb.ShellCommand(cmd);

            FlipPage(Page::Startup);

            break;
        }
        default:
        {
            LOGE("Unknown page %d when clicking next button", m_eCurrentPage);
        }
    }
}

void StartupWindow::OnBackButtonClicked() {
    LOGD("Back button clicked");
    switch (m_eCurrentPage) {
        case StartupWindow::Page::Record:
        case StartupWindow::Page::Replay:
        {
            FlipPage(Page::Startup);
            break;
        }
        case StartupWindow::Page::Activity:
        case StartupWindow::Page::Option:
        case StartupWindow::Page::FileSelect:
        {
            FlipPage(ENUM_PREV(m_eCurrentPage));
            break;
        }
        default:
        {
            LOGE("Unknown page %d when clicking back button", m_eCurrentPage);
        }
    }
}

void StartupWindow::OnOpenButtonClicked() {
    LOGD("Open button clicked");
    QString filepath = PopFileOpenWindow();
}

QString StartupWindow::PopFileOpenWindow() {
    static QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filepath = QFileDialog::getOpenFileName(this, "Open capture", defaultPath);
    defaultPath = filepath;
    return filepath;
}