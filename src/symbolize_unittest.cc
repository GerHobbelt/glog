// Copyright (c) 2006, Google Inc.
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
// Author: Satoru Takabayashi
//
// Unit tests for functions in symbolize.cc.

#include "symbolize.h"

#include <csignal>
#include <iostream>

#include "config.h"
#include "glog/logging.h"
#include "googletest.h"
#include "utilities.h"

#include "testing.h"

#ifdef HAVE_LIB_GFLAGS
#include <gflags/gflags.h>
using namespace GFLAGS_NAMESPACE;
#endif

using namespace std;
using namespace GOOGLE_NAMESPACE;

// Avoid compile error due to "cast between pointer-to-function and
// pointer-to-object is an extension" warnings.
#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

#if defined(HAVE_STACKTRACE)

#define always_inline

#if defined(__ELF__) || defined(GLOG_OS_WINDOWS) || defined(GLOG_OS_CYGWIN)
// A wrapper function for Symbolize() to make the unit test simple.
static const char *TrySymbolize(void *pc) {
  static char symbol[4096];
  if (Symbolize(pc, symbol, sizeof(symbol))) {
    return symbol;
  } else {
    return nullptr;
  }
}
#endif

# if defined(__ELF__)

// This unit tests make sense only with GCC.
// Uses lots of GCC specific features.
#if defined(__GNUC__) && !defined(__OPENCC__)
#  if __GNUC__ >= 4
#    define TEST_WITH_MODERN_GCC
#    if defined(__i386__) && __i386__  // always_inline isn't supported for x86_64 with GCC 4.1.0.
#      undef always_inline
#      define always_inline __attribute__((always_inline))
#      define HAVE_ALWAYS_INLINE
#    endif  // __i386__
#  else
#  endif  // __GNUC__ >= 4
#  define TEST_WITH_LABEL_ADDRESSES
#endif

// Make them C linkage to avoid mangled names.
extern "C" {
void nonstatic_func();
void nonstatic_func() {
  volatile int a = 0;
  // NOTE: In C++20, increment of object of volatile-qualified type is
  // deprecated.
  a = a + 1;
}

static void static_func() {
  volatile int a = 0;
  // NOTE: In C++20, increment of object of volatile-qualified type is
  // deprecated.
  a = a + 1;
}
}

TEST(Symbolize, Symbolize) {
  // We do C-style cast since GCC 2.95.3 doesn't allow
  // reinterpret_cast<void *>(&func).

  // Compilers should give us pointers to them.
  EXPECT_STREQ("nonstatic_func", TrySymbolize((void *)(&nonstatic_func)));

  // The name of an internal linkage symbol is not specified; allow either a
  // mangled or an unmangled name here.
  const char *static_func_symbol =
      TrySymbolize(reinterpret_cast<void *>(&static_func));

#if !defined(_MSC_VER) || !defined(NDEBUG)
  CHECK(nullptr != static_func_symbol);
  EXPECT_TRUE(strcmp("static_func", static_func_symbol) == 0 ||
              strcmp("static_func()", static_func_symbol) == 0);
#endif

  EXPECT_TRUE(nullptr == TrySymbolize(nullptr));
}

struct Foo {
  static void func(int x);
};

void ATTRIBUTE_NOINLINE Foo::func(int x) {
  volatile int a = x;
  // NOTE: In C++20, increment of object of volatile-qualified type is
  // deprecated.
  a = a + 1;
}

// With a modern GCC, Symbolize() should return demangled symbol
// names.  Function parameters should be omitted.
#ifdef TEST_WITH_MODERN_GCC
TEST(Symbolize, SymbolizeWithDemangling) {
  Foo::func(100);
#if !defined(_MSC_VER) || !defined(NDEBUG)
  EXPECT_STREQ("Foo::func()", TrySymbolize((void *)(&Foo::func)));
#endif
}
#endif

// Tests that verify that Symbolize footprint is within some limit.

