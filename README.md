[![Build Status](https://img.shields.io/travis/google/glog/master.svg?label=Travis)](https://travis-ci.org/google/glog/builds)
[![Grunt status](https://img.shields.io/appveyor/ci/google-admin/glog/master.svg?label=Appveyor)](https://ci.appveyor.com/project/google-admin/glog/history)

This repository contains a C++ implementation of the Google logging
module.  Documentation for the implementation is in doc/.

See INSTALL for (generic) installation instructions for C++: basically
```sh
./autogen.sh && ./configure && make && make install
```

## Changes that are in this repository and not present in google

* Using fmt library [ needs to be present on the host ]
* Allow customization of the format using FLAG. Named arguments added
```
                              fmt::arg(FMT_STRING("severity"), LogSeverityNames[severity]),
                              fmt::arg(FMT_STRING("year"), 1900 + data_->tm_time_.tm_year),
                              fmt::arg(FMT_STRING("month"),  1 + data_->tm_time_.tm_mon),
                              fmt::arg(FMT_STRING("month_day"),  data_->tm_time_.tm_mday),
                              fmt::arg(FMT_STRING("hour"),  data_->tm_time_.tm_mday),
                              fmt::arg(FMT_STRING("min"),  data_->tm_time_.tm_mday),
                              fmt::arg(FMT_STRING("second"),  static_cast<double>(data_->tm_time_.tm_sec) + static_cast<double>(data_->usecs_)/1E6),
                              fmt::arg(FMT_STRING("tid"),  static_cast<unsigned int>(GetTID())),
                              fmt::arg(FMT_STRING("basename"),  data_->basename_),
                              fmt::arg(FMT_STRING("line"),  data_->line_));
```