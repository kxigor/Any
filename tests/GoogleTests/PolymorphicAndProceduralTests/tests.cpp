#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "Polymorphic/AnyPolymorphic.hpp"
#include "Procedural/AnyProcedural.hpp"

struct TestStruct {
  int id;
  std::string name;

  TestStruct(int i, const std::string& n) : id(i), name(n) {}
  TestStruct() : id(0), name("default") {}

  bool operator==(const TestStruct& other) const {
    return id == other.id && name == other.name;
  }
};

struct LifetimeTracker {
  static int constructor_count;
  static int destructor_count;
  static int copy_count;
  static int move_count;

  int value;

  LifetimeTracker(int v = 0) : value(v) { ++constructor_count; }
  LifetimeTracker(const LifetimeTracker& other) : value(other.value) {
    ++constructor_count;
    ++copy_count;
  }
  LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value) {
    ++constructor_count;
    ++move_count;
    other.value = -1;
  }
  ~LifetimeTracker() { ++destructor_count; }

  LifetimeTracker& operator=(const LifetimeTracker&) = default;
  LifetimeTracker& operator=(LifetimeTracker&&) = default;

  static void reset() {
    constructor_count = destructor_count = copy_count = move_count = 0;
  }
};

int LifetimeTracker::constructor_count = 0;
int LifetimeTracker::destructor_count = 0;
int LifetimeTracker::copy_count = 0;
int LifetimeTracker::move_count = 0;

class AnyTest : public ::testing::Test {
 protected:
  void SetUp() override { LifetimeTracker::reset(); }

  void TearDown() override { LifetimeTracker::reset(); }
};

/* ==================== ТЕСТЫ КОНСТРУКТОРОВ ==================== */

TEST_F(AnyTest, DefaultConstructor) {
  Any::Polymorphic::Any any;
  EXPECT_FALSE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(void));
}

TEST_F(AnyTest, ValueConstructorWithBuiltInType) {
  Any::Polymorphic::Any any(42);
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(int));
  EXPECT_EQ(any.AnyCast<int>(), 42);
}

TEST_F(AnyTest, ValueConstructorWithString) {
  Any::Polymorphic::Any any(std::string("hello"));
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(any.AnyCast<std::string>(), "hello");
}

TEST_F(AnyTest, ValueConstructorWithClass) {
  TestStruct obj(123, "test");
  Any::Polymorphic::Any any(obj);
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(any.AnyCast<TestStruct>(), obj);
}

TEST_F(AnyTest, ValueConstructorMovesRvalue) {
  Any::Polymorphic::Any any(TestStruct(456, "move"));
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(any.AnyCast<TestStruct>().id, 456);
}

TEST_F(AnyTest, CopyConstructor) {
  Any::Polymorphic::Any original(100);
  Any::Polymorphic::Any copy(original);

  EXPECT_TRUE(original.HasValue());
  EXPECT_TRUE(copy.HasValue());
  EXPECT_EQ(original.Type(), typeid(int));
  EXPECT_EQ(copy.Type(), typeid(int));
  EXPECT_EQ(original.AnyCast<int>(), 100);
  EXPECT_EQ(copy.AnyCast<int>(), 100);

  // Проверяем, что это независимые копии
  original.AnyCast<int>() = 200;
  EXPECT_EQ(original.AnyCast<int>(), 200);
  EXPECT_EQ(copy.AnyCast<int>(), 100);
}

