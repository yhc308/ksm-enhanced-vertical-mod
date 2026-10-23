#pragma once
#include <Siv3D.hpp>
namespace MusicGame
{
	struct GaugeVisual
	{
		bool enabled = false;
		double percent = 0;
		double dodge = 0;
		double bannerOpacity = 0;
		double burn = 0;
	};
	// All values here are display-only. No scoring or result code reads this state.
	class GaugePresentation
	{
		bool m_initialized = false, m_transition = false;
		double m_transitionStartPercent = 0, m_transitionElapsedSeconds = 0, m_previousChartTimeSeconds = 0;
		GaugeVisual m_visual;

	public:
		static constexpr double kPlaybackSpeed = 1.5;
		static constexpr double kDodgeHold = .7;
		static constexpr double kDodgeDuration = .7;
		static constexpr double kFadeDuration = .6;
		static constexpr double kBurnStart = .4;
		static constexpr double kBurnDuration = .4;
		static constexpr double kDodgeReleaseStart = .8;
		static constexpr double kDodgeReleaseDuration = .4;
		static double FadeStart()
		{
			return .5;
		}
		static double EaseOut(double t);
		static Optional<double>& FixtureStep();
		static Optional<double>& FixtureChartTime();
		static double FrameDelta()
		{
			return FixtureStep().value_or(Scene::DeltaTime());
		}
		void reset()
		{
			*this = GaugePresentation{};
		}
		const GaugeVisual& visual() const
		{
			return m_visual;
		}
		GaugeVisual update(double actualPercent, bool isApsProtected, bool switched, bool paused, double deltaSeconds, double chartTimeSeconds);
	};
}
