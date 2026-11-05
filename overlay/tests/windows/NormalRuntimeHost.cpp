// Test host: run the exact normal EXE on a private, never-switched desktop.
// No global input injection or interactive-desktop activation is performed.
#include <windows.h>
#include <string>
#include <iostream>
static std::wstring expectedTitle = L"K-Shoot Mania V2.1.0 - Enhanced Vertical Mod 0.5.4-beta4";
struct WindowSearch { DWORD pid; HWND window=nullptr; std::wstring title; };
static BOOL CALLBACK Find(HWND h, LPARAM param) {
 auto& found=*reinterpret_cast<WindowSearch*>(param);DWORD pid=0;GetWindowThreadProcessId(h,&pid);
 if(pid==found.pid){wchar_t name[1024]{};GetWindowTextW(h,name,1024);std::wstring title=name;
  if(title.find(expectedTitle)!=std::wstring::npos){found.window=h;found.title=title;return FALSE;}}
 return TRUE;
}
static std::wstring Quote(const std::wstring& s){std::wstring out=L"\"";for(wchar_t c:s){if(c==L'\"')out+=L'\\';out+=c;}return out+L"\"";}
int wmain(int argc,wchar_t** argv){
 if(argc<3)return 2;
 wchar_t expected[256]{};if(GetEnvironmentVariableW(L"KSM_HOST_EXPECT_TITLE",expected,256))expectedTitle=expected;
 const auto desktopName=L"KsmValidation-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
 HDESK desktop=CreateDesktopW(desktopName.c_str(),nullptr,nullptr,0,GENERIC_ALL,nullptr);
 if(!desktop){std::cerr<<"CreateDesktop failed "<<GetLastError()<<'\n';return 2;}
 std::wstring command=Quote(argv[1]);for(int i=3;i<argc;i++)command+=L" "+Quote(argv[i]);
 STARTUPINFOW info{};info.cb=sizeof(info);info.lpDesktop=const_cast<wchar_t*>(desktopName.c_str());info.dwFlags=STARTF_USEPOSITION;info.dwX=static_cast<DWORD>(-1800);info.dwY=64;
 PROCESS_INFORMATION process{};
 if(!CreateProcessW(argv[1],command.data(),nullptr,nullptr,FALSE,CREATE_UNICODE_ENVIRONMENT|BELOW_NORMAL_PRIORITY_CLASS,nullptr,argv[2],&info,&process)){std::cerr<<"CreateProcess failed "<<GetLastError()<<'\n';CloseDesktop(desktop);return 2;}
 WindowSearch found{process.dwProcessId};const ULONGLONG start=GetTickCount64();bool ready=false;
 while(GetTickCount64()-start<30000 && WaitForSingleObject(process.hProcess,100)==WAIT_TIMEOUT){EnumDesktopWindows(desktop,Find,reinterpret_cast<LPARAM>(&found));if(found.window){ready=true;break;}}
 if(ready && WaitForSingleObject(process.hProcess,7000)==WAIT_TIMEOUT){
 HWND install=GetDlgItem(found.window,12),remove=GetDlgItem(found.window,13);
 if(install){wchar_t label[256]{};GetWindowTextW(install,label,256);char utf8[1024]{};WideCharToMultiByte(CP_UTF8,0,label,-1,utf8,1024,nullptr,nullptr);std::cout<<"installEnabled="<<IsWindowEnabled(install)<<", removeEnabled="<<IsWindowEnabled(remove)<<", installLabel="<<utf8<<'\n';}
 PostMessageW(found.window,WM_CLOSE,0,0);if(WaitForSingleObject(process.hProcess,30000)==WAIT_TIMEOUT)ready=false;}
 else ready=false;
 DWORD exit=0;if(WaitForSingleObject(process.hProcess,0)==WAIT_TIMEOUT){TerminateProcess(process.hProcess,91);WaitForSingleObject(process.hProcess,5000);ready=false;}
 GetExitCodeProcess(process.hProcess,&exit);CloseHandle(process.hThread);CloseHandle(process.hProcess);CloseDesktop(desktop);
 std::cout<<"privateDesktop=true, interactiveDesktopSwitched=false, startup="<<(ready?"PASS":"FAIL")<<", exitCode="<<exit<<'\n';return ready && exit==0?0:2;
}
