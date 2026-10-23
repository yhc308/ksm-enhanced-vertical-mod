#include "SkinOverlay.hpp"
#include "RuntimeConfig.hpp"
#include "Input/InputUtils.hpp"
#include "Vertical/MenuContentLayout.hpp"
#include "Diagnostics/SkinPresetFixture.hpp"
#include "UI/ModMenuStyle.hpp"
#include "MusicGame/HispeedUtils.hpp"
#include "Vertical/DiagnosticInputReplay.hpp"

namespace
{
    bool Portrait() {return Scene::Height()>Scene::Width();}
    double PortraitScale() {return Scene::Width()/720.0;}
    double Unit() { return Min(Scene::Height()/720.0,Scene::Width()/960.0); }
    double ContentOffset(){return Portrait()?Vertical::MenuContent::Rect().y-10*PortraitScale():0;}
    double PreviewTop() {return Portrait()?174*PortraitScale()+ContentOffset():172*Unit();}
    RectF PresetPanel(){const double u=Unit(),left=Scene::Width()*2.0/3.0;return Portrait()?RectF{left+12*u,93*PortraitScale()+ContentOffset(),Scene::Width()-left-24*u,850*PortraitScale()}:RectF{left+12*u,104*u,Scene::Width()-left-24*u,Scene::Height()-204*u};}
    RectF PresetList(){const auto panel=PresetPanel();return Portrait()?RectF{panel.x,129*PortraitScale()+ContentOffset(),panel.w,806*PortraitScale()}:RectF{panel.x,154*Unit(),panel.w,Scene::Height()-254*Unit()};}
    double ListCenterY(){return Portrait()?500*PortraitScale()+ContentOffset():Scene::Height()/2.0;}
    RectF PresetRow(double offset=0){const double u=Unit(),left=Scene::Width()*2.0/3.0;return {left+20*u,ListCenterY()+offset*52*u-24*u,Scene::Width()-left-44*u,48*u};}
    RectF PaletteRect() { const double u = Unit(); return {Scene::Width() - 76*u, 20*u, 56*u, 56*u}; }
    RectF CloseRect() {return ModMenuStyle::CloseRect();}
    RectF SettingRect(int32 field)
    {
        const double u=Unit(), width=(Scene::Width()*2.0/3.0-64*u)/2;
        int row=0,col=0;
        switch(field){case 6:col=1;break;case 7:case 9:row=1;break;case 8:row=1;col=1;break;
        case 2:row=2;break;case 3:row=3;break;case 4:row=2;col=1;break;case 5:row=3;col=1;break;}
        return {24*u+col*(width+16*u),(Portrait()?84*PortraitScale():76*u)+row*(Portrait()?28:22)*u+ContentOffset(),width,(Portrait()?26:20)*u};
    }
    RectF PolicyRect() {return SettingRect(1);}
    RectF DepthPlusRect(){const auto r=SettingRect(7);return {r.br().x-25*Unit(),r.y,25*Unit(),r.h};}
    RectF OptionsRect(){const auto a=SettingRect(1),b=SettingRect(5);return {a.x-4*Unit(),a.y-3*Unit(),b.br().x-a.x+8*Unit(),b.br().y-a.y+6*Unit()};}
    constexpr std::array<int32,11> FocusOrder{0,1,7,9,2,3,6,8,4,5,10};
    int32 StepFocus(int32 focus,int32 delta){for(size_t i=0;i<FocusOrder.size();++i)if(FocusOrder[i]==focus)return FocusOrder[(static_cast<int32>(i)+delta%11+11)%11];return 0;}
    int32 StepOptionFocus(int32 focus,int32 delta){
        constexpr std::array<int32,9> order{1,7,9,2,3,6,8,4,5};
        for(int32 i=0;i<9;++i)if(order[i]==focus)return order[Clamp(i+delta,0,8)];
        return 1;
    }
    int32 TabFocus(int32 focus,bool reverse){const int32 group=focus==0?0:focus==10?2:1;const std::array<int32,3> groups{0,1,10};return groups[(group+(reverse?2:1))%3];}
    RectF AdjustRect(int32 field, bool plus)
    {
        const auto r=SettingRect(field); const double u=Unit();
        return {r.br().x-(plus ? 25 : 55)*u,r.y,25*u,r.h};
    }
    String SettingText(int32 field, const SkinPreset::Settings& settings)
    {
        if (field==2) return I18n::Get(I18n::Mod::Background1Brightness ,settings.background1Brightness);
        if (field==3) return I18n::Get(I18n::Mod::Background2Brightness ,settings.background2Opacity);
        if (field==4)
        {
            const double value=settings.laserWidthPercent;
            if (Abs(value-100.0)<0.0001) return I18n::Get(I18n::Mod::LaserWidth ,100,U" (Default)");
            if (Abs(value-SkinPreset::kVerticalLaserPercent)<0.0001) return I18n::Get(I18n::Mod::LaserWidth ,72.73,U" (Vertical)");
            return I18n::Get(I18n::Mod::LaserWidth ,static_cast<int32>(Round(value)),U"");
        }
        return I18n::Get(I18n::Mod::NoteHeight ,settings.noteHeightPercent,settings.noteHeightPercent==100 ? U" (Default)" : U"");
    }
    String IndicatorText(const SkinPreset::Settings& settings)
    {
        if(!Vertical::ModConfig::Load().enabled)return settings.indicatorLandscape?U"On":U"Off";
        switch(settings.indicatorPortrait){case SkinPreset::IndicatorPortrait::Off:return U"Off";case SkinPreset::IndicatorPortrait::Both:return U"Both";default:return U"Bottom";}
    }
    String BeamText(SkinPreset::KeyBeamMode mode){const std::array<StringView,6> names{U"Off",U"Normal",U"All Crit",U"All P-Crit",U"Only Crit",U"Only P-Crit"};return String{names[static_cast<size_t>(mode)]};}
    struct ChoiceInfo {String title,value;int32 index,last;};
    ChoiceInfo Choice(int32 field,const SkinPreset::Settings& settings,bool depth){
        if(field==1)return {String{I18n::Get(I18n::Mod::MissingImageLabel)},settings.missing==SkinPreset::MissingMode::Default?U"Default":U"Yellow",static_cast<int32>(settings.missing),1};
        if(field==6)return {String{I18n::Get(I18n::Mod::IndicatorLabel)},IndicatorText(settings),Vertical::ModConfig::Load().enabled?static_cast<int32>(settings.indicatorPortrait):static_cast<int32>(settings.indicatorLandscape),Vertical::ModConfig::Load().enabled?2:1};
        if(field==7){if(!Portrait())return {String{I18n::Get(I18n::Mod::DepthLabel)},String{I18n::Get(I18n::Mod::NotImplemented)},0,0};return {String{I18n::Get(I18n::Mod::DepthLabel)},depth?U"On":U"Off",static_cast<int32>(depth),1};}
        return {String{I18n::Get(I18n::Mod::KeyBeamLabel)},BeamText(settings.keyBeamMode),static_cast<int32>(settings.keyBeamMode),5};
    }
    String ChoiceText(const ChoiceInfo& choice){return choice.title+(choice.index>0?U"◀ ":U"  ")+choice.value+(choice.index<choice.last?U" ▶":U"  ");}
    RectF ChoiceRow(int32 field){auto row=SettingRect(field);if(field==7)row.w-=30*Unit();return row;}
    RectF ChoiceArrow(int32 field,const ChoiceInfo& choice,bool right){
        const double u=Unit();const auto row=ChoiceRow(field);const auto font=AssetManagement::SystemFont();
        const double size=Min(17*u,(row.w-8*u)/Max(1.0,font(ChoiceText(choice)).region(1.0).w));
        const String before=right?choice.title+(choice.index>0?U"◀ ":U"  ")+choice.value+U" ":choice.title;
        return {row.x+4*u+font(before).region(size).w-2*u,row.y,Max(12*u,font(U"▶").region(size).w+4*u),row.h};
    }
    bool Click(const RectF& rect) { return InputUtils::IsUIMouseInputEnabled() && rect.leftClicked(); }
    void Label(StringView text, double size, const Vec2& pos, const ColorF& color = Palette::White)
    {
        const auto glyphs=AssetManagement::SystemFont()(text);
        // TextStyle::Outline takes a distance-field factor, not pixels. Draw an
        // one-pixel outer stroke with 90% opacity, shared by both orientations.
        for (const auto& delta : {Vec2{-1,0},Vec2{1,0},Vec2{0,-1},Vec2{0,1},Vec2{-.707,-.707},Vec2{.707,-.707},Vec2{-.707,.707},Vec2{.707,.707}})
            glyphs.draw(size,pos+delta,ColorF{0.0,0.9});
        glyphs.draw(size,pos,color);
    }
    void FitLabel(StringView text, double size, const Vec2& pos, double width)
    {
        const double natural=AssetManagement::SystemFont()(text).region(1.0).w;
        Label(text,Min(size,width/Max(natural,1.0)),pos);
    }
    MusicGame::GameCreateInfo PreviewInfo()
    {
        MusicGame::GameCreateInfo info;
        info.chartFilePath = FsUtils::GetResourcePath(U"skin_preview/Preview_Piano.kson");
        if (!FileSystem::IsFile(info.chartFilePath)) throw Error{I18n::Get(I18n::Mod::PreviewChartMissing ,U"skin_preview/Preview_Piano.kson")};
        info.playOption.skinPreview = true;
        info.playOption.isAutoPlay = MusicGame::IsAutoPlayYN::Yes;
        info.playOption.isAutoPlaySE = true;
        info.playOption.movieEnabled = false;
        info.playOption.autoSyncMode = AutoSyncMode::kOff;
        info.playOption.fastSlowMode=RuntimeConfig::LoadFastSlowModeFromConfigIni();
        info.playOption.timingHud=RuntimeConfig::LoadTimingHudSettingsFromConfigIni();
        info.playOption.availableHispeedTypes=ConfigIni::LoadAvailableHispeedTypes();
        info.playOption.hispeedSetting=MusicGame::HispeedUtils::FromConfigStringValue(U"450");
        return info;
    }
}

