#pragma once
#include <Siv3D.hpp>
#include <array>
#ifdef _WIN32
#include <Siv3D/Windows/Windows.hpp>
#endif
#include "ConfigIni.hpp"
#include "Common/FsUtils.hpp"
#include "ksmaudio/ksmaudio.hpp"
namespace AudioCalibration {
inline constexpr std::array<StringView, 6> Keys{ConfigIni::Key::kGlobalOffset, ConfigIni::Key::kInputDelay,
    ConfigIni::Key::kLaserInputDelay, ConfigIni::Key::kAudioProcDelay, ConfigIni::Key::kVisualOffset, ConfigIni::Key::kAutoSync};
using Values = std::array<int32, Keys.size()>;
inline JSON bank;
inline String active;
inline Values Read() { Values v{}; for (size_t i=0;i<Keys.size();++i) v[i]=ConfigIni::GetInt(Keys[i]); return v; }
inline String Key() {
    const auto state = ksmaudio::GetAudioLatencySnapshot();
    if (state.output.backend != ksmaudio::AudioBackend::ASIO) return U"legacy";
    return U"asio|{}|0,1|{}|{}"_fmt(Unicode::FromUTF8(state.output.driver), state.output.sampleRate, state.output.bufferSamples);
}
inline FilePath Path() { return FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(), U"audio_calibration_profiles.json"); }
inline bool Save() {
    const FilePath temporary = Path() + U".tmp";
    if (!bank.save(temporary)) { Logger << U"[audio] Cannot save calibration profiles"; return false; }
#ifdef _WIN32
    // Same-directory atomic replacement; a failed replacement keeps the previous file.
    if (!::MoveFileExW(temporary.toWstr().c_str(), Path().toWstr().c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        Logger << U"[audio] Cannot replace calibration profiles"; return false;
    }
#else
    if (!FileSystem::Rename(temporary, Path())) return false;
#endif
    return true;
}
inline void Put(const String& key, const Values& values) {
    for(size_t i=0;i<Keys.size();++i) bank[U"profiles"][key][Keys[i]]=values[i];
}
inline bool ValidProfile(const String& key) {
    const auto profile=bank[U"profiles"][key];
    if(!profile.isObject()) return false;
    for(const auto field:Keys) if(!profile[field].getOpt<int32>()) return false;
    return true;
}
inline void Initialize() {
    bank=JSON::Load(Path());
    if (!bank || bank[U"version"].getOr<int32>(0)!=1 || !bank[U"profiles"].isObject()) {
        bank=JSON{}; bank[U"version"]=1;
    }
    active=Key();
    const auto lastActive=bank[U"active_profile"].getOr<String>(U"");
    const auto current=Read();
    if(!lastActive.isEmpty() && ValidProfile(lastActive)) Put(lastActive,current);
    if(active != lastActive && ValidProfile(active)) {
        for(size_t i=0;i<Keys.size();++i) ConfigIni::SetInt(Keys[i],bank[U"profiles"][active][Keys[i]].get<int32>());
    } else Put(active,current);
    if(!ValidProfile(U"legacy")) Put(U"legacy",current);
    bank[U"active_profile"]=active;
    Save();
}
inline void Store() { if(!active.isEmpty()) { Put(active,Read()); bank[U"active_profile"]=active; Save(); } }
inline void ActivateKey(const String& key) {
    const auto seed=Read(); active=key; bank[U"active_profile"]=active;
    if(!ValidProfile(active)) Put(active,seed);
    for(size_t i=0;i<Keys.size();++i) ConfigIni::SetInt(Keys[i],bank[U"profiles"][active][Keys[i]].get<int32>());
    Save();
}
inline void Activate() { ActivateKey(Key()); }
// Deliberately gated: never run destructive fixture tests in a normal installation.
inline void RunFixtureTests() {
    if(!FileSystem::Exists(FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"audio-test-fixture")))
        throw Error{U"Calibration tests require an isolated audio-test-fixture directory"};
    TextWriter report{FileSystem::PathAppend(FsUtils::AppDataDirectoryPath(),U"audio-calibration-test.txt")};
    size_t checks=0;
    const auto check=[&](bool condition, StringView name) { if(!condition) throw Error{String{name}}; ++checks; report.writeln(U"PASS " + String{name}); };
    const auto original=Read();
    const Values legacy{11,12,13,14,15,0}, asio{21,22,23,24,25,1}, other{31,32,33,34,35,0};
    Put(U"legacy",legacy);Put(U"asio|fixture|0,1|44100|256",asio);Put(U"asio|fixture|0,1|48000|128",other);
    ActivateKey(U"legacy");check(Read()==legacy,U"native shared calibration restored");
    ActivateKey(U"asio|fixture|0,1|44100|256");check(Read()==asio,U"ASIO calibration restored");
    ActivateKey(U"asio|fixture|0,1|48000|128");check(Read()==other,U"ASIO rate/buffer profile isolated");
    ActivateKey(U"asio|fixture-new|0,1|44100|256");check(Read()==other,U"new ASIO profile preserves current values");
    check(Save(),U"atomic profile save");const auto loaded=JSON::Load(Path());
    check(loaded && loaded[U"version"].getOr<int32>(0)==1,U"versioned file reloads");
    check(loaded[U"profiles"][U"legacy"][Keys[0]].getOr<int32>(-1)==11,U"legacy values survive disk save");
    Put(U"legacy",legacy);bank[U"active_profile"]=U"asio|fixture|0,1|44100|256";Save();
    for(size_t i=0;i<Keys.size();++i) ConfigIni::SetInt(Keys[i],asio[i]);
    Initialize();check(Read()==legacy,U"startup fallback restores native profile instead of ASIO offsets");
    TextWriter{Path()}.write(U"{ broken JSON");
    for(size_t i=0;i<Keys.size();++i) ConfigIni::SetInt(Keys[i],original[i]);
    Initialize();check(Read()==original,U"corrupt profile file preserves current config");
    check(ValidProfile(U"legacy"),U"corrupt profile file rebuilt");
    TextWriter{Path()}.write(U"{\"version\":99}");Initialize();
    check(Read()==original,U"unknown version preserves current config");
    FileSystem::Remove(Path());Initialize();check(Read()==original,U"missing profile file preserves current config");
    const auto nativeBefore=bank[U"profiles"][U"legacy"][Keys[0]].get<int32>();
    FileSystem::CreateDirectories(Path()+U".tmp");
    check(!Save(),U"temporary path failure is reported without replacing saved profiles");
    check(JSON::Load(Path())[U"profiles"][U"legacy"][Keys[0]].get<int32>()==nativeBefore,U"failed save preserves previous profile file");
    FileSystem::Remove(Path()+U".tmp");check(Save(),U"profile save recovers after temporary failure");
    report.writeln(U"TOTAL {} checks passed"_fmt(checks));
}
}
