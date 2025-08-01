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

#include <libassert/assert.h>

DECLARE_bool(logtostderr);
DECLARE_bool(check_mode);

using std::string;
using namespace GOOGLE_NAMESPACE;


class ExpectFailureListener : public testing::EmptyTestEventListener {
 public:
  ExpectFailureListener() {}
  virtual ~ExpectFailureListener() {
	assert(mark_ == 0);
    singleton_instance = nullptr;
  }

  testing::TestPartResult OnTestPartResult(const testing::TestPartResult& result) override {
    testing::TestPartResult r = result;

    if (mark_ == 0)
	  return r;

    round_ = !round_;
    if (round_) {
      // FIRST call (of TWO): tweak the result/message so the error reporters
      // will report the proper state of affairs:
      buffered_result_.reset();
      switch (r.type()) {
        case testing::TestPartResult::kSuccess:
          // UNEXPECTED success!
          buffered_result_ = r;
          r.change_message(std::format("(**UNEXPECTED SUCCESS --> FAULT**) :: {}", r.message()));
		  // and make sure the reporters make some noise about this:
          r.change_type(testing::TestPartResult::kNonFatalFailure);
          break;

        case testing::TestPartResult::kSkip:
          break;

        case testing::TestPartResult::kFatalFailure:
        case testing::TestPartResult::kNonFatalFailure:
          buffered_result_ = r;
          r.change_message(std::format("(EXPECTED FAILURE --> SUCCESS) :: {}", r.message()));
#if 0
          r.change_type(testing::TestPartResult::kSuccess);
		  // ^^^^^^^^^^^^^^^^^^^ if we do that here, we will thwart our intent as
		  // PrettyUnitTestResultPrinter::OnTestPartResult() will take this as a hint to *be utterly silent*,
		  // which is emphathetically NOT what we want! So the tweaking to status=SUCCESS will be done
		  // only once the reporting has been done: during our SECOND ROUND, executed further below.
		  // 
		  // Alternatively we could go and "compensate" for this lack of communication by calling
		  // PrintTestPartResult(result) ourselves, but the current approach is more flexible, as we now
		  // support alternative/customized reporters, since we DO NOT assume anything about
		  // PrettyUnitTestResultPrinter being used or not...
#endif
          break;
      }
    } else {
      // SECOND = LAST call (of TWO): revert the result/message to its original
      // value & tweak the result so the lump sum / end result reports will say
      // the right thing at the conclusion of this run,
	  // e.g. "all tests pass" when all failures were indeed _expected_!
      r = buffered_result_.value_or(r);
      switch (r.type()) {
        case testing::TestPartResult::kSuccess:
          // UNEXPECTED success!
          r.change_type(testing::TestPartResult::kNonFatalFailure);
          break;

        case testing::TestPartResult::kSkip:
          break;

        case testing::TestPartResult::kFatalFailure:
        case testing::TestPartResult::kNonFatalFailure:
          --mark_;
          r.change_type(testing::TestPartResult::kSuccess);
          break;
      }
    }
    return r;
  }

  //void OnTestIterationStart(const testing::UnitTest& unit_test, int iteration) override {}

  void OnTestStart(const testing::TestInfo& test_info) override {
    mark_ = 0;

	// should be superfluous, but just to make sure we're in a known call order state when failure notices start passing through this class instance:
    round_ = false;
    buffered_result_.reset();
  }

  void OnTestEnd(const testing::TestInfo& test_info) override {
	// remove ourselves from the listeners queue!
    assert(mark_ == 0);
  }

  void OnTestIterationEnd(const testing::UnitTest& unit_test, int iteration) override {
  }

  static void SetUpExpectedFailureHandler(void) {
    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    if (!singleton_instance) {
      singleton_instance = new ExpectFailureListener();
    }
    if (!listeners.Has(singleton_instance)) {
      listeners.Prepend(singleton_instance);
      listeners.Append(singleton_instance);
	}
  }

  static void Mark(unsigned int count = 1) {
    SetUpExpectedFailureHandler();
    assert(!!singleton_instance);
    assert(singleton_instance->mark_ == 0);
    assert(count >= 1);
    singleton_instance->mark_ = count;
  }

  static bool IsMarkActivate(void) {
    SetUpExpectedFailureHandler();
    assert(!!singleton_instance);
    return singleton_instance->mark_ > 0;
  }

 protected:
  int mark_{0};

  // internal state & cache:
  bool round_{false};
  std::optional<testing::TestPartResult> buffered_result_{};

  // single instance management:
  static class ExpectFailureListener* singleton_instance;
};

ExpectFailureListener* ExpectFailureListener::singleton_instance{nullptr};

static void GTEST_MARK_NEXT_AS_EXPECTED_FAILURE(unsigned int count = 1) {
  ExpectFailureListener::Mark(count);
}


// --------------------------------------------------------------


// As a result of the test rig below, using the special 'expected failures' filter/listener
// above, we SHOULD end up with a lump sum of ZERO faults, once the exception-handling tests
// below have concluded...
//
// Note that we also install a special LOG(FATAL) abort/termination handler, which DOES NOT
// invoke posix::Abort() but throws a C++ exception instead, which can be caught by the
// GoogleTest test framework so very nicely!
// 
// (And 'marked' as an "expected fault" too!)
// 
// To verify this story, we use this dedicated rig/fixture:
class ExceptionHandlingTest : public ::testing::Test {
 protected:
  // invoked once, at the start of the suite's test *series*
  static void SetUpTestSuite() {
	  (void)0;
    google::InstallFailureFunction(nullptr);
  }

  // invoked once, at the end of the suite's test *series*
  static void TearDownTestSuite() {
	  (void)1;
    google::InstallFailureFunction(nullptr);
  }