SkinOverlay::SkinOverlay()
    : m_original(SkinPreset::Current()), m_draft(m_original), m_entries(SkinPreset::Discover())
    , m_cursor(CursorInput::CreateInfo{.type=CursorInput::Type::Vertical,
        .buttonFlags=CursorButtonFlags::kArrowOrBTAll | CursorButtonFlags::kMouseWheel})
    , m_adjust(CursorInput::CreateInfo{.type=CursorInput::Type::Horizontal,.buttonFlags=CursorButtonFlags::kArrow,
        .buttonIntervalSec=0.05,.buttonIntervalSecFirst=0.35})
    , m_knob(CursorInput::CreateInfo{.type=CursorInput::Type::Horizontal,
        .buttonFlags=CursorButtonFlags::kLaser | CursorButtonFlags::kLaserOpposite,
        .buttonIntervalSec=0.05,.buttonIntervalSecFirst=0.35})
    , m_fieldNav(CursorInput::CreateInfo{.type=CursorInput::Type::Vertical,
        .buttonFlags=CursorButtonFlags::kArrow,
        .buttonIntervalSec=0.15,.buttonIntervalSecFirst=0.35})
    , m_previewSize(m_previewContext.outputSize)
    // The main scene uses painter order. A depth attachment would let the
    // backdrop occlude the distant highway even though all projected coordinates match.
    , m_target(m_previewSize, HasDepth::No)
{
    for (int32 i=0; i<static_cast<int32>(m_entries.size()); ++i)
        if (m_entries[i].folder == m_draft.folder) m_index=i;
    m_scroll = m_index;
    Vertical::DepthPreviewDraft()=m_depthDraft;
    if (SkinFixture::Smoke()) ScreenCapture::SetScreenshotDirectory(FsUtils::AppDataDirectoryPath());
    if (SkinFixture::Smoke()) m_fixtureTrace=std::make_unique<TextWriter>(FsUtils::GetResourcePath(U"preview-trace.csv"));
    try {
        const GameRender::Scope renderScope{m_previewContext};
        m_game = std::make_unique<MusicGame::GameMain>(PreviewInfo());
        m_game->start();
    } catch (const Error& error) { m_error=error.what(); }
}

SkinOverlay::~SkinOverlay()
{
    m_game.reset(); // release preview audio before restoring the title
    Vertical::DepthPreviewDraft().reset();
    if (!m_finished) SkinPreset::Activate(m_original);
}

void SkinOverlay::changeSelection(int32 index)
{
    index = Clamp(index, 0, static_cast<int32>(m_entries.size())-1);
    if (index == m_index) return;
    m_index = index;
    m_draft.folder = m_entries[m_index].folder;
    refreshSkin();
}

void SkinOverlay::refreshSkin()
{
    const auto previous=SkinPreset::Current();
    try {
        const GameRender::Scope renderScope{m_previewContext};
        SkinPreset::Activate(m_draft);
        if (m_game) m_game->reloadPreviewGraphics();
        else { m_game=std::make_unique<MusicGame::GameMain>(PreviewInfo()); m_game->start(); }
        m_error.clear();
    } catch (const Error& error) { SkinPreset::Activate(previous);m_draft=previous;m_error = error.what(); }
}

