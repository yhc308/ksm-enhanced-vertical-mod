#pragma once
#include <Siv3D.hpp>

#ifndef KSM_ENABLE_DIAGNOSTICS
#define KSM_ENABLE_DIAGNOSTICS 0
#endif

namespace ModDiagnostics
{
	// Original test-play arguments are parsed separately and remain available in every build.
	inline const Array<String>& Arguments() noexcept
	{
#if KSM_ENABLE_DIAGNOSTICS
		return System::GetCommandLineArgs();
#else
		static const Array<String> empty;
		return empty;
#endif
	}
}
