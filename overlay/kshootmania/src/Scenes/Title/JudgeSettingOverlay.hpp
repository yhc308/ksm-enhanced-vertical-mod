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

class JudgeSettingOverlay
{
	std::shared_ptr<MusicGame::JudgeSettingSession> m_session = std::make_shared<MusicGame::JudgeSettingSession>();
	MusicGame::JudgeSettingValues m_saved;
	std::unique_ptr<MusicGame::GameMain> m_game;
	MusicGame::Graphics::PlayJudgeIndicator m_indicator;
	CommonOverlay m_back;
	std::shared_ptr<noco::Canvas> m_hispeedCanvas;
	bool m_startArmed = false;
	Texture m_menuPanel, m_panelGlow;
	Texture m_itemCursor{ SkinPreset::LoadTexture(U"standby_cur.gif") };
	const ksmaudio::Sample m_itemSe{ "se/sel_m.wav" }, m_valueSe{ "se/hover.wav" };
	static constexpr int32 kGlowPadding = 24;
	CursorInput m_navigation{ { .type = CursorInput::Type::Vertical, .buttonFlags = CursorButtonFlags::kArrow | CursorButtonFlags::kLaser, .buttonIntervalSec = .12, .buttonIntervalSecFirst = .35 } };
	CursorInput m_adjustment{ { .type = CursorInput::Type::Horizontal, .buttonFlags = CursorButtonFlags::kArrow | CursorButtonFlags::kLaser, .buttonIntervalSec = .08, .buttonIntervalSecFirst = .35 } };
	int32 m_focus = 5, m_dialogIndex = 0;
	bool m_finished = false, m_dialog = false, m_waitRelease = false, m_focusPaused = false, m_hoverPending = false;
	double m_unlockAt = 0;
	String m_error;
	struct Layout
	{
		RectF left, right, mini, bottom;
		Vec2 recent, last;
		double unit;
	};
	Layout layout() const;
	RectF statTitleRect(Vec2 pos, const Layout& l) const
	{
		return { pos.x - 105 * l.unit, pos.y, 210 * l.unit, 23 * l.unit };
	}
	RectF statValueRect(Vec2 pos, const Layout& l) const;
	RectF controlsRect() const;
	RectF stopRect() const;
	RectF row(int32 index) const;
	RectF titleRect(int32 index) const;
	RectF valueRect(int32 index) const;
	std::pair<int32, int32> choiceBounds(int32 i) const;
	int32 choiceValue(int32 i) const;
	void select(int32 index);
	void change(int32 index, int32 delta);
	void preparePanel();
	static MusicGame::JudgeSettingValues load();
	static String value(double ms)
	{
		return Abs(ms) < .05 ? U"0.0ms" : U"{:+.1f}ms"_fmt(ms);
	}
	void label(StringView text, const RectF& rect, double size, ColorF color = Palette::White, bool outlined = false) const;
	bool buttonDown() const;
	bool chord() const
	{
		return KeyConfig::Pressed(kButtonFX_L) && KeyConfig::Pressed(kButtonFX_R) && KeyConfig::Pressed(kButtonStart);
	}
	bool chordReleased() const
	{
		return !KeyConfig::Pressed(kButtonFX_L) && !KeyConfig::Pressed(kButtonFX_R) && !KeyConfig::Pressed(kButtonStart);
	}
	bool save();
	void activate();

public:
	JudgeSettingOverlay();
	~JudgeSettingOverlay();
	JSON layoutReport() const;
	const auto& session() const
	{
		return *m_session;
	}
	auto& session()
	{
		return *m_session;
	}
	auto& game()
	{
		return *m_game;
	}
	RectF settingRow(int32 index) const
	{
		return row(index);
	}
	RectF closeButton() const
	{
		return ModMenuStyle::BackRect();
	}
	std::pair<bool, bool> visibleArrows(int32 index) const;
	static ColorF inputColor(JudgeColor palette, double ms);
	int32 focus() const
	{
		return m_focus;
	}
	bool dialog() const
	{
		return m_dialog;
	}
	bool dirty() const
	{
		return !(m_session->values == m_saved);
	}
	bool finished() const
	{
		return m_finished;
	}
	void requestMenu(bool consumedChord = false);
	void requestExit();
	void pointerMoveAt(const Vec2& point);
	void pointerDownAt(const Vec2& point);
	void update(double delta = Scene::DeltaTime());
	RectF dialogRow(int32 index) const
	{
		return ModMenuStyle::DialogButton(3, index);
	}

	void drawWorld() const;
	void draw() const;
};
