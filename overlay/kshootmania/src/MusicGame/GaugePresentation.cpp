#include "GaugePresentation.hpp"

namespace MusicGame
{
	double GaugePresentation::EaseOut(double t)
	{
		t = Clamp(t, 0.0, 1.0);
		return 1 - (1 - t) * (1 - t);
	}

	Optional<double>& GaugePresentation::FixtureStep()
	{
		static thread_local Optional<double> step;
		return step;
	}

	Optional<double>& GaugePresentation::FixtureChartTime()
	{
		static thread_local Optional<double> time;
		return time;
	}

	GaugeVisual GaugePresentation::update(double actualPercent, bool isApsProtected, bool switched, bool paused, double deltaSeconds, double chartTimeSeconds)
	{
		if (!m_initialized || chartTimeSeconds < m_previousChartTimeSeconds - .001)
		{
			m_initialized = true;
			m_transition = false;
			m_visual = { true, actualPercent, 0, isApsProtected ? 1.0 : 0.0 };
		}
		m_previousChartTimeSeconds = chartTimeSeconds;
		if (switched)
		{
			m_transitionStartPercent = m_visual.percent;
			m_transitionElapsedSeconds = 0;
			m_transition = true;
		}
		else if (m_transition && !paused)
			m_transitionElapsedSeconds += Max(0.0, deltaSeconds) * kPlaybackSpeed;
		m_visual.enabled = true;
		if (m_transition)
		{
			const double blend = EaseOut(m_transitionElapsedSeconds);
			m_visual.percent = m_transitionStartPercent * (1 - blend) + actualPercent * blend;
			const double release = EaseOut((m_transitionElapsedSeconds - kDodgeReleaseStart) / kDodgeReleaseDuration);
			m_visual.dodge = EaseOut(m_transitionElapsedSeconds / kDodgeDuration) * (1 - (1 - kDodgeHold) * release);
			m_visual.burn = EaseOut((m_transitionElapsedSeconds - kBurnStart) / kBurnDuration);
			const double fade = Clamp((m_transitionElapsedSeconds - FadeStart()) / kFadeDuration, 0.0, 1.0);
			m_visual.bannerOpacity = 1 - fade * fade;
			if (m_transitionElapsedSeconds >= Max(FadeStart() + kFadeDuration, kDodgeReleaseStart + kDodgeReleaseDuration) - 1e-9)
			{
				m_visual.percent = actualPercent;
				m_visual.bannerOpacity = 0;
				m_transition = false;
			}
		}
		else
			m_visual = { true, actualPercent, 0, isApsProtected ? 1.0 : 0.0 };
		return m_visual;
	}
}