void SkinOverlay::adjustValue(int32 field, int32 delta)
{
    if (delta==0 || (!Portrait()&&(field==7||field==9))) return;
    if (field==1)
    {
        const auto value=delta>0 ? SkinPreset::MissingMode::Error : SkinPreset::MissingMode::Default;
        if (value!=m_draft.missing) {m_draft.missing=value;refreshSkin();}
        return;
    }
    if(field==6){
        if(Vertical::ModConfig::Load().enabled)m_draft.indicatorPortrait=static_cast<SkinPreset::IndicatorPortrait>(Clamp(static_cast<int32>(m_draft.indicatorPortrait)+delta,0,2));
        else m_draft.indicatorLandscape=Clamp(static_cast<int32>(m_draft.indicatorLandscape)+delta,0,1)!=0;
    }
    if(field==7){const bool enabled=Clamp(static_cast<int32>(m_depthDraft.backgroundDepthEnabled)+delta,0,1)!=0;if(enabled!=m_depthDraft.backgroundDepthEnabled){m_depthDraft.backgroundDepthEnabled=enabled;refreshDepth();}return;}
    if(field==8)m_draft.keyBeamMode=static_cast<SkinPreset::KeyBeamMode>(Clamp(static_cast<int32>(m_draft.keyBeamMode)+delta,0,5));
    if(field==9){m_depthDialog=true;m_depthEdit=-1;return;}
    if (field==2) m_draft.background1Brightness=Clamp(m_draft.background1Brightness+delta*10,0,100);
    if (field==3) m_draft.background2Opacity=Clamp(m_draft.background2Opacity+delta*10,0,100);
    if (field==4) for(int32 i=0;i<Abs(delta);++i) m_draft.laserWidthPercent=SkinPreset::StepLaserPercent(m_draft.laserWidthPercent,Sign(delta));
    if (field==5) m_draft.noteHeightPercent=Clamp(m_draft.noteHeightPercent+delta*5,50,200);
    SkinPreset::Activate(m_draft);
}

namespace {
    RectF DepthPanel(){const auto a=Portrait()?Vertical::MenuContent::Rect():RectF{Scene::Rect()};return {a.x+a.w*.05,a.y+a.h*.18,a.w*.9,Min(a.h*.65,530*Unit())};}
    RectF DepthRow(int32 i){const auto p=DepthPanel();return {p.x+16*Unit(),p.y+(62+i*43)*Unit(),p.w-32*Unit(),38*Unit()};}
    RectF DepthAction(int32 i,int32 action){const auto row=DepthRow(i);return {row.br().x-(action==0?158:action==1?120:82)*Unit(),row.y,action==2?82*Unit():34*Unit(),row.h};}
    RectF DepthDone(){const auto p=DepthPanel();return {p.br().x-100*Unit(),p.br().y-46*Unit(),84*Unit(),32*Unit()};}
    RectF DepthResetAll(){const auto p=DepthPanel();return {p.x+16*Unit(),p.br().y-46*Unit(),200*Unit(),32*Unit()};}
}
void SkinOverlay::refreshDepth()
{
    Vertical::DepthPreviewDraft()=m_depthDraft;
    try{const GameRender::Scope scope{m_previewContext};if(m_game)m_game->reloadPreviewGraphics();m_error.clear();}
    catch(const Error& error){m_error=error.what();}
}
void SkinOverlay::updateDepthDialog()
{
    if(m_depthEdit>=0){
        for(const char32 c:TextInput::GetRawInput()){
            if(c>=U'0' && c<=U'9' && m_depthDigits.size()<3)m_depthDigits.push_back(c);
            else if(c==U'\b' && !m_depthDigits.isEmpty())m_depthDigits.pop_back();
        }
        if(KeyConfig::Down(kButtonBack)||KeyEscape.down()){m_depthEdit=-1;return;}
        if(KeyEnter.down()){
            const auto value=Vertical::ParseDepthPercent(m_depthDigits);
            if(value){m_depthDraft.depthIntensities[m_depthEdit]=*value;m_depthEdit=-1;refreshDepth();}
        }
        return;
    }
    if(KeyConfig::Down(kButtonBack) || Click(DepthDone())){m_depthDialog=false;return;}
    const int32 knob=m_knob.deltaCursor();
    const bool modifier=KeyConfig::Pressed(kButtonFX_L)||KeyConfig::Pressed(kButtonFX_R);
    m_depthIndex=(m_depthIndex+m_fieldNav.deltaCursor()+(modifier?knob:0)+80)%8;
    const int32 delta=m_adjust.deltaCursor()+(modifier?0:knob);
    if(delta){m_depthDraft.depthIntensities[m_depthIndex]=Clamp(m_depthDraft.depthIntensities[m_depthIndex]+delta,0,200);refreshDepth();}
    bool enter=KeyConfig::Down(kButtonStart);
    for(int32 i=0;i<8;++i){
        if(Click(DepthAction(i,0)) || Click(DepthAction(i,1))){m_depthIndex=i;const int32 d=DepthAction(i,0).mouseOver()?-1:1;m_depthDraft.depthIntensities[i]=Clamp(m_depthDraft.depthIntensities[i]+d,0,200);refreshDepth();return;}
        if(Click(DepthAction(i,2))){m_depthDraft.ResetDepthImage(i);refreshDepth();return;}
        if(Click(DepthRow(i))){m_depthIndex=i;enter=true;}
    }
    if(Click(DepthResetAll())){m_depthDraft.ResetDepthImages();refreshDepth();return;}
    if(KeyDelete.down()){m_depthDraft.ResetDepthImage(m_depthIndex);refreshDepth();}
    if(enter){m_depthEdit=m_depthIndex;m_depthDigits.clear();}
}
void SkinOverlay::drawDepthDialog() const
{
    const double u=Unit();Scene::Rect().draw(ColorF{0,.75});const auto panel=DepthPanel();panel.draw(ColorF{.025,.07,.04}).drawFrame(u,ColorF{.6});
    FitLabel(I18n::Get(I18n::Mod::DepthTitle),24*u,panel.pos+Vec2{16*u,12*u},panel.w-32*u);
    FitLabel(U"{} · Enter: {} · Delete: {}"_fmt(I18n::Get(I18n::Mod::StrengthRange),I18n::Get(I18n::Mod::NumberInput),I18n::Get(I18n::Mod::Defaults)),14*u,panel.pos+Vec2{16*u,43*u},panel.w-32*u);
    for(int32 i=0;i<8;++i){
        const auto row=DepthRow(i);if(i==m_depthIndex)row.draw(ColorF{1.0,.15});ModMenuStyle::Hover(row,m_depthEdit<0);
        const String name=i==7?String{I18n::Get(I18n::Mod::CustomOther)}:String{Vertical::kDepthImageNames[i]};
        FitLabel(U"{}: {}%"_fmt(name,m_depthDraft.depthIntensities[i]),18*u,row.pos+Vec2{4*u,6*u},row.w-170*u);
        for(int32 action=0;action<3;++action){const auto b=DepthAction(i,action);b.draw(ColorF{1.0,.1});ModMenuStyle::Hover(b,m_depthEdit<0);FitLabel(action==0?U"−":action==1?U"+":I18n::Get(I18n::Mod::Defaults),16*u,b.pos+Vec2{5*u,7*u},b.w-10*u);}
    }
    DepthDone().draw(ColorF{1.0,.15});ModMenuStyle::Hover(DepthDone(),m_depthEdit<0);FitLabel(I18n::Get(I18n::Mod::Close),18*u,DepthDone().pos+Vec2{12*u,4*u},DepthDone().w-24*u);
    DepthResetAll().draw(ColorF{1.0,.1});ModMenuStyle::Hover(DepthResetAll(),m_depthEdit<0);FitLabel(I18n::Get(I18n::Mod::ResetAll),18*u,DepthResetAll().pos+Vec2{8*u,4*u},DepthResetAll().w-16*u);
    if(m_depthEdit>=0){
        const auto center=panel.center();const RectF box{center-Vec2{150*u,55*u},300*u,110*u};box.draw(ColorF{.02,.04,.03}).drawFrame(u,Palette::White);
        FitLabel(U"0–200% · Enter: {} / Esc: {}"_fmt(I18n::Get(I18n::Mod::NumberInput),I18n::Get(I18n::Mod::Cancel)),16*u,box.pos+Vec2{10*u,10*u},box.w-20*u);
        Label(m_depthDigits.isEmpty()?U"_":m_depthDigits,30*u,box.pos+Vec2{10*u,43*u});
    }
}

