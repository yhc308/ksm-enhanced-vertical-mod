#pragma once
#include "Scenes/Title/TitleMenu.hpp"
#include "Vertical/OptionsPresentation.hpp"
namespace EnhancedMenuFixture {
inline int stage=0,count=0;inline bool passed=true;inline Stopwatch clock{StartImmediately::Yes};
inline bool Active(){
 if(!System::GetCommandLineArgs().contains(U"--enhanced-menu-test"))return false;
 if(!FileSystem::IsFile(FsUtils::GetResourcePath(U"enhanced-menu-fixture")))throw Error{U"Menu test requires an isolated fixture"};
 return true;
}
inline void Record(StringView name,bool ok){++count;passed=passed&&ok;TextWriter{FsUtils::GetResourcePath(U"enhanced-menu.csv"),OpenMode::Append}.writeln(U"{},{}"_fmt(name,ok?U"PASS":U"FAIL"));}
inline void Advance(){++stage;clock.restart();}
inline void Title(TitleMenu& menu){
 if(!Active()||clock.sF()<.25)return;
 const Array<TitleMenuItem> expected{TitleMenuItem::kStart,TitleMenuItem::kCustom,TitleMenuItem::kVerticalOptions,TitleMenuItem::kCustom,TitleMenuItem::kStart,TitleMenuItem::kOption,TitleMenuItem::kInputGate,TitleMenuItem::kExit,TitleMenuItem::kInputGate,TitleMenuItem::kOption,TitleMenuItem::kStart,TitleMenuItem::kCustom};
 if(stage<static_cast<int>(expected.size())){
  Record(U"navigation_{}"_fmt(stage),menu.focused(expected[stage]));
  if(stage==0){ScreenCapture::SetScreenshotDirectory(FsUtils::AppDataDirectoryPath());ScreenCapture::SaveCurrentFrame(U"title-default.png");}
  if(stage==1)ScreenCapture::SaveCurrentFrame(U"title-custom.png");
  if(stage==2)ScreenCapture::SaveCurrentFrame(U"title-vertical.png");
  KeyConfig::RequestVirtualDown(stage==11?kButtonStart:(stage<2||stage>=7)?kButtonBT_C:kButtonBT_B);
  Advance();return;
 }
 if(stage==12){Record(U"custom_closed_to_title",menu.focused(TitleMenuItem::kCustom));KeyConfig::RequestVirtualDown(kButtonBT_C);Advance();return;}
 if(stage==13){Record(U"vertical_from_custom",menu.focused(TitleMenuItem::kVerticalOptions));KeyConfig::RequestVirtualDown(kButtonStart);Advance();return;}
 if(stage==16){Record(U"return_focus_vertical",menu.focused(TitleMenuItem::kVerticalOptions));Record(U"option_music_released",!Vertical::OptionsPresentation::Music());Record(U"overall",passed);TextWriter{FsUtils::GetResourcePath(U"enhanced-menu-passed.txt")}.writeln(U"TOTAL {} checks {}"_fmt(count,passed?U"PASS":U"FAIL"));System::Exit();}
}
inline bool Options(){
 if(!Active())return false;
 if(stage==14 && clock.sF()>.7){
  Record(U"main_title_backdrop",static_cast<bool>(Vertical::OptionsPresentation::Backdrop()));
  Record(U"option_bgm_playing",Vertical::OptionsPresentation::Music()&&Vertical::OptionsPresentation::Music()->isPlaying());
  ScreenCapture::SaveCurrentFrame(U"vertical-options-overlay.png");Advance();return false;
 }
 if(stage==15&&clock.sF()>.3){Advance();return true;}
 return false;
}
}
