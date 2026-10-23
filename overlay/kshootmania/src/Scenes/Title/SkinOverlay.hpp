#pragma once
#include "Vertical/MenuEntryGate.hpp"
#include "Vertical/VerticalModConfig.hpp"
#include "Common/SkinPreset.hpp"
#include "MusicGame/GameMain.hpp"
#include "Input/Cursor/CursorInput.hpp"
#include "Scenes/Common/CommonOverlay.hpp"

// An overlay owns a private preview session, never a PlayScene or a score writer.
class SkinOverlay
{
    CommonOverlay m_back;
    SkinPreset::Settings m_original;
    SkinPreset::Settings m_draft;
    Array<SkinPreset::Entry> m_entries;
    CursorInput m_cursor;
    CursorInput m_adjust;
    CursorInput m_knob;
    CursorInput m_fieldNav;
    std::unique_ptr<MusicGame::GameMain> m_game;
    const GameRender::Context m_previewContext=GameRender::Current();
    const Size m_previewSize;
    mutable RenderTexture m_target;
    int32 m_index = 0;
    double m_scroll = 0;
    int32 m_focus = 0; // 0 list, 1 policy, 2-5 numeric, 6 indicator, 7 depth, 8 beam, 9 depth details, 10 close
    bool m_keyboardRegion=false;
    Vertical::ModConfig m_depthDraft=Vertical::ModConfig::Load();
    const Vertical::ModConfig m_originalDepth=m_depthDraft;
    bool m_depthDialog=false;
    int32 m_depthIndex=0;
    int32 m_depthEdit=-1;
    String m_depthDigits;
    void refreshDepth();
    void updateDepthDialog();
    void drawDepthDialog() const;
    int32 m_mouseHeld = -1;
    Stopwatch m_mouseRepeat;
    double m_mouseNext = 0.35;
    Vertical::MenuEntryGate m_entryGate;
    Stopwatch m_entryClock{StartImmediately::Yes};
    bool m_confirm = false;
    bool m_yes = true;
    bool m_finished = false;
    String m_error;
    Stopwatch m_fixtureClock{StartImmediately::Yes};
    std::unique_ptr<TextWriter> m_fixtureTrace;
    HashSet<int32> m_fixtureStages;
    void updateFixture();
    void changeSelection(int32 index);
    void refreshSkin();
    void adjustValue(int32 field, int32 delta);
    bool dirty() const;
    void requestClose();
    void close(bool save);
public:
    SkinOverlay();
    ~SkinOverlay();
    void update();
    void draw() const;
    bool finished() const { return m_finished; }
    static bool PaletteClicked();
    static void DrawPalette(bool focused);
};
