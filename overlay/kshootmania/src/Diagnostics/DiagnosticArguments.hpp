#pragma once
#include <Siv3D.hpp>
#include "Common/CommandLine.hpp"

#ifndef KSM_ENABLE_DIAGNOSTICS
#define KSM_ENABLE_DIAGNOSTICS 0
#endif

namespace ModDiagnostics
{
	// Original test-play arguments are parsed separately and remain available in every build.
	inline const Array<String>& Arguments()
	{
#if KSM_ENABLE_DIAGNOSTICS
		return CommandLine::Arguments();
#else
		static const Array<String> empty;
		return empty;
#endif
	}
}
