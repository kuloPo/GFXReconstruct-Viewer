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

#include "JsonWrapper.hpp"

QString JsonScalarToString(const simdjson::dom::element& value) {
    if (value.is_string()) {
        std::string_view text;
        if (value.get_string().get(text) == simdjson::SUCCESS) {
            return QAnyStringView(text).toString();
        }
    }
    else if (value.is_bool()) {
        bool b = false;
        if (value.get_bool().get(b) == simdjson::SUCCESS) {
            return b ? QStringLiteral("true") : QStringLiteral("false");
        }
    }
    else if (value.is_int64()) {
        int64_t n = 0;
        if (value.get_int64().get(n) == simdjson::SUCCESS) {
            return QString::number(static_cast<qlonglong>(n));
        }
    }
    else if (value.is_uint64()) {
        uint64_t n = 0;
        if (value.get_uint64().get(n) == simdjson::SUCCESS) {
            return QString::number(static_cast<qulonglong>(n));
        }
    }
    else if (value.is_double()) {
        double d = 0.0;
        if (value.get_double().get(d) == simdjson::SUCCESS) {
            return QString::number(d, 'g', 17);
        }
    }
    else if (value.is_null()) {
        return QStringLiteral("NULL");
    }
    return QStringLiteral("?");
}

QString JsonElementToString(const simdjson::dom::element& value) {
    if (value.is_object()) {
        return value.get_object()->size() == 0 ? QStringLiteral("{}") : QStringLiteral("{...}");
    }
    if (value.is_array()) {
        return value.get_array()->size() == 0 ? QStringLiteral("[]") : QStringLiteral("[...]");
    }
    return JsonScalarToString(value);
}

simdjson::dom::element JsonGetElement(simdjson::dom::parser& parser, const simdjson::padded_string& json, size_t start, size_t len) {
    simdjson::padded_string_view view(json.data() + start, len, json.size() - start + simdjson::SIMDJSON_PADDING);
    simdjson::dom::element entry;
    (void)parser.parse(view).get(entry);
    return entry;
}

simdjson::dom::object JsonGetObject(const simdjson::dom::element& element) {
    simdjson::dom::object obj;
    (void)element.get_object().get(obj);
    return obj;
}

std::string_view JsonGetApiName(const simdjson::dom::object& entry) {
    std::string_view name;
    (void)entry["function"]["name"].get(name);
    return name;
}

simdjson::dom::element JsonGetApiArgs(const simdjson::dom::object& entry) {
    simdjson::dom::element args;
    (void)entry["function"]["args"].get(args);
    return args;
}

QString JsonGetApiReturn(const simdjson::dom::object& entry) {
    simdjson::dom::element ret;
    if (entry["function"]["return"].get(ret) != simdjson::SUCCESS) return {};
    std::string_view text;
    (void)ret.get_string().get(text);
    return QAnyStringView(text).toString();
}

uint64_t JsonGetApiIndex(const simdjson::dom::object& entry) {
    uint64_t value = 0;
    (void)entry["index"].get_uint64().get(value);
    return value;
}
