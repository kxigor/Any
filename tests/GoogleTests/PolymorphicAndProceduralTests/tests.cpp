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

// Определяем типы для параметризованного тестирования
template <typename T>
class AnyTest : public ::testing::Test {
 protected:
  void SetUp() override { LifetimeTracker::reset(); }
  void TearDown() override { LifetimeTracker::reset(); }

  // Псевдоним для текущего типа Any
  using AnyType = T;
};

// Список типов для тестирования
using AnyTypes = ::testing::Types<Any::Polymorphic::Any, Any::Procedural::Any>;
TYPED_TEST_SUITE(AnyTest, AnyTypes);

/* ==================== ТЕСТЫ КОНСТРУКТОРОВ ==================== */

TYPED_TEST(AnyTest, DefaultConstructor) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;
  EXPECT_FALSE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(void));
}

TYPED_TEST(AnyTest, ValueConstructorWithBuiltInType) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(int));
  EXPECT_EQ(any.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, ValueConstructorWithString) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(std::string("hello"));
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(any.template AnyCast<std::string>(), "hello");
}

TYPED_TEST(AnyTest, ValueConstructorWithClass) {
  using AnyType = typename TestFixture::AnyType;

  TestStruct obj(123, "test");
  AnyType any(obj);
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(any.template AnyCast<TestStruct>(), obj);
}

TYPED_TEST(AnyTest, ValueConstructorMovesRvalue) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(TestStruct(456, "move"));
  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(any.template AnyCast<TestStruct>().id, 456);
}

TYPED_TEST(AnyTest, CopyConstructor) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(100);
  AnyType copy(original);

  EXPECT_TRUE(original.HasValue());
  EXPECT_TRUE(copy.HasValue());
  EXPECT_EQ(original.Type(), typeid(int));
  EXPECT_EQ(copy.Type(), typeid(int));
  EXPECT_EQ(original.template AnyCast<int>(), 100);
  EXPECT_EQ(copy.template AnyCast<int>(), 100);

  // Проверяем, что это независимые копии
  original.template AnyCast<int>() = 200;
  EXPECT_EQ(original.template AnyCast<int>(), 200);
  EXPECT_EQ(copy.template AnyCast<int>(), 100);
}

TYPED_TEST(AnyTest, CopyConstructorWithEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original;
  AnyType copy(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(copy.HasValue());
  EXPECT_EQ(original.Type(), typeid(void));
  EXPECT_EQ(copy.Type(), typeid(void));
}

TYPED_TEST(AnyTest, MoveConstructor) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(3.14);
  AnyType moved(std::move(original));

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(moved.HasValue());
  EXPECT_EQ(moved.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(moved.template AnyCast<double>(), 3.14);
}

TYPED_TEST(AnyTest, MoveConstructorWithEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original;
  AnyType moved(std::move(original));

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(moved.HasValue());
}

/* ==================== ТЕСТЫ ОПЕРАТОРОВ ПРИСВАИВАНИЯ ==================== */

TYPED_TEST(AnyTest, CopyAssignment) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(42);
  AnyType assigned;

  assigned = original;

  EXPECT_TRUE(original.HasValue());
  EXPECT_TRUE(assigned.HasValue());
  EXPECT_EQ(original.template AnyCast<int>(), 42);
  EXPECT_EQ(assigned.template AnyCast<int>(), 42);

  original.template AnyCast<int>() = 100;
  EXPECT_EQ(assigned.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, CopyAssignmentSelf) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);
  any = any;  // self-assignment

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, CopyAssignmentWithEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(42);
  AnyType empty;

  empty = original;

  EXPECT_TRUE(empty.HasValue());
  EXPECT_EQ(empty.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, CopyAssignmentFromEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(42);
  AnyType empty;

  original = empty;

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(empty.HasValue());
}

TYPED_TEST(AnyTest, MoveAssignment) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original("test");
  AnyType assigned;

  assigned = std::move(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(assigned.HasValue());
  EXPECT_EQ(assigned.Type(), typeid(const char*));
  EXPECT_STREQ(assigned.template AnyCast<const char*>(), "test");
}

TYPED_TEST(AnyTest, MoveAssignmentSelf) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);
  any = std::move(any);

  SUCCEED();
}

TYPED_TEST(AnyTest, MoveAssignmentWithEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(3.14);
  AnyType empty;

  empty = std::move(original);

  EXPECT_FALSE(original.HasValue());
  EXPECT_TRUE(empty.HasValue());
  EXPECT_DOUBLE_EQ(empty.template AnyCast<double>(), 3.14);
}

