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

#include "ApiArgsTable.hpp"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QStringList>
#include <QTreeWidgetItem>

#include "JsonWrapper.hpp"

ApiArgsTable::ApiArgsTable(QWidget* parent)
    : QTreeWidget(parent)
{
}

void ApiArgsTable::SetArgs(const simdjson::dom::element& args) {
    clear();

    simdjson::dom::object obj;
    if (args.get_object().get(obj)) {
        return;
    }
    for (auto field : obj) {
        AddValueItem(invisibleRootItem(), QAnyStringView(field.key).toString(), field.value);
    }
    expandAll();
}

void ApiArgsTable::Clear() {
    clear();
}

void ApiArgsTable::AddValueItem(QTreeWidgetItem* parent, const QString& name, const simdjson::dom::element& value) {
    auto* item = new QTreeWidgetItem(parent);
    item->setText(0, name);

    if (value.is_object()) {
        simdjson::dom::object obj;
        if (value.get_object().get(obj)) {
            item->setText(1, QStringLiteral("{}"));
            return;
        }
        item->setText(1, QStringLiteral("{...}"));
        for (auto field : obj) {
            AddValueItem(item, QAnyStringView(field.key).toString(), field.value);
        }
    }
    else if (value.is_array()) {
        simdjson::dom::array arr;
        if (value.get_array().get(arr)) {
            item->setText(1, QStringLiteral("[]"));
            return;
        }
        item->setText(1, QStringLiteral("[...]"));
        size_t index = 0;
        for (auto element : arr) {
            AddValueItem(item, QStringLiteral("[%1]").arg(static_cast<qulonglong>(index)), element);
            ++index;
        }
    }
    else {
        item->setText(1, JsonScalarToString(value));
    }
}

void ApiArgsTable::contextMenuEvent(QContextMenuEvent* event) {
    QTreeWidgetItem* item = itemAt(event->pos());
    // Triggered when right-clicking the empty area below the last row: fall back to the selected row.
    if (item == nullptr) {
        item = currentItem();
    }
    // Triggered when nothing is under the cursor and no row is selected, e.g. an empty tree: skip the menu.
    if (item == nullptr) {
        QTreeWidget::contextMenuEvent(event);
        return;
    }

    const QModelIndex cellIndex = indexAt(event->pos());
    const int col = cellIndex.isValid() ? cellIndex.column() : currentColumn();

    QMenu menu(this);
    const QAction* copyAction = menu.addAction(QStringLiteral("Copy"));
    const QAction* copyRowAction = menu.addAction(QStringLiteral("Copy Row"));

    const QAction* selected = menu.exec(event->globalPos());
    if (selected == copyAction) {
        QApplication::clipboard()->setText(item->text(col));
    }
    else if (selected == copyRowAction) {
        QStringList rowText;
        for (int c = 0; c < columnCount(); ++c) {
            rowText.append(item->text(c));
        }
        QApplication::clipboard()->setText(rowText.join(' '));
    }
}
