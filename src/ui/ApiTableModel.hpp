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

#include <QAbstractTableModel>
#include <QString>
#include <QVariant>

#include <cstdint>
#include <vector>

#include "simdjson.h"

struct Range {
    size_t start = 0;
    size_t len = 0;
};

class ApiTableModel : public QAbstractTableModel {
public:
    explicit ApiTableModel(QObject* parent = nullptr);

    bool OnLoadFile(const QString& filePath);
    void SetFrame(int frame);
    void SetFilter(const QString& filter);

    size_t RawEntryIndex(int row) const;
    int FindRowByEntry(size_t entryIndex) const;
    simdjson::dom::object GetEntryObject(size_t entryIndex) const;
    size_t GetFrameCount() const;
    size_t GetFrameTotalAPICount() const;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex& index, int role) const override;

private:
    void RefreshFilteredRow();
    bool EntryNameMatches(size_t entryIndex, const QString& needle) const;

    simdjson::padded_string m_Json;
    mutable simdjson::dom::parser m_EntryParser;
    std::vector<Range> m_EntryRanges;
    std::vector<std::vector<size_t>> m_ApiEntries;
    std::vector<size_t> m_FilteredRowIndexes;
    int m_Frame = 0;
    QString m_FilterString;
};
