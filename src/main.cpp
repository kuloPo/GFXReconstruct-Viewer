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

#include <QApplication>
#include <QMainWindow>
#include <QWidget>

#include "ui_StartupWindow.h"
#include "StartupWindowBackground.hpp"

#include <iostream>
#include "common.hpp"

class StartupWindow : public QWidget {
    Q_OBJECT

public:
    StartupWindow(QWidget* parent = nullptr)
        : QWidget(parent), ui(new Ui::StartupWindow) {
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

        setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    }

    ~StartupWindow() {
        delete ui->background;
        delete ui;
    }

private:
    Ui::StartupWindow* ui;
};

int main(int argc, char *argv[]) {
    LOGD("Hello GFXReconstruct Viewer!");

    QApplication app(argc, argv);

    StartupWindow window;

    window.show();

    return app.exec();
}

#include "main.moc"