bool SkinOverlay::dirty() const{return m_draft!=m_original || m_depthDraft.backgroundDepthEnabled!=m_originalDepth.backgroundDepthEnabled || m_depthDraft.depthIntensities!=m_originalDepth.depthIntensities;}
void SkinOverlay::requestClose(){if(!dirty()){close(false);return;}m_confirm=true;m_yes=true;m_mouseHeld=-1;}

void SkinOverlay::close(bool save)
{
    if (save && !SkinPreset::Save(m_draft))
    {
        m_error=I18n::Get(I18n::Mod::SkinSaveFailed);
        m_confirm=false;
        return;
    }
    if(save){
        ConfigIni::SetBool(U"vertical_background_depth_enabled",m_depthDraft.backgroundDepthEnabled);
        for(size_t i=0;i<8;++i)ConfigIni::SetInt(U"vertical_depth_strength_"+String{Vertical::kDepthImageNames[i]},m_depthDraft.depthIntensities[i]);
        Vertical::SaveOptionsConfiguration();
    }
    Vertical::DepthPreviewDraft().reset();
    m_game.reset();
    SkinPreset::Activate(save ? m_draft : m_original);
    m_finished=true;
}

void SkinOverlay::update()
{
    m_back.setModScreenLayout();
    m_back.setBackButtonFocused(m_focus==10&&!m_confirm&&!m_depthDialog);
    m_back.update(noco::HitTestEnabledYN{InputUtils::IsUIMouseInputEnabled()});
    if (m_game)
    {
        try { (void)m_game->update(); }
        catch (const Error& error) { m_error=error.what(); m_game.reset(); }
    }
    m_scroll += (m_index-m_scroll)*(1.0-std::exp(-18.0*Scene::DeltaTime()));
    if (SkinFixture::Smoke()) { updateFixture(); return; }
    if(System::GetCommandLineArgs().contains(U"--enhanced-menu-test")){if(m_fixtureClock.sF()>.5)close(false);return;}
    // Read each device every frame so changing focus or releasing FX cannot
    // replay a stale delta. Both configured digital and analog knobs use this path.
    m_cursor.update();
    m_adjust.update();
    m_knob.update();
    m_fieldNav.update();
    if(!m_entryGate.accept(m_entryClock.sF(),KeyConfig::Pressed(kButtonStart)||MouseL.pressed(),KeyConfig::Down(kButtonStart)||MouseL.down()))return;
    const int32 knobDelta=m_knob.deltaCursor();
    const bool selectWithKnob=KeyConfig::Pressed(kButtonFX_L) || KeyConfig::Pressed(kButtonFX_R);
    if(m_depthDialog){updateDepthDialog();return;}
    if (m_confirm)
    {
        if (KeyConfig::Down(kButtonBack)) { m_confirm=false; return; }
        if (KeyLeft.down() || KeyRight.down() || KeyTab.down() || KeyConfig::Down(kButtonFX_L) || KeyConfig::Down(kButtonFX_R)) m_yes=!m_yes;
        const double u=Unit(), cx=Scene::Width()/2.0, cy=Scene::Height()/2.0;
        const RectF yes{cx-145*u,cy+12*u,120*u,44*u}, no{cx+25*u,cy+12*u,120*u,44*u};
        if (Click(yes)) { close(true); return; }
        if (Click(no)) { close(false); return; }
        if (KeyConfig::Down(kButtonStart)) close(m_yes);
        return;
    }
    if (KeyConfig::Down(kButtonBack)) { requestClose(); return; }
    if (KeyTab.down()) {m_focus=TabFocus(m_focus,KeyShift.pressed());m_keyboardRegion=true;m_mouseHeld=-1;}
    if(knobDelta!=0)m_keyboardRegion=false;
    if (selectWithKnob)
    {
        // FX is a modifier only: holding it must not alter the current value.
        // Arrows and knobs share TAB order and reverse navigation.
        const int32 selectionDelta=m_adjust.deltaCursor()+knobDelta;
        m_focus=StepFocus(m_focus,selectionDelta);
        m_keyboardRegion=false;
        m_mouseHeld=-1;
        return;
    }
    if (m_focus>=1 && m_focus<=9 && m_fieldNav.deltaCursor()!=0)
    {
        const int32 next=StepOptionFocus(m_focus,m_fieldNav.deltaCursor());
        if(next!=0 && next!=10)m_focus=next;
        m_mouseHeld=-1;
    }
    bool wheelHandled=false;
    for (int32 field=2;field<=5;++field)
    {
        if(Click(SettingRect(field))){m_focus=field;m_keyboardRegion=false;}
        if (InputUtils::IsUIMouseInputEnabled() && SettingRect(field).mouseOver() && Mouse::Wheel()!=0.0)
        {
            m_focus=field;
            adjustValue(field,Mouse::Wheel()<0 ? 1 : -1);
            wheelHandled=true;
        }
        for(int32 button=0;button<2;++button)
        {
            if(Click(AdjustRect(field,button==1)))
            {
                m_focus=field; m_keyboardRegion=false; adjustValue(field,button ? 1 : -1);
                m_mouseHeld=field*2+button; m_mouseNext=0.35; m_mouseRepeat.restart();
            }
        }
    }
    for(const int32 field:{1,6,7,8}){
        const auto hit=ChoiceRow(field);const auto choice=Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled);
        if(Click(hit)){m_focus=field;m_keyboardRegion=false;}
        const bool left=choice.index>0 && Click(ChoiceArrow(field,choice,false));
        const bool right=choice.index<choice.last && Click(ChoiceArrow(field,choice,true));
        if(left || right){m_focus=field;m_keyboardRegion=false;adjustValue(field,left?-1:1);}
        if(InputUtils::IsUIMouseInputEnabled() && hit.mouseOver() && Mouse::Wheel()!=0){m_focus=field;m_keyboardRegion=false;adjustValue(field,Mouse::Wheel()<0?1:-1);wheelHandled=true;}
    }
    if(Click(DepthPlusRect())){m_focus=9;m_keyboardRegion=false;adjustValue(9,1);}
    if (!MouseL.pressed()) m_mouseHeld=-1;
    if (m_mouseHeld>=0 && InputUtils::IsUIMouseInputEnabled() && AdjustRect(m_mouseHeld/2,m_mouseHeld%2==1).mouseOver() && m_mouseRepeat.sF()>=m_mouseNext)
    {
        adjustValue(m_mouseHeld/2,m_mouseHeld%2 ? 1 : -1);
        m_mouseNext=m_mouseRepeat.sF()+0.05;
    }
    if (m_focus==0 && !wheelHandled)
    {
        if (!m_entries.isEmpty()) changeSelection(m_index+m_cursor.deltaCursor()+knobDelta);
    }
    {
        const double u=Unit(), x=Scene::Width()*2.0/3.0+20*u, w=Scene::Width()/3.0-44*u;
        for (int32 i=0; i<static_cast<int32>(m_entries.size()); ++i)
        {
            const double y=ListCenterY()+(i-m_scroll)*52*u;
            if (y>=PresetList().y+24*u && y<=PresetList().br().y-24*u && Click(PresetRow(i-m_scroll))) { m_focus=0; changeSelection(i); }
        }
    }
    if(m_focus>=1 && m_focus<=9) adjustValue(m_focus,m_adjust.deltaCursor()+knobDelta);
    if(KeyConfig::Down(kButtonStart) && m_focus>=1 && m_focus<=9)adjustValue(m_focus,1);
    else if(KeyConfig::Down(kButtonStart) && (m_focus==0 || m_focus==10)) requestClose();
}

