#pragma once
#include "InstallerCore.hpp"
#include <array>
#include <regex>

namespace Setup
{
 inline int RevisionNumber(const Json& value)
 {
  if (!value.contains("installerRevisionNumber")) return 0;
  const auto& n = value.at("installerRevisionNumber");
  if (!n.is_number_integer() || n.get<int64_t>() < 0 || n.get<int64_t>() > 1000000000)
   throw Error("version", "Invalid installer revision number");
  return n.get<int>();
 }
 inline std::array<unsigned long, 5> VersionParts(const std::string& value)
 {
  std::smatch m;
  if (!std::regex_match(value, m, std::regex("([0-9]{1,6})\\.([0-9]{1,6})\\.([0-9]{1,6})(?:-[Bb]eta\\.?([0-9]{1,6}))?")))
   throw Error("version", "Unknown version format");
  return {std::stoul(m[1]), std::stoul(m[2]), std::stoul(m[3]), m[4].matched ? 0UL : 1UL, m[4].matched ? std::stoul(m[4]) : 0UL};
 }
 inline int ComparePackages(const Json& a, const Json& b)
 {
  const auto av = VersionParts(a.at("version")), bv = VersionParts(b.at("version"));
  if (av != bv) return av < bv ? -1 : 1;
  const auto ab = a.value("buildId", ""), bb = b.value("buildId", "");
  const std::regex format("[0-9]{8}-[0-9]{6}-[0-9a-fA-F]{8}");
  if (std::regex_match(ab, format) && std::regex_match(bb, format) && ab.substr(0,15) != bb.substr(0,15))
   return ab.substr(0,15) < bb.substr(0,15) ? -1 : 1;
  if (ab != bb) return 0; // Do not order arbitrary or same-timestamp build identifiers.
  const auto ar = RevisionNumber(a), br = RevisionNumber(b);
  return ar == br ? 0 : ar < br ? -1 : 1;
 }
}