// To measure the stack footprint of the Symbolize function, we create
// a signal handler (for SIGUSR1 say) that calls the Symbolize function
// on an alternate stack. This alternate stack is initialized to some
// known pattern (0x55, 0x55, 0x55, ...). We then self-send this signal,
// and after the signal handler returns, look at the alternate stack
// buffer to see what portion has been touched.
//
// This trick gives us the the stack footprint of the signal handler.
// But the signal handler, even before the call to Symbolize, consumes
// some stack already. We however only want the stack usage of the
// Symbolize function. To measure this accurately, we install two signal
// handlers: one that does nothing and just returns, and another that
// calls Symbolize. The difference between the stack consumption of these
// two signals handlers should give us the Symbolize stack foorprint.

static void *g_pc_to_symbolize;
static char g_symbolize_buffer[4096];
static char *g_symbolize_result;

static void EmptySignalHandler(int /*signo*/) {}

static void SymbolizeSignalHandler(int /*signo*/) {
  if (Symbolize(g_pc_to_symbolize, g_symbolize_buffer,
                sizeof(g_symbolize_buffer))) {
    g_symbolize_result = g_symbolize_buffer;
  } else {
    g_symbolize_result = nullptr;
  }
}

const int kAlternateStackSize = 8096;
const char kAlternateStackFillValue = 0x55;

// These helper functions look at the alternate stack buffer, and figure
// out what portion of this buffer has been touched - this is the stack
// consumption of the signal handler running on this alternate stack.
static ATTRIBUTE_NOINLINE bool StackGrowsDown(int *x) {
  int y;
  return &y < x;
}
static int GetStackConsumption(const char* alt_stack) {
  int x;
  if (StackGrowsDown(&x)) {
    for (int i = 0; i < kAlternateStackSize; i++) {
      if (alt_stack[i] != kAlternateStackFillValue) {
        return (kAlternateStackSize - i);
      }
    }
  } else {
    for (int i = (kAlternateStackSize - 1); i >= 0; i--) {
      if (alt_stack[i] != kAlternateStackFillValue) {
        return i;
      }
    }
  }
  return -1;
}

#ifdef HAVE_SIGALTSTACK

// Call Symbolize and figure out the stack footprint of this call.
static const char *SymbolizeStackConsumption(void *pc, int *stack_consumed) {

  g_pc_to_symbolize = pc;

  // The alt-signal-stack cannot be heap allocated because there is a
  // bug in glibc-2.2 where some signal handler setup code looks at the
  // current stack pointer to figure out what thread is currently running.
  // Therefore, the alternate stack must be allocated from the main stack
  // itself.
  char altstack[kAlternateStackSize];
  memset(altstack, kAlternateStackFillValue, kAlternateStackSize);

  // Set up the alt-signal-stack (and save the older one).
  stack_t sigstk;
  memset(&sigstk, 0, sizeof(stack_t));
  stack_t old_sigstk;
  sigstk.ss_sp = altstack;
  sigstk.ss_size = kAlternateStackSize;
  sigstk.ss_flags = 0;
  CHECK_ERR(sigaltstack(&sigstk, &old_sigstk));

  // Set up SIGUSR1 and SIGUSR2 signal handlers (and save the older ones).
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  struct sigaction old_sa1, old_sa2;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_ONSTACK;

  // SIGUSR1 maps to EmptySignalHandler.
  sa.sa_handler = EmptySignalHandler;
  CHECK_ERR(sigaction(SIGUSR1, &sa, &old_sa1));

  // SIGUSR2 maps to SymbolizeSignalHanlder.
  sa.sa_handler = SymbolizeSignalHandler;
  CHECK_ERR(sigaction(SIGUSR2, &sa, &old_sa2));

  // Send SIGUSR1 signal and measure the stack consumption of the empty
  // signal handler.
  CHECK_ERR(kill(getpid(), SIGUSR1));
  int stack_consumption1 = GetStackConsumption(altstack);

  // Send SIGUSR2 signal and measure the stack consumption of the symbolize
  // signal handler.
  CHECK_ERR(kill(getpid(), SIGUSR2));
  int stack_consumption2 = GetStackConsumption(altstack);

  // The difference between the two stack consumption values is the
  // stack footprint of the Symbolize function.
  if (stack_consumption1 != -1 && stack_consumption2 != -1) {
    *stack_consumed = stack_consumption2 - stack_consumption1;
  } else {
    *stack_consumed = -1;
  }

  // Log the stack consumption values.
  LOG(INFO) << "Stack consumption of empty signal handler: "
            << stack_consumption1;
  LOG(INFO) << "Stack consumption of symbolize signal handler: "
            << stack_consumption2;
  LOG(INFO) << "Stack consumption of Symbolize: " << *stack_consumed;

  // Now restore the old alt-signal-stack and signal handlers.
  CHECK_ERR(sigaltstack(&old_sigstk, nullptr));
  CHECK_ERR(sigaction(SIGUSR1, &old_sa1, nullptr));
  CHECK_ERR(sigaction(SIGUSR2, &old_sa2, nullptr));

  return g_symbolize_result;
}

