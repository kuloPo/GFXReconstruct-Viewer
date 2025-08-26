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

#pragma once

#include <QString>
#include <QFileInfo>

#include <vector>
#include <string>
#include <filesystem>

class ADB {
public:
    ADB();
    ~ADB();
    std::vector<std::string> GetDevices();
    bool ConnectDevice(std::string serial);
    QString ShellCommand(QString cmd);
    std::string ShellCommand(std::string cmd);
    std::string ShellCommand(const char* cmd);
    QString ShellCommandPrivileged(QString cmd);
    std::string ShellCommandPrivileged(std::string cmd);
    std::string ShellCommandPrivileged(const char* cmd);
    std::vector<std::string> GetPackages();
    std::string GetAppAbi(std::string package);
    std::string GetAppLibDir(std::string package);
    bool PushFile(QFileInfo src, QString dst);
    bool InstallReplayApk();
    bool PushRecordLayer(std::string package);
    bool AlreadyUploaded(QFileInfo local, QString remote);
    void SetRecordProp(std::string package);

private:
    QString runProgram(const QString& program, const QStringList& args);
    bool pushFileStreaming(std::string serial, QFileInfo src, QString dst);
    qint64 GetRemoteSize(QString remotePath);

private:
    std::string serial;
};