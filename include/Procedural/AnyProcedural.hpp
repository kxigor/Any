#pragma once

// NOLINTBEGIN(cppcoreguidelines-owning-memory)

#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace Any::Procedural {
class Any {
  template <typename ValueType>
  struct VtableTyped;

  struct Vtable {
   private:
    using CloneFuncT = Vtable* (*)(const Vtable* self);
    using TypeInfoFuncT = const std::type_info& (*)();
    using DestroyFuncT = void (*)(Vtable* self);

    using FuncTableT = struct {
      CloneFuncT clone_func{};
      TypeInfoFuncT get_type_info_func{};
      DestroyFuncT destroy_func{};
    };

    template <typename ValueType>
    struct MakeFuncHelper {
      [[nodiscard]] static Vtable* CloneFuncImpl(const Vtable* self) {
        return new VtableTyped<ValueType>{
            static_cast<const VtableTyped<ValueType>*>(self)->GetValue()};
      }

      [[nodiscard]] static const std::type_info& TypeInfoFuncImpl() {
        return typeid(ValueType);
      };

      static void DestroyFuncImpl(Vtable* self) {
        delete static_cast<VtableTyped<ValueType>*>(self);
      }
    };

    template <typename ValueType>
    static FuncTableT MakeFuncTable() {
      using MakeFuncHelperTyped = MakeFuncHelper<ValueType>;
      return {.clone_func = &MakeFuncHelperTyped::CloneFuncImpl,
              .get_type_info_func = &MakeFuncHelperTyped::TypeInfoFuncImpl,
              .destroy_func = &MakeFuncHelperTyped::DestroyFuncImpl};
    }

   public:
    template <typename ValueType>
    explicit Vtable(std::type_identity<ValueType> /*unused*/)
        : func_table_(MakeFuncTable<ValueType>()) {}

    [[nodiscard]] Vtable* Clone() const { return func_table_.clone_func(this); }

    [[nodiscard]] const std::type_info& GetTypeInfo() const {
      return func_table_.get_type_info_func();
    }

    void Destroy() { func_table_.destroy_func(this); }

   private:
    FuncTableT func_table_;
  };

  template <typename ValueType>
  struct VtableTyped : Vtable {
    /*==== Member functions =====*/
    template <typename... Args>
    VtableTyped(Args&&... args)
        : Vtable(std::type_identity<ValueType>{}),
          value_(std::forward<Args>(args)...) {}

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

  ~Any() { DestroyVtable(vtable_); }

  Any(const Any& other) : vtable_(other.CloneVtable()) {}

  Any(Any&& other) noexcept : vtable_(other.vtable_) {
    other.vtable_ = nullptr;
  }

  template <typename ValueType>
  requires(not std::is_same_v<std::decay_t<ValueType>, Any>)
  Any(ValueType&& value)
      : vtable_(new VtableTyped<std::decay_t<ValueType>>(
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
    Vtable* safe_copy = vtable_;
    vtable_ =
        new VtableTyped<std::decay_t<ValueType>>(std::forward<Args>(args)...);
    DestroyVtable(safe_copy);

    return AnyCast<std::decay_t<ValueType>>();
  }

  void Reset() noexcept {
    DestroyVtable(vtable_);
    NullifyVtable();
  }

  void Swap(Any& other) noexcept { std::swap(vtable_, other.vtable_); }

  /*======================= Observers ========================*/
  [[nodiscard]] bool HasValue() const noexcept { return vtable_ != nullptr; }

  [[nodiscard]] const std::type_info& Type() const noexcept {
    if (not HasValue()) {
      return typeid(void);
    }
    return vtable_->GetTypeInfo();
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
    if (vtable_ == nullptr or vtable_->GetTypeInfo() != typeid(ValueType)) {
      throw BadCast{};
    }
    auto* typed_vtable = static_cast<VtableTyped<ValueType>*>(vtable_);
    return typed_vtable->GetValue();
  }

  [[nodiscard]] Vtable* CloneVtable() const {
    if (not HasValue()) {
      return nullptr;
    }
    return vtable_->Clone();
  }

  static void DestroyVtable(Vtable* vtable) noexcept {
    if (vtable != nullptr) {
      vtable->Destroy();
    }
  }

  void NullifyVtable() noexcept { vtable_ = nullptr; }

  /*========================= Fields =========================*/
  Vtable* vtable_{};
};
}  // namespace Any::Procedural

// NOLINTEND(cppcoreguidelines-owning-memory)