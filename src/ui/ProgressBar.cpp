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

#include "ProgressBar.hpp"
#include <QProgressDialog>
#include <QProgressBar>
#include <QApplication>

class ProgressBar::ProgressDialog : public QProgressDialog {
public:
    ProgressDialog(QWidget* parent) : QProgressDialog(parent)
    {
        setWindowModality(Qt::ApplicationModal);
        setCancelButton(nullptr);
        setRange(0, 100);
        setMinimumDuration(0);
        setWindowFlag(Qt::FramelessWindowHint, true);

        auto* bar = new QProgressBar(this);
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        setBar(bar);
    }
};

ProgressBar::ProgressBar(QString text) {
    bar = new ProgressDialog(QApplication::activeWindow());
    bar->setLabelText(text);
    bar->setValue(0);
    bar->show();
    QCoreApplication::processEvents();
}

ProgressBar::~ProgressBar() {
    close();
}

void ProgressBar::update(int percent) {
    bar->setValue(percent);
    QCoreApplication::processEvents();
}

void ProgressBar::close() {
    bar->hide();
    bar->deleteLater();
    QCoreApplication::processEvents();
}