TYPED_TEST(AnyTest, MoveAssignmentFromEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType original(42);
  AnyType empty;

  original = std::move(empty);

  EXPECT_FALSE(original.HasValue());
  EXPECT_FALSE(empty.HasValue());
}

/* ==================== ТЕСТЫ МОДИФИКАТОРОВ ==================== */

TYPED_TEST(AnyTest, EmplaceNewValue) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;

  auto& value = any.template Emplace<std::string>("emplaced");

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(value, "emplaced");
  EXPECT_EQ(any.template AnyCast<std::string>(), "emplaced");
}

TYPED_TEST(AnyTest, EmplaceOverExistingValue) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(100);

  auto& value = any.template Emplace<double>(2.71);

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(value, 2.71);
  EXPECT_DOUBLE_EQ(any.template AnyCast<double>(), 2.71);
}

TYPED_TEST(AnyTest, EmplaceWithMultipleArgs) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;

  auto& value = any.template Emplace<TestStruct>(789, "emplaced");

  EXPECT_TRUE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(TestStruct));
  EXPECT_EQ(value.id, 789);
  EXPECT_EQ(value.name, "emplaced");
}

TYPED_TEST(AnyTest, Reset) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);

  any.Reset();

  EXPECT_FALSE(any.HasValue());
  EXPECT_EQ(any.Type(), typeid(void));
}

TYPED_TEST(AnyTest, ResetEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;

  any.Reset();

  EXPECT_FALSE(any.HasValue());
}

TYPED_TEST(AnyTest, Swap) {
  using AnyType = typename TestFixture::AnyType;

  AnyType a(42);
  AnyType b(std::string("hello"));

  a.Swap(b);

  EXPECT_TRUE(a.HasValue());
  EXPECT_TRUE(b.HasValue());
  EXPECT_EQ(a.Type(), typeid(std::string));
  EXPECT_EQ(b.Type(), typeid(int));
  EXPECT_EQ(a.template AnyCast<std::string>(), "hello");
  EXPECT_EQ(b.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, SwapWithEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType a(42);
  AnyType b;

  a.Swap(b);

  EXPECT_FALSE(a.HasValue());
  EXPECT_TRUE(b.HasValue());
  EXPECT_EQ(b.template AnyCast<int>(), 42);
}

TYPED_TEST(AnyTest, SwapBothEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType a;
  AnyType b;

  a.Swap(b);

  EXPECT_FALSE(a.HasValue());
  EXPECT_FALSE(b.HasValue());
}

/* ==================== ТЕСТЫ НАБЛЮДАТЕЛЕЙ ==================== */

TYPED_TEST(AnyTest, HasValue) {
  using AnyType = typename TestFixture::AnyType;

  AnyType empty;
  AnyType with_value(42);

  EXPECT_FALSE(empty.HasValue());
  EXPECT_TRUE(with_value.HasValue());
}

TYPED_TEST(AnyTest, Type) {
  using AnyType = typename TestFixture::AnyType;

  AnyType empty;
  AnyType int_any(42);
  AnyType string_any(std::string("test"));
  AnyType double_any(3.14);

  EXPECT_EQ(empty.Type(), typeid(void));
  EXPECT_EQ(int_any.Type(), typeid(int));
  EXPECT_EQ(string_any.Type(), typeid(std::string));
  EXPECT_EQ(double_any.Type(), typeid(double));
}

TYPED_TEST(AnyTest, AnyCastCorrectType) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);

  int& value = any.template AnyCast<int>();
  EXPECT_EQ(value, 42);

  value = 100;
  EXPECT_EQ(any.template AnyCast<int>(), 100);
}

TYPED_TEST(AnyTest, AnyCastConstCorrectType) {
  using AnyType = typename TestFixture::AnyType;

  const AnyType any(42);

  const int& value = any.template AnyCast<int>();
  EXPECT_EQ(value, 42);
}

TYPED_TEST(AnyTest, AnyCastWrongType) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any(42);

  EXPECT_THROW(std::ignore = any.template AnyCast<std::string>(),
               typename AnyType::BadCast);
}

TYPED_TEST(AnyTest, AnyCastEmpty) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;

  EXPECT_THROW(std::ignore = any.template AnyCast<int>(),
               typename AnyType::BadCast);
}

/* ==================== ТЕСТЫ ВРЕМЕНИ ЖИЗНИ ==================== */

