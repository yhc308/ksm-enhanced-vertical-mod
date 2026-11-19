#include "UpdateCheck.hpp"
#include "PackageVersion.hpp"
#include <iostream>
#include <thread>

int main(int argc,char** argv)
{
 using namespace Setup;
 if(argc!=4)return 2;
 std::atomic_bool cancel{false};std::thread stopper;
 try
 {
  auto bytes=Read(Wide(argv[1]));const auto input=Json::parse(bytes);
  if(input.value("cancel",false))stopper=std::thread([&]{Sleep(100);cancel=true;});
  const std::string endpoint=argv[2];
  auto transport=[&](const std::string& url,std::atomic_bool& stop,std::chrono::steady_clock::time_point deadline)
  {
   std::string path="/releases";
   if(url.find("api.github.com")==std::string::npos)
   {
    const auto start=url.find("/download/")+10;const auto end=url.find('/',start);path="/metadata/"+url.substr(start,end-start);
   }
   return UpdateHttpGet(endpoint+path,stop,deadline);
  };
  const auto start=std::chrono::steady_clock::now();
  auto r=endpoint=="github" ? CheckOnlineUpdate(input.at("current"),cancel) : CheckOnlineUpdate(input.at("current"),cancel,transport);
  if(stopper.joinable())stopper.join();
  Json out{{"status",r.status},{"version",r.version},{"revision",r.revision},{"url",r.releaseUrl},{"detail",r.detail},{"elapsedMs",std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()}};
  WriteJson(Wide(argv[3]),out);return 0;
 }
 catch(const std::exception& e){if(stopper.joinable())stopper.join();std::cerr<<e.what();return 1;}
}
