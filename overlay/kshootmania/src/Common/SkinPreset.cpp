#include "SkinPreset.hpp"
#include "I18n/I18n.hpp"
#include <cstdio>
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif

namespace SkinPreset
{
	namespace
	{
		Settings g_currentSettings;
		HashTable<String, FilePath> g_resolvedImagePaths;
		Array<String> g_registeredTextureNames;
		template <class T>
		T Read(const JSON& json, StringView key, const T& fallback)
		{
			return json.isObject() && json.hasElement(key) ? json[key].getOr<T>(fallback) : fallback;
		}
		FilePath GetBaseImageDirectory()
		{
			return FsUtils::GetResourcePath(U"imgs/");
		}
		FilePath GetUserSkinDirectory()
		{
			return FsUtils::GetResourcePath(U"skins/");
		}
		FilePath GetSettingsFilePath()
		{
			return FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"skin_preset.json");
		}
		bool IsImage(FilePathView path)
		{
			const auto ext = FileSystem::Extension(path).lowercased();
			return ext == U"png" || ext == U"gif" || ext == U"jpg" || ext == U"jpeg" || ext == U"bmp" || ext == U"webp";
		}
		String GetLogicalImagePath(FilePathView input)
		{
			String path{ input };
			path.replace(U'\\', U'/');
			if (path.starts_with(U"imgs/"))
				return path.substr(5);
			const auto root = GetBaseImageDirectory();
			if (path.starts_with(root))
				return path.substr(root.size());
			return path;
		}
		FilePath CreateMissingImagePlaceholder(FilePathView name, FilePathView base)
		{
			// Keep sprite-sheet geometry; a zero-sized Texture would divide tiles by zero.
			const auto info = ImageDecoder::GetImageInfo(base);
			const Size size = info ? info->size : Size{ 32, 32 };
			const auto dir = FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"cache/skin-errors/");
			FileSystem::CreateDirectories(dir);
			const auto path = FileSystem::PathAppend(dir, Format(size.x, U"x", size.y, U".png"));
			if (!FileSystem::IsFile(path) && !Image{ size, Palette::Yellow }.save(path))
				throw Error{ U"Cannot create missing-image placeholder: " + String{ name } };
			return path;
		}
	}

	Array<Entry> Discover()
	{
		Array<Entry> result;
		const auto addEntry = [&](const String& name, FilePathView path)
		{
			Entry entry{ name, U"", U"" };
			const auto metaPath = FileSystem::PathAppend(path, U"skin.json");
			if (FileSystem::IsFile(metaPath))
			{
				const auto meta = JSON::Load(metaPath);
				if (meta && meta.isObject())
				{
					entry.author = Read<String>(meta, U"author", U"");
					entry.description = Read<String>(meta, U"description", U"");
				}
				else
					entry.description = I18n::Get(I18n::Mod::SkinReadError);
			}
			result.push_back(std::move(entry));
		};
		addEntry(U"default", GetBaseImageDirectory());
		if (FileSystem::IsDirectory(GetUserSkinDirectory()))
			for (const auto& path : FileSystem::DirectoryContents(GetUserSkinDirectory(), Recursive::No))
			{
				if (!FileSystem::IsDirectory(path))
					continue;
				const String name = FsUtils::DirectoryNameByDirectoryPath(path);
				if (name.lowercased() != U"default")
					addEntry(name, path);
			}
		result.sort_by([](const Entry& a, const Entry& b)
					   {
						   if (a.folder == U"default" || b.folder == U"default")
							   return a.folder == U"default" && b.folder != U"default";
						   const auto al = a.folder.lowercased(), bl = b.folder.lowercased();
						   return al == bl ? a.folder < b.folder : al < bl;
					   });
		return result;
	}

	const Settings& Current()
	{
		return g_currentSettings;
	}

	void Settings::sanitize()
	{
		background1Brightness = Clamp(background1Brightness, 0, 100);
		background2Opacity = Clamp(background2Opacity, 0, 100);
		laserWidthPercent = std::isfinite(laserWidthPercent) ? Clamp(laserWidthPercent, 50.0, 125.0) : 100.0;
		noteHeightPercent = Clamp(noteHeightPercent, 50, 200);
		indicatorPortrait = static_cast<IndicatorPortrait>(Clamp(static_cast<int32>(indicatorPortrait), 0, 2));
		keyBeamMode = static_cast<KeyBeamMode>(Clamp(static_cast<int32>(keyBeamMode), 0, 5));
	}

	double StepLaserPercent(double value, int32 direction)
	{
		if (direction == 0)
			return value;
		if (Abs(value - kVerticalLaserPercent) < 0.00001)
			return direction > 0 ? 75.0 : 70.0;
		if (direction > 0)
		{
			if (value < kVerticalLaserPercent && value + 5.0 > kVerticalLaserPercent)
				return kVerticalLaserPercent;
			return Min(125.0, value + 5.0);
		}
		if (value > kVerticalLaserPercent && value - 5.0 < kVerticalLaserPercent)
			return kVerticalLaserPercent;
		return Max(50.0, value - 5.0);
	}

	void Initialize()
	{
		if (!FileSystem::IsDirectory(GetBaseImageDirectory()))
			throw Error{ I18n::Get(I18n::Mod::DefaultSkinMissing, U"imgs") };
		const auto json = JSON::Load(GetSettingsFilePath());
		Settings settings;
		if (json)
		{
			settings.folder = Read<String>(json, U"folder", U"default");
			settings.missing = Read<String>(json, U"missing", U"default") == U"error" ? MissingMode::Error : MissingMode::Default;
			settings.background1Brightness = Read<int32>(json, U"background1Brightness", 100);
			settings.background2Opacity = Read<int32>(json, U"background2Opacity", 100);
			settings.laserWidthPercent = Read<double>(json, U"laserWidthPercent", 100.0);
			settings.noteHeightPercent = Read<int32>(json, U"noteHeightPercent", 100);
			settings.indicatorPortrait = static_cast<IndicatorPortrait>(Read<int32>(json, U"indicatorPortrait", 1));
			settings.indicatorLandscape = Read<bool>(json, U"indicatorLandscape", true);
			settings.keyBeamMode = static_cast<KeyBeamMode>(Read<int32>(json, U"keyBeamMode", 1));
		}
		settings.sanitize();
		const auto entries = Discover();
		if (!entries.any([&](const Entry& e)
						 {
							 return e.folder == settings.folder;
						 }))
			settings.folder = U"default";
		g_currentSettings = settings;
		g_resolvedImagePaths.clear();
		noco::Asset::SetTextureLoader([](FilePathView path, noco::MipmapEnabledYN mip)
									  {
										  String normalized{ path };
										  normalized.replace(U'\\', U'/');
										  if (normalized.starts_with(U"imgs/"))
											  return LoadTexture(path, mip ? TextureDesc::Mipped : TextureDesc::Unmipped);
										  return Texture{ noco::Asset::GetFullPath(path), mip ? TextureDesc::Mipped : TextureDesc::Unmipped };
									  });
	}

	FilePath Resolve(FilePathView logicalPath)
	{
		const String name = GetLogicalImagePath(logicalPath);
		if (const auto it = g_resolvedImagePaths.find(name); it != g_resolvedImagePaths.end())
			return it->second;
		const auto base = FileSystem::PathAppend(GetBaseImageDirectory(), name);
		const auto selected = g_currentSettings.folder == U"default" ? base : FileSystem::PathAppend(GetUserSkinDirectory(), g_currentSettings.folder + U"/" + name);
		// Unknown legacy background/icon names retain the original caller's fallback.
		if (!FileSystem::IsFile(base) && !FileSystem::IsFile(selected))
			return selected;
		FilePath path;
		// Decode validation also treats corrupt images as missing, once per selection.
		if (FileSystem::IsFile(selected) && !Image{ selected }.isEmpty())
			path = selected;
		else if (g_currentSettings.missing == MissingMode::Default || g_currentSettings.folder == U"default")
		{
			if (!FileSystem::IsFile(base) || Image{ base }.isEmpty())
				throw Error{ I18n::Get(I18n::Mod::SkinImageUnreadable, name) };
			path = base;
		}
		else
			path = CreateMissingImagePlaceholder(name, base);
		g_resolvedImagePaths[name] = path;
		return path;
	}

	Texture LoadTexture(FilePathView logicalPath, TextureDesc desc)
	{
		return Texture{ Resolve(logicalPath), desc };
	}

	void RegisterTextures()
	{
		for (const auto& key : g_registeredTextureNames)
			TextureAsset::Unregister(key);
		g_registeredTextureNames.clear();
		HashSet<String> names;
		for (const auto& root : { GetBaseImageDirectory(), g_currentSettings.folder == U"default" ? GetBaseImageDirectory() : FileSystem::PathAppend(GetUserSkinDirectory(), g_currentSettings.folder + U"/") })
		{
			if (!FileSystem::IsDirectory(root))
				continue;
			for (const auto& path : FileSystem::DirectoryContents(root, Recursive::Yes))
				if (FileSystem::IsFile(path) && IsImage(path))
				{
					const auto name = FileSystem::RelativePath(path, root);
					if (name != U"preview.png")
						names.insert(name);
				}
		}
		for (const auto& name : names)
		{
			auto data = std::make_unique<TextureAssetData>();
			data->onLoad = [name](TextureAssetData& asset, const String&)
			{
				asset.texture = LoadTexture(name);
				return !asset.texture.isEmpty();
			};
			TextureAsset::Register(name, std::move(data));
			g_registeredTextureNames.push_back(name);
		}
	}

	void Activate(const Settings& settings)
	{
		const bool imagesChanged = g_currentSettings.folder != settings.folder || g_currentSettings.missing != settings.missing;
		g_currentSettings = settings;
		g_currentSettings.sanitize();
		if (!imagesChanged)
			return; // Numeric edits never re-register textures or disturb playback.
		g_resolvedImagePaths.clear();
		noco::Asset::UnloadAllTextures();
		RegisterTextures();
	}

	bool Save(const Settings& settings)
	{
		auto clean = settings;
		clean.sanitize();
		JSON json;
		json[U"folder"] = settings.folder;
		json[U"missing"] = settings.missing == MissingMode::Error ? U"error" : U"default";
		json[U"background1Brightness"] = clean.background1Brightness;
		json[U"background2Opacity"] = clean.background2Opacity;
		json[U"laserWidthPercent"] = clean.laserWidthPercent;
		json[U"noteHeightPercent"] = clean.noteHeightPercent;
		json[U"indicatorPortrait"] = static_cast<int32>(clean.indicatorPortrait);
		json[U"indicatorLandscape"] = clean.indicatorLandscape;
		json[U"keyBeamMode"] = static_cast<int32>(clean.keyBeamMode);
		const auto path = GetSettingsFilePath(), temp = path + U".tmp";
		if (!json.save(temp))
			return false;
		// Same-directory atomic replacement; preserve the previous settings on failure.
#ifdef _WIN32
		return MoveFileExW(temp.toWstr().c_str(), path.toWstr().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
		return std::rename(temp.toUTF8().c_str(), path.toUTF8().c_str()) == 0;
#endif
	}
}