  // invoked once for every test in the suite's test series
  virtual void SetUp() override {
    google::InstallFailureFunction(count_another_failure);

    libassert::deinit_handler_for_uncaught_exceptions();

	libassert::setup_handler_for_uncaught_exceptions(handler_for_uncaught_exceptions);

    libassert::set_failure_handler(assert_failure_handler);
  }

  // invoked once for every test in the suite's test series
  virtual void TearDown() override {
    google::InstallFailureFunction(nullptr);

    libassert::deinit_handler_for_uncaught_exceptions();

    EXPECT_FALSE(HasFatalFailure());
    EXPECT_FALSE(HasFailure());
    EXPECT_FALSE(HasNonfatalFailure());
  }

  static void count_another_failure(void) {
    throw new std::exception("Customized LOG(FATAL) handler kicked in!");
  }

  static void handler_for_uncaught_exceptions(void) {
    LIBASSERT_BREAKPOINT_IF_DEBUGGING_ON_FAIL();

    auto eptr = std::current_exception();
    try {
      if (eptr)
        std::rethrow_exception(eptr);
      else
        throw std::runtime_error("INTERNAL ERROR? handler_for_uncaught_exceptions() failed to grab the pending exception!");
    } catch (const std::exception& e) {
      throw std::runtime_error(std::format("Unhandled exception: {}", e.what()));
    } catch (const std::exception* e) {
      throw std::runtime_error(std::format("Unhandled exception: {}", e->what()));
      delete e;
    } catch (...) {
      throw std::runtime_error("Unhandled exception: UNKNOWN TYPE");
    }
  }

  static bool assert_failure_handler(const libassert::assertion_info& info) {
	using namespace libassert;

    LIBASSERT_BREAKPOINT_IF_DEBUGGING_ON_FAIL();

	std::string message = info.to_string(
		terminal_width(fileno(stderr)),
        /* ::isatty(fileno(stderr)) ? get_color_scheme() : */ color_scheme::blank
	);

#if 0
	switch (info.type) {
	case assert_type::debug_assertion:
	case assert_type::assertion:
	case assert_type::assumption:
	case assert_type::panic:
	case assert_type::unreachable:
		(void)fflush(stderr);
		return false;

	default:
		LIBASSERT_PRIMITIVE_PANIC("Unknown assertion type in assertion failure handler");
	}
#endif

    throw std::runtime_error(std::move(message));
  }
};


// --------------------------------------------------

// Test the specialized "expected failure" GoogleTest filter/listener defined above.

#ifdef __COUNTER__  // not standard and may be missing for some compilers
#define TESTCOUNTER()  __COUNTER__
#else  // __COUNTER__
#define TESTCOUNTER()  __LINE__
#endif  // __COUNTER__

#define CONCAT1(a, b)   a ## b
#define CONCAT(a, b)    CONCAT1(a, b)

#define BOUNDARY_TEST()                                                               \
TEST(ExpectedFailureHandling, CONCAT(CheckAutoNilOfSingleMark___, TESTCOUNTER())) {   \
  EXPECT_FALSE(ExpectFailureListener::IsMarkActivate());                              \
}

BOUNDARY_TEST();

TEST(ExpectedFailureHandling, CheckAutoResetOfSingleMark) {
  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE();
  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_FALSE(true);   // expected to fail!

  EXPECT_FALSE(ExpectFailureListener::IsMarkActivate());

  EXPECT_TRUE(true);
}

BOUNDARY_TEST();

TEST(ExpectedFailureHandling, CheckAutoResetOfMultipleMark) {
  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE(5);
  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_FALSE(true);         // expected to fail!

  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_FALSE(true);         // expected to fail!
  EXPECT_EQ(0x1337, 0xDEAD);  // expected to fail!
  EXPECT_NE(7, 7);            // expected to fail!

  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_TRUE(false);         // expected to fail!

  EXPECT_FALSE(ExpectFailureListener::IsMarkActivate());
}

BOUNDARY_TEST();

TEST(ExpectedFailureHandling, CheckDetectionOfUnexpectedPrecedingSuccesses) {
  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE(1);
  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_TRUE(true);   // expected to fail? but... DOES NOT! --> nothing happens, as no part-report is produced, ...

  // ... hence the mark remains active, waiting for a failure to occur...
  EXPECT_TRUE(ExpectFailureListener::IsMarkActivate());

  EXPECT_FALSE(true);  // ... and this one thusly is expected to fail -- which it does.

  // which implies our mark must have been released by now:
  EXPECT_FALSE(ExpectFailureListener::IsMarkActivate());
}

BOUNDARY_TEST();

// --------------------------------------------------

TEST_F(ExceptionHandlingTest, LoggerAbortsViaException) {
  EXPECT_TRUE(HasInstalledCustomFailureFunction());

  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE();

  // Log(FATAL) should throw an exception...
  LOG(FATAL) << "Buggerit Millenium Hand & Shrimp!";

  ASSERT_FALSE("Never to arrive here...");
}

static void fake_library_call(void) {
  throw std::exception("blargh!");
}

TEST_F(ExceptionHandlingTest, InnerCodeThrows) {
  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE();

  fake_library_call();

  ASSERT_FALSE("Never to arrive here...");
}

static void fake_library_call2(void) {
  LIBASSERT_DEBUG_ASSERT(false, "Expects to kick the debugger (iff you've attached one)...");
  LIBASSERT_ASSERT(false, "Expects to throw...");
}

TEST_F(ExceptionHandlingTest, InnerCodeHitsAnAssertionAndThrows) {
  GTEST_MARK_NEXT_AS_EXPECTED_FAILURE();

  fake_library_call2();

  ASSERT_FALSE("Never to arrive here...");
}
