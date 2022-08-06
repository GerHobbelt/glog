// Copyright (c) 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Sergey Ioffe

// The common part of the striplog tests.

#include <cstdio>
#include <string>
#include <iosfwd>
#include <assert.h>
#include "config.h"
#include <glog/logging.h>
#include "base/commandlineflags.h"

#include "testing.h"

DECLARE_bool(logtostderr);

#if defined(BUILD_MONOLITHIC) && (GOOGLE_STRIP_LOG != 0)
DECLARE_bool(check_mode);
#else
GLOG_DEFINE_bool(check_mode, false, "Prints 'opt' or 'dbg'");
#endif

using std::string;
using namespace GOOGLE_NAMESPACE;

#if defined(BUILD_MONOLITHIC) && (GOOGLE_STRIP_LOG != 0)

extern int CheckNoReturn(bool b);

#else

int CheckNoReturn(bool b) {
  string s;
  if (b) {
    LOG(FATAL) << "Fatal";
  } 
  return 0;
}

#endif

struct A { };
static std::ostream &operator<<(std::ostream &str, const A&) {return str;}


static int fatal_fail_handler_hits = 0;
static int fatal_fail_catcher_hits = 0;
static int fatal_fail_catcher_rethrow_hits = 0;

static void dummy_logging_fail_handler_4_test()
{
	fatal_fail_handler_hits++;
}

static void handle_exception_eptr_4_fatal_fail(std::exception_ptr eptr) // passing by value is ok
{
	fatal_fail_catcher_hits++;
	if (eptr) {
		fatal_fail_catcher_rethrow_hits++;
		std::rethrow_exception(eptr);
	}
}

typedef void exec_f();

static void contain_fatal_testcode(exec_f f) {
	try
	{
		fatal_fail_handler_hits = 0;
		fatal_fail_catcher_hits = 0;
		fatal_fail_catcher_rethrow_hits = 0;

		// make sure the old_handler is always reverted to at the end of the test, i.e. no matter whether f() has completed or aborted.
		class auto_scope {
		public:
			logging_fail_func_t old_handler;
			~auto_scope() {
				InstallFailureFunction(old_handler);
			}
		};
		auto_scope h{GetInstalledFailureFunction()};

		InstallFailureFunction(dummy_logging_fail_handler_4_test);
		assert(HasInstalledCustomFailureFunction());

		f();
	}
	catch (...)
	{
		auto e = std::current_exception(); // capture
		handle_exception_eptr_4_fatal_fail(e);
	}
}

static void log_fatal()
{
	LOG(FATAL) << "TESTMESSAGE FATAL";
}


//-----------------------------------------------------------------------//

#if !defined(GOOGLE_STRIP_LOG)
#define GOOGLE_STRIP_LOG        1
#endif

#if defined(BUILD_MONOLITHIC)
#define CAT2(a, b, c)       a ## b ## c
#define CAT(a, b, c)        CAT2(a, b, c)
#define main(cnt, arr)      CAT(glog_logging_striptest, GOOGLE_STRIP_LOG, _main)(cnt, arr)
#endif

int main(int argc, const char** argv) {
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  if (FLAGS_check_mode) {
    printf("%s\n", DEBUG_MODE ? "dbg" : "opt");
    return 0;
  }
#if !defined(BUILD_MONOLITHIC)
  assert(!HasInstalledCustomFailureFunction());
#endif

  LOG(INFO) << "TESTMESSAGE INFO";
  LOG(WARNING) << 2 << "something" << "TESTMESSAGE WARNING"
               << 1 << 'c' << A() << std::endl;
  LOG(ERROR) << "TESTMESSAGE ERROR";
  bool flag = true;
  (flag ? LOG(INFO) : LOG(ERROR)) << "TESTMESSAGE COND";

  contain_fatal_testcode(log_fatal);

#if 0
  contain_fatal_testcode([]{
	  LOG(FATAL) << "TESTMESSAGE FATAL";
  });
#endif

  assert(fatal_fail_handler_hits == 1);
  assert(fatal_fail_catcher_hits == 1);
  assert(fatal_fail_catcher_rethrow_hits == 1);

  ShutdownGoogleLogging();
  return 0;
}
