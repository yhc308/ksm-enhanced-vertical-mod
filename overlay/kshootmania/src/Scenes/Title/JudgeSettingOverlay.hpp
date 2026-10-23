#pragma once
#include "MusicGame/GameMain.hpp"
#include "MusicGame/Graphics/HUD/PlayJudgeIndicator.hpp"
#include "Common/AssetManagement.hpp"
#include "RuntimeConfig.hpp"
#include "Input/Cursor/CursorInput.hpp"
#include "Input/InputUtils.hpp"
#include "Vertical/DiagnosticInputReplay.hpp"
#include "UI/ModMenuStyle.hpp"
#include "Scenes/Common/CommonOverlay.hpp"
#include "MusicGame/HispeedUtils.hpp"

class JudgeSettingOverlay {
    std::shared_ptr<MusicGame::JudgeSettingSession> m_session=std::make_shared<MusicGame::JudgeSettingSession>();
    MusicGame::JudgeSettingValues m_saved;
    std::unique_ptr<MusicGame::GameMain> m_game;
    MusicGame::Graphics::PlayJudgeIndicator m_indicator;
    CommonOverlay m_back;
    std::shared_ptr<noco::Canvas> m_hispeedCanvas;
    bool m_startArmed=false;
    Texture m_menuPanel,m_panelGlow;
    Texture m_itemCursor{SkinPreset::LoadTexture(U"standby_cur.gif")};
    const ksmaudio::Sample m_itemSe{"se/sel_m.wav"},m_valueSe{"se/hover.wav"};
    static constexpr int32 kGlowPadding=24;
    CursorInput m_navigation{{.type=CursorInput::Type::Vertical,.buttonFlags=CursorButtonFlags::kArrow|CursorButtonFlags::kLaser,.buttonIntervalSec=.12,.buttonIntervalSecFirst=.35}};
    CursorInput m_adjustment{{.type=CursorInput::Type::Horizontal,.buttonFlags=CursorButtonFlags::kArrow|CursorButtonFlags::kLaser,.buttonIntervalSec=.08,.buttonIntervalSecFirst=.35}};
    int32 m_focus=5,m_dialogIndex=0;
    bool m_finished=false,m_dialog=false,m_waitRelease=false,m_focusPaused=false,m_hoverPending=false;
    double m_unlockAt=0;
    String m_error;
    struct Layout {
        RectF left,right,mini,bottom;Vec2 recent,last;double unit;
    };
    Layout layout() const {
        if(GameRender::VerticalEnabled()){
            const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
            return {{v.x(20),v.y(424),152*v.scale,219*v.scale},{v.x(548),v.y(424),152*v.scale,219*v.scale},
                {v.x(24),v.y(273),147*v.scale,168*v.scale},
                {v.x(0),v.y(963),720*v.scale,242*v.scale},
                {v.x(360),v.y(312)},{v.x(360),v.y(382)},v.scale};
        }
        const double s=GameRender::Height()/480.0,c=GameRender::Width()/2.0;
        const double edge=Max(8.0,c-312*s);
        return {{edge,196*s,140*s,201.37*s},{GameRender::Width()-edge-140*s,196*s,140*s,201.37*s},
            {edge,55*s,140*s,160*s},{0,0,0,0},
            {c,36*s},{c,72*s},s};
    }
    RectF statTitleRect(Vec2 pos,const Layout& l) const{return {pos.x-105*l.unit,pos.y,210*l.unit,23*l.unit};}
    RectF statValueRect(Vec2 pos,const Layout& l) const{
        // Preserve glyph size; halve only the old blank space between the four lines.
        const double offset=GameRender::VerticalEnabled()?25:18.25;
        return {pos.x-105*l.unit,pos.y+offset*l.unit,210*l.unit,24*l.unit};
    }
    RectF controlsRect() const{
        const double u=ModMenuStyle::Unit();const auto title=ModMenuStyle::TitlePosition(181);
        return {GameRender::Width()*.36,10*u,GameRender::Width()*.64-12*u,18*u};
    }
    RectF stopRect() const{auto r=controlsRect();r.y+=18*ModMenuStyle::Unit();return r;}
    RectF row(int32 index) const {
        const auto l=layout();const auto panel=index<5?l.left:l.right;
        double top,bottom;
        if(index<5){const int32 n=index==0?4:index-1;top=(36+37.5*n)/242;bottom=(74+37.5*n)/242;}
        else{constexpr std::array<double,5> tops{48,90,131,176,204},ends{90,131,173,202,232};top=tops[index-5]/247;bottom=ends[index-5]/247;}
        return {panel.x+panel.w*.09,panel.y+panel.h*top,panel.w*.82,panel.h*(bottom-top)};
    }
    RectF titleRect(int32 index) const {
        const auto l=layout();const auto p=index<5?l.left:l.right;
        const double center=index<5?(46.5+37.5*(index==0?4:index-1))/242:
            std::array<double,5>{61,100,142,190,217}[index-5]/247;
        const double height=p.h*(index>=8?24.0/247:16.0/242);
        return {p.x+p.w*.09,p.y+p.h*center-height/2,p.w*.82,height};
    }
    RectF valueRect(int32 index) const {
        const auto l=layout();const auto p=index<5?l.left:l.right;
        const double center=index<5?(65+37.5*(index==0?4:index-1))/242:std::array<double,3>{79,120,160}[index-5]/247;
        const double height=p.h*16/242;
        return {p.x+p.w*.09,p.y+p.h*center-height/2,p.w*.82,height};
    }
    std::pair<int32,int32> choiceBounds(int32 i) const {
        if(i==0)return {0,2};if(i==2)return {0,3};if(i==1)return {0,1};if(i==3||i==4)return {0,3};return {-10000,10000};
    }
    int32 choiceValue(int32 i) const {
        const auto& v=m_session->values;
        if(i==0)return static_cast<int32>(v.mode);if(i==1)return static_cast<int32>(v.hud.judgeColor);
        if(i==2)return static_cast<int32>(v.hud.color);if(i==3)return MusicGame::TimingDisplayCursor(v.hud.display);if(i==4)return static_cast<int32>(v.hud.position);
        return i==5?v.input:i==6?v.visual:v.laser;
    }
    void select(int32 index){if(index!=m_focus){m_focus=index;m_itemSe.play();}}
    void change(int32 index,int32 delta){const auto before=m_session->values;m_session->change(index,delta);if(!(before==m_session->values))m_valueSe.play();}
    void preparePanel(){
        const Image source{FsUtils::GetResourcePath(U"mods/enhanced/judge_setting/Judge_Setting_Mode_banner.png")};
        m_menuPanel=Texture{source};
        Image alpha{Size{source.width()+2*kGlowPadding,source.height()+2*kGlowPadding},Color{255,255,255,0}};
        for(int32 y=0;y<source.height();++y)for(int32 x=0;x<source.width();++x)alpha[y+kGlowPadding][x+kGlowPadding].a=source[y][x].a;
        alpha.gaussianBlur(31);
        for(int32 y=0;y<alpha.height();++y)for(int32 x=0;x<alpha.width();++x){
            const bool inside=x>=kGlowPadding&&y>=kGlowPadding&&x<source.width()+kGlowPadding&&y<source.height()+kGlowPadding;
            const double outside=inside?1-source[y-kGlowPadding][x-kGlowPadding].a/255.0:1;
            alpha[y][x].a=static_cast<uint8>(Min(190.0,alpha[y][x].a*1.8)*outside);
        }
        m_panelGlow=Texture{alpha}; // One cached alpha glow; no image work during playback.
    }
    static MusicGame::JudgeSettingValues load() {
        MusicGame::JudgeSettingValues v;v.mode=RuntimeConfig::LoadFastSlowModeFromConfigIni();v.hud=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
        v.input=ConfigIni::GetInt(ConfigIni::Key::kInputDelay);v.visual=ConfigIni::GetInt(ConfigIni::Key::kVisualOffset);v.laser=ConfigIni::GetInt(ConfigIni::Key::kLaserInputDelay);return v;
    }
    static String value(double ms){return Abs(ms)<.05?U"0.0ms":U"{:+.1f}ms"_fmt(ms);}
    void label(StringView text,const RectF& rect,double size,ColorF color=Palette::White,bool outlined=false) const {
        const auto font=AssetManagement::SystemFontBold();
        const double actual=Min(size,rect.w/Max(1.0,font(text).region(1.0).w));
        if(outlined)font(text).drawAt(TextStyle::Outline(0,.16,ColorF{0,.9}),actual,rect.center(),color);
        else{font(text).drawAt(actual,rect.center()+Vec2{1,1},ColorF{0,color.a*.85});font(text).drawAt(actual,rect.center(),color);}
    }
    bool buttonDown() const {
        for(const auto button:{kButtonBT_A,kButtonBT_B,kButtonBT_C,kButtonBT_D,kButtonFX_L,kButtonFX_R})if(KeyConfig::Down(button))return true;
        return false;
    }
    bool chord() const {return KeyConfig::Pressed(kButtonFX_L)&&KeyConfig::Pressed(kButtonFX_R)&&KeyConfig::Pressed(kButtonStart);}
    bool chordReleased() const {return !KeyConfig::Pressed(kButtonFX_L)&&!KeyConfig::Pressed(kButtonFX_R)&&!KeyConfig::Pressed(kButtonStart);}
    bool save() {
        const auto& v=m_session->values;
        const bool ok=ConfigIni::SaveSelectedIntegers({
            {String{ConfigIni::Key::kShowFastSlow},static_cast<int32>(v.mode)},
            {String{ConfigIni::Key::kJudgeColor},static_cast<int32>(v.hud.judgeColor)},
            {String{ConfigIni::Key::kTimingMsColor},static_cast<int32>(v.hud.color)},
            {String{ConfigIni::Key::kTimingMsDisplay},static_cast<int32>(v.hud.display)},
            {String{ConfigIni::Key::kTimingPosition},static_cast<int32>(v.hud.position)},
            {String{ConfigIni::Key::kInputDelay},v.input},{String{ConfigIni::Key::kVisualOffset},v.visual},{String{ConfigIni::Key::kLaserInputDelay},v.laser}});
        if(ok){m_saved=v;m_error.clear();}else m_error=I18n::Get(I18n::Mod::SaveFailed);
        return ok;
    }
    void activate() {
        if(m_dialog){
            if(m_dialogIndex==0){if(save())m_finished=true;}
            else if(m_dialogIndex==1)m_finished=true;
            else m_dialog=false;
        }else if(m_focus==8)save();
        else if(m_focus==9)requestExit();
    }
public:
    JudgeSettingOverlay() {
        preparePanel();m_session->values=m_saved=load();
        MusicGame::GameCreateInfo info;info.chartFilePath=FsUtils::GetResourcePath(U"mods/enhanced/judge_setting/Preview_Piano.kson");
        if(!FileSystem::IsFile(info.chartFilePath))throw Error{I18n::Get(I18n::Mod::JudgeChartMissing)};
        info.assistTickMode=static_cast<AssistTickMode>(Clamp(ConfigIni::GetInt(ConfigIni::Key::kAssistTick),0,2));
        info.playOption.judgeSetting=m_session;info.playOption.movieEnabled=false;info.playOption.autoSyncMode=AutoSyncMode::kOff;
        info.playOption.globalOffsetMs=ConfigIni::GetInt(ConfigIni::Key::kGlobalOffset);
        info.playOption.audioProcDelayMs=ConfigIni::GetInt(ConfigIni::Key::kAudioProcDelay);
        info.playOption.fastSlowMode=m_session->values.mode;info.playOption.timingHud=m_session->values.hud;
        info.playOption.availableHispeedTypes=ConfigIni::LoadAvailableHispeedTypes();
        info.playOption.hispeedSetting=MusicGame::HispeedUtils::FromConfigStringValue(ConfigIni::GetString(ConfigIni::Key::kHispeed));
        m_hispeedCanvas=noco::Canvas::LoadFromFile(FsUtils::GetResourcePath(GameRender::VerticalEnabled()?U"mods/enhanced/judge_setting/hispeed-portrait.noco":U"mods/enhanced/judge_setting/hispeed-landscape.noco"));
        m_game=std::make_unique<MusicGame::GameMain>(info);m_game->start();
        m_indicator.update(m_game->previewView(),m_session->values.hud.judgeColor,true);
    }
    ~JudgeSettingOverlay(){if(m_game)m_game->terminate();}
    JSON layoutReport() const {
        const auto l=layout();JSON result;
        const auto add=[&](StringView name,const RectF& rect){result[name]=Array<double>{rect.x,rect.y,rect.w,rect.h};};
        add(U"left",l.left);add(U"right",l.right);add(U"mini",l.mini);add(U"bottom",l.bottom);
        add(U"judgeColor",valueRect(1));add(U"close",closeButton());
        const bool portrait=GameRender::VerticalEnabled();const double speedScale=portrait?GameRender::Width()/720.0:GameRender::Height()/480.0;
        const double panelScale=portrait?.56:.5;
        add(U"hispeed",{GameRender::Width()-452*panelScale*speedScale,GameRender::Height()-54*panelScale*speedScale,452*panelScale*speedScale,54*panelScale*speedScale});
        add(U"recentTitle",statTitleRect(l.recent,l));add(U"lastTitle",statTitleRect(l.last,l));
        add(U"recentValue",statValueRect(l.recent,l));add(U"lastValue",statValueRect(l.last,l));
        const auto heading=ModMenuStyle::TitlePosition(181);
        add(U"heading",{heading,GameRender::Width()*(GameRender::VerticalEnabled()?.48:.34),ModMenuStyle::TitleHeight()});
        add(U"headerClose",ModMenuStyle::CloseRect(181));
        if(!GameRender::VerticalEnabled()){add(U"controls",controlsRect());add(U"stop",stopRect());}
        result[U"statFontSize"]=(GameRender::VerticalEnabled()?16:12)*l.unit;
        result[U"windowTitle"]=Window::GetTitle();
        for(int32 i=0;i<10;++i){add(U"title{}"_fmt(i),titleRect(i));if(i<8){add(U"value{}"_fmt(i),valueRect(i));const auto a=visibleArrows(i);result[U"arrows{}"_fmt(i)]=Array<bool>{a.first,a.second};}}
        result[U"activePanel"]=m_session->menu?(m_focus<5?0:1):-1;
        result[U"palette"]=static_cast<int32>(m_session->values.hud.judgeColor);
        result[U"worldScale"]=1.0;result[U"width"]=GameRender::Width();result[U"height"]=GameRender::Height();return result;
    }
    const auto& session() const {return *m_session;}
    auto& session(){return *m_session;}
    auto& game(){return *m_game;}
    RectF settingRow(int32 index) const{return row(index);}
    RectF closeButton() const{
        return ModMenuStyle::BackRect();
    }
    std::pair<bool,bool> visibleArrows(int32 index) const {const auto [lo,hi]=choiceBounds(index);const int32 value=choiceValue(index);return {value>lo,value<hi};}
    static ColorF inputColor(JudgeColor palette,double ms){
        const ColorF base{Color{208,192,93}};
        return base.lerp(ColorF{MusicGame::JudgeDirectionColor(palette,ms>=0)},Clamp(std::floor(Abs(ms)/10)*.1,0.0,1.0));
    }
    int32 focus() const{return m_focus;}
    bool dialog() const{return m_dialog;}
    bool dirty() const{return !(m_session->values==m_saved);}
    bool finished() const{return m_finished;}
    void requestMenu(bool consumedChord=false){m_session->menu=true;m_waitRelease=consumedChord;m_unlockAt=m_session->time+MusicGame::JudgeSettingSession::kGuardSeconds;}
    void requestExit(){
        if(!dirty()){m_finished=true;return;}
        requestMenu();m_dialog=true;m_dialogIndex=0;
    }
    void pointerMoveAt(const Vec2& point){
        if(!m_session->menu||m_waitRelease||m_session->time<m_unlockAt)return;
        if(m_dialog){for(int32 i=0;i<3;++i)if(dialogRow(i).contains(point)){m_dialogIndex=i;return;}return;}
        for(int32 i=0;i<10;++i)if(m_session->itemEnabled(i)&&row(i).contains(point)){select(i);return;}
    }
    void pointerDownAt(const Vec2& point){
        if(!GameRender::Rect().contains(point))return;
        if(!m_session->menu){requestMenu();m_hoverPending=true;return;}
        if(m_waitRelease||m_session->time<m_unlockAt)return;
        if(closeButton().contains(point)){if(m_dialog)m_dialog=false;else requestExit();return;}
        if(m_dialog){for(int32 i=0;i<3;++i)if(dialogRow(i).contains(point)){m_dialogIndex=i;activate();return;}}
        else for(int32 i=0;i<10;++i)if(m_session->itemEnabled(i)&&row(i).contains(point)){
            select(i);if(i>=8)activate();else change(i,point.x<row(i).center().x?-1:1);return;
        }
    }
    void update(double delta=Scene::DeltaTime()) {
        if(m_finished)return;
        auto& state=*m_session;
        const bool focused=Window::GetState().focused || System::GetCommandLineArgs().contains(U"--judge-setting-test");
        state.focused=focused;
        if(!focused){m_startArmed=false;if(!m_focusPaused){m_game->pauseJudgeSetting(true);m_focusPaused=true;requestMenu();}return;}
        if(m_focusPaused){m_game->pauseJudgeSetting(false);m_focusPaused=false;state.clearRecent();requestMenu();}
        state.advance(delta);m_navigation.update();m_adjustment.update();
        m_back.setModScreenLayout();
        m_back.setBackButtonFocused(GameRender::VerticalEnabled());
        const bool backClick=m_back.update(noco::HitTestEnabledYN{InputUtils::IsUIMouseInputEnabled()},false);
        const bool startHeld=KeyConfig::Pressed(kButtonStart);
        const bool backDown=KeyConfig::Down(kButtonBack)||backClick;
        const bool mouse=!Vertical::DiagnosticInputReplay::Active()&&MouseL.down()&&!InputUtils::WindowFocusedThisFrame();
        if(!state.menu){
            if(KeyConfig::Down(kButtonStart))m_startArmed=true;
            if(backDown){m_startArmed=false;requestMenu(true);}
            else if(m_startArmed && !startHeld){m_startArmed=false;requestMenu(true);}
            else if(mouse && !closeButton().contains(Cursor::PosF()))pointerDownAt(Cursor::PosF());
        }else{
            m_startArmed=false;
            if(m_waitRelease && chordReleased())m_waitRelease=false;
            if(!m_waitRelease && state.time>=m_unlockAt){
                if(buttonDown()){state.menu=false;state.clearRecent();m_dialog=false;m_error.clear();}
                else if(m_dialog){
                    if(!startHeld)m_dialogIndex=Clamp(m_dialogIndex+m_navigation.deltaCursor(),0,2);
                    if(backDown)m_dialog=false;
                    if(KeyConfig::Down(kButtonStart))activate();
                    if(mouse && !closeButton().contains(Cursor::PosF()))pointerDownAt(Cursor::PosF());
                }else{
                    if(!startHeld){select(state.navigate(m_focus,m_navigation.deltaCursor()));change(m_focus,m_adjustment.deltaCursor());}
                    if(!state.itemEnabled(m_focus))select(0);
                    if(KeyConfig::Down(kButtonStart))activate();
                    if(backDown)requestExit();
                    if(mouse && !closeButton().contains(Cursor::PosF()))pointerDownAt(Cursor::PosF());
                }
            }
        }
        if(!state.menu)m_hoverPending=false;
        if(!Vertical::DiagnosticInputReplay::Active()&&InputUtils::IsUIMouseInputEnabled()){
            if(state.menu&&Cursor::DeltaF().lengthSq()>0)m_hoverPending=true;
            if(m_hoverPending&&state.menu&&!startHeld&&!m_waitRelease&&state.time>=m_unlockAt){pointerMoveAt(Cursor::PosF());m_hoverPending=false;}
        }
        if(state.menu)Cursor::RequestStyle(CursorStyle::Default);
        else if(!closeButton().mouseOver()&&ConfigIni::GetInt(ConfigIni::Key::kHideMouseCursor,ConfigIni::Value::HideMouseCursor::kPlayOnly)!=ConfigIni::Value::HideMouseCursor::kOff)Cursor::RequestStyle(CursorStyle::Hidden);
        m_game->update();m_indicator.update(m_game->previewView(),state.values.hud.judgeColor,true);
        m_hispeedCanvas->setParamValues({{U"hispeedValueText",MusicGame::HispeedUtils::ToDisplayString(m_game->settingsHispeed())},{U"hispeedValueTextEffective",Format(m_game->settingsEffectiveHispeed())}});
        m_hispeedCanvas->update(noco::HitTestEnabledYN::No);
    }
    RectF dialogRow(int32 index) const {
        return ModMenuStyle::DialogButton(3,index);
    }

