#pragma once

#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace Any::Polymorphic {
class Any {
  /*======================== Helpers =========================*/
  struct AnyBase;

  using AnyBasePtr = std::unique_ptr<AnyBase>;

  template <typename AnyTypedType, typename... Args>
  static AnyBasePtr MakeAnyBasePtr(Args&&... args) {
    return std::make_unique<AnyTypedType>(std::forward<Args>(args)...);
  }

  struct AnyBase {
    /*==== Member functions =====*/
    AnyBase() = default;

    AnyBase(const AnyBase& /*unused*/) = delete;
    AnyBase(AnyBase&& /*unused*/) = delete;

    AnyBase& operator=(const AnyBase& /*unused*/) = delete;
    AnyBase& operator=(AnyBase&& /*unused*/) = delete;

    virtual ~AnyBase() = default;

    [[nodiscard]] virtual AnyBasePtr Clone() = 0;
    [[nodiscard]] virtual const std::type_info& GetTypeInfo() const = 0;
  };

  template <typename ValueType>
  struct AnyTyped : AnyBase {
    /*==== Member functions =====*/
    AnyTyped() = default;

    template <typename... Args>
    AnyTyped(Args&&... args) : value_(std::forward<Args>(args)...) {}

    AnyTyped(const AnyTyped& /*unused*/) = delete;
    AnyTyped(AnyTyped&& /*unused*/) = delete;

    AnyTyped& operator=(const AnyTyped& /*unused*/) = delete;
    AnyTyped& operator=(AnyTyped&& /*unused*/) = delete;

    ~AnyTyped() override = default;

    [[nodiscard]] AnyBasePtr Clone() override {
      return MakeAnyBasePtr<AnyTyped>(value_);
    }

    [[nodiscard]] const std::type_info& GetTypeInfo() const override {
      return typeid(ValueType);
    }

    [[nodiscard]] ValueType& GetValue() noexcept { return value_; }
    [[nodiscard]] const ValueType& GetValue() const noexcept { return value_; }

    /*========= Fields ==========*/
   private:
    ValueType value_;
  };

 public:
  struct BadCast : public std::bad_cast {
    using std::bad_cast::bad_cast;
  };

  /*==================== Member functions ====================*/
  Any() = default;
  ~Any() = default;

  Any(const Any& other) : base_(other.CloneBase()) {}

  Any(Any&& other) noexcept : base_(std::move(other.base_)) {}

  template <typename ValueType>
  requires(not std::is_same_v<std::decay_t<ValueType>, Any>)
  Any(ValueType&& value)
      : base_(MakeAnyBasePtr<AnyTyped<std::decay_t<ValueType>>>(
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
    base_ = MakeAnyBasePtr<AnyTyped<std::decay_t<ValueType>>>(
        std::forward<Args>(args)...);
    return AnyCast<std::decay_t<ValueType>>();
  }

  void Reset() noexcept { base_.reset(); }

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
    if (not HasValue() or typeid(ValueType) != Type()) {
      throw BadCast{};
    }
    return static_cast<AnyTyped<ValueType>*>(base_.get())->GetValue();
  }

  [[nodiscard]] AnyBasePtr CloneBase() const {
    if (not HasValue()) {
      return nullptr;
    }
    return base_->Clone();
  }

  /*========================= Fields =========================*/
  AnyBasePtr base_;
};
}  // namespace Any::Polymorphic