#include "InstallerCore.hpp"
#include <commctrl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <thread>
#include <memory>
#include <map>

namespace
{
	using namespace Setup;
	constexpr UINT kProgress = WM_APP + 1, kFinished = WM_APP + 2, kPercent = WM_APP + 3;
	std::unique_ptr<Package> g_package;
	HWND g_window{}, g_language{}, g_path{}, g_status{}, g_install{}, g_remove{}, g_browse{}, g_progress{}, g_hint{};
	HFONT g_font{};
	std::atomic_bool g_cancel{false};
	bool g_working = false, g_close = false;
	int g_lang = 1, g_percent = 0;
	Path g_target;
	std::string g_currentAction;
	const wchar_t* T(const char* key)
	{
		static const std::map<std::string, std::vector<const wchar_t*>> text{
			{"title", {L"Enhanced Vertical Mod — Setup", L"Enhanced Vertical Mod — インストーラー", L"Enhanced Vertical Mod — 설치", L"Enhanced Vertical Mod — 安装程序", L"Enhanced Vertical Mod — 安裝程式"}},
			{"browse", {L"Choose folder", L"フォルダー選択", L"폴더 선택", L"选择文件夹", L"選擇資料夾"}},
			{"install", {L"Install mod", L"モードをインストール", L"모드 설치", L"安装模组", L"安裝模組"}},
			{"remove", {L"Remove mod", L"モードを削除", L"모드 제거", L"移除模组", L"移除模組"}},
			{"select", {L"Choose the KSM 2.1.0 folder containing kshootmania.exe.", L"kshootmania.exe がある KSM 2.1.0 フォルダーを選択してください。", L"kshootmania.exe가 있는 KSM 2.1.0 폴더를 선택하세요.", L"请选择包含 kshootmania.exe 的 KSM 2.1.0 文件夹。", L"請選擇包含 kshootmania.exe 的 KSM 2.1.0 資料夾。"}},
			{"ready", {L"Compatible KSM 2.1.0 — ready to install", L"対応 KSM 2.1.0 — インストールできます", L"호환 KSM 2.1.0 — 설치 가능", L"兼容 KSM 2.1.0 — 可以安装", L"相容 KSM 2.1.0 — 可以安裝"}},
			{"installed", {L"Installed version: ", L"インストール済み: ", L"설치된 버전: ", L"已安装版本：", L"已安裝版本："}},
			{"working", {L"Working... Closing this window requests a safe rollback.", L"処理中… 閉じると安全に取り消します。", L"작업 중… 창을 닫으면 안전하게 취소·복구합니다.", L"处理中… 关闭窗口将安全取消并回滚。", L"處理中… 關閉視窗將安全取消並還原。"}},
			{"installDone", {L"Installation complete. Run ksm-enhanced-vertical.exe in the selected folder.", L"インストール完了。選択したフォルダーの ksm-enhanced-vertical.exe を起動してください。", L"설치 완료. 선택한 폴더의 ksm-enhanced-vertical.exe를 실행하세요.", L"安装完成。请运行所选文件夹中的 ksm-enhanced-vertical.exe。", L"安裝完成。請執行所選資料夾中的 ksm-enhanced-vertical.exe。"}},
			{"removeDone", {L"Mod removed. Scores and settings preserved. Edited images remain in place.", L"削除完了。スコアと設定は保存され、編集した画像はそのまま残ります。", L"모드 제거 완료. 점수·설정과 직접 수정한 이미지는 유지했습니다.", L"模组已移除。分数、设置及修改过的图片已保留。", L"模組已移除。分數、設定及修改過的圖片已保留。"}},
			{"hint", {L"You can still play the original K-Shoot Mania after installing the mod.\r\nRemoving the mod removes only mod files. EX scores, records and skin settings are preserved. Use this same setup EXE to remove it.", L"モードのインストール後も原版の K-Shoot Mania をプレイできます。\r\n削除時はモードファイルのみを削除し、EXスコア・記録・スキン設定などは保持します。削除にも同じインストーラーEXEを使用してください。", L"모드 설치 시에도 원본 K-Shoot Mania 플레이가 가능합니다.\r\n모드를 삭제 시, 모드 파일만 제거되며, 모드로 플레이했던 EX 스코어, 기록, 스킨 설정 파일 등은 보존됩니다. 제거 시에도 해당 설치기 EXE를 사용해주세요.", L"安装模组后，仍可游玩原版 K-Shoot Mania。\r\n移除时仅删除模组文件，保留使用模组时的 EX 分数、记录及皮肤设置等。移除时也请使用同一个安装程序 EXE。", L"安裝模組後，仍可遊玩原版 K-Shoot Mania。\r\n移除時僅刪除模組檔案，保留使用模組時的 EX 分數、記錄及外觀設定等。移除時也請使用同一個安裝程式 EXE。"}},
			{"cleanupWarning", {L"Mod removal completed, but some empty folders could not be cleaned up:", L"モードの削除は完了しましたが、一部の空フォルダーを整理できませんでした:", L"모드 제거는 완료됐지만 일부 빈 폴더를 정리하지 못했습니다:", L"模组已移除，但部分空文件夹无法清理：", L"模組已移除，但部分空資料夾無法清理："}},
			{"cleanupRecovered", {L"Recovered an interrupted mod removal.", L"中断されたモード削除を復旧しました。", L"중단된 모드 제거 작업을 복구했습니다.", L"已恢复中断的模组移除操作。", L"已恢復中斷的模組移除作業。"}},
			{"missingExe", {L"Cannot install because kshootmania.exe is missing from the selected folder. Details: ", L"選択したフォルダーに kshootmania.exe がないためインストールできません。詳細: ", L"선택된 경로에 kshootmania.exe가 없어 설치를 진행할 수 없습니다. 상세: ", L"所选路径中找不到 kshootmania.exe，无法继续安装。详情：", L"所選路徑中找不到 kshootmania.exe，無法繼續安裝。詳細："}},
			{"error", {L"Action stopped; files were preserved or rolled back. Details: ", L"処理を停止しました。ファイルは保持または復元されました。詳細: ", L"작업을 중단했습니다. 파일은 보존하거나 작업 전 상태로 복구했습니다. 상세: ", L"操作已停止，文件已保留或回滚。详情：", L"操作已停止，檔案已保留或還原。詳細："}},
			{"permission", {L"Write permission is required. Retry this action as administrator?", L"書き込み権限が必要です。管理者として再試行しますか？", L"쓰기 권한이 필요합니다. 관리자 권한으로 같은 작업을 재시도할까요?", L"需要写入权限。是否以管理员身份重试？", L"需要寫入權限。是否以管理員身分重試？"}},
			{"busy", {L"Close the game in this folder, then retry.", L"このフォルダーのゲームを終了してから再試行してください。", L"이 폴더에서 실행 중인 게임을 종료한 뒤 다시 시도하세요.", L"请关闭此文件夹中运行的游戏后重试。", L"請關閉此資料夾中執行的遊戲後重試。"}},
			{"target", {L"Unsupported folder or changed original resources. Select verified KSM 2.1.0.", L"未対応のフォルダーまたは変更された原本です。対応 KSM 2.1.0 を選択してください。", L"지원하지 않는 폴더 또는 변경된 원본 리소스입니다. 지원 KSM 2.1.0을 선택하세요.", L"文件夹不受支持或原版资源已更改。请选择受支持的 KSM 2.1.0。", L"資料夾不受支援或原版資源已變更。請選擇受支援的 KSM 2.1.0。"}},
			{"recovery", {L"Recovery files: ", L"復元用ファイル: ", L"복구 자료: ", L"恢复文件：", L"還原資料："}},
			{"kept", {L"Preserved edited/deleted image paths: ", L"維持した編集・削除画像: ", L"유지한 수정·삭제 이미지: ", L"保留的修改或删除图片：", L"保留的修改或刪除圖片："}}
		};
		return text.at(key).at(g_lang);
	}
	void Text(HWND control, const std::wstring& value) { SetWindowTextW(control, value.c_str()); }
	void SetProgress(int percent, int state = PBST_NORMAL)
	{
		g_percent = percent;
		SendMessageW(g_progress, PBM_SETMARQUEE, FALSE, 0);
		SetWindowLongPtrW(g_progress, GWL_STYLE, GetWindowLongPtrW(g_progress, GWL_STYLE) & ~PBS_MARQUEE);
		SendMessageW(g_progress, PBM_SETRANGE32, 0, 100);
		SendMessageW(g_progress, PBM_SETSTATE, state, 0);
		SendMessageW(g_progress, PBM_SETPOS, percent, 0);
		InvalidateRect(g_progress, nullptr, TRUE);
	}