#ifdef __ppc64__
// Symbolize stack consumption should be within 4kB.
const int kStackConsumptionUpperLimit = 4096;
#else
// Symbolize stack consumption should be within 2kB.
const int kStackConsumptionUpperLimit = 2048;
#endif

TEST(Symbolize, SymbolizeStackConsumption) {
  int stack_consumed;
  const char* symbol;

  symbol = SymbolizeStackConsumption(reinterpret_cast<void *>(&nonstatic_func),
                                     &stack_consumed);
  EXPECT_STREQ("nonstatic_func", symbol);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, kStackConsumptionUpperLimit);

  // The name of an internal linkage symbol is not specified; allow either a
  // mangled or an unmangled name here.
  symbol = SymbolizeStackConsumption(reinterpret_cast<void *>(&static_func),
                                     &stack_consumed);
  CHECK(nullptr != symbol);
  EXPECT_TRUE(strcmp("static_func", symbol) == 0 ||
              strcmp("static_func()", symbol) == 0);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, kStackConsumptionUpperLimit);
}

#ifdef TEST_WITH_MODERN_GCC
TEST(Symbolize, SymbolizeWithDemanglingStackConsumption) {
  Foo::func(100);
  int stack_consumed;
  const char* symbol;

  symbol = SymbolizeStackConsumption(reinterpret_cast<void *>(&Foo::func),
                                     &stack_consumed);

  EXPECT_STREQ("Foo::func()", symbol);
  EXPECT_GT(stack_consumed, 0);
  EXPECT_LT(stack_consumed, kStackConsumptionUpperLimit);
}
#endif

#endif  // HAVE_SIGALTSTACK

// x86 specific tests.  Uses some inline assembler.
extern "C" {
inline void* always_inline inline_func() {
  void *pc = nullptr;
#ifdef TEST_WITH_LABEL_ADDRESSES
  pc = &&curr_pc;
  curr_pc:
#endif
  return pc;
}

void* ATTRIBUTE_NOINLINE non_inline_func();
void* ATTRIBUTE_NOINLINE non_inline_func() {
  void *pc = nullptr;
#ifdef TEST_WITH_LABEL_ADDRESSES
  pc = &&curr_pc;
  curr_pc:
#endif
  return pc;
}

static void ATTRIBUTE_NOINLINE TestWithPCInsideNonInlineFunction() {
#if defined(TEST_WITH_LABEL_ADDRESSES) && defined(HAVE_ATTRIBUTE_NOINLINE)
  void *pc = non_inline_func();
  const char *symbol = TrySymbolize(pc);

#if !defined(_MSC_VER) || !defined(NDEBUG)
  CHECK(symbol != nullptr);
  CHECK_STREQ(symbol, "non_inline_func");
#endif
  cout << "Test case TestWithPCInsideNonInlineFunction passed." << endl;
#endif
}

static void ATTRIBUTE_NOINLINE TestWithPCInsideInlineFunction() {
#if defined(TEST_WITH_LABEL_ADDRESSES) && defined(HAVE_ALWAYS_INLINE)
  void *pc = inline_func();  // Must be inlined.
  const char *symbol = TrySymbolize(pc);

#if !defined(_MSC_VER) || !defined(NDEBUG)
  CHECK(symbol != nullptr);
  CHECK_STREQ(symbol, __FUNCTION__);
#endif
  cout << "Test case TestWithPCInsideInlineFunction passed." << endl;
#endif
}
}