TYPED_TEST(AnyTest, LifetimeManagement) {
  using AnyType = typename TestFixture::AnyType;

  {
    AnyType any(LifetimeTracker(42));
    EXPECT_EQ(LifetimeTracker::constructor_count, 2);
    EXPECT_EQ(LifetimeTracker::destructor_count, 1);
  }
  EXPECT_EQ(LifetimeTracker::destructor_count, 2);
}

TYPED_TEST(AnyTest, CopyLifetime) {
  using AnyType = typename TestFixture::AnyType;

  LifetimeTracker tracker(42);
  LifetimeTracker::reset();

  {
    AnyType original(tracker);
    EXPECT_EQ(LifetimeTracker::copy_count, 1);

    AnyType copy = original;
    EXPECT_EQ(LifetimeTracker::copy_count, 2);
  }

  EXPECT_EQ(LifetimeTracker::destructor_count, 2);
}

TYPED_TEST(AnyTest, MoveLifetime) {
  using AnyType = typename TestFixture::AnyType;

  LifetimeTracker tracker(42);
  LifetimeTracker::reset();

  {
    AnyType original(std::move(tracker));
    EXPECT_EQ(LifetimeTracker::move_count, 1);

    AnyType moved = std::move(original);
    EXPECT_EQ(LifetimeTracker::copy_count, 0);
  }

  EXPECT_EQ(LifetimeTracker::destructor_count, 1);
}

/* ==================== ТЕСТЫ РАЗНЫХ ТИПОВ ==================== */

TYPED_TEST(AnyTest, WithVector) {
  using AnyType = typename TestFixture::AnyType;

  std::vector<int> vec{1, 2, 3};
  AnyType any(vec);

  auto& stored_vec = any.template AnyCast<std::vector<int>>();
  EXPECT_EQ(stored_vec.size(), 3);
  EXPECT_EQ(stored_vec[0], 1);

  stored_vec.push_back(4);
  EXPECT_EQ(stored_vec.size(), 4);
}

TYPED_TEST(AnyTest, WithPointer) {
  using AnyType = typename TestFixture::AnyType;

  int value = 42;
  AnyType any(&value);

  int* ptr = any.template AnyCast<int*>();
  EXPECT_EQ(ptr, &value);
  EXPECT_EQ(*ptr, 42);
}

TYPED_TEST(AnyTest, WithConstType) {
  using AnyType = typename TestFixture::AnyType;

  const int value = 100;
  AnyType any(value);

  EXPECT_EQ(any.template AnyCast<int>(), 100);
}

/* ==================== ТЕСТЫ ИСКЛЮЧЕНИЙ ==================== */

TYPED_TEST(AnyTest, ExceptionSafetyOnCopy) {
  using AnyType = typename TestFixture::AnyType;

  // Этот тест проверяет, что копирование сохраняет сильную гарантию исключений
  AnyType original(42);

  try {
    AnyType copy = original;
    SUCCEED();  // Если не брошено исключение
  } catch (...) {
    FAIL() << "Copy constructor should not throw for simple types";
  }
}

/* ==================== КОМПЛЕКСНЫЕ СЦЕНАРИИ ==================== */

TYPED_TEST(AnyTest, ReassignDifferentTypes) {
  using AnyType = typename TestFixture::AnyType;

  AnyType any;

  any = 42;
  EXPECT_EQ(any.Type(), typeid(int));
  EXPECT_EQ(any.template AnyCast<int>(), 42);

  any = std::string("hello");
  EXPECT_EQ(any.Type(), typeid(std::string));
  EXPECT_EQ(any.template AnyCast<std::string>(), "hello");

  any = 3.14;
  EXPECT_EQ(any.Type(), typeid(double));
  EXPECT_DOUBLE_EQ(any.template AnyCast<double>(), 3.14);

  any.Reset();
  EXPECT_FALSE(any.HasValue());
}

TYPED_TEST(AnyTest, AnyInContainer) {
  using AnyType = typename TestFixture::AnyType;

  std::vector<AnyType> container;

  container.template emplace_back(42);
  container.template emplace_back(std::string("test"));
  container.template emplace_back(TestStruct(1, "item"));

  EXPECT_EQ(container.size(), 3);
  EXPECT_EQ(container[0].template AnyCast<int>(), 42);
  EXPECT_EQ(container[1].template AnyCast<std::string>(), "test");
  EXPECT_EQ(container[2].template AnyCast<TestStruct>().id, 1);

  std::vector<AnyType> copy = container;
  EXPECT_EQ(copy[0].template AnyCast<int>(), 42);
  EXPECT_EQ(copy[1].template AnyCast<std::string>(), "test");
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}