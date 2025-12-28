/// @file callable_test.cpp
/// @brief Unit tests for all supported callable types

#include <fb/signal.hpp>

#include <gtest/gtest.h>

#include <functional>
#include <string>

namespace fb {
namespace test {

// ============================================================================
// Test Fixtures and Helpers
// ============================================================================

namespace {

int g_free_function_value = 0;

void free_function(int value) { g_free_function_value = value; }

void free_function_noargs() { g_free_function_value = 999; }

struct Functor {
  int call_count = 0;
  int last_value = 0;

  void operator()(int value) {
    ++call_count;
    last_value = value;
  }
};

class TestClass {
public:
  int call_count = 0;
  int last_value = 0;

  void member_function(int value) {
    ++call_count;
    last_value = value;
  }

  void const_member_function(int value) const {
    // Can't modify members, but we can verify it compiles
  }

  static void static_member_function(int value) { s_static_value = value; }

  static int s_static_value;
};

int TestClass::s_static_value = 0;

} // anonymous namespace

// ============================================================================
// Lambda Tests
// ============================================================================

TEST(CallableTest, Lambda_NonCapturing_Works) {
  signal<int> sig;
  static int value = 0;

  sig.connect([](int v) { value = v; });

  sig.emit(42);

  EXPECT_EQ(42, value);
}

TEST(CallableTest, Lambda_CapturingByReference_Works) {
  signal<int> sig;
  int captured = 0;

  sig.connect([&captured](int value) { captured = value; });

  sig.emit(123);

  EXPECT_EQ(123, captured);
}

TEST(CallableTest, Lambda_CapturingByValue_Works) {
  signal<int> sig;
  int multiplier = 10;
  int result = 0;

  sig.connect(
      [multiplier, &result](int value) { result = value * multiplier; });

  sig.emit(5);

  EXPECT_EQ(50, result);
}

TEST(CallableTest, Lambda_LargeCapture_Works) {
  signal<int> sig;

  // Capture many variables (exceeds SBO threshold)
  int a = 1, b = 2, c = 3, d = 4, e = 5, f = 6, g = 7, h = 8;
  int result = 0;

  sig.connect([a, b, c, d, e, f, g, h, &result](int value) {
    result = a + b + c + d + e + f + g + h + value;
  });

  sig.emit(100);

  EXPECT_EQ(136, result); // 1+2+3+4+5+6+7+8+100
}

TEST(CallableTest, Lambda_MutableCapture_Works) {
  signal<int> sig;

  int counter = 0;
  sig.connect([counter, &counter2 = counter](int) mutable {
    ++counter;  // Modifies lambda's copy
    ++counter2; // Modifies external variable
  });

  sig.emit(0);
  sig.emit(0);

  EXPECT_EQ(2, counter);
}

// ============================================================================
// Free Function Tests
// ============================================================================

TEST(CallableTest, FreeFunction_Pointer_Works) {
  signal<int> sig;
  g_free_function_value = 0;

  sig.connect(&free_function);

  sig.emit(42);

  EXPECT_EQ(42, g_free_function_value);
}

TEST(CallableTest, FreeFunction_NoArgs_Works) {
  signal<> sig;
  g_free_function_value = 0;

  sig.connect(&free_function_noargs);

  sig.emit();

  EXPECT_EQ(999, g_free_function_value);
}

TEST(CallableTest, StaticMemberFunction_Works) {
  signal<int> sig;
  TestClass::s_static_value = 0;

  sig.connect(&TestClass::static_member_function);

  sig.emit(77);

  EXPECT_EQ(77, TestClass::s_static_value);
}

// ============================================================================
// Member Function Tests
// ============================================================================

TEST(CallableTest, MemberFunction_Works) {
  signal<int> sig;
  TestClass obj;

  sig.connect(&obj, &TestClass::member_function);

  sig.emit(42);

  EXPECT_EQ(1, obj.call_count);
  EXPECT_EQ(42, obj.last_value);
}

TEST(CallableTest, ConstMemberFunction_Works) {
  signal<int> sig;
  const TestClass obj{};

  sig.connect(&obj, &TestClass::const_member_function);

  // Should compile and not crash
  sig.emit(42);
}

TEST(CallableTest, MemberFunction_MultipleCalls_Works) {
  signal<int> sig;
  TestClass obj;

  sig.connect(&obj, &TestClass::member_function);

  sig.emit(1);
  sig.emit(2);
  sig.emit(3);

  EXPECT_EQ(3, obj.call_count);
  EXPECT_EQ(3, obj.last_value);
}

// ============================================================================
// Functor Tests
// ============================================================================

TEST(CallableTest, Functor_Works) {
  signal<int> sig;
  Functor functor;

  sig.connect(std::ref(functor));

  sig.emit(42);

  EXPECT_EQ(1, functor.call_count);
  EXPECT_EQ(42, functor.last_value);
}

TEST(CallableTest, Functor_Copy_Works) {
  signal<int> sig;
  Functor functor;

  sig.connect(functor); // Copy, not reference

  sig.emit(42);

  // Original functor not modified (copy was invoked)
  EXPECT_EQ(0, functor.call_count);
}

// ============================================================================
// std::function Tests
// ============================================================================

TEST(CallableTest, StdFunction_FromLambda_Works) {
  signal<int> sig;
  int received = 0;

  std::function<void(int)> func = [&received](int value) { received = value; };

  sig.connect(func);

  sig.emit(42);

  EXPECT_EQ(42, received);
}

TEST(CallableTest, StdFunction_FromFreeFunction_Works) {
  signal<int> sig;
  g_free_function_value = 0;

  std::function<void(int)> func = &free_function;

  sig.connect(func);

  sig.emit(88);

  EXPECT_EQ(88, g_free_function_value);
}

TEST(CallableTest, StdFunction_Move_Works) {
  signal<int> sig;
  int received = 0;

  std::function<void(int)> func = [&received](int value) { received = value; };

  sig.connect(std::move(func));

  sig.emit(42);

  EXPECT_EQ(42, received);
}

// ============================================================================
// Complex Argument Type Tests
// ============================================================================

TEST(CallableTest, ComplexArguments_Works) {
  signal<const std::string &, int, double> sig;

  std::string received_str;
  int received_int = 0;
  double received_double = 0.0;

  sig.connect([&](const std::string &s, int i, double d) {
    received_str = s;
    received_int = i;
    received_double = d;
  });

  sig.emit("test", 42, 3.14);

  EXPECT_EQ("test", received_str);
  EXPECT_EQ(42, received_int);
  EXPECT_DOUBLE_EQ(3.14, received_double);
}

TEST(CallableTest, RValueReference_Arguments_Works) {
  signal<std::string &&> sig;
  std::string received;

  sig.connect([&received](std::string &&s) { received = std::move(s); });

  std::string test = "hello";
  sig.emit(std::move(test));

  EXPECT_EQ("hello", received);
}

} // namespace test
} // namespace fb