void SkinOverlay::updateFixture()
{
    const double wall=m_fixtureClock.sF();
    if (!m_error.isEmpty()) throw Error{m_error};
    if (m_game && m_fixtureTrace)
    {
        const auto& view=m_game->previewView();
        const auto& d=view.detailedJudgments;
        const int32 judged=d.fastError+d.fastNear+d.fastCritical+d.perfectCritical+d.slowCritical+d.slowNear+d.slowError;
        m_fixtureTrace->writeln(wall,U",",m_game->previewTime(),U",",view.score,U",",view.gaugePercentage,U",",m_draft.folder,U",",static_cast<int>(m_draft.missing),U",",m_draft.background1Brightness,U",",m_draft.background2Opacity,U",",m_draft.laserWidthPercent,U",",m_draft.noteHeightPercent,U",",judged,U",",view.inputTiming.count,U",",view.inputTiming.meanMs(),U",",view.inputTiming.absoluteMeanMs(),U",",view.previewChipDemoCount,U",",d.fastError,U",",d.fastNear,U",",d.fastCritical,U",",d.perfectCritical,U",",d.slowCritical,U",",d.slowNear,U",",d.slowError,U",",view.exScore,U",",view.exMaximum);
    }
    const auto once=[&](int32 id,double time){ if(wall<time || m_fixtureStages.contains(id)) return false; m_fixtureStages.insert(id); return true; };
    if(System::GetCommandLineArgs().contains(U"--viewport-options-smoke")){
        if(once(100,.25)){
            // Render each supported locale with this real preview and its existing hit geometry.
            const String originalLanguage{ConfigIni::GetString(ConfigIni::Key::kLanguage)};
            const std::array<StringView,5> languages{U"English",U"Japanese",U"Korean",U"Simplified Chinese",U"Traditional Chinese"};
            RenderTexture localizedOutput{Scene::Size(),HasDepth::Yes};
            for(size_t language=0;language<languages.size();++language){
                I18n::LoadLanguage(languages[language]);
                for(const auto& entry:m_entries)if(entry.folder==U"default")SkinFixture::Check(entry.description==U"K-Shoot MANIA v2.1.0 default skin",U"skin description metadata stays English across language changes");
                for(const int32 field:{1,6,7,8}){
                    const auto choice=Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled);const auto row=ChoiceRow(field);
                    const auto arrow=ChoiceArrow(field,choice,choice.index<choice.last);
                    SkinFixture::Check(arrow.x>=row.x && arrow.rightX()<=row.rightX()+2*Unit(),U"localized choice arrow stays in its clickable row");
                }
                for(int stage=0;stage<3;++stage){
                    m_confirm=stage==1;m_depthDialog=stage==2;
                    {const ScopedRenderTarget3D t3{localizedOutput.clear(Palette::Black)};const ScopedRenderTarget2D t2{localizedOutput};draw();Graphics3D::Flush();Graphics2D::Flush();}
                    Image image;localizedOutput.readAsImage(image);image.save(FsUtils::GetResourcePath(U"viewport-l10n-{}-{}-{}-{}.png"_fmt(language,Scene::Width(),Scene::Height(),stage)));
                }
                m_confirm=false;m_depthDialog=false;
            }
            I18n::LoadLanguage(originalLanguage);
            SkinFixture::Check(TabFocus(0,false)==1 && TabFocus(1,false)==10 && TabFocus(10,false)==0,U"Tab cycles list-options-close");
            SkinFixture::Check(TabFocus(5,false)==10 && TabFocus(8,true)==0 && TabFocus(10,true)==1,U"Tab treats all options as one region and resets on entry");
            SkinFixture::Check(StepFocus(1,1)==7 && StepFocus(7,1)==9 && StepFocus(3,1)==6 && StepFocus(0,-1)==10,U"knob preserves individual targets including depth plus");
            SkinFixture::Check(SettingRect(6).x>PolicyRect().br().x && SettingRect(8).y>SettingRect(6).y,U"two-column option order");
            SkinFixture::Check(SettingRect(5).br().y<PreviewTop() && OptionsRect().y>=Vertical::MenuContent::Rect().y,U"option grid stays above preview inside letterbox");
            {
                Vertical::DiagnosticInputReplay::Session replay;
                const int32 oldType=ConfigIni::GetInt(ConfigIni::Key::kLaserInputType);
                ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,ConfigIni::Value::LaserInputType::kKeyboard);
                const auto pulse=[&](Button button,bool knob){
                    Vertical::DiagnosticInputReplay::Frame(0);if(knob)m_knob.update();else m_fieldNav.update();
                    Vertical::DiagnosticInputReplay::Frame(Vertical::DiagnosticInputReplay::Mask(button));
                    if(knob){m_knob.update();return m_knob.deltaCursor();}m_fieldNav.update();return m_fieldNav.deltaCursor();
                };
                const int32 down=pulse(kButtonDown,false),up=pulse(kButtonUp,false);
                SkinFixture::Check(StepOptionFocus(7,down)==9&&StepOptionFocus(9,down)==2&&StepOptionFocus(2,up)==9,U"arrow device reaches depth plus between depth and background brightness");
                const int32 right=pulse(kButtonLeftLaserR,true),left=pulse(kButtonLeftLaserL,true);
                SkinFixture::Check(StepFocus(7,right)==9&&StepFocus(9,right)==2&&StepFocus(2,left)==9,U"laser device reaches depth plus in both directions");
                ConfigIni::SetInt(ConfigIni::Key::kLaserInputType,oldType);
            }
            SkinFixture::Check(!dirty(),U"unchanged viewport has no save confirmation requirement");
            const auto depthBefore=m_depthDraft.backgroundDepthEnabled;m_depthDraft.backgroundDepthEnabled=!depthBefore;
            SkinFixture::Check(dirty(),U"depth draft participates in the save decision");m_depthDraft.backgroundDepthEnabled=depthBefore;
            SkinFixture::Check(!dirty(),U"restoring the last saved state suppresses save confirmation again");
            {
                const GameRender::Scope scope{m_previewContext};
                SkinFixture::Check(m_previewSize==Scene::Size() && m_game->renderContext().outputSize==m_previewSize,U"preview uses the actual gameplay output resolution rather than a fixed portrait size");
                auto info=PreviewInfo();info.playOption.skinPreview=false;
                SkinFixture::Check(m_game->settingsHispeed().type==MusicGame::HispeedType::OMod && m_game->settingsHispeed().value==450 && m_game->settingsEffectiveHispeed()==450,U"preview always uses OMod 450 independently of saved hispeed");
                const auto& chart=m_game->chartData();const auto cache=kson::CreateTimingCache(chart.beat);
                MusicGame::Graphics::GraphicsMain ordinary{chart,cache,FileSystem::ParentPath(info.chartFilePath),info.playOption};
                ordinary.update(MusicGame::GameStatus{},m_game->previewView(),cache);
                const auto expected=ordinary.geometrySignature(m_game->previewView());const auto actual=m_game->geometrySignature();
                SkinFixture::Check(actual.size()==expected.size(),U"preview and gameplay report the same geometry points");
                for(size_t point=0;point<actual.size();++point)SkinFixture::Check(Abs(actual[point]-expected[point])<.0001,U"preview camera gear endpoints judgment anchor and cursor plane match gameplay projection");
                JSON report;report[U"width"]=m_previewSize.x;report[U"height"]=m_previewSize.y;report[U"actual"]=actual;report[U"gameplay"]=expected;report.save(FsUtils::GetResourcePath(U"viewport-native-geometry.json"));
            }
            const auto savedDraft=m_draft;const auto savedDepth=m_depthDraft;
            const std::array<int32,9> columnOrder{1,7,9,2,3,6,8,4,5};
            for(int i=0;i<8;++i)SkinFixture::Check(StepOptionFocus(columnOrder[i],1)==columnOrder[i+1] && StepOptionFocus(columnOrder[i+1],-1)==columnOrder[i],U"up/down follows left column then right column");
            for(const int32 field:{1,6,7,8}){
                if(field==7&&!Portrait()){const auto old=m_depthDraft.backgroundDepthEnabled;adjustValue(7,1);SkinFixture::Check(Choice(7,m_draft,old).last==0&&m_depthDraft.backgroundDepthEnabled==old,U"landscape depth is explicitly unavailable and cannot change");continue;}
                adjustValue(field,-100);const auto first=Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled);
                SkinFixture::Check(first.index==0 && !ChoiceText(first).includes(U"◀") && ChoiceText(first).includes(U"▶"),U"first choice hides left arrow");
                adjustValue(field,-1);SkinFixture::Check(Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled).index==0,U"first choice blocks previous input");
                adjustValue(field,100);const auto last=Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled);
                SkinFixture::Check(last.index==last.last && ChoiceText(last).includes(U"◀") && !ChoiceText(last).includes(U"▶"),U"last choice hides right arrow");
                adjustValue(field,1);SkinFixture::Check(Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled).index==last.last,U"last choice blocks next input");
            }
            m_draft=savedDraft;m_depthDraft=savedDepth;refreshDepth();refreshSkin();
            for(const int32 a:{2,3,4,5})for(const int32 b:{2,3,4,5})if(a!=b)for(const bool pa:{false,true})for(const bool pb:{false,true})
                SkinFixture::Check(!AdjustRect(a,pa).intersects(AdjustRect(b,pb)),U"numeric button boxes do not overlap across rows or columns");
            SkinFixture::Check(SettingRect(3).x==SettingRect(2).x && SettingRect(4).x==SettingRect(5).x && SettingRect(4).y==SettingRect(2).y,U"screenshot layout groups both background values left and widths right");
            m_focus=TabFocus(0,false);m_keyboardRegion=true;
        }
        if(once(101,1))ScreenCapture::SaveCurrentFrame(U"viewport-options-group.png");
        if(once(102,1.5)){m_draft.indicatorPortrait=SkinPreset::IndicatorPortrait::Off;SkinPreset::Activate(m_draft);}
        if(once(103,2))ScreenCapture::SaveCurrentFrame(U"viewport-indicator-off.png");
        if(once(104,2.5)){m_draft.indicatorPortrait=SkinPreset::IndicatorPortrait::Both;SkinPreset::Activate(m_draft);}
        if(once(105,3))ScreenCapture::SaveCurrentFrame(U"viewport-indicator-both.png");
        if(once(106,3.5)){m_draft.indicatorPortrait=SkinPreset::IndicatorPortrait::Bottom;SkinPreset::Activate(m_draft);}
        if(once(107,4)){m_depthDraft.backgroundDepthEnabled=false;refreshDepth();}
        if(once(108,4.5))ScreenCapture::SaveCurrentFrame(U"viewport-depth-off.png");
        if(once(109,5)){m_depthDraft.backgroundDepthEnabled=true;refreshDepth();m_depthDialog=true;m_depthIndex=5;}
        if(once(110,5.5))ScreenCapture::SaveCurrentFrame(U"viewport-depth-details.png");
        if(once(111,6)){
            m_depthDraft.depthIntensities[5]=0;refreshDepth();
            SkinFixture::Check(Vertical::EffectiveDepthConfig().depthIntensities[5]==0,U"depth preview uses unsaved draft");
            m_depthDraft.ResetDepthImage(5);refreshDepth();
            SkinFixture::Check(Vertical::EffectiveDepthConfig().depthIntensities[5]==35,U"depth individual reset keeps defaults");
            SkinFixture::Check(Vertical::ModConfig::Load().depthIntensities[5]==35,U"depth preview did not persist draft");
            m_depthDialog=false;
        }
        if(once(112,6.2)){m_focus=TabFocus(1,false);m_keyboardRegion=true;}
        if(once(113,6.6)){SkinFixture::Check(m_focus==10&&m_back.backButtonFocused(),U"BACK group remains visible and focused without a mouse hover");ScreenCapture::SaveCurrentFrame(U"viewport-back-group.png");}
        if(once(114,6.8)){m_focus=TabFocus(10,false);m_keyboardRegion=true;}
        const std::array<SkinPreset::KeyBeamMode,6> modes{SkinPreset::KeyBeamMode::Off,SkinPreset::KeyBeamMode::AllCrit,SkinPreset::KeyBeamMode::AllPerfect,SkinPreset::KeyBeamMode::OnlyCrit,SkinPreset::KeyBeamMode::OnlyPerfect,SkinPreset::KeyBeamMode::Normal};
        for(int i=0;i<6;++i){
            if(once(120+i,7+i)){m_draft.keyBeamMode=modes[i];SkinPreset::Activate(m_draft);m_focus=8;m_keyboardRegion=false;}
            if(once(130+i,7.5+i))ScreenCapture::SaveCurrentFrame(U"viewport-beam-{}.png"_fmt(i));
        }
        if(once(140,14)){TextWriter{FsUtils::GetResourcePath(U"viewport-options-passed.txt")}.writeln(U"PASS: group/individual focus, option bounds, depth draft/reset and six beam modes");close(false);}
        return;
    }
    if (once(0,2.5)) ScreenCapture::SaveCurrentFrame(U"preview-default.png");
    if (once(1,4.0))
        for(int32 i=0;i<static_cast<int32>(m_entries.size());++i) if(m_entries[i].folder==U"한글 Partial") changeSelection(i);
    if (once(2,6.0)) ScreenCapture::SaveCurrentFrame(U"preview-partial.png");
    if (once(3,7.0)) {m_draft.missing=SkinPreset::MissingMode::Error;refreshSkin();}
    if (once(4,9.0)) ScreenCapture::SaveCurrentFrame(U"preview-error.png");
    if (once(5,10.0)) {m_draft.missing=SkinPreset::MissingMode::Default;refreshSkin();}
    if (once(20,11.0)) {adjustValue(2,-10);}
    if (once(21,11.5)) ScreenCapture::SaveCurrentFrame(U"viewport-bg1-zero.png");
    if (once(22,12.0)) {adjustValue(3,-10);}
    if (once(23,12.5)) ScreenCapture::SaveCurrentFrame(U"viewport-both-zero.png");
    if (once(24,13.0)) {adjustValue(2,5);adjustValue(3,5);}
    if (once(25,13.5)) ScreenCapture::SaveCurrentFrame(U"viewport-background-half.png");
    if (once(26,14.0)) {adjustValue(2,5);adjustValue(3,5);}
    if (once(6,16.9)) ScreenCapture::SaveCurrentFrame(U"preview-before-reset.png");
    if (once(7,17.25)) ScreenCapture::SaveCurrentFrame(U"preview-reset-half.png");
    if (once(8,17.7)) ScreenCapture::SaveCurrentFrame(U"preview-reset-zero.png");
    if (once(9,18.3)) ScreenCapture::SaveCurrentFrame(U"preview-loop.png");
    if (once(10,25.0)) ScreenCapture::SaveCurrentFrame(U"preview-second-loop.png");
    if (once(30,20.0)) {adjustValue(4,-11);adjustValue(5,-10);}
    if (once(31,21.0)) ScreenCapture::SaveCurrentFrame(U"viewport-width-50-note-50.png");
    if (once(32,22.0)) {m_draft.laserWidthPercent=SkinPreset::kVerticalLaserPercent;adjustValue(5,10);}
    if (once(33,23.0)) ScreenCapture::SaveCurrentFrame(U"viewport-vertical-note-100.png");
    if (once(34,24.0)) {adjustValue(4,11);adjustValue(5,20);}
    if (once(35,25.5)) ScreenCapture::SaveCurrentFrame(U"viewport-width-125-note-200.png");
    if (once(37,31.5)) ScreenCapture::SaveCurrentFrame(U"viewport-curves-125.png");
    if (once(36,34.0))
    {
        m_draft.background1Brightness=75;m_draft.background2Opacity=40;
        m_draft.laserWidthPercent=SkinPreset::kVerticalLaserPercent;m_draft.noteHeightPercent=125;
        SkinPreset::Activate(m_draft);
    }
    if (once(11,38.5)) {m_confirm=true; ScreenCapture::SaveCurrentFrame(U"preview-confirm.png");}
    if (once(12,39.0)) close(System::GetCommandLineArgs().contains(U"--skin-smoke-save"));
}

