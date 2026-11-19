#include "UpdateCheck.hpp"
#include "PackageVersion.hpp"
#include <winhttp.h>
#include <regex>
#include <algorithm>

namespace Setup
{
 namespace
 {
  constexpr auto kRepo = "https://github.com/yhc308/ksm-enhanced-vertical-mod/";
  constexpr auto kMetadata = "update-info-ksm-2.2.0.json";
  struct Internet
  {
   HINTERNET value;
   explicit Internet(HINTERNET v) : value(v) { if (!v) throw Error("network", "Cannot create HTTP handle"); }
   ~Internet() { WinHttpCloseHandle(value); }
   Internet(const Internet&) = delete;
  };
  void CheckTime(std::atomic_bool& cancel, std::chrono::steady_clock::time_point deadline)
  {
   if (cancel || std::chrono::steady_clock::now() >= deadline) throw Error("network", "Cancelled or request time limit reached");
  }
  bool ApprovedNetworkUrl(const std::string& url)
  {
   if (url.rfind("https://api.github.com/repos/yhc308/ksm-enhanced-vertical-mod/releases",0)==0) return true;
   if (url.rfind(kRepo,0)==0) return true;
   for (const auto* host : {"https://release-assets.githubusercontent.com/", "https://objects.githubusercontent.com/"})
    if (url.rfind(host,0)==0) return true;
#ifdef KSM_INSTALLER_TEST
   if (std::regex_match(url,std::regex("http://127\\.0\\.0\\.1:[0-9]+/[^\\r\\n]*"))) return true;
#endif
   return false;
  }
  void Require(bool test, const char* reason) { if (!test) throw Error("update_metadata",reason); }
 }
 bool IsReleasePage(const std::string& url)
 {
  return std::regex_match(url,std::regex("https://github\\.com/yhc308/ksm-enhanced-vertical-mod/releases/tag/[A-Za-z0-9][A-Za-z0-9._-]{0,127}"));
 }
 std::string UpdateHttpGet(const std::string& requested, std::atomic_bool& cancel, std::chrono::steady_clock::time_point deadline)
 {
  std::string url=requested;
  Internet session(WinHttpOpen(L"KsmEnhancedVerticalSetup/1",WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
  WinHttpSetTimeouts(session.value,1000,1000,1000,1000);
  for (int redirect=0;redirect<=4;++redirect)
  {
   CheckTime(cancel,deadline);Require(ApprovedNetworkUrl(url),"Unapproved update URL");
   const auto wide=Wide(url);URL_COMPONENTS c{};c.dwStructSize=sizeof(c);c.dwHostNameLength=c.dwUrlPathLength=c.dwExtraInfoLength=DWORD(-1);
   Require(WinHttpCrackUrl(wide.c_str(),0,0,&c),"Invalid HTTP URL");
   const std::wstring host(c.lpszHostName,c.dwHostNameLength);
   std::wstring path(c.lpszUrlPath,c.dwUrlPathLength);if(c.dwExtraInfoLength)path.append(c.lpszExtraInfo,c.dwExtraInfoLength);
   Internet connection(WinHttpConnect(session.value,host.c_str(),c.nPort,0));
   Internet request(WinHttpOpenRequest(connection.value,L"GET",path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,c.nScheme==INTERNET_SCHEME_HTTPS?WINHTTP_FLAG_SECURE:0));
   DWORD policy=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(request.value,WINHTTP_OPTION_REDIRECT_POLICY,&policy,sizeof(policy));
   const wchar_t* headers=L"Accept: application/vnd.github+json\r\n";
   Require(WinHttpSendRequest(request.value,headers,DWORD(-1),WINHTTP_NO_REQUEST_DATA,0,0,0)&&WinHttpReceiveResponse(request.value,nullptr),"HTTP request failed");
   CheckTime(cancel,deadline);DWORD status=0,size=sizeof(status);
   Require(WinHttpQueryHeaders(request.value,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,WINHTTP_HEADER_NAME_BY_INDEX,&status,&size,WINHTTP_NO_HEADER_INDEX),"Cannot read HTTP status");
   if(status==301||status==302||status==303||status==307||status==308)
   {
    DWORD bytes=0;WinHttpQueryHeaders(request.value,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,nullptr,&bytes,WINHTTP_NO_HEADER_INDEX);
    Require(bytes>0&&bytes<32768,"Invalid redirect");std::wstring location(bytes/sizeof(wchar_t),L'\0');
    Require(WinHttpQueryHeaders(request.value,WINHTTP_QUERY_LOCATION,WINHTTP_HEADER_NAME_BY_INDEX,location.data(),&bytes,WINHTTP_NO_HEADER_INDEX),"Invalid redirect");
    location.resize(wcslen(location.c_str()));url=Utf8(location);continue;
   }
   Require(status==200,"GitHub unavailable or request limited");std::string result;
   for(;;)
   {
    CheckTime(cancel,deadline);char buffer[8192];DWORD read=0;
    Require(WinHttpReadData(request.value,buffer,sizeof(buffer),&read),"HTTP read failed");if(!read)break;
    Require(result.size()+read<=2*1024*1024,"Update response exceeds size limit");result.append(buffer,read);
   }
   return result;
  }
  throw Error("network","Too many HTTP redirects");
 }
 OnlineUpdate CheckOnlineUpdate(const Json& current, std::atomic_bool& cancel, UpdateTransport transport)
 {
  OnlineUpdate answer;bool compatible=false;Json best;std::string bestUrl;bool complete=false;
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(8);
  try
  {
   for(int page=1;page<=5;++page)
   {
    CheckTime(cancel,deadline);
    auto releases=Json::parse(transport("https://api.github.com/repos/yhc308/ksm-enhanced-vertical-mod/releases?per_page=100&page="+std::to_string(page),cancel,deadline));
    Require(releases.is_array()&&releases.size()<=100,"Invalid release list");
    for(const auto& release:releases)
    {
     CheckTime(cancel,deadline);if(release.value("draft",false))continue;
     const auto tag=release.value("tag_name","");const std::string releaseUrl=std::string(kRepo)+"releases/tag/"+tag;
     if(!IsReleasePage(releaseUrl)||release.value("html_url","")!=releaseUrl)continue;
     if(!release.contains("assets")||!release.at("assets").is_array())continue;
     for(const auto& asset:release.at("assets"))
     {
      if(asset.value("name","")!=kMetadata)continue;
      const auto assetUrl=asset.value("browser_download_url","");
      Require(assetUrl==std::string(kRepo)+"releases/download/"+tag+"/"+kMetadata,"Unexpected metadata URL");
      auto m=Json::parse(transport(assetUrl,cancel,deadline));
      Require(m.is_object()&&m.at("schemaVersion")==1,"Unsupported update metadata");
      if(m.at("supportedGame")!="2.2.0"||m.at("runtimeContract")!="ksm-2.2.0-enhanced-skin-v1")continue;
      const auto& package=m.at("package");VersionParts(package.at("version"));RevisionNumber(package);
      Require(std::regex_match(package.at("buildId").get<std::string>(),std::regex("[0-9]{8}-[0-9]{6}-[0-9a-fA-F]{8}")),"Invalid game build ID");
      Require(package.contains("installerRevisionNumber")&&package.at("installerRevision").is_string(),"Missing installer revision");
      if(package.contains("displayVersion")) Require(VersionParts(package.at("displayVersion")) == VersionParts(package.at("version")),"Display version does not match comparison version");
      Require(std::regex_match(m.at("zipName").get<std::string>(),std::regex("[A-Za-z0-9._-]+\\.zip"))&&std::regex_match(m.at("zipSha256").get<std::string>(),std::regex("[a-f0-9]{64}")),"Invalid archive metadata");
      const bool zipPresent=std::any_of(release.at("assets").begin(),release.at("assets").end(),[&](const auto& a){return a.value("name","")==m.at("zipName").template get<std::string>();});
      Require(zipPresent,"Referenced installer ZIP is missing");
      compatible=true;
      if(best.is_null()||ComparePackages(best,package)<0){best=package;bestUrl=releaseUrl;}
     }
    }
    if(releases.size()<100){complete=true;break;}
   }
   if(!complete)throw Error("network","Release listing exceeded the bounded scan");
   if(!compatible){answer.status="unknown";return answer;}
   answer.status=ComparePackages(current,best)<0?"available":"current";
   answer.version=best.value("displayVersion",best.at("version").get<std::string>());answer.revision=best.at("installerRevision");answer.releaseUrl=bestUrl;return answer;
  }
  catch(const std::exception& e){answer.status=cancel?"cancelled":"unavailable";answer.detail=e.what();return answer;}
 }
}
