#include "InstallerCore.hpp"
#include <bcrypt.h>
#include <compressapi.h>
#include <tlhelp32.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <regex>
#include <set>
#include <sstream>
#include <cstring>
#include <memory>

namespace Setup
{
	namespace
	{
		constexpr auto kState = ".ksm-enhanced-install.json";
		constexpr auto kJournal = ".ksm-enhanced-journal.json";
		constexpr auto kContract = "ksm-2.1.0-enhanced-skin-v2";
		void Need(bool condition, const char* code, const std::string& message)
		{
			if (!condition) throw Error(code, message);
		}
		void Win(bool condition, const std::string& message)
		{
			if (!condition)
			{
				const DWORD e = GetLastError();
				throw Error(e == ERROR_ACCESS_DENIED ? "permission" : "io", message, e);
			}
		}
		std::string Lower(std::string text)
		{
			for (auto& c : text) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
			return text;
		}
		std::string Id()
		{
			unsigned char value[16];
			Need(BCryptGenRandom(nullptr, value, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0, "crypto", "Random ID failed");
			std::ostringstream out;
			for (auto c : value) out << std::hex << std::setw(2) << std::setfill('0') << int(c);
			return out.str();
		}
		bool Digest(const std::string& value)
		{
			return std::regex_match(value, std::regex("[a-fA-F0-9]{64}"));
		}
		void Regular(const Path& path)
		{
			Handle handle(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
			Win(handle.value != INVALID_HANDLE_VALUE, "Cannot open file: " + Utf8(path.wstring()));
			BY_HANDLE_FILE_INFORMATION info{};
			Win(GetFileInformationByHandle(handle.value, &info) != 0, "Cannot inspect file");
			Need(!(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) && info.nNumberOfLinks == 1, "unsafe_path", "Linked or nonregular file rejected");
		}
		void Writable(const Path& path)
		{
			if (!std::filesystem::exists(path)) return;
			Regular(path);
			Handle handle(CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
			Win(handle.value != INVALID_HANDLE_VALUE, "File is locked or not writable: " + Utf8(path.wstring()));
		}
		Json ReadJson(const Path& path)
		{
			const auto data = Read(path);
			Need(data.size() <= 8 * 1024 * 1024, "state", "Metadata too large");
			return Json::parse(data.begin(), data.end());
		}
		Bytes JsonBytes(const Json& value)
		{
			const auto text = value.dump(2) + "\n";
			return Bytes(text.begin(), text.end());
		}
		void Erase(const Path& path)
		{
			if (!std::filesystem::exists(path)) return;
			Regular(path);
			Win(DeleteFileW(path.c_str()) != 0, "Cannot remove: " + Utf8(path.wstring()));
		}
		void Atomic(const Path& path, const Bytes& bytes)
		{
			const Path temporary = path.parent_path() / (L".ksm-enhanced-tmp-" + Wide(Id()));
			try
			{
				Write(temporary, bytes);
				Writable(path);
				Win(MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0, "Atomic replacement failed: " + Utf8(path.wstring()));
			}
			catch (...) { if (std::filesystem::exists(temporary)) Erase(temporary); throw; }
		}
		uint64_t Number(const Bytes& data, size_t offset)
		{
			Need(offset <= data.size() && data.size() - offset >= 8, "package", "Truncated package");
			uint64_t value;
			memcpy(&value, data.data() + offset, 8);
			return value;
		}
		Json Operation(const std::string& path, const Json& before, const Json& undo, const Json& after, const Json& stage)
		{
			return Json{{"path", path}, {"before", before}, {"undo", undo}, {"after", after}, {"stage", stage}};
		}
	}

	Error::Error(std::string code, std::string message, DWORD systemCode)
		: std::runtime_error(message), code(std::move(code)), systemCode(systemCode) {}
	Handle::~Handle() { if (value != INVALID_HANDLE_VALUE && value != nullptr) CloseHandle(value); }
	std::string Utf8(std::wstring_view value)
	{
		if (value.empty()) return {};
		const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), nullptr, 0, nullptr, nullptr);
		Need(size > 0, "encoding", "Invalid Unicode path");
		std::string result(size, 0);
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), int(value.size()), result.data(), size, nullptr, nullptr);
		return result;
	}
	std::wstring Wide(std::string_view value)
	{
		if (value.empty()) return {};
		const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), int(value.size()), nullptr, 0);
		Need(size > 0, "encoding", "Invalid UTF-8");
		std::wstring result(size, 0);
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), int(value.size()), result.data(), size);
		return result;
	}
	Path ModulePath()
	{
		std::wstring buffer(32768, 0);
		DWORD size = GetModuleFileNameW(nullptr, buffer.data(), DWORD(buffer.size()));
		Need(size && size < buffer.size(), "path", "Cannot locate installer");
		buffer.resize(size); return Path(buffer);
	}
	Bytes Read(const Path& path)
	{
		Regular(path);
		std::ifstream in(path, std::ios::binary | std::ios::ate);
		Need(bool(in), "io", "Cannot read file: " + Utf8(path.wstring()));
		const auto length = in.tellg();
		Need(length >= 0 && length <= 1024LL * 1024 * 1024, "io", "Invalid file size");
		Bytes data(static_cast<size_t>(length)); in.seekg(0);
		if (!data.empty()) in.read(reinterpret_cast<char*>(data.data()), data.size());
		Need(bool(in), "io", "Incomplete read"); return data;
	}
	void Write(const Path& path, const Bytes& bytes)
	{
		std::filesystem::create_directories(path.parent_path());
		Handle handle(CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr));
		Win(handle.value != INVALID_HANDLE_VALUE, "Cannot create file: " + Utf8(path.wstring()));
		size_t position = 0;
		while (position < bytes.size())
		{
			DWORD written = 0;
			Win(WriteFile(handle.value, bytes.data() + position, DWORD(std::min<size_t>(bytes.size() - position, 1024 * 1024)), &written, nullptr) && written > 0, "Write failed");
			position += written;
		}
		Win(FlushFileBuffers(handle.value) != 0, "Flush failed");
	}
	void WriteJson(const Path& path, const Json& value) { Atomic(path, JsonBytes(value)); }
	std::string Hash(const Bytes& value)
	{
		BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr;
		Need(BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == 0, "crypto", "SHA256 unavailable");
		unsigned char digest[32];
		const bool success = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0
			&& BCryptHashData(hash, const_cast<PUCHAR>(value.data()), ULONG(value.size()), 0) == 0
			&& BCryptFinishHash(hash, digest, 32, 0) == 0;
		if (hash) BCryptDestroyHash(hash); BCryptCloseAlgorithmProvider(algorithm, 0);
		Need(success, "crypto", "SHA256 failed");
		std::ostringstream out;
		for (auto c : digest) out << std::hex << std::setw(2) << std::setfill('0') << int(c);
		return out.str();
	}
	std::string FileHash(const Path& path) { return Hash(Read(path)); }
	Package::Package(const Path& executable)
	{
		const Bytes image = Read(executable);
		Need(image.size() >= 88, "package", "Missing embedded package");
		const size_t footer = image.size() - 88;
		Need(memcmp(image.data() + footer, "KSMTAIL1", 8) == 0, "package", "Invalid embedded package footer");
		const auto offset = Number(image, footer + 8), size = Number(image, footer + 16);
		Need(offset <= footer && size == footer - offset, "package", "Invalid embedded package bounds");
		blob.assign(image.begin() + offset, image.begin() + footer);
		Need(Hash(blob) == std::string(image.begin() + footer + 24, image.end()), "package", "Embedded package checksum mismatch");
		Need(blob.size() > 16 && memcmp(blob.data(), "KSMSET01", 8) == 0, "package", "Invalid package header");
		const auto length = Number(blob, 8);
		Need(length < 8 * 1024 * 1024 && length <= blob.size() - 16, "package", "Invalid package index");
		index = Json::parse(blob.begin() + 16, blob.begin() + 16 + length); dataOffset = 16 + length;
		Need(index.at("schemaVersion") == 1 && index.at("modId") == "ksm-enhanced-vertical", "package", "Unknown package");
		Need(index.at("files").is_array() && index.at("files").size() <= 4096, "package", "Invalid package inventory");
		std::set<std::string> paths;
		for (const auto& file : index.at("files"))
		{
			const auto path = file.at("path").get<std::string>(); Engine::DataPath(path);
			Need(paths.insert(Lower(path)).second, "package", "Duplicate package path");
			Need(file.at("kind") == Engine::Kind(path) && Digest(file.at("sha256")), "package", "Invalid package file metadata");
		}
		Need(paths.count("ksm-enhanced-vertical.exe") == 1, "package", "Missing mod executable");
	}
	Bytes Package::Extract(const Json& file) const
	{
		const uint64_t offset = file.at("offset"), size = file.at("compressedSize"), raw = file.at("size");
		Need(offset <= blob.size() - dataOffset && size <= blob.size() - dataOffset - offset && raw <= 512 * 1024 * 1024, "package", "Invalid compressed file bounds");
		DECOMPRESSOR_HANDLE handle = nullptr;
		Win(CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &handle) != 0, "Decompressor unavailable");
		Bytes result(static_cast<size_t>(raw)); SIZE_T written = 0;
		const bool success = Decompress(handle, blob.data() + dataOffset + offset, SIZE_T(size), result.data(), result.size(), &written) != 0;
		CloseDecompressor(handle);
		Need(success && written == raw && Hash(result) == file.at("sha256"), "package", "Payload checksum mismatch: " + file.at("path").get<std::string>());
		return result;
	}
	void Package::Validate() const { for (const auto& file : index.at("files")) Extract(file); }
	Engine::Engine(const Package& package, Path root) : package(package), root(std::filesystem::absolute(root).lexically_normal())
	{
		if (this->root.filename().empty()) this->root = this->root.parent_path();
		Need(std::filesystem::is_directory(this->root) && this->root != this->root.root_path(), "target", "Select the folder containing kshootmania.exe");
		Safe("kshootmania.exe");
	}
	Path Engine::Safe(const std::string& relative, bool inspectLinks) const
	{
		Need(!relative.empty() && relative.size() < 512 && relative.front() != '/' && relative.find('\\') == std::string::npos && relative.find(':') == std::string::npos && relative.back() != '/', "unsafe_path", "Unsafe relative path");
		Path path = root;
		auto Check = [inspectLinks](const Path& part)
		{
			if (!inspectLinks) return;
			const DWORD attr = GetFileAttributesW(part.c_str());
			if (attr != INVALID_FILE_ATTRIBUTES) Need(!(attr & FILE_ATTRIBUTE_REPARSE_POINT), "unsafe_path", "Reparse point rejected");
		};
		Path scan;
		for (const auto& part : root) { scan /= part; Check(scan); }
		std::istringstream stream(relative); std::string part;
		while (std::getline(stream, part, '/'))
		{
			Need(!part.empty() && part != "." && part != ".." && part.back() != '.' && part.back() != ' ' && part.find_first_of("<>\"|?*") == std::string::npos, "unsafe_path", "Unsafe path component");
			const auto stem = Lower(part.substr(0, part.find('.')));
			Need(!std::regex_match(stem, std::regex("con|prn|aux|nul|com[0-9]|lpt[0-9]")), "unsafe_path", "Reserved device path");
			path /= Wide(part); Check(path);
		}
		return path;
	}
	void Engine::DataPath(const std::string& relative)
	{
		const std::string path = Lower(relative);
		Need(path.size() < 384 && !std::regex_search(path, std::regex("(^|/)(score|score_backup|songs|songs_default|courses|skins|cache|screenshot)(/|$)"))
			&& !std::regex_search(path, std::regex("\\.(ksc|ini|sqlite|sqlite3)($|[./])"))
			&& !std::regex_search(path, std::regex("(^|/)(config|skin_preset|audio_calibration_profiles)\\.json$")), "protected_path", "Protected user-data path: " + relative);
		Need(path == "ksm-enhanced-vertical.exe" || path == "bassasio.dll" || path == "bassmix.dll"
			|| std::regex_match(relative, std::regex("lang/(English|Japanese|Korean|Simplified Chinese|Traditional Chinese)\\.json"))
			|| std::regex_match(path, std::regex("(imgs|skin_preview)/[a-z0-9_./-]+"))
			|| std::regex_match(path, std::regex("mods/(enhanced|vertical)/[a-z0-9_./-]+"))
			|| std::regex_match(path, std::regex("shaders/aps_color_dodge\\.(hlsl|frag|essl\\.frag)"))
			|| std::regex_match(path, std::regex("ui/menu_(custom|judge|vertical)_icon\\.png")), "protected_path", "Not an owned mod path: " + relative);
		Need(path.rfind("imgs/default/", 0) != 0 && path.find("/../") == std::string::npos, "unsafe_path", "Retired or unsafe payload path");
	}
	std::string Engine::Kind(const std::string& relative)
	{
		if (relative.rfind("lang/", 0) == 0) return "language";
		const auto extension = Lower(Utf8(Path(Wide(relative)).extension().wstring()));
		if (extension == ".png" || extension == ".gif" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".webp" || extension == ".ico") return "image";
		return "other";
	}
	void Engine::CheckIdle()
	{
		Handle snapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
		Win(snapshot.value != INVALID_HANDLE_VALUE, "Cannot inspect running processes");
		PROCESSENTRY32W entry{sizeof(entry)};
		for (BOOL ok = Process32FirstW(snapshot.value, &entry); ok; ok = Process32NextW(snapshot.value, &entry))
		{
			if (_wcsicmp(entry.szExeFile, L"kshootmania.exe") && _wcsicmp(entry.szExeFile, L"ksm-enhanced-vertical.exe")) continue;
			Handle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID));
			if (process.value == nullptr) continue;
			wchar_t buffer[32768]; DWORD length = 32768;
			if (QueryFullProcessImageNameW(process.value, 0, buffer, &length))
			{
				if (_wcsicmp(Path(std::wstring(buffer, length)).parent_path().c_str(), root.c_str()) == 0) throw Error("busy", "Close the game in the selected folder before continuing");
			}
		}
	}
	void Engine::CheckTarget(bool installing)
	{
		const auto executable = Safe("kshootmania.exe");
		Need(std::filesystem::exists(executable), "missing_executable", "Cannot open file: " + Utf8(executable.wstring()));
		const auto hash = FileHash(executable);
		bool matched = false;
		for (const auto& profile : package.index.at("targets").at("profiles"))
		{
			for (const auto& value : profile.at("executableHashes"))
			{
				if (Lower(value) != hash) continue;
				matched = true;
				Need(profile.at("runtimeContract") == kContract, "target", "Unsupported runtime contract");
				if (installing) for (const auto& file : profile.at("requiredFiles"))
				{
					const auto path = Safe(file.at("path"));
					Need(std::filesystem::is_regular_file(path) && FileHash(path) == Lower(file.at("sha256")), "target", "Original resource contract mismatch: " + file.at("path").get<std::string>());
				}
			}
		}
		Need(matched, "target", "This installer supports the verified KSM 2.1.0 Windows build only");
	}
	void Engine::Say(const std::wstring& message) { if (progress) progress(message); }
	void Engine::ReportProgress(int percent) { if (progressValue) progressValue(percent); }
	void Engine::CheckCancel() { if (cancel && cancel->load()) throw Error("cancelled", "Cancelled; changes rolled back"); }
	void Engine::BackupPath(const std::string& path, const std::string& id) const
	{
		Need(std::regex_match(id, std::regex("[a-f0-9]{32}")), "state", "Invalid transaction ID");
		Need(std::regex_match(path, std::regex("\\.ksm-enhanced-backup/" + id + "/(original/|undo/|stage/)?[0-9]+\\.bin")), "state", "Invalid backup path");
		Safe(path);
	}
	std::set<std::string> Engine::DirectoryParents(const Json& sources) const
	{
		Need(sources.is_array() && sources.size() <= 4096, "state", "Invalid directory source inventory");
		std::set<std::string> parents, seen;
		for (const auto& source : sources)
		{
			Need(source.is_string(), "state", "Invalid directory source");
			const auto name = source.get<std::string>(); DataPath(name); Safe(name, false);
			Need(seen.insert(Lower(name)).second, "state", "Duplicate directory source");
			for (auto parent = Path(Wide(name)).parent_path(); !parent.empty(); parent = parent.parent_path())
				parents.insert(Lower(Utf8(parent.generic_wstring())));
			Need(parents.size() <= 8192, "state", "Directory source inventory is too large");
		}
		return parents;
	}
	void Engine::ValidateDirectories(const Json& directories, const Json& sources, bool inspectLinks) const
	{
		Need(directories.is_array() && directories.size() <= 8192, "state", "Invalid directory inventory");
		const auto parents = DirectoryParents(sources);
		std::set<std::string> seen;
		for (const auto& entry : directories)
		{
			Need(entry.is_string(), "state", "Invalid directory entry");
			const auto name = entry.get<std::string>(), key = Lower(name);
			Safe(name, inspectLinks);
			Need(!std::regex_search(key, std::regex("(^|/)(score|score_backup|songs|songs_default|courses|skins|cache|screenshot|screenshots|log|logs|settings|config|\\.ksm-enhanced-backup|\\.ksm-enhanced-recovery)(/|$)")), "protected_path", "User or recovery directory is protected: " + name);
			Need(key != "imgs" && key != "lang" && key != "shaders" && key != "ui", "protected_path", "Shared game directory is protected: " + name);
			Need(parents.count(key) && seen.insert(key).second, "state", "Unrelated or duplicate directory: " + name);
			// Sources have already passed the independent user-data denylist.
			Need(key == "mods" || key == "skin_preview" || key.rfind("mods/enhanced", 0) == 0 || key.rfind("mods/vertical", 0) == 0 || key.rfind("skin_preview/", 0) == 0 || key.rfind("imgs/", 0) == 0,
				"protected_path", "Not a removable mod directory: " + name);
		}
	}
	Json Engine::RemovalDirectories(const Json& state) const
	{
		Json sources = Json::array();
		for (const auto& file : state.at("files")) sources.push_back(file.at("path"));
		Json directories = state.value("createdDirectories", Json::array());
		if (!state.contains("createdDirectories"))
		{
			for (const auto& name : DirectoryParents(sources))
				if (name == "mods/enhanced" || name.rfind("mods/enhanced/", 0) == 0 || name == "mods/vertical" || name.rfind("mods/vertical/", 0) == 0 || name == "skin_preview" || name.rfind("skin_preview/", 0) == 0) directories.push_back(name);
		}
		ValidateDirectories(directories, sources, true);
		return directories;
	}
	Json Engine::CleanupDirectories(const Json& directories)
	{
		Json result{{"removedDirectories", Json::array()}, {"preservedDirectories", Json::array()}, {"cleanupWarnings", Json::array()}};
		std::vector<std::string> ordered = directories.get<std::vector<std::string>>();
		std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b)
		{
			const auto ad = std::count(a.begin(), a.end(), '/'), bd = std::count(b.begin(), b.end(), '/');
			return ad != bd ? ad > bd : a < b;
		});
		size_t count = 0;
		for (const auto& name : ordered)
		{
			try
			{
				const Path target = Safe(name);
				const DWORD attributes = GetFileAttributesW(target.c_str());
				if (attributes == INVALID_FILE_ATTRIBUTES)
				{
					const DWORD code = GetLastError();
					if (code == ERROR_FILE_NOT_FOUND || code == ERROR_PATH_NOT_FOUND) { result["preservedDirectories"].push_back(Json{{"path", name}, {"reason", "already_absent"}}); continue; }
					throw Error("inspection_failed", "Cannot inspect empty directory", code);
				}
				// Pin every ancestor without FILE_SHARE_DELETE. The leaf is opened without
				// following reparse points, and disposition deletes only an empty directory.
				std::vector<std::unique_ptr<Handle>> guards;
				Path scan;
				for (const auto& part : target)
				{
					scan /= part;
					if (!scan.is_absolute()) continue;
					const bool leaf = scan == target;
					auto handle = std::make_unique<Handle>(CreateFileW(scan.c_str(), FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES | (leaf ? DELETE : 0), FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
					Win(handle->value != INVALID_HANDLE_VALUE, "Directory is locked or not writable: " + name);
					BY_HANDLE_FILE_INFORMATION info{};
					Win(GetFileInformationByHandle(handle->value, &info) != 0, "Cannot inspect directory handle");
					Need((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT), "unsafe_path", "Linked or non-directory cleanup target");
					guards.push_back(std::move(handle));
				}
#ifdef KSM_INSTALLER_TEST
				if (package.index.value("fixture", false))
				{
					wchar_t value[512];
					if (GetEnvironmentVariableW(L"KSM_SETUP_DIRECTORY_DENY", value, 512) && name == Utf8(value)) throw Error("permission", "Injected directory permission failure", ERROR_ACCESS_DENIED);
					if (GetEnvironmentVariableW(L"KSM_SETUP_DIRECTORY_RACE", value, 512) && name == Utf8(value)) Write(target / L"concurrent-file.txt", Bytes{'R', 'A', 'C', 'E'});
					if (GetEnvironmentVariableW(L"KSM_SETUP_DIRECTORY_RENAME_RACE", value, 512) && name == Utf8(value))
					{
						const auto moved = target.wstring() + L".moved";
						Need(!MoveFileExW(target.c_str(), moved.c_str(), 0), "race_test", "Directory rename was not pinned");
						result["guardedRenameAttempts"] = 1;
					}
				}
#endif
				FILE_DISPOSITION_INFO disposition{TRUE};
				if (!SetFileInformationByHandle(guards.back()->value, FileDispositionInfo, &disposition, sizeof(disposition)))
				{
					const DWORD code = GetLastError();
					if (code == ERROR_DIR_NOT_EMPTY) { result["preservedDirectories"].push_back(Json{{"path", name}, {"reason", "not_empty"}}); continue; }
					throw Error("directory_remove_failed", "Cannot remove empty directory", code);
				}
				guards.clear();
				result["removedDirectories"].push_back(name); ++count;
#ifdef KSM_INSTALLER_TEST
				if (package.index.value("fixture", false))
				{
					wchar_t value[32];
					if (GetEnvironmentVariableW(L"KSM_SETUP_CLEANUP_CRASH_AFTER", value, 32) && _wtoi(value) == count) TerminateProcess(GetCurrentProcess(), 94);
				}
#endif
			}
			catch (const Error& error)
			{
				const Json warning{{"path", name}, {"reason", error.code}, {"systemCode", error.systemCode}};
				result["preservedDirectories"].push_back(warning); result["cleanupWarnings"].push_back(warning);
			}
		}
		return result;
	}
	Json Engine::LoadState()
	{
		auto state = ReadJson(Safe(kState));
		Need((state.at("schemaVersion") == 2 || state.at("schemaVersion") == 3) && state.at("modId") == "ksm-enhanced-vertical", "state", "Unknown installation record");
		const auto id = state.at("transaction").get<std::string>();
		Need(std::regex_match(id, std::regex("[a-f0-9]{32}")) && state.at("files").is_array() && state.at("files").size() <= 4096, "state", "Invalid installation inventory");
		std::set<std::string> seen;
		for (auto& file : state["files"])
		{
			const auto path = file.at("path").get<std::string>(); DataPath(path); Safe(path);
			Need(seen.insert(Lower(path)).second, "state", "Duplicate installed path");
			if (state["schemaVersion"] == 2) file["installedHash"] = Lower(file.at("sha256"));
			Need(Digest(file.at("installedHash")), "state", "Invalid installed checksum");
			file["installedHash"] = Lower(file.at("installedHash"));
			file["kind"] = Kind(path);
			if (!file.value("owned", true)) continue;
			if (state["schemaVersion"] == 2)
			{
				file["originalHash"] = file.value("originalSha256", Json(nullptr));
				file["originalBackup"] = file.value("backup", Json(nullptr));
			}
			if (!file.at("originalHash").is_null())
			{
				Need(Digest(file.at("originalHash")), "state", "Invalid original checksum");
				file["originalHash"] = Lower(file.at("originalHash"));
				BackupPath(file.at("originalBackup"), id);
				Need(FileHash(Safe(file.at("originalBackup"))) == Lower(file.at("originalHash")), "state", "Original backup missing or damaged");
			}
			else Need(file.at("originalBackup").is_null(), "state", "Unexpected backup");
		}
		Need(Digest(state.at("upstreamHash")), "state", "Invalid original game hash");
		RemovalDirectories(state);
		return state;
	}
	void Engine::CheckOp(const Json& op) const
	{
		const auto path = op.at("path").get<std::string>();
		if (path != kState) DataPath(path);
		Safe(path);
		for (const auto* key : {"before", "after"}) Need(op.at(key).is_null() || (op.at(key).is_string() && Digest(op.at(key))), "journal", "Invalid journal hash");
	}
	void Engine::Apply(const Json& op, bool undo)
	{
		CheckOp(op);
		const auto target = Safe(op.at("path"));
		const auto& hash = op.at(undo ? "before" : "after");
		if (hash.is_null()) { Erase(target); return; }
		const auto data = Read(Safe(op.at(undo ? "undo" : "stage")));
		Need(Hash(data) == hash, "journal", "Recovery/staged data checksum mismatch");
		Safe(op.at("path")); Atomic(target, data);
	}
	void Engine::Rollback(const Json& journal)
	{
		Need(journal.at("modId") == "ksm-enhanced-vertical" && journal.at("schemaVersion") == 1, "journal", "Unknown transaction journal");
		const auto id = journal.at("transaction").get<std::string>();
		const auto& operations = journal.at("operations");
		Need(operations.is_array() && operations.size() <= 4097, "journal", "Invalid journal inventory");
		std::set<std::string> paths;
		for (const auto& op : operations)
		{
			CheckOp(op); Need(paths.insert(Lower(op.at("path"))).second, "journal", "Duplicate journal operation");
			for (const auto& pair : {std::pair{"before", "undo"}, std::pair{"after", "stage"}})
			{
				if (op.at(pair.first).is_null()) { Need(op.at(pair.second).is_null(), "journal", "Unexpected recovery path"); continue; }
				BackupPath(op.at(pair.second), id);
				Need(FileHash(Safe(op.at(pair.second))) == op.at(pair.first), "journal", "Damaged recovery data");
			}
			const Path path = Safe(op.at("path"));
			const Json current = std::filesystem::exists(path) ? Json(FileHash(path)) : Json(nullptr);
			Need(current == op.at("before") || current == op.at("after"), "recovery_conflict", "File changed since the interrupted operation; retained: " + op.at("path").get<std::string>());
		}
		for (auto i = operations.rbegin(); i != operations.rend(); ++i) Apply(*i, true);
	}
	void Engine::Recover()
	{
		recoveredCleanup = nullptr;
		if (!std::filesystem::exists(Safe(kJournal))) return;
		Say(L"Recovering interrupted operation...");
		const auto journal = ReadJson(Safe(kJournal));
		Need(journal.at("phase") == "applying" || journal.at("phase") == "committed", "journal", "Invalid journal phase");
		Need(journal.at("modId") == "ksm-enhanced-vertical" && journal.at("schemaVersion") == 1, "journal", "Invalid journal owner");
		if (journal.contains("directoryCleanup")) ValidateDirectories(journal.at("directoryCleanup"), journal.at("directorySources"), false);
		if (journal.at("phase") == "applying") Rollback(journal);
		else
		{
			Need(journal.at("operations").is_array() && journal.at("operations").size() <= 4097, "journal", "Invalid committed operations");
			for (const auto& op : journal.at("operations")) CheckOp(op);
			if (journal.contains("directoryCleanup")) recoveredCleanup = CleanupDirectories(journal.at("directoryCleanup"));
		}
		Erase(Safe(kJournal));
	}
	Json Engine::Transaction(const Json& operations, const std::string& id, const Json& cleanup, const Json& sources)
	{
		Json journal{{"schemaVersion", 1}, {"modId", "ksm-enhanced-vertical"}, {"transaction", id}, {"phase", "applying"}, {"operations", operations}};
		if (!sources.empty())
		{
			ValidateDirectories(cleanup, sources, true);
			journal["directoryCleanup"] = cleanup; journal["directorySources"] = sources;
		}
		for (const auto& op : operations) { CheckOp(op); Writable(Safe(op.at("path"))); }
		WriteJson(Safe(kJournal), journal);
		ReportProgress(60);
		try
		{
			size_t count = 0;
			for (const auto& op : operations)
			{
				CheckCancel(); Apply(op, false); ++count;
				ReportProgress(60 + int(39 * count / operations.size()));
				Say(Wide(std::to_string(count) + "/" + std::to_string(operations.size()) + " " + op.at("path").get<std::string>()));
#ifdef KSM_INSTALLER_TEST
				if (package.index.value("fixture", false))
				{
					wchar_t value[32];
					if (GetEnvironmentVariableW(L"KSM_SETUP_CRASH_AFTER", value, 32) && _wtoi(value) == count) TerminateProcess(GetCurrentProcess(), 93);
					if (GetEnvironmentVariableW(L"KSM_SETUP_FAIL_AFTER", value, 32) && _wtoi(value) == count) throw Error("injected_failure", "Injected operation failure");
				}
#endif
			}
			journal["phase"] = "committed"; WriteJson(Safe(kJournal), journal);
		}
		catch (...) { Rollback(journal); Erase(Safe(kJournal)); throw; }
		Json report = CleanupDirectories(cleanup);
		try { Erase(Safe(kJournal)); }
		catch (const Error& error) { report["cleanupWarnings"].push_back(Json{{"path", kJournal}, {"reason", "journal_finalize_failed"}, {"systemCode", error.systemCode}}); }
		return report;
	}
	Json Engine::Install()
	{
		Need(!std::filesystem::exists(Safe(kState)), "installed", "Mod is already installed; remove it before replacing the version");
		Need(!std::filesystem::exists(Safe("ksm-enhanced-vertical.exe")), "manual_install", "Untracked manual mod installation detected; original files were not changed");
		CheckTarget(true); CheckIdle(); Say(L"Validating embedded files..."); package.Validate(); CheckCancel(); ReportProgress(10);
		uint64_t required = 16 * 1024 * 1024;
		for (const auto& file : package.index.at("files"))
		{
			const auto path = Safe(file.at("path"));
			required += file.at("size").get<uint64_t>() * 2;
			if (std::filesystem::exists(path)) required += std::filesystem::file_size(path) * 3;
			Writable(path);
		}
		ULARGE_INTEGER available{};
		Win(GetDiskFreeSpaceExW(root.c_str(), &available, nullptr, nullptr), "Cannot inspect free space");
#ifdef KSM_INSTALLER_TEST
		if (package.index.value("fixture", false) && GetEnvironmentVariableW(L"KSM_SETUP_LOW_SPACE", nullptr, 0)) available.QuadPart = 0;
#endif
		Need(available.QuadPart >= required, "space", "Insufficient free space for installation and rollback");
		Json directorySources = Json::array(), createdDirectories = Json::array();
		for (const auto& file : package.index.at("files")) directorySources.push_back(file.at("path"));
		for (const auto& name : DirectoryParents(directorySources))
			if (name != "imgs" && name != "lang" && name != "shaders" && name != "ui" && !std::filesystem::exists(Safe(name))) createdDirectories.push_back(name);
		ValidateDirectories(createdDirectories, directorySources, true);
		const std::string id = Id(), base = ".ksm-enhanced-backup/" + id;
		Json entries = Json::array(), operations = Json::array(); size_t number = 0;
		for (const auto& file : package.index.at("files"))
		{
			CheckCancel();
			const auto name = file.at("path").get<std::string>(), expected = file.at("sha256").get<std::string>();
			const auto path = Safe(name);
			const Json before = std::filesystem::exists(path) ? Json(FileHash(path)) : Json(nullptr);
			const bool owned = before != expected;
			const auto original = base + "/original/" + std::to_string(number) + ".bin";
			const auto stage = base + "/stage/" + std::to_string(number) + ".bin";
			entries.push_back(Json{{"path", name}, {"kind", Kind(name)}, {"owned", owned}, {"installedHash", expected}, {"originalHash", owned ? before : Json(nullptr)}, {"originalBackup", owned && !before.is_null() ? Json(original) : Json(nullptr)}});
			if (owned)
			{
				if (!before.is_null()) Write(Safe(original), Read(path));
				Write(Safe(stage), package.Extract(file));
				operations.push_back(Operation(name, before, before.is_null() ? Json(nullptr) : Json(original), expected, stage));
			}
			++number;
			ReportProgress(10 + int(50 * number / package.index.at("files").size()));
		}
		Json state{{"schemaVersion", 3}, {"modId", "ksm-enhanced-vertical"}, {"version", package.index.at("version")}, {"buildId", package.index.at("buildId")}, {"runtimeContract", kContract}, {"upstreamHash", FileHash(Safe("kshootmania.exe"))}, {"transaction", id}, {"files", entries}, {"createdDirectories", createdDirectories}};
		const auto stage = base + "/stage/" + std::to_string(number) + ".bin";
		const auto data = JsonBytes(state); Write(Safe(stage), data);
		operations.push_back(Operation(kState, nullptr, nullptr, Hash(data), stage));
		CheckIdle(); Transaction(operations, id);
		return Json{{"status", "PASS"}, {"action", "install"}, {"version", state["version"]}, {"files", entries.size()}, {"message", "Installed. Original executable, settings, songs, skins and all records preserved."}};
	}
	Json Engine::Verify()
	{
		CheckTarget(false); const auto state = LoadState();
		Need(FileHash(Safe("kshootmania.exe")) == Lower(state.at("upstreamHash")), "target", "Original game changed since installation");
		Json modified = Json::array();
		for (const auto& file : state.at("files"))
		{
			const auto path = Safe(file.at("path"));
			if (!std::filesystem::exists(path) || FileHash(path) != file.at("installedHash")) modified.push_back(file.at("path"));
		}
		return Json{{"status", "PASS"}, {"action", "verify"}, {"modifiedFiles", modified}, {"message", "Installation record and original backups verified. User modifications were not changed."}};
	}
	Json Engine::Remove()
	{
		const auto state = LoadState(); CheckIdle();
		uint64_t required = 16 * 1024 * 1024;
		for (const auto& file : state.at("files"))
		{
			if (!file.value("owned", true)) continue;
			const auto path = Safe(file.at("path"));
			if (std::filesystem::exists(path)) required += std::filesystem::file_size(path) * 3;
			if (!file.at("originalHash").is_null()) required += std::filesystem::file_size(Safe(file.at("originalBackup"))) * 2;
		}
		ULARGE_INTEGER available{};
		Win(GetDiskFreeSpaceExW(root.c_str(), &available, nullptr, nullptr), "Cannot inspect free space");
#ifdef KSM_INSTALLER_TEST
		if (package.index.value("fixture", false) && GetEnvironmentVariableW(L"KSM_SETUP_LOW_SPACE", nullptr, 0)) available.QuadPart = 0;
#endif
		Need(available.QuadPart >= required, "space", "Insufficient free space for removal and rollback");
		Need(FileHash(Safe("kshootmania.exe")) == Lower(state.at("upstreamHash")), "target", "Original game changed; shared-file restoration requires review");
		const std::string id = Id(), base = ".ksm-enhanced-backup/" + id, recovery = ".ksm-enhanced-recovery/" + id;
		Json operations = Json::array(), preserved = Json::array(), recovered = Json::array(); size_t number = 0, processed = 0;
		for (const auto& file : state.at("files"))
		{
			CheckCancel();
			ReportProgress(10 + int(50 * ++processed / state.at("files").size()));
			if (!file.value("owned", true)) continue;
			const auto name = file.at("path").get<std::string>();
			const auto path = Safe(name);
			const Json current = std::filesystem::exists(path) ? Json(FileHash(path)) : Json(nullptr);
			const bool changed = current != file.at("installedHash");
			if (file.at("kind") == "image" && (changed || file.value("retainOnRemove", false)))
			{
				preserved.push_back(name);
				if (!file.at("originalHash").is_null())
				{
					const auto destination = recovery + "/before-install/" + name;
					Write(Safe(destination), Read(Safe(file.at("originalBackup")))); recovered.push_back(destination);
				}
				continue;
			}
			Writable(path);
			if (changed && !current.is_null())
			{
				const auto destination = recovery + "/edited/" + name;
				Write(Safe(destination), Read(path)); recovered.push_back(destination);
			}
			const auto undo = base + "/undo/" + std::to_string(number) + ".bin";
			const auto stage = base + "/stage/" + std::to_string(number) + ".bin";
			if (!current.is_null()) Write(Safe(undo), Read(path));
			if (!file.at("originalHash").is_null()) Write(Safe(stage), Read(Safe(file.at("originalBackup"))));
			operations.push_back(Operation(name, current, current.is_null() ? Json(nullptr) : Json(undo), file.at("originalHash"), file.at("originalHash").is_null() ? Json(nullptr) : Json(stage)));
			++number;
		}
		const auto undo = base + "/undo/" + std::to_string(number) + ".bin";
		const auto before = Read(Safe(kState)); Write(Safe(undo), before);
		operations.push_back(Operation(kState, Hash(before), undo, nullptr, nullptr));
		Json directorySources = Json::array();
		for (const auto& file : state.at("files")) directorySources.push_back(file.at("path"));
		CheckIdle(); const auto cleanup = Transaction(operations, id, RemovalDirectories(state), directorySources);
		Json result{{"status", "PASS"}, {"action", "remove"}, {"preservedImages", preserved}, {"recoveryFiles", recovered}, {"recoveryDirectory", recovered.empty() ? "" : recovery}, {"message", "Mod removed. All records and settings preserved; edited images retained. Original language files restored."}};
		result.update(cleanup); return result;
	}
	Json Engine::Inspect()
	{
		CheckTarget(!std::filesystem::exists(Safe(kState)));
		if (std::filesystem::exists(Safe(kState)))
		{
			const auto state = LoadState();
			return Json{{"status", "PASS"}, {"action", "inspect"}, {"installed", true}, {"version", state.value("version", state.value("buildId", "legacy"))}, {"sameVersion", state.value("version", "") == package.index.at("version")}, {"target", Utf8(root.wstring())}};
		}
		Need(!std::filesystem::exists(Safe("ksm-enhanced-vertical.exe")), "manual_install", "Manual installation without ownership metadata detected");
		return Json{{"status", "PASS"}, {"action", "inspect"}, {"installed", false}, {"version", ""}, {"sameVersion", false}, {"target", Utf8(root.wstring())}};
	}
	Json Engine::Run(const std::string& action)
	{
		Need(action == "inspect" || action == "install" || action == "remove" || action == "verify", "arguments", "Unknown action");
		if (action == "inspect" && !std::filesystem::exists(Safe(kJournal))) return Inspect();
		CheckIdle();
		Handle lock(CreateFileW(Safe(".ksm-enhanced-install.lock").c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr));
		Win(lock.value != INVALID_HANDLE_VALUE, "Installer already running or target is not writable");
		Recover();
		Json result;
		if (!recoveredCleanup.is_null() && action == "remove") result = Json{{"status", "PASS"}, {"action", "remove"}, {"message", "Committed removal recovered; empty-directory cleanup completed."}};
		else if (action == "inspect") result = Inspect();
		else if (action == "install") result = Install();
		else if (action == "verify") result = Verify();
		else result = Remove();
		if (!recoveredCleanup.is_null()) { result.update(recoveredCleanup); result["cleanupRecovered"] = true; }
		return result;
	}
}
