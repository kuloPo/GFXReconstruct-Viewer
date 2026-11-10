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

#include "ApiTableView.hpp"

#include <QAbstractItemModel>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QScrollBar>
#include <QStringList>

ApiTableView::ApiTableView(QWidget* parent)
    : QTableView(parent)
{
}

void ApiTableView::scrollTo(const QModelIndex& index, QAbstractItemView::ScrollHint hint) {
    // QAbstractItemView posts a delayed scrollTo() ~500ms after a mouse press to ensure the pressed cell is visible;
    // with a wide Args column that drags the horizontal scrollbar to the right.
    // Preserve the horizontal offset and only let the vertical position adjust.
    const int offset = horizontalScrollBar()->value();
    QTableView::scrollTo(index, hint);
    horizontalScrollBar()->setValue(offset);
}

void ApiTableView::contextMenuEvent(QContextMenuEvent* event) {
    QModelIndex target = indexAt(event->pos());
    // Triggered when no cell is under the cursor, e.g. right-clicking the empty area below the last row: fall back to the selected row.
    if (!target.isValid()) {
        target = selectionModel()->currentIndex();
    }
    // Triggered when no cell is under the cursor and no row is selected, e.g. an empty table: skip the menu.
    if (!target.isValid()) {
        QTableView::contextMenuEvent(event);
        return;
    }

    const QAbstractItemModel* model = this->model();

    QMenu menu(this);
    const QAction* copyAction = menu.addAction(QStringLiteral("Copy"));
    const QAction* copyRowAction = menu.addAction(QStringLiteral("Copy Row"));

    const QAction* selected = menu.exec(event->globalPos());
    if (selected == copyAction) {
        QApplication::clipboard()->setText(model->data(target, Qt::DisplayRole).toString());
    }
    else if (selected == copyRowAction) {
        QStringList rowText;
        for (int col = 0; col < model->columnCount(); ++col) {
            rowText.append(model->data(model->index(target.row(), col), Qt::DisplayRole).toString());
        }
        QApplication::clipboard()->setText(rowText.join(' '));
    }
}