void SkinOverlay::draw() const
{
    // Render at the original full logical size, then uniformly scale the complete scene.
    if (m_game)
    {
        Graphics2D::Flush();
        const auto oldCamera=Graphics3D::GetCameraTransform();
        const auto oldEye=Graphics3D::GetEyePosition();
        {
            const ScopedRenderTarget2D target2D{m_target.clear(Palette::Black)};
            const ScopedRenderTarget3D target3D{m_target};
            m_game->draw();
            Graphics3D::Flush();
            Graphics2D::Flush();
        }
        Graphics3D::SetCameraTransform(oldCamera,oldEye);
        if(SkinFixture::Smoke()) {
            static int renderedFrames=0;
            if(++renderedFrames==40) {
                Image source;m_target.readAsImage(source);source.save(FsUtils::GetResourcePath(U"viewport-offscreen-native.png"));
                m_game->draw();Graphics3D::Flush();Graphics2D::Flush();
                ScreenCapture::SaveCurrentFrame(U"viewport-direct-native.png");
                return;
            }
        }
    }
    const double u=Unit(), leftWidth=Scene::Width()*2.0/3.0;
    Scene::Rect().draw(ColorF{0.0,0.7});
    auto heading=ModMenuStyle::TitlePosition();
    ModMenuStyle::Title(U"VIEWPORT CUSTOM",heading,Scene::Width()*.48);
    const auto policy=PolicyRect();
    if(m_keyboardRegion && m_focus>=1 && m_focus<=9)OptionsRect().drawFrame(1*u,ColorF{1.0,.75});

    for(int32 field=2;field<=5;++field)
    {
        const auto row=SettingRect(field);
        if(m_focus==field) row.drawFrame(1*u,ColorF{1.0,0.65});
        ModMenuStyle::Hover(row,!m_confirm&&!m_depthDialog);
        FitLabel(SettingText(field,m_draft),17*u,row.pos+Vec2{4*u,3*u},row.w-64*u);
        for(int32 button=0;button<2;++button)
        {
            const auto rect=AdjustRect(field,button==1);
            rect.draw(ColorF{1.0,0.1});ModMenuStyle::Hover(rect,!m_confirm&&!m_depthDialog);
            Label(button ? U"+" : U"−",(Portrait()?18:16)*u,rect.pos+Vec2{6*u,1*u});
        }
    }
    for(const int32 field:{1,6,7,8}){
        const auto rect=ChoiceRow(field);
        if(m_focus==field)rect.drawFrame(1*u,ColorF{1.0,.65});
        ModMenuStyle::Hover(rect,!m_confirm&&!m_depthDialog);
        const auto choice=Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled);
        ModMenuStyle::Hover(ChoiceArrow(field,choice,false),!m_confirm&&!m_depthDialog&&choice.index>0);
        ModMenuStyle::Hover(ChoiceArrow(field,choice,true),!m_confirm&&!m_depthDialog&&choice.index<choice.last);
        FitLabel(ChoiceText(Choice(field,m_draft,m_depthDraft.backgroundDepthEnabled)),17*u,rect.pos+Vec2{4*u,3*u},rect.w-8*u);
    }
    const auto depthPlus=DepthPlusRect();depthPlus.draw(ColorF{1.0,.1});if(m_focus==9)depthPlus.drawFrame(1*u,Palette::White);
    ModMenuStyle::Hover(depthPlus,Portrait()&&!m_confirm&&!m_depthDialog);
    Label(U"+",18*u,depthPlus.pos+Vec2{6*u,2*u});
    const double scale=0.95*Min((leftWidth-48*u)/m_previewSize.x, (Scene::Height()-230*u)/m_previewSize.y);
    const Vec2 previewSize=Vec2{m_previewSize}*scale;
    const Vec2 previewPos{(leftWidth-previewSize.x)/2.0, PreviewTop()};
    RectF{previewPos,previewSize}.draw(Palette::Black).drawFrame(1*u,ColorF{0.45});
    if (m_game) m_target.scaled(scale).draw(previewPos);
    Line{leftWidth,Portrait()?PresetPanel().y:76*u,leftWidth,Portrait()?PresetPanel().br().y:Scene::Height()-70*u}.draw(1*u,ColorF{1.0,0.2});
    if (m_focus==0)
    {
        PresetPanel().drawFrame(Portrait()?PortraitScale():2*u,Palette::White);
        const auto panel=PresetPanel();
        RectF{panel.x+2*u,panel.y+2*u,panel.w-4*u,40*u}.draw(ColorF{1.0,0.12});
    }
    Label(U"SKIN PRESET", 23*u, {leftWidth+24*u,Portrait()?104*PortraitScale()+ContentOffset():112*u});
    const RectF listArea=PresetList();
    {
        struct RestoreScissor {Rect old=Graphics2D::GetScissorRect();~RestoreScissor(){Graphics2D::SetScissorRect(old);}} restoreScissor;
        const ScopedRenderStates2D raster{RasterizerState::SolidCullNoneScissor};
        Graphics2D::SetScissorRect(listArea.asRect());
        const RectF selectedRow=PresetRow();
        selectedRow.draw(ColorF{1.0,m_focus==0 ? 0.23 : 0.13});
        if (m_focus==0) selectedRow.drawFrame(1*u,ColorF{1.0,0.8});
        for (int32 i=0; i<static_cast<int32>(m_entries.size()); ++i)
        {
            const double y=ListCenterY()+(i-m_scroll)*52*u;
            if (y < listArea.y+24*u || y > listArea.br().y-24*u) continue;
            ModMenuStyle::Hover(PresetRow(i-m_scroll),!m_confirm&&!m_depthDialog);
            // Keep full folder names, shrink long names to the available width.
            const auto text=AssetManagement::SystemFont()(m_entries[i].folder);
            const double size=Min(24*u,(listArea.w-32*u)/Max(1.0,text.region(1.0).w));
            Label(m_entries[i].folder,size,Vec2{leftWidth+32*u,y-14*u});
        }
    }
    if (!m_entries.isEmpty())
    {
        const auto& entry=m_entries[m_index];
        // Reserve a dedicated author row and three description rows above the hint.
        const Vec2 detailsPos{24*u,(Portrait()?188*PortraitScale()+ContentOffset():previewPos.y)+previewSize.y+8*u};
        const double detailsSize=(Portrait()?17:14)*u;
        const double detailsStep=(Portrait()?20:16)*u;
        FitLabel(String{I18n::Get(I18n::Mod::AuthorLabel)} + entry.author,detailsSize,detailsPos,leftWidth-48*u);
        // Skin-authored metadata is language-independent; only the Author label is localized.
        const auto lines=entry.description.split(U'\n');
        for (size_t line=0;line<Min(size_t{3},lines.size());++line)
            FitLabel(lines[line],detailsSize,detailsPos+Vec2{0,(line+1)*detailsStep},leftWidth-48*u);
    }
    m_back.draw();
    ModMenuStyle::Footer(U"↑↓: {}   ←→ / LASER: {}"_fmt(I18n::Get(I18n::Mod::ItemSelect),I18n::Get(I18n::Mod::ValueAdjust)),U"TAB / FX + LASER: {}   START: {}   BACK: {}"_fmt(I18n::Get(I18n::Mod::GroupItemMove),I18n::Get(I18n::Mod::Enter),I18n::Get(I18n::Mod::Close)));
    if (!m_error.isEmpty()) AssetManagement::SystemFont()(m_error).draw(16*u,RectF{24*u,Portrait()?1107*PortraitScale():160*u,leftWidth-48*u,25*u},Palette::Orange);
    if(m_depthDialog){drawDepthDialog();m_back.draw();return;}
    if(m_confirm){ModMenuStyle::Dialog(I18n::Get(I18n::Mod::SaveQuestion),{U"YES",U"NO"},m_yes?0:1);m_back.draw();}

}

bool SkinOverlay::PaletteClicked() { return Click(PaletteRect()); }

void SkinOverlay::DrawPalette(bool focused)
{
    const auto rect=PaletteRect();
    const double u=Unit();
    const Vec2 c=rect.center();
    if (focused || (InputUtils::IsUIMouseInputEnabled() && rect.mouseOver()))
        rect.rounded(9*u).draw(ColorF{0.0,0.35}).drawFrame(1*u,Palette::White);
    // Shared UI icon stays available independently of the selected skin.
    static const Texture icon{FsUtils::GetResourcePath(U"ui/custom_icon.png")};
    const double iconScale=48*u/Max(icon.width(),icon.height());
    icon.scaled(iconScale).drawAt(c);
}

