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

#include "adb.hpp"

#include <QProcess>
#include <QFile>
#include <QFileInfo>

#include <sstream>
#include <format>
#include <fstream>
#include <thread>
#include <chrono>
#include "common.hpp"

static std::string rstrip(const std::string & s) {
	return s.substr(0, s.find_last_not_of(" \t\n\r\f\v") + 1);
}

void ADB::runProgram(const QString& program, const QStringList& args) {
	QProcess p;
	p.setProgram(program);
	p.setArguments(args);
	p.start();
	p.waitForFinished(-1);

	exitCode = p.exitCode();
	stdOut = QString::fromUtf8(p.readAllStandardOutput());
	stdErr = QString::fromUtf8(p.readAllStandardError());
}

bool ADB::pushFileStreaming(std::string serial, std::filesystem::path src, std::string dst)
{
	QProcess p;
	QFile f(src);
	if (!f.open(QIODevice::ReadOnly))
		return false;

	p.setProgram("adb");
	p.setArguments({ "-s", serial.c_str(), "exec-in", "sh", "-c", QString("cat > %1").arg(dst.c_str()) });
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.start();

	if (!p.waitForStarted())
		return false;

	QByteArray buf;
	buf.resize(1 << 20);

	while (true) {
		qint64 off = 0;
		const qint64 n = f.read(buf.data(), buf.size());

		if (n < 0) return false;
		if (n == 0) break;

		while (off < n) {
			const qint64 w = p.write(buf.constData() + off, n - off);
			if (w <= 0) return false; else off += w;
			if (!p.waitForBytesWritten(-1)) return false;
		}
	}

	size_t lastRemoteSize = 0;
	size_t currentRemoteSize = 0;

	do {
		lastRemoteSize = currentRemoteSize;
		std::string strRemoteSize = this->ShellCommandAsGFXR(std::format("stat -c%s {}", dst));
		std::stringstream ss(strRemoteSize);
		ss >> currentRemoteSize;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while ((long long)currentRemoteSize < f.size() && currentRemoteSize != lastRemoteSize);

	p.closeWriteChannel();
	p.waitForFinished(-1);
	return (p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0);
}

ADB::ADB() {
}

ADB::~ADB() {
}

std::vector<std::string> ADB::GetDevices() {
	runProgram("adb", {"devices"});

	if (stdOut.isEmpty())
		LOGE("adb not found!");

	std::vector<std::string> devices;
	std::istringstream iss(stdOut.toStdString());
	std::string line;
	while (std::getline(iss, line, '\n')) {
		line = rstrip(line);
		const size_t pos = line.find('\t');
		std::string serial = line.substr(0, pos);
		std::string status = line.substr(pos + 1);
		if (status == "device")
			devices.push_back(serial);
	}
	return devices;
}

bool ADB::ConnectDevice(std::string serial) {
	runProgram("adb", { "connect", serial.c_str()});

	std::vector<std::string> devices = this->GetDevices();
	if (std::find(devices.begin(), devices.end(), serial) == devices.end())
		return false;

	this->serial = serial;
	return true;
}

std::string ADB::ShellCommand(std::string cmd) {
	runProgram("adb", { "-s", serial.c_str(), "shell", cmd.c_str() });
	return rstrip(stdOut.toStdString());
}

std::vector<std::string> ADB::GetPackages() {
	std::vector<std::string> packages;
	std::string raw = this->ShellCommand("pm list packages -3");
	std::istringstream iss(raw);
	std::string line;
	while (std::getline(iss, line, '\n')) {
		std::string package = line.substr(strlen("package:"));
		if (GetAppLibDir(package).starts_with("/data/app/"))
			packages.push_back(package);
	}
	return packages;
}

std::string ADB::GetAppAbi(std::string package) {
	std::string cmd = std::format("dumpsys package {} | grep primaryCpuAbi | cut -d= -f2", package);
	std::string abi = this->ShellCommand(cmd);
	return abi;
}

std::string ADB::GetAppLibDir(std::string package) {
	std::string cmd = std::format("dumpsys package {} | grep legacyNativeLibraryDir | cut -d= -f2-", package);
	std::string str = this->ShellCommand(cmd);
	return str;
}

bool ADB::PushFile(std::filesystem::path src, std::string dst) {
	dst = dst.ends_with('/') ? dst + src.filename().string() : dst;
	return pushFileStreaming(serial.c_str(), src.string().c_str(), dst.c_str());
}

bool ADB::InstallReplayApk(std::filesystem::path localReplayApkPath) {
	std::string cmd = "pm list packages -3 | grep com.lunarg.gfxreconstruct.replay";
	if (this->ShellCommand(cmd).length()) {
		LOGD("Replay APK already installed");
		return true;
	}

	LOGD("Installing replay APK");
	if (!std::filesystem::exists(localReplayApkPath)) {
		LOGW("Failed to find replay APK at %s", localReplayApkPath.string().c_str());
		return false;
	}

	if (!this->PushFile(localReplayApkPath, "/sdcard/Download/gfxr_replay.apk")) {
		LOGW("Failed to push replay APK to /sdcard/Download/");
		return false;
	}

	std::string result = this->ShellCommand("pm install -g -t -r /sdcard/Download/gfxr_replay.apk");
	if (result.find("success") == std::string::npos) {
		LOGW("Failed to install replay APK");
		return false;
	}

	return true;
}

std::string ADB::ShellCommandAsGFXR(std::string cmd) {
	std::string result;

	result = this->ShellCommand(std::format("run-as com.lunarg.gfxreconstruct.replay sh -c '{}' || echo GFXRCommandFailed", cmd));
	if (result.find("GFXRCommandFailed") == std::string::npos)
		return result;

	result = this->ShellCommand(std::format("su 0 sh -c '{}' || echo GFXRCommandFailed", cmd));
	if (result.find("GFXRCommandFailed") == std::string::npos)
		return result;

	return this->ShellCommand(cmd);
}

bool ADB::AlreadyUploaded(std::filesystem::path local, std::string remote) {
	std::ifstream localFile(local, std::ios::binary | std::ios::ate);
	std::streamsize localSize = localFile.tellg();
	
	std::string strRemoteSize = this->ShellCommandAsGFXR(std::format("stat -c%s {}", remote));
	std::stringstream ss(strRemoteSize);
	size_t remoteSize = 0;
	ss >> remoteSize;

	LOGD("local size %zd remote size %zu", localSize, remoteSize);

	return localSize == remoteSize;
}