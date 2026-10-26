#pragma once
#include <cstddef>
#include <deque>
#include <optional>

namespace MusicGame::Judgment
{
	class RecentTimingWindow
	{
	private:
		struct Sample
		{
			double timeSeconds;
			double offsetMs;
		};
		std::deque<Sample> m_samples;
		double m_sumMs = 0;

	public:
		void clear()
		{
			m_samples.clear();
			m_sumMs = 0;
		}
		void add(double timeSeconds, double offsetMs)
		{
			m_samples.push_back({ timeSeconds, offsetMs });
			m_sumMs += offsetMs;
		}
		void expire(double nowSeconds, double windowSeconds)
		{
			while (!m_samples.empty() && m_samples.front().timeSeconds <= nowSeconds - windowSeconds)
			{
				m_sumMs -= m_samples.front().offsetMs;
				m_samples.pop_front();
			}
		}
		bool empty() const
		{
			return m_samples.empty();
		}
		std::size_t size() const
		{
			return m_samples.size();
		}
		std::optional<double> mean() const
		{
			return empty() ? std::nullopt : std::optional<double>{ m_sumMs / m_samples.size() };
		}
	};
}
