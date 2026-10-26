#pragma once

namespace DiagnosticRunner
{
	// Return true only when a requested diagnostic consumed startup and cleaned up.
	bool RunAfterAudioInitialization();
	bool RunAfterAssetRegistration();
	bool RunAfterInputInitialization();
}
