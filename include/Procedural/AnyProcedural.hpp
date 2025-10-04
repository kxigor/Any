#pragma once

#include <memory>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace Any::Procedural {
class Any {
  struct Vtable {
   private:
    using CloneFuncT = Vtable* (*)(const Vtable* self);
    using TypeInfoFuncT = const std::type_info& (*)();
    using DestroyFuncT = void (*)(const Vtable* self);

    using FuncTableT = struct {
      CloneFuncT clone_func_{};
      TypeInfoFuncT get_type_info_func_{};
      DestroyFuncT destroy_func_{};
    };

    template <typename T>
    struct MakeFuncHelper {
      static Vtable* CloneFuncImpl(const Vtable* self) {
        return new T{static_cast<VtableTyped<T>*>(self)->value_};
      }

      static const std::type_info& TypeInfoFuncImpl() { return typeid(T); };

      static void DestroyFuncImpl(const Vtable* self) {
        delete static_cast<VtableTyped<T>*>(self);
      }
    };

    template <typename T>
    static FuncTableT MakeFuncTable() {
      using MakeFuncHelperTyped = MakeFuncHelper<T>;
      return {
          .clone_func_ = &MakeFuncHelperTyped::CloneFuncImpl,
          .get_type_info_func_ = &MakeFuncHelperTyped::TypeInfoFuncImpl,
          .destroy_func_ = &MakeFuncHelperTyped::DestroyFuncImpl,
      };
    }

   public:
    template <typename T>
    Vtable(std::type_identity<T> /*unused*/)
        : func_table_(MakeFuncTable<T>()) {}

    Vtable* Clone() { return func_table_.clone_func_(this); }

    const std::type_info& GetTypeInfo() {
      return func_table_.get_type_info_func_();
    }

    void Destroy() { return func_table_.destroy_func_(this); }

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

    /*========= Fields ==========*/
    ValueType value_;
  };

 public:
  struct BadCast : public std::bad_cast {
    using std::bad_cast::bad_cast;
  };

  /*==================== Member functions ====================*/
  Any() = default;

  ~Any() { vtable_->Destroy(); }

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
    safe_copy->Destroy();

    return AnyCast<std::decay_t<ValueType>>();
  }

  void Reset() noexcept {
    DestroyVtable();
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
    auto* typed_vtable = dynamic_cast<VtableTyped<ValueType>*>(vtable_);
    if (typed_vtable == nullptr) {
      throw BadCast{};
    }
    return typed_vtable->value_;
  }

  Vtable* CloneVtable() const {
    if (not HasValue()) {
      return nullptr;
    }
    return vtable_->Clone();
  }

  void DestroyVtable() noexcept { vtable_->Destroy(); }

  void NullifyVtable() noexcept { vtable_ = nullptr; }

  /*========================= Fields =========================*/
  Vtable* vtable_{};
};
}  // namespace Any::Procedural