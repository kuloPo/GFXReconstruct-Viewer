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

#include "ApiTableModel.hpp"

#include <algorithm>
#include <limits>

#include "common.hpp"

#define SIMDJSON_CHECK_ERROR(expr, fmt, ...) \
    do { \
        if (const auto e_ = (expr); e_) { \
            LOGE(fmt ": %s", ##__VA_ARGS__, simdjson::error_message(e_)); \
        } \
    } while (false)

ApiTableModel::ApiTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

bool ApiTableModel::OnLoadFile(const QString& filePath) {
    std::string path = filePath.toStdString();

    auto error = simdjson::padded_string::load(path).get(m_Json);
    if (error) {
        LOGW("Failed to load \"%s\": %s.\n\n"
             "The file may have been moved, deleted, or is locked by another process.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    simdjson::ondemand::parser scanner;
    simdjson::ondemand::document doc;
    error = scanner.iterate(m_Json).get(doc);
    if (error) {
        LOGW("Failed to parse \"%s\": %s.\n\n"
             "The file may be corrupted or is not a valid GFXReconstruct capture.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    simdjson::ondemand::array arr;
    error = doc.get_array().get(arr);
    if (error) {
        LOGW("Unexpected JSON structure in \"%s\": %s.\n\n"
             "The file does not contain a valid GFXReconstruct capture array.",
             path.c_str(), simdjson::error_message(error));
        return false;
    }

    size_t idx = 0;
    m_EntryRanges.clear();
    m_ApiEntries.clear();
    m_ApiEntries.push_back({});

    beginResetModel();

    for (auto elemResult : arr) {
        simdjson::ondemand::value elem;
        SIMDJSON_CHECK_ERROR(elemResult.get(elem), "element %zu", idx);

        std::string_view raw;
        SIMDJSON_CHECK_ERROR(elem.raw_json().get(raw), "element %zu", idx);

        const size_t start = static_cast<size_t>(raw.data() - m_Json.data());
        const size_t len = raw.size();
        m_EntryRanges.push_back({start, len});

        simdjson::padded_string_view view(
            m_Json.data() + start, len,
            m_Json.size() - start + simdjson::SIMDJSON_PADDING);

        simdjson::dom::element entry;
        SIMDJSON_CHECK_ERROR(m_EntryParser.parse(view).get(entry), "element %zu", idx);

        simdjson::dom::object obj;
        SIMDJSON_CHECK_ERROR(entry.get_object().get(obj), "element %zu", idx);

        try {
            std::string_view name;
            if (!obj["function"]["name"].get(name)) {
                m_ApiEntries.back().push_back(idx);
                if (name == "vkQueuePresentKHR") {
                    m_ApiEntries.push_back({});
                }
            }
        } catch (const simdjson::simdjson_error&) {
        }
        idx++;
    }
    // A trailing vkQueuePresentKHR would have opened an empty frame; drop it.
    if (m_ApiEntries.size() > 1 && m_ApiEntries.back().empty()) {
        m_ApiEntries.pop_back();
    }

    m_Frame = 0;
    RefreshFilteredRow();
    endResetModel();
    return true;
}

void ApiTableModel::SetFrame(int frame) {
    if (frame == m_Frame) return;
    beginResetModel();
    m_Frame = frame;
    RefreshFilteredRow();
    endResetModel();
}

void ApiTableModel::SetFilter(const QString& filter) {
    if (filter == m_FilterString) return;
    m_FilterString = filter;
    beginResetModel();
    RefreshFilteredRow();
    endResetModel();
}

size_t ApiTableModel::RawEntryIndex(int row) const {
    return m_FilteredRowIndexes[row];
}

int ApiTableModel::FindRowByEntry(size_t entryIndex) const {
    auto it = std::lower_bound(m_FilteredRowIndexes.begin(), m_FilteredRowIndexes.end(), entryIndex);
    if (it != m_FilteredRowIndexes.end() && *it == entryIndex) {
        return static_cast<int>(it - m_FilteredRowIndexes.begin());
    }
    return -1;
}

simdjson::dom::object ApiTableModel::GetEntryObject(size_t entryIndex) const {
    simdjson::dom::element entry;
    simdjson::dom::object obj;
    const Range& range = m_EntryRanges[entryIndex];
    simdjson::padded_string_view view(
        m_Json.data() + range.start,
        range.len,
        m_Json.size() - range.start + simdjson::SIMDJSON_PADDING);
    SIMDJSON_CHECK_ERROR(m_EntryParser.parse(view).get(entry), "entry %zu", entryIndex);
    SIMDJSON_CHECK_ERROR(entry.get_object().get(obj), "entry %zu", entryIndex);
    return obj;
}

size_t ApiTableModel::GetFrameCount() const {
    return m_ApiEntries.size();
}

size_t ApiTableModel::GetFrameTotalAPICount() const {
    return m_ApiEntries.empty() ? 0 : m_ApiEntries[m_Frame].size();
}

void ApiTableModel::RefreshFilteredRow() {
    m_FilteredRowIndexes.clear();
    const auto& frame = m_ApiEntries[m_Frame];
    for (const size_t entryIndex : frame) {
        if (m_FilterString.isEmpty() || EntryNameMatches(entryIndex, m_FilterString)) {
            m_FilteredRowIndexes.push_back(entryIndex);
        }
    }
}

bool ApiTableModel::EntryNameMatches(size_t entryIndex, const QString& needle) const {
    const simdjson::dom::object obj = GetEntryObject(entryIndex);
    std::string_view name;
    SIMDJSON_CHECK_ERROR(obj["function"]["name"].get(name), "entry %zu has no function name", entryIndex);
    return QAnyStringView(name).toString().contains(needle, Qt::CaseInsensitive);
}

int ApiTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : m_FilteredRowIndexes.size();
}

int ApiTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : 3;
}

QVariant ApiTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal) return {};
    switch (section) {
        case 0: return QStringLiteral("Index");
        case 1: return QStringLiteral("Return");
        case 2: return QStringLiteral("Name");
        default: {
            LOGE("ApiTableModel::headerData: unexpected section %d", section);
            return {};
        }
    }
}

QVariant ApiTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || role != Qt::DisplayRole) return {};

    simdjson::dom::object obj = GetEntryObject(m_FilteredRowIndexes[index.row()]);

    try {
        switch (index.column()) {
            case 0: {
                uint64_t value = 0;
                SIMDJSON_CHECK_ERROR(obj["index"].get_uint64().get(value), "row %d, column 0", index.row());
                return QString::number(value);
            }
            case 1: {
                simdjson::dom::element returnElem;
                if (obj["function"]["return"].get(returnElem) == simdjson::SUCCESS) {
                    std::string_view text;
                    SIMDJSON_CHECK_ERROR(returnElem.get_string().get(text), "row %d, column 1", index.row());
                    return QAnyStringView(text).toString();
                }
                else {
                    return {};
                }
            }
            case 2: {
                std::string_view name;
                SIMDJSON_CHECK_ERROR(obj["function"]["name"].get(name), "row %d, column 2", index.row());
                return QAnyStringView(name).toString();
            }
            default: {
                LOGE("ApiTableModel::data: unexpected column %d", index.column());
                return {};
            }
        }
    } catch (const simdjson::simdjson_error&) {
        return {};
    }
}
