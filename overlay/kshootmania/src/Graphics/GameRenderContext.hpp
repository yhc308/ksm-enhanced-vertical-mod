#pragma once
#include <Siv3D.hpp>
#include "Vertical/VerticalModConfig.hpp"
#include "Ini/ConfigIni.hpp"
namespace GameRender
{
	struct Context
	{
		Size outputSize;
		bool vertical;
	};
	inline Optional<Context>& Override()
	{
		static thread_local Optional<Context> value;
		return value;
	}
	inline Context Current()
	{
		// Avoid evaluating the settings fallback while a render scope is active.
		if (const auto& context = Override())
			return *context;
		return Context{ Scene::Size(), ConfigIni::GetBool(U"vertical_enabled", false) };
	}
	inline Size Size()
	{
		return Current().outputSize;
	}
	inline int32 Width()
	{
		return Size().x;
	}
	inline int32 Height()
	{
		return Size().y;
	}
	inline Point Center()
	{
		return Size() / 2;
	}
	inline Vec2 CenterF()
	{
		return Vec2{ Size() } * .5;
	}
	inline Rect Rect()
	{
		return s3d::Rect{ Size() };
	}
	inline bool VerticalEnabled()
	{
		return Current().vertical;
	}
	class Scope
	{
		Optional<Context> m_old;

	public:
		explicit Scope(const Context& context)
			: m_old(Override())
		{
			Override() = context;
		}
		~Scope()
		{
			Override() = m_old;
		}
		Scope(const Scope&) = delete;
		Scope& operator=(const Scope&) = delete;
	};
}
