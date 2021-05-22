#ifndef GLOG_TESTING_H_
#define GLOG_TESTING_H_

//#if defined(BUILD_MONOLITHIC)

int glog_demangle_unittest_main(int argc, const char** argv);
int glog_logging_custom_prefix_unittest_main(int argc, const char** argv);
int glog_logging_striptest_main(int argc, const char** argv);
int glog_logging_unittest_main(int argc, const char** argv);
int glog_mock_log_test_main(int argc, const char** argv);
int glog_signalhandler_unittest_main(int argc, const char** argv);
int glog_stacktrace_unittest_main(int argc, const char** argv);
int glog_stl_logging_unittest_main(int argc, const char** argv);
int glog_symbolize_unittest_main(int argc, const char** argv);
int glog_utilities_unittest_main(int argc, const char** argv);

//#endif

#endif