    void drawWorld() const {
        // Use the unscaled gameplay renderer and the mode's own camera and judgment bar.
        m_game->draw();Graphics3D::Flush();
    }
    void draw() const {
        drawWorld();
        const auto l=layout();const auto& state=*m_session;const auto& v=state.values;
        if(GameRender::VerticalEnabled()){
            const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
            RectF{0,0,static_cast<double>(GameRender::Width()),v.y(181.33)}.draw(ColorF{0,.85});
        }
        // Screen-edge decoration is independent of the lane and HUD layout.
        const double frameUnit=GameRender::VerticalEnabled()?GameRender::Width()/720.0:Min(GameRender::Width()/800.0,GameRender::Height()/600.0);
        ModMenuStyle::Title(U"JUDGE TIMING SETTING",ModMenuStyle::TitlePosition(181.0),GameRender::Width()*.48);
        const String controls=U"BT / FX: {}     ↑↓ / R: {}     ←→ / L: {}     START: {}"_fmt(I18n::Get(I18n::Mod::Play),I18n::Get(I18n::Mod::Select),I18n::Get(I18n::Mod::Change),I18n::Get(I18n::Mod::Enter));
        const String stop=U"START or ESC or MOUSE Click : {}"_fmt(I18n::Get(I18n::Mod::PlayStop));
        const auto hint=[&](StringView text,RectF rect,double size,bool alignRight=false){
            if(alignRight){const auto font=AssetManagement::SystemFontBold();size=Min(size,rect.w/Max(1.0,font(text).region(1.0).w));const double w=font(text).region(size).w;rect.x=rect.rightX()-w;rect.w=w;}
            label(text,rect,size,ColorF{.8});
        };
        if(GameRender::VerticalEnabled()){
            const auto v=Vertical::Viewport::Fit(GameRender::Width(),GameRender::Height());
            hint(stop,{v.x(12),v.y(133),696*v.scale,20*v.scale},12*v.scale);
            hint(controls,{v.x(12),v.y(155),696*v.scale,20*v.scale},12*v.scale);
            hint(controls,{v.x(12),v.y(1194),696*v.scale,20*v.scale},12*v.scale);
            hint(stop,{v.x(12),v.y(1216),696*v.scale,20*v.scale},12*v.scale);
        }else{
            hint(controls,controlsRect(),14*ModMenuStyle::Unit(),true);
            hint(stop,stopRect(),12*ModMenuStyle::Unit(),true);
        }
        m_indicator.drawSettings(l.bottom,l.mini);
        for(int32 side=0;side<2;++side){
            const auto panel=side?l.right:l.left;
            if(state.menu && (m_focus>=5)==static_cast<bool>(side)){
                const Vec2 scale{panel.w/m_menuPanel.width(),panel.h/m_menuPanel.height()};
                m_panelGlow.resized(m_panelGlow.width()*scale.x,m_panelGlow.height()*scale.y).draw(panel.pos-Vec2{kGlowPadding*scale.x,kGlowPadding*scale.y});
            }
            m_menuPanel.resized(panel.size).draw(panel.pos);
            label(side?U"TIMING SETTING":U"VISUAL SETTING",{panel.x+panel.w*.09,panel.y+panel.h*.062,panel.w*.82,panel.h*.066},panel.w*12/170);
        }
        const std::array<StringView,10> titles{U"FAST/SLOW",U"JUDGE COLOR",U"±ms FAST/SLOW",U"±ms DISPLAY",U"TIMING POSITION",U"ALL Judge Offset",U"Note Visual Offset",U"Laser Judge Offset",U"SAVE",U"EXIT"};
        const std::array<StringView,3> modes{U"OFF",U"ORIGIN",U"NEW+"};
        const auto& colors=MusicGame::kTimingColorLabels;
        const auto& displays=MusicGame::kTimingDisplayLabels;
        const std::array<StringView,4> positions{U"UPPER+",U"UPPER",U"STANDARD",U"LOWER"};
        for(int32 i=0;i<10;++i){
            ModMenuStyle::Hover(row(i),state.menu&&!m_dialog&&state.itemEnabled(i)&&!m_waitRelease&&state.time>=m_unlockAt);
            const auto title=titleRect(i);const bool enabled=state.itemEnabled(i);const ColorF tint=enabled?ColorF{1}:ColorF{.4};
            const auto panel=i<5?l.left:l.right;
            if(state.menu&&!m_dialog&&m_focus==i){
                const ScopedRenderStates2D additive{BlendState::Additive};
                const double wave=(1-std::cos(state.time*Math::Pi/ .462))*.5;
                m_itemCursor.resized(panel.w*.96,panel.w*.96*75.0/525).drawAt((i>=8?title:valueRect(i)).center(),ColorF{1,(121+90*wave)/255.0});
            }
            label(titles[i],title,panel.w*(i>=8?20.0/172:i<5?12.0/170:14.0/172),tint);
            if(i>=8)continue;
            const RectF val=valueRect(i);const auto [left,right]=visibleArrows(i);
            const ColorF option{Color{204,204,204,static_cast<uint8>(enabled?255:100)}};
            const String before=left?U"< ":U"",after=right?U" >":U"";
            if(i==1){
                const auto font=AssetManagement::SystemFontBold();const double size=panel.w*12/170;
                const double width=font(before+U"F / S"+after).region(size).w;Vec2 pos{val.center().x-width/2,val.center().y-font(U"F / S").region(size).h/2};
                for(const auto& part:std::array<std::pair<String,ColorF>,5>{{{before,option},{U"F",enabled?ColorF{MusicGame::JudgeDirectionColor(v.hud.judgeColor,true)}:option},{U" / ",option},{U"S",enabled?ColorF{MusicGame::JudgeDirectionColor(v.hud.judgeColor,false)}:option},{after,option}}}){
                    font(part.first).draw(size,pos,part.second);pos.x+=font(part.first).region(size).w;
                }
            }else{
                String text;
                if(i==0)text=modes[static_cast<int32>(v.mode)];
                if(i==2)text=colors[static_cast<int32>(v.hud.color)];
                if(i==3)text=displays[MusicGame::TimingDisplayCursor(v.hud.display)];
                if(i==4)text=positions[static_cast<int32>(v.hud.position)];
                if(i>=5)text=U"{:+} ms"_fmt(i==5?v.input:i==6?v.visual:v.laser);
                label(before+text+after,val,panel.w*(i<5?12.0/170:13.0/172),option);
            }
        }
        const auto stat=[&](StringView title,Optional<double> mean,Vec2 pos){
            label(title,statTitleRect(pos,l),(GameRender::VerticalEnabled()?16:12)*l.unit,Palette::White,true);
            label(mean?value(*mean):U"—",statValueRect(pos,l),(GameRender::VerticalEnabled()?16:12)*l.unit,
                mean?inputColor(v.hud.judgeColor,*mean):ColorF{.7},true);
        };
        stat(U"Recent Average",state.recentMean(),l.recent);stat(U"Last Input",state.last,l.last);
        label(U"{} FPS"_fmt(static_cast<int32>(Profiler::FPS())),{12*l.unit,GameRender::Height()-38*l.unit,140*l.unit,20*l.unit},16*l.unit);
        if(dirty())label(U"UNSAVED",{l.right.x,l.right.y-20*l.unit,l.right.w,17*l.unit},12*l.unit,ColorF{1,.8,.2});
        if(!m_error.isEmpty())label(m_error,{10.0,GameRender::Height()-52*l.unit,GameRender::Width()-20.0,22*l.unit},14*l.unit,ColorF{1,.4,.4});
        if(m_dialog)ModMenuStyle::Dialog(dirty()?I18n::Get(I18n::Mod::SaveQuestion):I18n::Get(I18n::Mod::JudgeExitQuestion),{I18n::Get(I18n::Mod::SaveExit),I18n::Get(I18n::Mod::DiscardExit),I18n::Get(I18n::Mod::Cancel)},m_dialogIndex);
        m_hispeedCanvas->draw();
        m_back.draw();
    }
};
