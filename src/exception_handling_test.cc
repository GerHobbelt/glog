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
#include <assert.h>
#include <string>

#include "googletest.h"

#include "testing.h"
#include "glog/logging.h"

DECLARE_bool(logtostderr);
DECLARE_bool(check_mode);

using std::string;
using namespace GOOGLE_NAMESPACE;



TEST(ExceptionHandling, LoggerAbortsViaException) {
  InstallFailureFunction([]() {
    //throw std::exception("LoggerAbortsViaException::Fail::aborting...");
  });

  EXPECT_TRUE(HasInstalledCustomFailureFunction());

  try {
    // Log(FATAL) should throw an exception...
    LOG(FATAL) << "Buggerit Millenium Hand & Shrimp!";
  } catch (const std::exception& e) {
    throw e;
  }
  catch (...) {
    auto e = std::current_exception();
    std::rethrow_exception(e);
  }

  InstallFailureFunction(nullptr);
}

static void fake_library_call(void) {
  throw std::exception("blargh!");
}

TEST(ExceptionHandling, InnerCodeThrows) {
  fake_library_call();
}

static void fake_library_call2(void) {
  LIBASSERT_ASSERT(false, "Expects to throw...");
}

TEST(ExceptionHandling, InnerCodeHitsAnAssertionAndThrows) {
  fake_library_call2();
}
