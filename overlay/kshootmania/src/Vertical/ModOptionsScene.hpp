#pragma once
#include "Diagnostics/DiagnosticArguments.hpp"
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
	enum Section
	{
		Root = 0,
		Graphics = 1,
		Developer = 2,
		DepthImages = 3
	};
	int m_section;
	Vertical::ModConfig m_config = Vertical::ModConfig::Load();
	CommonOverlay m_back;
	LinearMenu m_menu;
	CursorInput m_values{ MenuInputProfiles::OptionValues() };
	String m_message;
	int m_editIndex = -1;
	String m_digits;
	bool m_replaceOnType = false;
	Vertical::MenuEntryGate m_entryGate;
	Stopwatch m_entryClock{ StartImmediately::Yes };
	static int Count(int section)
	{
		return section == DepthImages ? 9 : section == Graphics ? 2
										: section == Developer	? 6
																: (Vertical::OptionProbe::active() ? 4 : 3);
	}
	RectF rowRect(int i) const;
	RectF resetRect(int i) const;
	void saveGraphics();
	void resetImage(int i);
	void resetAllImages();
	void beginNumber(int index);
	void cancelNumber();
	bool commitNumber();
	void feedNumber(StringView text);
	RectF numberPanel() const;
	RectF hintRect() const;
	RectF numberButton(bool apply) const;
	void returnRoot(int cursor)
	{
		requestNextScene<ModOptionsScene>(Root, cursor);
	}
	void saveDeveloper();
	void back();
	void toggleMode();
	void edit(int delta);
	void decide();
	bool probe();

public:
	explicit ModOptionsScene(int section = Root, int cursor = 0);
	void update() override;
	void draw() const override;
};
