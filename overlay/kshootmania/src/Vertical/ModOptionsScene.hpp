#pragma once
#include "VerticalDisplay.hpp"
#include "OptionsPresentation.hpp"
#include "MenuContentLayout.hpp"
#include "Diagnostics/EnhancedMenuFixture.hpp"
#include "Scenes/Title/TitleScene.hpp"
#include "Scenes/Option/OptionScene.hpp"
#include "Input/InputUtils.hpp"
#include "Input/MenuInputProfiles.hpp"
#include "OptionFlowTest.hpp"
#include "MenuEntryGate.hpp"
#include "UI/ModMenuStyle.hpp"
#include "Scenes/Common/CommonOverlay.hpp"

class ModOptionsScene : public Co::UpdaterSceneBase
{
    enum Section {Root=0,Graphics=1,Developer=2,DepthImages=3};
    int m_section;
    Vertical::ModConfig m_config=Vertical::ModConfig::Load();
    CommonOverlay m_back;
    LinearMenu m_menu;
    CursorInput m_values{MenuInputProfiles::OptionValues()};
    String m_message;
    int m_editIndex=-1;
    String m_digits;
    bool m_replaceOnType=false;
    Vertical::MenuEntryGate m_entryGate;
    Stopwatch m_entryClock{StartImmediately::Yes};
    static int Count(int section) {return section==DepthImages ? 9 : section==Graphics ? 2 : section==Developer ? 6 : (Vertical::OptionProbe::active()?4:3);}
    RectF rowRect(int i) const {
        const double h=m_section==DepthImages ? Min(Vertical::MenuContent::Rect().h*.58/9,58.0) : Min(Vertical::MenuContent::Rect().h/12.0,58.0);
        return {Scene::Width()*.06,(Scene::Height()>Scene::Width()?Vertical::MenuContent::Rect().y+Vertical::MenuContent::Rect().h*.14:Scene::Height()*.2)+i*h,Scene::Width()*.88,h-4};
    }
    RectF resetRect(int i) const {const auto r=rowRect(i);return {r.x+r.w*.77,r.y+3,r.w*.22,r.h-6};}
    void saveGraphics() {m_config.Save();Vertical::SaveOptionsConfiguration();}
    void resetImage(int i) {if(!m_config.backgroundDepthEnabled)return;m_config.ResetDepthImage(i);saveGraphics();}
    void resetAllImages() {if(!m_config.backgroundDepthEnabled)return;m_config.ResetDepthImages();saveGraphics();}
    void beginNumber(int index) {
        if(!m_config.backgroundDepthEnabled || index<0 || index>=8)return;
        m_editIndex=index;m_digits=Format(m_config.depthIntensities[index]);m_replaceOnType=true;m_message.clear();
    }
    void cancelNumber() {m_editIndex=-1;m_digits.clear();m_message.clear();}
    bool commitNumber() {
        const auto value=Vertical::ParseDepthPercent(m_digits);
        if(!value || m_editIndex<0){m_message=I18n::Get(I18n::Mod::InvalidPercent);return false;}
        m_config.depthIntensities[m_editIndex]=*value;saveGraphics();cancelNumber();return true;
    }
    void feedNumber(StringView text) {
        for(char32 c:text) {
            if(c>=U'0' && c<=U'9') {if(m_replaceOnType)m_digits.clear();m_replaceOnType=false;if(m_digits.size()<3)m_digits.push_back(c);}
            else if(c==U'\b') {if(m_replaceOnType)m_digits.clear();else if(!m_digits.empty())m_digits.pop_back();m_replaceOnType=false;}
        }
    }
    RectF numberPanel() const {const auto area=Vertical::MenuContent::Rect();return {Scene::Width()*.08,area.y+area.h*.32,Scene::Width()*.84,area.h*.30};}
    RectF hintRect() const {if(Scene::Height()>Scene::Width()){const auto area=Vertical::MenuContent::Rect();const auto p=Vertical::MenuContent::Scale();return {Scene::Width()*.06,area.br().y-110*p,Scene::Width()*.88,90*p};}return {Scene::Width()*.06,Scene::Height()*.83,Scene::Width()*.88,Scene::Height()*.15};}
    RectF numberButton(bool apply) const {const auto r=numberPanel();return {r.x+r.w*(apply?.05:.53),r.y+r.h*.65,r.w*.42,r.h*.22};}
    void returnRoot(int cursor) {requestNextScene<ModOptionsScene>(Root,cursor);}
    void saveDeveloper() {
        const auto previous=Vertical::ModConfig::Load();
        if(previous.monitorId!=m_config.monitorId) {
            if(!Vertical::BeginDisplayChange(m_config)) {m_message=I18n::Get(I18n::Mod::DisplayFailed);return;}
        } else {m_config.Save();Vertical::SaveOptionsConfiguration();}
        returnRoot(Vertical::OptionProbe::active()?3:2);
    }
    void back() {
        if(m_editIndex>=0){cancelNumber();return;}
        if(m_section==Root)requestNextScene<TitleScene>(TitleMenuItem::kVerticalOptions);
        else if(m_section==Graphics){saveGraphics();returnRoot(2);}
        else if(m_section==DepthImages){saveGraphics();requestNextScene<ModOptionsScene>(Graphics,1);}
        else saveDeveloper();
    }
    void toggleMode() {
        auto next=Vertical::ModConfig::Load();next.enabled=!next.enabled;
        if(!Vertical::BeginDisplayChange(next)) m_message=I18n::Get(I18n::Mod::DisplayFailed);
    }
    void edit(int delta) {
        if(m_section==Root) {if(m_menu.cursor()==0) toggleMode();return;}
        if(m_section==Graphics) {if(m_menu.cursor()==0){m_config.backgroundDepthEnabled=!m_config.backgroundDepthEnabled;saveGraphics();}return;}
        if(m_section==DepthImages) {
            if(m_config.backgroundDepthEnabled && m_menu.cursor()<8)m_config.depthIntensities[m_menu.cursor()]=std::clamp(m_config.depthIntensities[m_menu.cursor()]+(delta<0?-1:1),0,200);
            return;
        }
        const int direction=delta<0 ? -1 : 1;
        switch(m_menu.cursor()) {
        case 0: {
            const auto monitors=System::EnumerateMonitors();if(monitors.isEmpty()) break;
            int index=0;for(int i=0;i<static_cast<int>(monitors.size());++i) if(monitors[i].id==m_config.monitorId) index=i;
            m_config.monitorId=monitors[(index+direction+static_cast<int>(monitors.size()))%static_cast<int>(monitors.size())].id;
            break;
        }
        case 1:m_config.layout.laneWidth+=direction*.01;break;
        case 2:m_config.layout.judgmentLineY+=direction*.01;break;
        case 3:m_config.ResetLayout();break;
        default:break;
        }
        m_config.layout.sanitize();
    }
    void decide() {
        if(m_section==Root) {
            switch(m_menu.cursor()) {
            case 0:toggleMode();break;
            case 1:Vertical::OptionsPresentation::Pause();requestNextScene<OptionScene>(true);break;
            case 2:requestNextScene<ModOptionsScene>(Vertical::OptionProbe::active()?Graphics:Developer,0);break;
            case 3:requestNextScene<ModOptionsScene>(Developer,0);break;
            }
        } else if(m_section==Graphics) {
            if(m_menu.cursor()==0)edit(1);
            else if(m_config.backgroundDepthEnabled){saveGraphics();requestNextScene<ModOptionsScene>(DepthImages,0);}
        } else if(m_section==DepthImages) {
            if(m_menu.cursor()==8)resetAllImages();else beginNumber(m_menu.cursor());
        }
        else if(m_menu.cursor()==4) saveDeveloper();
        else if(m_menu.cursor()==5) returnRoot(Vertical::OptionProbe::active()?3:2);
        else if(m_menu.cursor()==3) edit(1);
    }
    bool probe() {
        using namespace Vertical::OptionProbe;
        if(timer.sF()<.2 && stage!=18 && stage!=19) return true;
        if(m_section==Root) {
            switch(stage) {
            case 0: record(U"root_four_items",Count(Root)==4);m_menu.setCursor(2);advance(1);decide();break;
            case 2: record(U"graphics_back_saved_off",!m_config.backgroundDepthEnabled);m_menu.setCursor(1);advance(3);decide();break;
            case 4: record(U"display_returns_to_shortcut",m_menu.cursor()==1);m_menu.setCursor(3);advance(5);decide();break;
            case 14: record(U"developer_discard_keeps_layout",Abs(m_config.layout.laneWidth-Vertical::kDefaultLaneWidth)<1e-10);m_menu.setCursor(3);advance(15);decide();break;
            case 6: record(U"reset_keeps_graphics_choice",!m_config.backgroundDepthEnabled && Abs(m_config.layout.laneWidth-Vertical::kDefaultLaneWidth)<1e-10);toggleMode();advance(7);break;
            case 7: record(U"mode_off_confirmed",!m_config.enabled && Window::GetState().focused);record(U"mode_off_restores_landscape",Scene::Width()>Scene::Height());toggleMode();advance(9);break;
            case 9: record(U"mode_cancel_restores_off",!m_config.enabled && Window::GetState().focused);record(U"mode_cancel_preserves_landscape",Scene::Width()>Scene::Height());toggleMode();advance(11);break;
            case 11: record(U"mode_on_confirmed",m_config.enabled && Window::GetState().focused);m_menu.setCursor(2);advance(12);decide();break;
            }
        } else if(m_section==Graphics) {
            if(stage==1) {record(U"graphics_two_items",Count(Graphics)==2);record(U"depth_default_on",m_config.backgroundDepthEnabled);edit(1);
                m_menu.setCursor(1);decide();beginNumber(0);record(U"depth_off_blocks_entry_and_editor",!m_config.backgroundDepthEnabled && m_editIndex<0);record(U"toggle_saved_immediately",!Vertical::ModConfig::Load().backgroundDepthEnabled);m_menu.setCursor(0);m_config.Save();Vertical::SaveOptionsConfiguration();advance(2);returnRoot(2);}
            else if(stage==12) {m_menu.setCursor(0);edit(1);saveGraphics();record(U"graphics_restored_on",m_config.backgroundDepthEnabled);m_menu.setCursor(1);advance(18);decide();}
            else if(stage==17) {record(U"depth_back_keeps_on_and_defaults",m_config.backgroundDepthEnabled && m_config.depthIntensities==Vertical::kDepthImageDefaults);record(U"overall",passed);advance(13);}
        } else if(m_section==DepthImages && stage==18) {
            // Exercise the actual update gate under repeated opening Down events.
            if(timer.sF()<.35)return false;
            record(U"actual_scene_blocks_opening_enter_repeats",m_editIndex<0);
            advance(19);KeyConfig::RequestVirtualDown(kButtonStart);return true;
        } else if(m_section==DepthImages && stage==19) {
            if(timer.sF()<.15)return false;
            record(U"fresh_enter_after_release_opens_editor",m_editIndex==0);
            cancelNumber();advance(16);return true;
        } else if(m_section==DepthImages && stage==16) {
            record(U"depth_image_rows_and_reset_all",Count(DepthImages)==9);
            Vertical::MenuEntryGate gate;
            bool gateOk=!gate.accept(0,true,true) && !gate.accept(.2,true,false) && !gate.accept(.3,true,true)
                && !gate.accept(.4,false,false) && gate.accept(.41,true,true);
            record(U"enter_hold_repeat_blocked_until_release",gateOk);
            record(U"depth_scene_does_not_open_editor_on_entry",m_editIndex<0);
            beginNumber(0);m_digits=U"201";record(U"reject_201",!commitNumber() && m_editIndex==0 && m_config.depthIntensities[0]==100);
            m_digits=U"200";record(U"accept_200",commitNumber() && Vertical::ModConfig::Load().depthIntensities[0]==200);
            beginNumber(0);m_digits=U"0";record(U"accept_zero",commitNumber() && m_config.depthIntensities[0]==0);
            m_config.depthIntensities[0]=123;m_config.depthIntensities[5]=99;resetImage(5);
            record(U"individual_reset_preserves_other_image",m_config.depthIntensities[0]==123 && m_config.depthIntensities[5]==35);
            beginNumber(5);m_digits=U"120";cancelNumber();record(U"cancel_keeps_previous",m_config.depthIntensities[5]==35);
            resetAllImages();record(U"all_reset_preserves_toggle",m_config.depthIntensities==Vertical::kDepthImageDefaults && m_config.backgroundDepthEnabled);
            advance(17);requestNextScene<ModOptionsScene>(Graphics,1);
        } else if(m_section==Developer && stage==5) {
            record(U"developer_has_existing_controls",Count(Developer)==6);m_menu.setCursor(1);edit(1);m_menu.setCursor(5);advance(14);decide();
        }
        if(m_section==Developer && stage==15) {m_menu.setCursor(1);edit(1);m_menu.setCursor(3);edit(1);advance(6);saveDeveloper();}
        return true;
    }
public:
    explicit ModOptionsScene(int section=Root,int cursor=0):m_section(section),m_menu(LinearMenu::CreateInfoWithEnumCount{
        .cursorInputCreateInfo=MenuInputProfiles::OptionRows(),.enumCount=Count(section),.defaultCursor=cursor}) {Vertical::OptionsPresentation::Begin();if(System::GetCommandLineArgs().contains(U"--vertical-preview-number"))beginNumber(0);}
    void update() override {
        m_back.setModScreenLayout();
        const bool mouseBack=m_back.update(noco::HitTestEnabledYN{InputUtils::IsUIMouseInputEnabled()&&!Vertical::IsDisplayChangePending()},false);
        if(Vertical::IsDisplayChangePending()) return;
        if(EnhancedMenuFixture::Active()){if(EnhancedMenuFixture::Options()&&EnhancedMenuFixture::stage==16)requestNextScene<TitleScene>(TitleMenuItem::kVerticalOptions);return;}
        if(m_section==Root) m_config=Vertical::ModConfig::Load();
        if(Vertical::OptionProbe::active() && probe()) return;
        if(!m_entryGate.accept(m_entryClock.sF(),KeyConfig::Pressed(kButtonStart)||KeyEnter.pressed()||MouseL.pressed(),KeyConfig::Down(kButtonStart)||KeyEnter.down()||MouseL.down()))return;
        if(mouseBack){back();return;}
        if(m_editIndex>=0) {
            feedNumber(TextInput::GetRawInput());
            if(KeyEnter.down() || numberButton(true).leftClicked())commitNumber();
            else if(KeyConfig::Down(kButtonBack) || KeyEscape.down() || numberButton(false).leftClicked())cancelNumber();
            return;
        }
        m_menu.update();m_values.update();
        if(m_section==Graphics && !m_config.backgroundDepthEnabled && m_menu.cursor()==1)m_menu.setCursor(0);
        if(m_section==DepthImages && KeyDelete.down() && m_menu.cursor()<8){resetImage(m_menu.cursor());return;}
        bool decided=KeyConfig::Down(kButtonStart);
        for(int i=0;i<Count(m_section);++i) if(InputUtils::IsUIMouseInputEnabled() && rowRect(i).leftClicked()) {
            if(m_section==Graphics && i==1 && !m_config.backgroundDepthEnabled)continue;
            if(m_section==DepthImages && !m_config.backgroundDepthEnabled)continue;
            m_menu.setCursor(i);
            if(m_section==DepthImages && i<8 && resetRect(i).leftClicked()){resetImage(i);return;}
            decided=true;
        }
        if(m_values.deltaCursor()!=0) edit(m_values.deltaCursor());
        else if(decided) decide();
        if(KeyConfig::Down(kButtonBack))back();
    }
    void draw() const override {
        Vertical::OptionsPresentation::Draw();
        const auto font=AssetManagement::SystemFont();const double size=Min(Scene::Width()/27.0,24.0);
        const String title=m_section==DepthImages ? U"DEPTH IMAGE SETTINGS" : m_section==Graphics ? U"GRAPHICS OPTIONS" : m_section==Developer ? U"DEVELOPER OPTIONS" : U"VERTICAL OPTION";
        ModMenuStyle::Title(title,ModMenuStyle::TitlePosition(),Scene::Width()*.48);
        m_back.draw();
        Array<String> labels;
        if(m_section==Root) labels={
            U"1 - Vertical Mode: "+String{m_config.enabled ? U"ON" : U"OFF"},
            U"2 - Display Options",U"3 - Graphics Options",U"99 - Developer Options"};
        else if(m_section==Graphics) labels={U"Background Depth Effect: "+String{m_config.backgroundDepthEnabled ? U"ON" : U"OFF"},U"Depth Image Settings"};
        else if(m_section==DepthImages) {
            for(size_t i=0;i<8;++i)labels.push_back(U"{}: {}%"_fmt(i==7 ? String{U"Custom / Other"} : String{Vertical::kDepthImageNames[i]},m_config.depthIntensities[i]));
            labels.push_back(U"RESTORE ALL IMAGE DEFAULTS");
        }
        else {
            String monitorLabel=U"Current / fallback";
            for(const auto& monitor:System::EnumerateMonitors()) if(monitor.id==m_config.monitorId) monitorLabel=monitor.name;
            labels={U"Monitor: "+monitorLabel,U"Lane width: {:.0f}%"_fmt(m_config.layout.laneWidth*100),
                U"Judgment anchor Y: {:.1f}%"_fmt(m_config.layout.judgmentLineY*100),U"RESET LAYOUT",U"APPLY / CONFIRM",U"DISCARD CHANGES"};
        }
        if(m_section==Root && !Vertical::OptionProbe::active()) labels={labels[0],labels[1],U"99 - Developer Options"};
        for(int i=0;i<Count(m_section);++i) {
            const auto rect=rowRect(i);
            const bool disabled=!m_config.backgroundDepthEnabled && ((m_section==Graphics && i==1)||m_section==DepthImages);
            ModMenuStyle::Panel(rect);
            if(!disabled && i==m_menu.cursor())rect.draw(ColorF{1,.18}).drawFrame(ModMenuStyle::Unit(),Palette::White);
            ModMenuStyle::Hover(rect,!disabled && m_editIndex<0);
            auto labelRect=rect.stretched(-6);if(m_section==DepthImages && i<8)labelRect.w=rect.w*.73;
            ModMenuStyle::Text(labels[i],labelRect,23*ModMenuStyle::Unit(),disabled ? ColorF{.35} : ColorF{Palette::White});
            if(m_section==DepthImages && i<8) {
                const auto button=resetRect(i);ModMenuStyle::Panel(button);ModMenuStyle::Hover(button,!disabled && m_editIndex<0);
                ModMenuStyle::Text(U"RESET",button.stretched(-3),18*ModMenuStyle::Unit(),disabled ? ColorF{.35} : ColorF{Palette::White});
            }
        }
        if(m_editIndex>=0) {
            Scene::Rect().draw(ColorF{0,.75});m_back.draw();const auto r=numberPanel();r.draw(ColorF{.035,.09,.13});
            font(I18n::Get(I18n::Mod::NumberRange ,Vertical::kDepthImageNames[m_editIndex])).draw(size,r.x+12,r.y+10,Palette::White);
            font(m_digits.isEmpty()?U"_":m_digits).draw(size*1.8,r.x+12,r.y+r.h*.25,Palette::White);
            ModMenuStyle::Panel(numberButton(true));ModMenuStyle::Panel(numberButton(false));
            ModMenuStyle::Hover(numberButton(true));ModMenuStyle::Hover(numberButton(false));
            font(U"APPLY").draw(size,numberButton(true).stretched(-5),Palette::White);
            font(U"CANCEL").draw(size,numberButton(false).stretched(-5),Palette::White);
            font(m_message.isEmpty()?U"{} · ENTER: {} · ESC: {}"_fmt(I18n::Get(I18n::Mod::NumberHint),I18n::Get(I18n::Mod::Apply),I18n::Get(I18n::Mod::Cancel)):m_message).draw(size*.7,RectF{r.x,r.y+r.h+8,r.w,Scene::Height()*.12},Palette::White);
            return;
        }
        if(!m_message.isEmpty()){ModMenuStyle::Footer(m_message);return;}
        if(m_section==DepthImages){ModMenuStyle::Footer(U"↑↓ / R: {}   ←→ / L: {}   START: {}"_fmt(I18n::Get(I18n::Mod::Select),I18n::Get(I18n::Mod::Edit),I18n::Get(I18n::Mod::TypeNumber)),U"DELETE / RESET: {}   BACK: {}"_fmt(I18n::Get(I18n::Mod::ImageDefault),I18n::Get(I18n::Mod::SaveReturn)));return;}
        ModMenuStyle::Footer(U"↑↓ / R: {}   ←→ / L: {}"_fmt(I18n::Get(I18n::Mod::Select),I18n::Get(I18n::Mod::Edit)),U"START: {}   BACK: {}"_fmt(I18n::Get(I18n::Mod::Enter),I18n::Get(I18n::Mod::SaveReturn)));
    }
};