// Test with a return address.
static void ATTRIBUTE_NOINLINE TestWithReturnAddress() {
#if defined(HAVE_ATTRIBUTE_NOINLINE)
  void *return_address = __builtin_return_address(0);
  const char *symbol = TrySymbolize(return_address);

#if !defined(_MSC_VER) || !defined(NDEBUG)
  CHECK(symbol != nullptr);
  CHECK_STREQ(symbol, "main");
#endif
  cout << "Test case TestWithReturnAddress passed." << endl;
#endif
}

# elif defined(GLOG_OS_WINDOWS) || defined(GLOG_OS_CYGWIN)

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#endif

struct Foo {
  static void func(int x);
};

__declspec(noinline) void Foo::func(int x) {
  volatile int a = x;
  // NOTE: In C++20, increment of object of volatile-qualified type is
  // deprecated.
  a = a + 1;
}

TEST(Symbolize, SymbolizeWithDemangling) {
  Foo::func(100);
  const char* ret = TrySymbolize((void *)(&Foo::func));

#if defined(HAVE_DBGHELP) && !defined(NDEBUG)
  EXPECT_STREQ("public: static void __cdecl Foo::func(int)", ret);
#endif
}

__declspec(noinline) void TestWithReturnAddress() {
  void *return_address =
#ifdef __GNUC__ // Cygwin and MinGW support
	  __builtin_return_address(0)
#else
	  _ReturnAddress()
#endif
	  ;
  const char *symbol = TrySymbolize(return_address);
#if !defined(_MSC_VER) || !defined(NDEBUG)
  CHECK(symbol != nullptr);
#if defined(BUILD_MONOLITHIC)
  CHECK_STREQ(symbol, "glog_symbolize_unittest_main");
#else
  CHECK_STREQ(symbol, "main");
#endif
#endif
  cout << "Test case TestWithReturnAddress passed." << endl;
}
# endif  // __ELF__
#endif  // HAVE_STACKTRACE

#if defined(BUILD_MONOLITHIC)
#define main(cnt, arr)      glog_symbolize_unittest_main(cnt, arr)
#endif

extern "C"
int main(int argc, const char** argv) {
  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  InitGoogleTest(&argc, argv);
#if defined(HAVE_SYMBOLIZE) && defined(HAVE_STACKTRACE)
# if defined(__ELF__)
  // We don't want to get affected by the callback interface, that may be
  // used to install some callback function at InitGoogle() time.
  InstallSymbolizeCallback(nullptr);

  TestWithPCInsideInlineFunction();
  TestWithPCInsideNonInlineFunction();
  TestWithReturnAddress();
  return RUN_ALL_TESTS();
# elif defined(GLOG_OS_WINDOWS) || defined(GLOG_OS_CYGWIN)
  TestWithReturnAddress();
  return RUN_ALL_TESTS();
# else  // GLOG_OS_WINDOWS
  printf("PASS (no symbolize_unittest support)\n");
  return 0;
# endif  // __ELF__
#else
  printf("PASS (no symbolize support)\n");
  return 0;
#endif  // HAVE_SYMBOLIZE
}

#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
