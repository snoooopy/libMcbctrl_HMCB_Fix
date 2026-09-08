#define VERSION_MAJOR               
#define VERSION_MINOR               
#define VERSION_PATCH               
#define VERSION_OSS                 "1.0.0.919"
#define VERSION_BUILD_TIME          "2025-09-19 16:08:10"
#define VERSION_BUILD_TYPE          "Release"
#include <oss_version.h>

extern std::string g_oss_version_ossVersion;
extern std::string g_oss_version_buildTime;
extern std::string g_oss_version_buildType;

bool init_version(int argc, char* argv[])
{
    g_oss_version_ossVersion = VERSION_OSS;
    g_oss_version_buildTime = VERSION_BUILD_TIME;
    g_oss_version_buildType = VERSION_BUILD_TYPE;
    if( inquiry_oss_version(argc, argv) ){
        return true;
    }
    return false;
}
