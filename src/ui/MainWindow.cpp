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

#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QTimer>
#include <QWheelEvent>
#include <QResizeEvent>

#include "common.hpp"

MainWindow::MainWindow(const QString& filePath, QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    showMaximized();

    m_ApiModel = new ApiTableModel(this);
    ui->apiTableView->setModel(m_ApiModel);

    ui->apiTableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->apiTableView->horizontalHeader()->resizeSection(0, 100);
    ui->apiTableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    ui->apiTableView->horizontalHeader()->resizeSection(1, 180);
    ui->apiTableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->argsTableView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->argsTableView->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_ArgsTable = ui->argsTableView;
    auto* tableSplitter = ui->tableSplitter;
    tableSplitter->setStretchFactor(0, 3);
    tableSplitter->setStretchFactor(1, 2);
    tableSplitter->setSizes({ 600, 250 });
    ui->verticalLayout->setStretch(ui->verticalLayout->indexOf(ui->tableSplitter), 1);

    ui->apiFilterEdit->setFixedWidth(250);

    connect(ui->apiTableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
        this, [this](const QModelIndex& current, const QModelIndex&) {
            UpdateArgsTable(current.row());
        });

    connect(ui->frameSlider, &QSlider::valueChanged, this, &MainWindow::OnFrameChanged);
    ui->frameSlider->installEventFilter(this);
    connect(ui->prevFrameButton, &QPushButton::clicked, this, [this]() {
        ui->frameSlider->setValue(ui->frameSlider->value() - 1);
    });
    connect(ui->nextFrameButton, &QPushButton::clicked, this, [this]() {
        ui->frameSlider->setValue(ui->frameSlider->value() + 1);
    });

    connect(ui->apiFilterEdit, &QLineEdit::textChanged, this, &MainWindow::OnTextChanged);
    UpdateListingLabel();

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
    if (!m_ApiModel->OnLoadFile(filePath)) return;

    LOGD("Total frames: %zu", m_ApiModel->GetFrameCount());

    ui->frameSlider->setRange(0, static_cast<int>(m_ApiModel->GetFrameCount() - 1));
    OnFrameChanged(ui->frameSlider->value());
}

void MainWindow::OnFrameChanged(int frame) {
    LOGD("Jump to frame %d", frame);
    m_CurrentFrame = frame;
    ui->frameLabel->setText(QString("%1/%2").arg(frame).arg(m_ApiModel->GetFrameCount() - 1));
    m_ApiModel->SetFrame(frame);

    if (m_ApiModel->rowCount() > 0) {
        ui->apiTableView->selectRow(0);
    }
    else {
        m_ArgsTable->Clear();
    }
    UpdateListingLabel();
}

void MainWindow::OnTextChanged(const QString& text) {
    QItemSelectionModel* selection = ui->apiTableView->selectionModel();
    int oldRow = -1;
    if (selection->currentIndex().isValid()) {
        oldRow = selection->currentIndex().row();
    }
    else if (!selection->selectedRows().isEmpty()) {
        oldRow = selection->selectedRows().first().row();
    }
    const size_t selectedEntry = oldRow >= 0 ? m_ApiModel->RawEntryIndex(oldRow) : -1;

    m_ApiModel->SetFilter(text);

    int targetRow = oldRow >= 0 ? m_ApiModel->FindRowByEntry(selectedEntry) : -1;
    if (targetRow < 0 && m_ApiModel->rowCount() > 0) {
        targetRow = 0;
    }
    if (targetRow >= 0) {
        ui->apiTableView->selectRow(targetRow);
    }
    else {
        m_ArgsTable->Clear();
    }
    UpdateListingLabel();
}

void MainWindow::UpdateListingLabel() {
    const int total = m_ApiModel->GetFrameTotalAPICount();
    if (ui->apiFilterEdit->text().isEmpty()) {
        ui->apiListingLabel->setText(QStringLiteral("Listing %1 API Calls").arg(total));
    }
    else {
        ui->apiListingLabel->setText(QStringLiteral("Listing %1/%2 API Calls").arg(m_ApiModel->rowCount()).arg(total));
    }
}

void MainWindow::UpdateArgsTable(int row) {
    LOGD("%s: row %d", __func__, row);

    const size_t entryIndex = m_ApiModel->RawEntryIndex(row);
    simdjson::dom::object obj = m_ApiModel->GetEntryObject(entryIndex);

    try {
        simdjson::dom::element argsElem;
        if (obj["function"]["args"].get(argsElem) != simdjson::SUCCESS) {
            m_ArgsTable->Clear();
            return;
        }

        m_ArgsTable->SetArgs(argsElem);
    } catch (const simdjson::simdjson_error&) {
        m_ArgsTable->Clear();
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
