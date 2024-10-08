// Copyright (c) 2008, Google Inc.
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
// Author: Shinichiro Hamaji
#include "utilities.h"

#include "glog/logging.h"
#include "googletest.h"

#include "testing.h"

#ifdef HAVE_LIB_GFLAGS
#include <gflags/gflags.h>
using namespace GFLAGS_NAMESPACE;
#endif

using namespace GOOGLE_NAMESPACE;

TEST(utilities, sync_val_compare_and_swap) {
  bool now_entering = false;
  EXPECT_FALSE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
  EXPECT_TRUE(sync_val_compare_and_swap(&now_entering, false, true));
}

TEST(utilities, InitGoogleLoggingDeathTest) {
  ASSERT_DEATH(InitGoogleLogging("foobar"), "");
}

#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      glog_utilities_unittest_main(cnt, arr)
#endif

extern "C"
int main(int argc, const char** argv) {
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);

  CHECK_EQ(RUN_ALL_TESTS(), 0);

  return 0;
}
