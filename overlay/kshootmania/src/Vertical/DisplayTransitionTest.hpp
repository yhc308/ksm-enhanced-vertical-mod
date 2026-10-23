#pragma once
#include "VerticalDisplay.hpp"
#include "Input/KeyConfig.hpp"

namespace Vertical
{
    // Exercises the real display transaction from the normal main loop.
    // No synthetic OS input; confirmation uses the game's existing virtual key.
    class DisplayTransitionTest
    {
        TextWriter m_report{U"vertical-display-test.csv"};
        Stopwatch m_timer{StartImmediately::Yes};
        ModConfig m_config = ModConfig::Load();
        int m_step = 0;
        bool m_passed = true;
        bool m_confirmSent = false;
        Size m_borderlessSize;
        Point m_borderlessPosition;

        void record(StringView name, bool passed)
        {
            m_passed = m_passed && passed;
            const auto& state = Window::GetState();
            m_report.writeln(U"{},{},{},{},{},{}"_fmt(name, Scene::Width(), Scene::Height(),
                state.frameBufferSize.x, state.frameBufferSize.y, passed ? U"PASS" : U"FAIL"));
        }
        void begin(DisplayMode mode, Resolution size)
        {
            m_config.enabled = true;
            m_config.displayMode = mode;
            m_config.windowSize = size;
            record(U"begin_transaction", BeginDisplayChange(m_config));
            m_timer.restart();
            m_confirmSent = false;
        }
    public:
        DisplayTransitionTest() { m_report.writeln(U"check,scene_width,scene_height,physical_width,physical_height,status"); }
        bool update()
        {
            switch (m_step)
            {
            case 0:
                if (m_timer.sF() < 0.5) break;
                begin(DisplayMode::Windowed, {720,1280});
                ++m_step;
                break;
            case 1:
                if (m_timer.sF() < 0.8) break;
                if (!m_confirmSent)
                {
                    record(U"windowed_resize", Window::GetState().style == WindowStyle::Fixed &&
                        Scene::Size() == Window::GetState().frameBufferSize &&
                        Abs(Scene::Width() / static_cast<double>(Scene::Height()) - 720.0/1280) < 0.003);
                    record(U"focus_after_change", Window::GetState().focused);
                    record(U"keep_selected_by_default",DisplayConfirmationSelection()==0);
                    ScreenCapture::SaveCurrentFrame(U"vertical-display-confirmation-preview.png");
                    KeyConfig::RequestVirtualDown(kButtonStart);
                    m_confirmSent = true;
                }
                else if (!IsDisplayChangePending())
                {
                    record(U"mapped_start_confirmation", true);
                    begin(DisplayMode::BorderlessFullscreen, {720,1280});
                    ++m_step;
                }
                break;
            case 2:
                if (m_timer.sF() < 0.3) break;
                if (!m_confirmSent)
                {
                    const auto monitors = System::EnumerateMonitors();
                    const auto& monitor = monitors[System::GetCurrentMonitorIndex()];
                    record(U"borderless_desktop_size", Window::GetState().style == WindowStyle::Frameless &&
                        Window::GetState().frameBufferSize == monitor.displayRect.size && Scene::Size() == Size{720,1280});
                    record(U"fullscreen_keeps_render_resolution", Scene::Size() == Size{720,1280} &&
                        Scene::GetResizeMode() == ResizeMode::Keep && Scene::GetLetterBox() == ColorF{Palette::Black});
                    record(U"focus_after_borderless", Window::GetState().focused);
                    m_borderlessSize = Scene::Size();
                    m_borderlessPosition = Window::GetPos();
                    KeyConfig::RequestVirtualDown(kButtonStart);
                    m_confirmSent = true;
                }
                else if (!IsDisplayChangePending())
                {
                    begin(DisplayMode::Windowed, {360,640});
                    ++m_step;
                }
                break;
            case 3:
                if (m_timer.sF() < 0.3) break;
                record(U"small_window_resize", Scene::Size() == Size{360,640} && Window::GetState().style == WindowStyle::Fixed);
                KeyConfig::RequestVirtualDown(kButtonBack);
                ++m_step;
                break;
            case 4:
                if (IsDisplayChangePending()) break;
                record(U"mapped_back_restores_borderless", Scene::Size() == m_borderlessSize &&
                    Window::GetPos() == m_borderlessPosition && Window::GetState().style == WindowStyle::Frameless);
                m_report.writeln(U"overall,,,,,{}"_fmt(m_passed ? U"PASS" : U"FAIL"));
                ++m_step;
                return true;
            }
            if (m_step < 5 && m_timer.sF() > 4.0)
            {
                record(U"transaction_timeout", false);
                m_report.writeln(U"overall,,,,,FAIL");
                m_step = 5;
                return true;
            }
            return m_step == 5;
        }
    };
}
