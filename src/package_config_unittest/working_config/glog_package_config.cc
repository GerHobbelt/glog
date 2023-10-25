#define GLOG_NO_ABBREVIATED_SEVERITIES
#include "glog/logging.h"
#include "testing.h"


#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      glog_package_config_unittest_main(cnt, arr)
#endif

int main(int argc, const char** argv)
{
    google::InitGoogleLogging(argv[0]);

	return 0;
}
