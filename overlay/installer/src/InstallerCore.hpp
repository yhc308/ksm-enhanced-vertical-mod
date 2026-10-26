#pragma once
#include <windows.h>
#include <filesystem>
#include <functional>
#include <atomic>
#include <string>
#include <vector>
#include <set>
#include "../third_party/json.hpp"

namespace Setup
{
	using Json = nlohmann::json;
	using Path = std::filesystem::path;
	using Bytes = std::vector<unsigned char>;
	struct Error : std::runtime_error
	{
		std::string code;
		DWORD systemCode;
		Error(std::string code, std::string message, DWORD systemCode = 0);
	};
	struct Handle
	{
		HANDLE value = INVALID_HANDLE_VALUE;
		explicit Handle(HANDLE value = INVALID_HANDLE_VALUE) : value(value) {}
		~Handle();
		Handle(const Handle&) = delete;
		Handle& operator=(const Handle&) = delete;
	};
	std::string Utf8(std::wstring_view value);
	std::wstring Wide(std::string_view value);
	Bytes Read(const Path& path);
	void Write(const Path& path, const Bytes& bytes);
	void WriteJson(const Path& path, const Json& value);
	std::string Hash(const Bytes& value);
	std::string FileHash(const Path& path);
	Path ModulePath();
	struct Package
	{
		Json index;
		Bytes blob;
		size_t dataOffset = 0;
		explicit Package(const Path& executable);
		Bytes Extract(const Json& file) const;
		void Validate() const;
	};
	struct Engine
	{
		const Package& package;
		Path root;
		std::function<void(const std::wstring&)> progress;
		std::function<void(int)> progressValue;
		std::atomic_bool* cancel = nullptr;
		Engine(const Package& package, Path root);
		Json Run(const std::string& action);
		Json Inspect();
		void CheckTarget(bool installing);
		void CheckIdle();
		void CheckCancel();
		void Recover();
		Path Safe(const std::string& relative, bool inspectLinks = true) const;
		static void DataPath(const std::string& relative);
		static std::string Kind(const std::string& relative);
	private:
		Json LoadState();
		Json Install();
		Json Remove();
		Json Verify();
		Json Transaction(const Json& operations, const std::string& id, const Json& cleanup = Json::array(), const Json& sources = Json::array());
		Json recoveredCleanup = nullptr;
		std::set<std::string> DirectoryParents(const Json& sources) const;
		void ValidateDirectories(const Json& directories, const Json& sources, bool inspectLinks) const;
		Json RemovalDirectories(const Json& state) const;
		Json CleanupDirectories(const Json& directories);
		void Rollback(const Json& journal);
		void Apply(const Json& operation, bool undo);
		void CheckOp(const Json& operation) const;
		void BackupPath(const std::string& path, const std::string& id) const;
		void Say(const std::wstring& message);
		void ReportProgress(int percent);
	};
}
