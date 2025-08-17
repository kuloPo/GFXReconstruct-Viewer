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

#include <sstream>
#include <format>
#include "common.hpp"

#define CHECK_ADB_ERROR() if (ec) LOGE("%s failed with %s", __func__, ec.message().c_str());

const int64_t g_timeout = 5000;

static std::string rstrip(std::string & s) {
	return s.substr(0, s.find_last_not_of(" \t\n\r\f\v") + 1);
}

ADB::ADB() {
}

ADB::~ADB() {
}

std::vector<std::string> ADB::GetDevices() {
	std::string raw = adb::devices(ec, g_timeout);
	CHECK_ADB_ERROR();

	std::vector<std::string> devices;
	std::istringstream iss(raw);
	std::string line;
	while (std::getline(iss, line, '\n')) {
		const size_t pos = line.find('\t');
		std::string serial = line.substr(0, pos);
		std::string status = line.substr(pos + 1);
		if (status == "device")
			devices.push_back(serial);
	}
	return devices;
}

bool ADB::ConnectDevice(std::string serial) {
	m_client = adb::client::create(serial);
	m_client->start();
	m_client->connect(ec, g_timeout);
	CHECK_ADB_ERROR();
	if (ec) {
		LOGW("Failed to connect %s", serial.c_str());
		return false;
	}
	m_bRooted = false;
	return true;
}

bool ADB::RootDevice() {
	if (m_bRooted)
		return true;
	std::string result = m_client->root(ec, g_timeout);
	if (result.find("cannot") != std::string::npos) {
		LOGW("Failed to root device");
		return false;
	}
	m_bRooted = true;
	return true;
}

std::string ADB::ShellCommand(std::string cmd) {
	std::string result = m_client->shell(cmd, ec, g_timeout);
	CHECK_ADB_ERROR();
	return result;
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
	return rstrip(abi);
}

std::string ADB::GetAppLibDir(std::string package) {
	std::string cmd = std::format("dumpsys package {} | grep legacyNativeLibraryDir | cut -d= -f2-", package);
	std::string str = this->ShellCommand(cmd);
	return rstrip(str);
}

void ADB::PushFile(std::filesystem::path src, std::string dst) {
	m_client->push(src, dst, 777, ec, g_timeout);
	CHECK_ADB_ERROR();
}

bool ADB::ReplayApkInstalled() {
	std::string cmd = "pm list packages -3 | grep com.lunarg.gfxreconstruct.replay";
	return this->ShellCommand(cmd).length();
}