#include "InstallerCore.hpp"
#include <commctrl.h>
#include <iostream>
#include <fstream>
using namespace Setup;
namespace
{
 DWORD targetPid=0;HWND found=nullptr,progress=nullptr,header=nullptr;
 void Check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
 BOOL CALLBACK Find(HWND window,LPARAM){DWORD pid=0;GetWindowThreadProcessId(window,&pid);wchar_t name[128];GetClassNameW(window,name,128);if(pid==targetPid&&wcscmp(name,L"KsmEnhancedVerticalSetup")==0)found=window;return TRUE;}
 BOOL CALLBACK Children(HWND window,LPARAM){wchar_t name[128],text[2048];GetClassNameW(window,name,128);GetWindowTextW(window,text,2048);if(wcscmp(name,PROGRESS_CLASSW)==0)progress=window;if(wcscmp(name,L"Static")==0&&wcsstr(text,L"0.5.5-Beta5")&&wcsstr(text,L"2.2.0"))header=window;return TRUE;}
 std::wstring Text(HWND w){wchar_t text[4096];GetWindowTextW(w,text,4096);return text;}
 void Capture(HWND window,const Path& path)
 {
  RECT r{};GetWindowRect(window,&r);const int width=r.right-r.left,height=r.bottom-r.top;
  HDC dc=GetDC(window),mem=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,width,height);const auto previous=SelectObject(mem,bitmap);
  Check(PrintWindow(window,mem,2)!=0,"PrintWindow failed");
  // Private desktops can have a small visible surface. Render each native child
  // into its full window rectangle instead of inheriting desktop clipping.
  struct Context { HDC dc; RECT parent; } context{mem,r};
  EnumChildWindows(window,[](HWND child,LPARAM parameter)->BOOL
  {
   const auto& context=*reinterpret_cast<Context*>(parameter);RECT bounds{};GetWindowRect(child,&bounds);
   const int saved=SaveDC(context.dc);SetViewportOrgEx(context.dc,bounds.left-context.parent.left,bounds.top-context.parent.top,nullptr);
   IntersectClipRect(context.dc,0,0,bounds.right-bounds.left,bounds.bottom-bounds.top);
   SendMessageW(child,WM_PRINT,reinterpret_cast<WPARAM>(context.dc),PRF_CLIENT|PRF_NONCLIENT|PRF_ERASEBKGND);RestoreDC(context.dc,saved);return TRUE;
  },reinterpret_cast<LPARAM>(&context));
  SelectObject(mem,previous);
  BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
  Bytes pixels(size_t(width)*height*4);Check(GetDIBits(mem,bitmap,0,height,pixels.data(),&bi,DIB_RGB_COLORS)!=0,"GetDIBits failed");
  BITMAPFILEHEADER file{};file.bfType=0x4D42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=DWORD(file.bfOffBits+pixels.size());
  std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<char*>(&file),sizeof(file));output.write(reinterpret_cast<char*>(&bi.bmiHeader),sizeof(bi.bmiHeader));output.write(reinterpret_cast<char*>(pixels.data()),pixels.size());
  DeleteObject(bitmap);DeleteDC(mem);ReleaseDC(window,dc);
 }
 bool Complete(HWND window, bool installed)
 {
  EnumChildWindows(window,Children,0);
  return progress&&SendMessageW(progress,PBM_GETPOS,0,0)==100&&bool(IsWindowEnabled(GetDlgItem(window,13)))==installed;
 }
 void WaitComplete(HWND window,bool installed)
 {
  const auto until=GetTickCount64()+90000;while(GetTickCount64()<until){if(Complete(window,installed))return;Sleep(100);}throw std::runtime_error("GUI operation timeout");
 }
}
int wmain(int argc,wchar_t** argv)
{
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
 if(argc!=4)return 2;const Path setup=argv[1],app=argv[2],output=argv[3];std::filesystem::create_directories(output);
 PROCESS_INFORMATION pi{};HDESK desktop=nullptr;Json result;
 try
 {
  const auto name=L"KsmSetupPrivate-"+std::to_wstring(GetCurrentProcessId());desktop=CreateDesktopW(name.c_str(),nullptr,nullptr,0,GENERIC_ALL,nullptr);Check(desktop!=nullptr,"CreateDesktop failed");
  auto command=L"\""+setup.wstring()+L"\" --target \""+app.wstring()+L"\" --language 0 --gui-action install";
  STARTUPINFOW si{};si.cb=sizeof(si);std::wstring full=L"winsta0\\"+name;si.lpDesktop=full.data();
  Check(CreateProcessW(setup.c_str(),command.data(),nullptr,nullptr,FALSE,0,nullptr,app.c_str(),&si,&pi)!=0,"CreateProcess failed");targetPid=pi.dwProcessId;
  const auto until=GetTickCount64()+15000;while(!found&&GetTickCount64()<until){EnumDesktopWindows(desktop,Find,0);Sleep(100);}Check(found!=nullptr,"GUI missing");WaitComplete(found,true);
  Json languages=Json::array();
  for(int language=0;language<5;++language)
  {
   SendMessageW(GetDlgItem(found,10),CB_SETCURSEL,language,0);SendMessageW(found,WM_COMMAND,MAKEWPARAM(10,CBN_SELCHANGE),0);Sleep(150);EnumChildWindows(found,Children,0);
   Check(header!=nullptr,"Version/support label missing");Check(IsWindowVisible(GetDlgItem(found,14))&&IsWindowVisible(GetDlgItem(found,15)),"Online controls missing");
   RECT client{},bounds{};GetClientRect(found,&client);GetWindowRect(GetDlgItem(found,15),&bounds);MapWindowPoints(nullptr,found,reinterpret_cast<POINT*>(&bounds),2);Check(bounds.bottom<=client.bottom&&bounds.right<=client.right,"Online controls outside client");
   const auto file=output/(L"installer-language-"+std::to_wstring(language)+L".bmp");Capture(found,file);languages.push_back(Json{{"language",language},{"title",Utf8(Text(found))},{"versionLabel",Utf8(Text(header))},{"checkButton",Utf8(Text(GetDlgItem(found,14)))},{"releaseButton",Utf8(Text(GetDlgItem(found,15)))},{"screenshot",Utf8(file.wstring())}});
  }
  SendMessageW(found,WM_COMMAND,13,0);WaitComplete(found,false);Check(std::filesystem::is_directory(app/L"skins"),"skins removed by GUI removal");Capture(found,output/L"installer-removal-complete.bmp");
  SendMessageW(found,WM_COMMAND,14,0);Check(!IsWindowEnabled(GetDlgItem(found,14)),"Online request did not start");
  PostMessageW(found,WM_CLOSE,0,0);Check(WaitForSingleObject(pi.hProcess,15000)==WAIT_OBJECT_0,"GUI did not close bounded online worker");
  result=Json{{"status","PASS"},{"languages",5},{"progressComplete",true},{"closedDuringOnlineCheck",true},{"privateDesktop",true},{"interactiveDesktopSwitched",false},{"globalInputInjected",false},{"screenshots",languages},{"setupSha256",FileHash(setup)}};
 }
 catch(const std::exception& e){result=Json{{"status","FAIL"},{"error",e.what()}};if(found)PostMessageW(found,WM_CLOSE,0,0);if(pi.hProcess)WaitForSingleObject(pi.hProcess,15000);}
 if(pi.hThread)CloseHandle(pi.hThread);if(pi.hProcess)CloseHandle(pi.hProcess);if(desktop)CloseDesktop(desktop);
 WriteJson(output/L"result.json",result);std::cout<<result.dump(2);return result.at("status")=="PASS"?0:1;
}
