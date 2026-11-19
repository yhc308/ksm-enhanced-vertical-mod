#pragma once
#include "InstallerCore.hpp"
#include <chrono>

namespace Setup
{
 struct OnlineUpdate
 {
  std::string status = "unavailable";
  std::string version, revision, releaseUrl, detail;
 };
 using UpdateTransport = std::function<std::string(const std::string&, std::atomic_bool&, std::chrono::steady_clock::time_point)>;
 bool IsReleasePage(const std::string& url);
 std::string UpdateHttpGet(const std::string& url, std::atomic_bool& cancel, std::chrono::steady_clock::time_point deadline);
 OnlineUpdate CheckOnlineUpdate(const Json& current, std::atomic_bool& cancel, UpdateTransport transport = UpdateHttpGet);
}
