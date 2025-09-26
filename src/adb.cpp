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
#include <QDir>
#include <QCoreApplication>

#include <sstream>
#include <format>
#include <fstream>
#include <thread>
#include <chrono>
#include "common.hpp"
#include "ProgressBar.hpp"

static std::string rstrip(const std::string & s) {
	return s.substr(0, s.find_last_not_of(" \t\n\r\f\v") + 1);
}

QString ADB::runProgram(const QString& program, const QStringList& args) {
	QProcess p;
	p.setProgram(program);
	p.setArguments(args);
	p.start();
	p.waitForFinished(-1);

	QString output = QString::fromUtf8(p.readAllStandardOutput());
	return output.trimmed();
}

bool ADB::pushFileStreaming(std::string serial, QFileInfo src, QString dst)
{
	QProcess p;
	QFile f(src.absoluteFilePath());
	if (!f.open(QIODevice::ReadOnly))
		return false;

	if (this->ShellCommand(QString("touch %1 || echo failed $?").arg(dst)).contains("failed")) {
		LOGD("Failed to create remote file %s", dst.toStdString().c_str());
		return false;
	}

	LOGD("Transferring %s to %s", src.absoluteFilePath().toStdString().c_str(), dst.toStdString().c_str());

	p.setProgram("adb");
	p.setArguments({ "-s", serial.c_str(), "exec-in", "sh", "-c", QString("cat > %1").arg(dst) });
	p.setProcessChannelMode(QProcess::MergedChannels);
	p.start();

	if (!p.waitForStarted())
		return false;

	const qint64 totalSize = f.size();
	qint64 sent = 0;

	QByteArray buf;
	buf.resize(1 << 20);

	ProgressBar progress(QString("Transferring %1").arg(src.fileName()));
	
	while (true) {
		qint64 off = 0;
		const qint64 n = f.read(buf.data(), buf.size());

		if (n < 0) return false;
		if (n == 0) break;

		while (off < n) {
			const qint64 w = p.write(buf.constData() + off, n - off);
			if (w <= 0) return false;
			if (!p.waitForBytesWritten(-1)) return false;
			off += w;
			sent += w;
			progress.update(sent * 100 / totalSize);
		}
	}

	qint64 lastRemoteSize = 0;
	qint64 currentRemoteSize = 0;

	do {
		lastRemoteSize = currentRemoteSize;
		currentRemoteSize = this->GetRemoteSize(dst);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	} while (currentRemoteSize < totalSize && currentRemoteSize != lastRemoteSize);

	p.closeWriteChannel();
	p.waitForFinished(-1);
	progress.close();

	LOGD("%lld out of %lld transferred", currentRemoteSize, totalSize);

	return currentRemoteSize == f.size();
}

ADB::ADB() {
}

ADB::~ADB() {
}