	std::wstring ErrorText(const Json& result)
	{
		const auto code = result.value("code", "error");
		if (code == "missing_executable") return std::wstring(T("missingExe")) + Wide(result.value("message", ""));
		if (code == "busy") return T("busy");
		if (code == "target") return T("target");
		return std::wstring(T("error")) + Wide(result.value("message", "Unknown error"));
	}
	std::wstring CleanupNotice(std::wstring text, const Json& result)
	{
		if (result.contains("cleanupWarnings") && !result.at("cleanupWarnings").empty())
		{
			text += L"\r\n" + std::wstring(T("cleanupWarning"));
			for (const auto& warning : result.at("cleanupWarnings")) text += L"\r\n" + Wide(warning.at("path")) + L" (" + Wide(warning.at("reason")) + L")";
		}
		return text;
	}
	void SaveGuiReport(const Json& result)
	{
		wchar_t temp[32768];
		if (GetTempPathW(32768, temp)) try { WriteJson(Path(temp) / L"KsmEnhancedVerticalSetup-last-result.json", result); } catch (...) {}
	}
	void Refresh()
	{
		SetWindowTextW(g_window, T("title")); Text(g_browse, T("browse")); Text(g_install, T("install")); Text(g_remove, T("remove")); Text(g_hint, T("hint"));
		EnableWindow(g_browse, !g_working); EnableWindow(g_language, !g_working);
		EnableWindow(g_install, FALSE); EnableWindow(g_remove, FALSE);
		if (g_working) { Text(g_status, T("working")); return; }
		if (g_target.empty()) { Text(g_status, T("select")); return; }
		Text(g_path, g_target.wstring());
		try
		{
			Engine engine(*g_package, g_target); const auto state = engine.Run("inspect");
			auto status = state.at("installed").get<bool>() ? std::wstring(T("installed")) + Wide(state.at("version")) : std::wstring(T("ready"));
			if (state.value("cleanupRecovered", false)) { status = T("cleanupRecovered"); SaveGuiReport(state); }
			Text(g_status, CleanupNotice(status, state));
			EnableWindow(g_install, !state.at("installed").get<bool>()); EnableWindow(g_remove, state.at("installed").get<bool>());
		}
		catch (const Error& e) { Text(g_status, ErrorText(Json{{"code", e.code}, {"message", e.what()}})); }
		catch (const std::exception& e) { Text(g_status, std::wstring(T("error")) + Wide(e.what())); }
	}
	void Choose()
	{
		const Path previous = g_target;
		IFileDialog* dialog = nullptr;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
		{
			DWORD flags = 0; dialog->GetOptions(&flags); dialog->SetOptions(flags | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
			dialog->SetTitle(T("select"));
			if (SUCCEEDED(dialog->Show(g_window)))
			{
				IShellItem* item = nullptr;
				if (SUCCEEDED(dialog->GetResult(&item)))
				{
					PWSTR value = nullptr;
					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &value))) { g_target = value; CoTaskMemFree(value); }
					item->Release();
				}
			}
			dialog->Release();
		}
		if (previous != g_target) SetProgress(0);
		Refresh();
	}
	Json Execute(const std::string& action, const Path& target, std::function<void(const std::wstring&)> progress = {}, std::atomic_bool* cancel = nullptr, std::function<void(int)> progressValue = {})
	{
		try { Engine engine(*g_package, target); engine.progress = std::move(progress); engine.cancel = cancel; engine.progressValue = std::move(progressValue); return engine.Run(action); }
		catch (const Error& e) { return Json{{"status", "FAIL"}, {"action", action}, {"code", e.code}, {"message", e.what()}, {"systemCode", e.systemCode}}; }
		catch (const std::exception& e) { return Json{{"status", "FAIL"}, {"action", action}, {"code", "invalid_data"}, {"message", e.what()}}; }
	}
	void Start(const std::string& action)
	{
		if (g_working || g_target.empty()) return;
		g_working = true; g_cancel = false; g_currentAction = action; Refresh();
		SetProgress(0);
		SetWindowLongPtrW(g_progress, GWL_STYLE, GetWindowLongPtrW(g_progress, GWL_STYLE) | PBS_MARQUEE);
		SendMessageW(g_progress, PBM_SETMARQUEE, TRUE, 40);
		const Path target = g_target;
		std::thread([action, target]
		{
			auto result = Execute(action, target, [](const std::wstring& message) { PostMessageW(g_window, kProgress, 0, reinterpret_cast<LPARAM>(new std::wstring(message))); }, &g_cancel, [](int percent) { PostMessageW(g_window, kPercent, percent, 0); });
			PostMessageW(g_window, kFinished, 0, reinterpret_cast<LPARAM>(new Json(std::move(result))));
		}).detach();
	}
	LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
	{
		if (message == WM_COMMAND)
		{
			const int id = LOWORD(wparam);
			if (id == 10 && HIWORD(wparam) == CBN_SELCHANGE) { g_lang = int(SendMessageW(g_language, CB_GETCURSEL, 0, 0)); Refresh(); }
			if (id == 11) Choose();
			if (id == 12) Start("install");
			if (id == 13) Start("remove");
			return 0;
		}
		if (message == kPercent) { SetProgress(int(wparam)); return 0; }
		if (message == kProgress)
		{
			std::unique_ptr<std::wstring> text(reinterpret_cast<std::wstring*>(lparam)); Text(g_status, std::wstring(T("working")) + L"\r\n" + *text); return 0;
		}
		if (message == kFinished)
		{
			std::unique_ptr<Json> result(reinterpret_cast<Json*>(lparam));
			g_working = false;
			SetProgress((*result)["status"] == "PASS" ? 100 : g_percent, result->value("code", "") == "cancelled" ? PBST_PAUSED : ((*result)["status"] == "PASS" ? PBST_NORMAL : PBST_ERROR));
			Refresh();
			if (result->value("code", "") == "permission" && !g_close)
			{
				if (MessageBoxW(window, T("permission"), T("title"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					const std::wstring params = L"--target \"" + g_target.wstring() + L"\" --language " + std::to_wstring(g_lang) + L" --gui-action " + Wide(g_currentAction);
					SHELLEXECUTEINFOW info{sizeof(info)}; info.lpVerb = L"runas"; const auto executable = ModulePath().wstring(); info.lpFile = executable.c_str(); info.lpParameters = params.c_str(); info.nShow = SW_SHOWNORMAL;
					if (ShellExecuteExW(&info)) { DestroyWindow(window); return 0; }
				}
			}
			std::wstring text;
			if ((*result)["status"] == "PASS")
			{
				text = T(g_currentAction == "install" ? "installDone" : "removeDone");
				if (result->contains("recoveryDirectory") && !(*result)["recoveryDirectory"].get<std::string>().empty()) text += L"\r\n" + std::wstring(T("recovery")) + (g_target / Wide((*result)["recoveryDirectory"])).wstring();
				if (result->contains("preservedImages")) for (const auto& path : (*result)["preservedImages"]) text += L"\r\n" + std::wstring(T("kept")) + Wide(path);
			}
			else text = ErrorText(*result);
			Text(g_status, CleanupNotice(text, *result));
			SaveGuiReport(*result);
			if (g_close) DestroyWindow(window);
			return 0;
		}
		if (message == WM_CLOSE)
		{
			if (g_working) { g_cancel = true; g_close = true; return 0; }
			DestroyWindow(window); return 0;
		}
		if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
		return DefWindowProcW(window, message, wparam, lparam);
	}
	void Control(const wchar_t* type, const wchar_t* text, DWORD style, int x, int y, int width, int height, int id, HWND& out)
	{
		out = CreateWindowExW(0, type, text, WS_CHILD | WS_VISIBLE | style, x, y, width, height, g_window, reinterpret_cast<HMENU>(INT_PTR(id)), GetModuleHandleW(nullptr), nullptr);
		SendMessageW(out, WM_SETFONT, reinterpret_cast<WPARAM>(g_font), TRUE);
	}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show)
{
	using namespace Setup;
	Path report; std::string action, guiAction; bool quiet = false;
	try
	{
		int argc = 0; LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		for (int i = 1; i < argc; ++i)
		{
			const std::wstring arg = argv[i];
			if (arg == L"--quiet") quiet = true;
			else if (arg == L"--action" && i + 1 < argc) action = Utf8(argv[++i]);
			else if (arg == L"--gui-action" && i + 1 < argc) guiAction = Utf8(argv[++i]);
			else if (arg == L"--target" && i + 1 < argc) g_target = argv[++i];
			else if (arg == L"--report" && i + 1 < argc) report = argv[++i];
			else if (arg == L"--language" && i + 1 < argc) g_lang = _wtoi(argv[++i]);
			else throw Error("arguments", "Unknown or incomplete argument");
		}
		LocalFree(argv);
		if (g_lang < 0 || g_lang > 4) throw Error("arguments", "Invalid language index");
		const auto executable = ModulePath(); g_package = std::make_unique<Package>(executable);
		if (g_target.empty() && std::filesystem::exists(executable.parent_path() / L"kshootmania.exe")) g_target = executable.parent_path();
		if (!action.empty() || quiet)
		{
			if (action.empty()) throw Error("arguments", "--action is required with --quiet");
			if (g_target.empty()) throw Error("arguments", "--target is required outside the KSM folder");
			const auto result = Execute(action, g_target);
			if (!report.empty()) WriteJson(std::filesystem::absolute(report), result);
			return result.at("status") == "PASS" ? 0 : 2;
		}
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
		CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
		INITCOMMONCONTROLSEX common{sizeof(common), ICC_PROGRESS_CLASS}; InitCommonControlsEx(&common);
		WNDCLASSW type{}; type.lpfnWndProc = Procedure; type.hInstance = instance; type.hIcon = reinterpret_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED)); type.hCursor = LoadCursorW(nullptr, IDC_ARROW); type.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); type.lpszClassName = L"KsmEnhancedVerticalSetup"; RegisterClassW(&type);
		g_window = CreateWindowExW(0, type.lpszClassName, T("title"), WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 800, 480, nullptr, nullptr, instance, nullptr);
		SendMessageW(g_window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(LoadImageW(instance, MAKEINTRESOURCEW(101), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED)));
		g_font = CreateFontW(-17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
		Control(WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP, 588, 20, 175, 200, 10, g_language);
		for (const auto* value : {L"EN — English", L"JP — 日本語", L"KR — 한국어", L"SC — 简体中文", L"TC — 繁體中文"}) SendMessageW(g_language, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value));
		SendMessageW(g_language, CB_SETCURSEL, g_lang, 0);
		Control(L"STATIC", L"KSM 2.1.0 / Enhanced Vertical 0.5.1-beta1", SS_LEFT, 24, 24, 555, 28, 0, g_hint);
		Control(L"EDIT", L"", ES_READONLY | ES_AUTOHSCROLL | WS_BORDER, 24, 70, 558, 29, 0, g_path);
		Control(L"BUTTON", T("browse"), BS_PUSHBUTTON | WS_TABSTOP, 595, 68, 167, 32, 11, g_browse);
		Control(L"EDIT", L"", ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL, 24, 120, 738, 118, 0, g_status);
		Control(PROGRESS_CLASSW, L"", PBS_MARQUEE, 24, 245, 738, 16, 0, g_progress);
		Control(L"STATIC", T("hint"), SS_LEFT, 24, 275, 738, 82, 0, g_hint);
		Control(L"BUTTON", T("install"), BS_PUSHBUTTON | WS_TABSTOP, 365, 365, 190, 43, 12, g_install);
		Control(L"BUTTON", T("remove"), BS_PUSHBUTTON | WS_TABSTOP, 572, 365, 190, 43, 13, g_remove);
		Refresh(); ShowWindow(g_window, show); UpdateWindow(g_window);
		if (g_target.empty()) Choose();
		if (!guiAction.empty() && !g_target.empty()) Start(guiAction);
		MSG message{};
		while (GetMessageW(&message, nullptr, 0, 0) > 0) if (!IsDialogMessageW(g_window, &message)) { TranslateMessage(&message); DispatchMessageW(&message); }
		DeleteObject(g_font); CoUninitialize(); return 0;
	}
	catch (const std::exception& e)
	{
		Json result{{"status", "FAIL"}, {"code", "startup"}, {"message", e.what()}};
		if (!report.empty()) try { WriteJson(std::filesystem::absolute(report), result); } catch (...) {}
		if (!quiet) MessageBoxW(nullptr, Wide(e.what()).c_str(), L"KSM Enhanced Vertical Setup", MB_ICONERROR);
		return 2;
	}
}
