/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include <string>
#include <string_view>

namespace aida {

void startNotifyServer();
std::string createCodexTask(std::string_view prompt, std::string_view workspace, std::string_view title, bool notify);
std::string getCodexTask(std::string_view taskId);
std::string cancelCodexTask(std::string_view taskId);

}  // namespace aida