std::vector<std::string> ADB::GetDevices() {
	std::string output = runProgram("adb", {"devices"}).toStdString();

	if (output.empty())
		LOGE("adb not found!");

	std::vector<std::string> devices;
	std::istringstream iss(output);
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

QString ADB::ShellCommand(QString cmd) {
	return runProgram("adb", { "-s", serial.c_str(), "shell", cmd });
}

std::string ADB::ShellCommand(std::string cmd) {
	return this->ShellCommand(QString::fromStdString(cmd)).toStdString();
}

std::string ADB::ShellCommand(const char* cmd) {
	return this->ShellCommand(std::string(cmd));
}

QString ADB::ShellCommandPrivileged(QString cmd) {
	QString result;

	LOGD("Running previleged command \"%s\"", cmd.toStdString().c_str());

	result = this->ShellCommand(QString("su 0 sh -c '%1' || echo ShellCommandPrivileged $?").arg(cmd));
	if (!result.contains("ShellCommandPrivileged"))
		return result;
	LOGD("Failed to run previleged command as root");

	result = this->ShellCommand(QString("run-as com.lunarg.gfxreconstruct.replay sh -c '%1' || echo ShellCommandPrivileged $?").arg(cmd));
	if (!result.contains("ShellCommandPrivileged"))
		return result;
	LOGD("Failed to run previleged command as gfxr");

	return this->ShellCommand(cmd);
}

std::string ADB::ShellCommandPrivileged(std::string cmd) {
	return this->ShellCommandPrivileged(QString::fromStdString(cmd)).toStdString();
}

std::string ADB::ShellCommandPrivileged(const char* cmd) {
	return this->ShellCommandPrivileged(std::string(cmd));
}

std::vector<std::string> ADB::GetPackages() {
	std::vector<std::string> packages;
	std::string raw = this->ShellCommand("pm list packages -3");
	std::istringstream iss(raw);
	std::string line;
	while (std::getline(iss, line, '\n')) {
		std::string package = rstrip(line.substr(strlen("package:")));
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
	std::string cmd = std::format("pm list packages -3 -f | sed -n 's/^package:\\(.*\\)base\\.apk={}$/\\1/p'", package);
	std::string str = this->ShellCommand(cmd) + "lib/";
	return str;
}

bool ADB::PushFile(QFileInfo src, QString dst) {
	QString filename = src.fileName();
	dst = dst.endsWith('/') ? dst + filename : dst;
	LOGD("Pushing %s", dst.toStdString().c_str());
	if (!pushFileStreaming(serial.c_str(), src, dst)) {
		LOGD("Unable to push directly. Try pushing to Download folder");
		QString staging = "/sdcard/Download/" + filename;
		if (!pushFileStreaming(serial.c_str(), src, staging)) {
			LOGD("Failed to push to Download folder");
			return false;
		}
		this->ShellCommandPrivileged(QString("mv /sdcard/Download/%1 %2").arg(filename, dst));
	}
	this->ShellCommandPrivileged(QString("chmod 777 %1").arg(dst));
	return this->GetRemoteSize(dst);
}

bool ADB::InstallReplayApk() {
	QFileInfo localReplayApkPath(QDir(QCoreApplication::applicationDirPath()), "tools/replay-debug.apk");

	std::string cmd = "pm list packages -3 | grep com.lunarg.gfxreconstruct.replay";
	if (this->ShellCommand(cmd).length()) {
		LOGD("Replay APK already installed");
		return true;
	}

	LOGD("Installing replay APK");
	if (!localReplayApkPath.isFile()) {
		LOGW("Failed to find replay APK at %s", localReplayApkPath.absoluteFilePath().toStdString().c_str());
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

bool ADB::PushRecordLayer(std::string package) {
	std::string abi = this->GetAppAbi(package);
	std::string arch;
	if (abi.empty()) {
		LOGW("Failed to get ABI of %s", package.c_str());
		return false;
	}

	if (abi == "armeabi")
		abi = "armeabi-v7a";

	if (abi == "arm64-v8a")
		arch = "arm64";
	else if (abi == "armeabi-v7a")
		arch = "arm";
	else if (abi == "x86_64")
		arch = "x86_64";
	else if (abi == "x86")
		arch = "x86";
	else {
		LOGW("Unknown ABI %s", abi.c_str());
		return false;
	}
	LOGD("ABI of %s is %s arch %s", package.c_str(), abi.c_str(), arch.c_str());

	QFileInfo localRecordLayerPath(QDir(QCoreApplication::applicationDirPath()), QString("layer/%1/libVkLayer_gfxreconstruct.so").arg(abi.c_str()));
	if (!localRecordLayerPath.isFile()) {
		LOGW("Failed to find debug layer at %s", localRecordLayerPath.absoluteFilePath().toStdString().c_str());
		return false;
	}

	QString dstPath = QString("%1%2/").arg(this->GetAppLibDir(package).c_str(), arch.c_str());
	if (!this->PushFile(localRecordLayerPath, dstPath)) {
		LOGW("Failed to push layer to app lib path %s", dstPath.toStdString().c_str());
		return false;
	}

	return true;
}

bool ADB::AlreadyUploaded(QFileInfo local, QString remote) {
	qint64 localSize = local.size();
	size_t remoteSize = this->GetRemoteSize(remote);

	LOGD("local size %zd remote size %zu", localSize, remoteSize);

	return localSize == remoteSize;
}

void ADB::SetRecordProp(std::string package) {
	this->ShellCommand("settings put global enable_gpu_debug_layers 1");
	this->ShellCommand(std::format("settings put global gpu_debug_app {}", package));
	this->ShellCommand("settings put global gpu_debug_layers VK_LAYER_LUNARG_gfxreconstruct");
	this->ShellCommand(std::format("setprop debug.gfxrecon.capture_file /sdcard/Download/{}.gfxr", package));
}

qint64 ADB::GetRemoteSize(QString remotePath) {
	QString strRemoteSize = this->ShellCommandPrivileged(QString("stat -c%s %1").arg(remotePath));
	return strRemoteSize.toLongLong();
}