TEST_F(AnyTest, CopyConstructorWithEmpty) {
  Any::Polymorphic::Any original;
  Any::Polymorphic::Any copy(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(copy.HasValue());
  EXPECT_EQ(original.Type(), typeid(void));
  EXPECT_EQ(copy.Type(), typeid(void));
}

TEST_F(AnyTest, MoveConstructor) {
  Any::Polymorphic::Any original(3.14);
  Any::Polymorphic::Any moved(std::move(original));

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(moved.HasValue());
  EXPECT_EQ(moved.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(moved.AnyCast<double>(), 3.14);
}

TEST_F(AnyTest, MoveConstructorWithEmpty) {
  Any::Polymorphic::Any original;
  Any::Polymorphic::Any moved(std::move(original));

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(moved.HasValue());
}

/* ==================== ТЕСТЫ ОПЕРАТОРОВ ПРИСВАИВАНИЯ ==================== */

TEST_F(AnyTest, CopyAssignment) {
  Any::Polymorphic::Any original(42);
  Any::Polymorphic::Any assigned;

  assigned = original;

  EXPECT_TRUE(original.HasValue());
  EXPECT_TRUE(assigned.HasValue());
  EXPECT_EQ(original.AnyCast<int>(), 42);
  EXPECT_EQ(assigned.AnyCast<int>(), 42);

  original.AnyCast<int>() = 100;
  EXPECT_EQ(assigned.AnyCast<int>(), 42);
}

TEST_F(AnyTest, CopyAssignmentSelf) {
  Any::Polymorphic::Any any(42);
  any = any;  // self-assignment

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.AnyCast<int>(), 42);
}

TEST_F(AnyTest, CopyAssignmentWithEmpty) {
  Any::Polymorphic::Any original(42);
  Any::Polymorphic::Any empty;

  empty = original;

  EXPECT_TRUE(empty.HasValue());
  EXPECT_EQ(empty.AnyCast<int>(), 42);
}

TEST_F(AnyTest, CopyAssignmentFromEmpty) {
  Any::Polymorphic::Any original(42);
  Any::Polymorphic::Any empty;

  original = empty;

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(empty.HasValue());
}

TEST_F(AnyTest, MoveAssignment) {
  Any::Polymorphic::Any original("test");
  Any::Polymorphic::Any assigned;

  assigned = std::move(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(assigned.HasValue());
  EXPECT_EQ(assigned.Type(), typeid(const char*));
  EXPECT_STREQ(assigned.AnyCast<const char*>(), "test");
}

TEST_F(AnyTest, MoveAssignmentSelf) {
  Any::Polymorphic::Any any(42);
  any = std::move(any);

  SUCCEED();
}

TEST_F(AnyTest, MoveAssignmentWithEmpty) {
  Any::Polymorphic::Any original(3.14);
  Any::Polymorphic::Any empty;

  empty = std::move(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(empty.HasValue());
  EXPECT_DOUBLE_EQ(empty.AnyCast<double>(), 3.14);
}

TEST_F(AnyTest, MoveAssignmentFromEmpty) {
  Any::Polymorphic::Any original(42);
  Any::Polymorphic::Any empty;

  original = std::move(empty);

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(empty.HasValue());
}

/* ==================== ТЕСТЫ МОДИФИКАТОРОВ ==================== */

TEST_F(AnyTest, EmplaceNewValue) {
  Any::Polymorphic::Any any;

  auto& value = any.Emplace<std::string>("emplaced");

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(value, "emplaced");
  EXPECT_EQ(any.AnyCast<std::string>(), "emplaced");
}

TEST_F(AnyTest, EmplaceOverExistingValue) {
  Any::Polymorphic::Any any(100);

  auto& value = any.Emplace<double>(2.71);

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(value, 2.71);
  EXPECT_DOUBLE_EQ(any.AnyCast<double>(), 2.71);
}

TEST_F(AnyTest, EmplaceWithMultipleArgs) {
  Any::Polymorphic::Any any;

  auto& value = any.Emplace<TestStruct>(789, "emplaced");

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(value.id, 789);
  EXPECT_EQ(value.name, "emplaced");
}

TEST_F(AnyTest, Reset) {
  Any::Polymorphic::Any any(42);

  any.Reset();

  EXPECT_FALSE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(void));
}

TEST_F(AnyTest, ResetEmpty) {
  Any::Polymorphic::Any any;

  any.Reset();

  EXPECT_FALSE(any.HasValue());
}

TEST_F(AnyTest, Swap) {
  Any::Polymorphic::Any a(42);
  Any::Polymorphic::Any b(std::string("hello"));

  a.Swap(b);

  EXPECT_TRUE(a.HasValue());
  EXPECT_TRUE(b.HasValue());
  EXPECT_EQ(a.Type(), typeid(std::string));
  EXPECT_EQ(b.Type(), typeid(int));
  EXPECT_EQ(a.AnyCast<std::string>(), "hello");
  EXPECT_EQ(b.AnyCast<int>(), 42);
}

TEST_F(AnyTest, SwapWithEmpty) {
  Any::Polymorphic::Any a(42);
  Any::Polymorphic::Any b;

  a.Swap(b);

  EXPECT_FALSE(a.HasValue());
  EXPECT_TRUE(b.HasValue());
  EXPECT_EQ(b.AnyCast<int>(), 42);
}

TEST_F(AnyTest, SwapBothEmpty) {
  Any::Polymorphic::Any a;
  Any::Polymorphic::Any b;

  a.Swap(b);

  EXPECT_FALSE(a.HasValue());
  EXPECT_FALSE(b.HasValue());
}

/* ==================== ТЕСТЫ НАБЛЮДАТЕЛЕЙ ==================== */

TEST_F(AnyTest, HasValue) {
  Any::Polymorphic::Any empty;
  Any::Polymorphic::Any with_value(42);

  EXPECT_FALSE(empty.HasValue());
  EXPECT_TRUE(with_value.HasValue());
}

TEST_F(AnyTest, Type) {
  Any::Polymorphic::Any empty;
  Any::Polymorphic::Any int_any(42);
  Any::Polymorphic::Any string_any(std::string("test"));
  Any::Polymorphic::Any double_any(3.14);

  EXPECT_EQ(empty.Type(), typeid(void));
  EXPECT_EQ(int_any.Type(), typeid(int));
  EXPECT_EQ(string_any.Type(), typeid(std::string));
  EXPECT_EQ(double_any.Type(), typeid(double));
}

TEST_F(AnyTest, AnyCastCorrectType) {
  Any::Polymorphic::Any any(42);

  int& value = any.AnyCast<int>();
  EXPECT_EQ(value, 42);

  value = 100;
  EXPECT_EQ(any.AnyCast<int>(), 100);
}

TEST_F(AnyTest, AnyCastConstCorrectType) {
  const Any::Polymorphic::Any any(42);

  const int& value = any.AnyCast<int>();
  EXPECT_EQ(value, 42);
}

TEST_F(AnyTest, AnyCastWrongType) {
  Any::Polymorphic::Any any(42);

  EXPECT_THROW(std::ignore = any.AnyCast<std::string>(),
               Any::Polymorphic::Any::BadCast);
}

TEST_F(AnyTest, AnyCastEmpty) {
  Any::Polymorphic::Any any;

  EXPECT_THROW(std::ignore = any.AnyCast<int>(),
               Any::Polymorphic::Any::BadCast);
}

/* ==================== ТЕСТЫ ВРЕМЕНИ ЖИЗНИ ==================== */

TEST_F(AnyTest, LifetimeManagement) {
  {
    Any::Polymorphic::Any any(LifetimeTracker(42));
    EXPECT_EQ(LifetimeTracker::constructor_count, 2);
    EXPECT_EQ(LifetimeTracker::destructor_count, 1);
  }
  EXPECT_EQ(LifetimeTracker::destructor_count, 2);
}

TEST_F(AnyTest, CopyLifetime) {
  LifetimeTracker tracker(42);
  LifetimeTracker::reset();

  {
    Any::Polymorphic::Any original(tracker);
    EXPECT_EQ(LifetimeTracker::copy_count, 1);

    Any::Polymorphic::Any copy = original;
    EXPECT_EQ(LifetimeTracker::copy_count, 2);
  }

  EXPECT_EQ(LifetimeTracker::destructor_count, 2);
}

TEST_F(AnyTest, MoveLifetime) {
  LifetimeTracker tracker(42);
  LifetimeTracker::reset();

  {
    Any::Polymorphic::Any original(std::move(tracker));
    EXPECT_EQ(LifetimeTracker::move_count, 1);

    Any::Polymorphic::Any moved = std::move(original);
    EXPECT_EQ(LifetimeTracker::copy_count, 0);
  }

  EXPECT_EQ(LifetimeTracker::destructor_count, 1);
}

/* ==================== ТЕСТЫ РАЗНЫХ ТИПОВ ==================== */

TEST_F(AnyTest, WithVector) {
  std::vector<int> vec{1, 2, 3};
  Any::Polymorphic::Any any(vec);

  auto& stored_vec = any.AnyCast<std::vector<int>>();
  EXPECT_EQ(stored_vec.size(), 3);
  EXPECT_EQ(stored_vec[0], 1);

  stored_vec.push_back(4);
  EXPECT_EQ(stored_vec.size(), 4);
}

TEST_F(AnyTest, WithPointer) {
  int value = 42;
  Any::Polymorphic::Any any(&value);

  int* ptr = any.AnyCast<int*>();
  EXPECT_EQ(ptr, &value);
  EXPECT_EQ(*ptr, 42);
}

TEST_F(AnyTest, WithConstType) {
  const int value = 100;
  Any::Polymorphic::Any any(value);

  EXPECT_EQ(any.AnyCast<int>(), 100);
}

/* ==================== ТЕСТЫ ИСКЛЮЧЕНИЙ ==================== */

TEST_F(AnyTest, ExceptionSafetyOnCopy) {
  // Этот тест проверяет, что копирование сохраняет сильную гарантию исключений
  Any::Polymorphic::Any original(42);

  try {
    Any::Polymorphic::Any copy = original;
    SUCCEED();  // Если не брошено исключение
  } catch (...) {
    FAIL() << "Copy constructor should not throw for simple types";
  }
}

/* ==================== КОМПЛЕКСНЫЕ СЦЕНАРИИ ==================== */

TEST_F(AnyTest, ReassignDifferentTypes) {
  Any::Polymorphic::Any any;

  any = 42;
  EXPECT_EQ(any.Type(), typeid(int));
  EXPECT_EQ(any.AnyCast<int>(), 42);

  any = std::string("hello");
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(any.AnyCast<std::string>(), "hello");

  any = 3.14;
  EXPECT_EQ(any.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(any.AnyCast<double>(), 3.14);

  any.Reset();
  EXPECT_FALSE(any.HasValue());
}

TEST_F(AnyTest, AnyInContainer) {
  std::vector<Any::Polymorphic::Any> container;

  container.emplace_back(42);
  container.emplace_back(std::string("test"));
  container.emplace_back(TestStruct(1, "item"));

  EXPECT_EQ(container.size(), 3);
  EXPECT_EQ(container[0].AnyCast<int>(), 42);
  EXPECT_EQ(container[1].AnyCast<std::string>(), "test");
  EXPECT_EQ(container[2].AnyCast<TestStruct>().id, 1);

  std::vector<Any::Polymorphic::Any> copy = container;
  EXPECT_EQ(copy[0].AnyCast<int>(), 42);
  EXPECT_EQ(copy[1].AnyCast<std::string>(), "test");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}