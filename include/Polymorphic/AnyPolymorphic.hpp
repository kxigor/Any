#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace Any::Polymorphic {
class Any {
  /*======================== Helpers =========================*/
  struct AnyBase {
    /*==== Member functions =====*/
    virtual AnyBase* Clone() = 0;
    virtual const std::type_info& GetTypeInfo() const = 0;
    virtual ~AnyBase() = default;
  };

  template <typename T>
  struct AnyTyped : AnyBase {
    template <typename... Args>
    /*==== Member functions =====*/
    AnyTyped(Args&&... args) : value_(std::forward<Args>(args)...) {}

    AnyTyped* Clone() override { return new AnyTyped(value_); }
    const std::type_info& GetTypeInfo() const { return typeid(T); }
    ~AnyTyped() override = default;

    /*========= Fields ==========*/
    T value_;
  };

 public:
  struct BadCast : public std::bad_cast {
    using std::bad_cast::bad_cast;
  };

  /*==================== Member functions ====================*/
  Any() = default;

  ~Any() { delete base_; }

  Any(const Any& other) : base_(other.CloneBase()) {}

  Any(Any&& other) noexcept : base_(other.base_) { other.base_ = nullptr; }

  template <typename ValueType>
  requires(not std::is_same_v<std::decay_t<ValueType>, Any>)
  Any(ValueType&& value)
      : base_(new AnyTyped<std::decay_t<ValueType>>(
            std::forward<ValueType>(value))) {}

  Any& operator=(const Any& other) {
    if (this == std::addressof(other)) {
      return *this;
    }

    Any temp(other);
    this->Swap(temp);

    return *this;
  }

  Any& operator=(Any&& other) noexcept {
    if (this == std::addressof(other)) {
      return *this;
    }

    Any temp(std::move(other));
    this->Swap(temp);

    return *this;
  }

  /*======================= Modifiers ========================*/
  template <typename ValueType, typename... Args>
  std::decay_t<ValueType>& Emplace(Args&&... args) {
    AnyBase* safe_copy = base_;
    base_ = new AnyTyped<std::decay_t<ValueType>>(std::forward<Args>(args)...);
    delete safe_copy;

    return AnyCast<std::decay_t<ValueType>>();
  }

  void Reset() noexcept {
    delete base_;
    base_ = nullptr;
  }

  void Swap(Any& other) noexcept { std::swap(base_, other.base_); }

  /*======================= Observers ========================*/
  [[nodiscard]] bool HasValue() const noexcept { return base_ != nullptr; }

  [[nodiscard]] const std::type_info& Type() const noexcept {
    if (not HasValue()) {
      return typeid(void);
    }
    return base_->GetTypeInfo();
  }

  template <typename ValueType>
  [[nodiscard]] ValueType& AnyCast() {
    return AnyCastImpl<ValueType>();
  }

  template <typename ValueType>
  [[nodiscard]] const ValueType& AnyCast() const {
    return AnyCastImpl<ValueType>();
  }

 private:
  /*========================= Impls ==========================*/
  template <typename ValueType>
  [[nodiscard]] ValueType& AnyCastImpl() const {
    auto* typed_base = dynamic_cast<AnyTyped<ValueType>*>(base_);
    if (typed_base == nullptr) {
      throw BadCast{};
    }
    return dynamic_cast<AnyTyped<ValueType>*>(base_)->value_;
  }

  AnyBase* CloneBase() const {
    if (not HasValue()) {
      return nullptr;
    }
    return base_->Clone();
  }

  /*========================= Fields =========================*/
  AnyBase* base_{};
};
}  // namespace Any::Polymorphic