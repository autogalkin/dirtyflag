#pragma once
#include <cstdint>
#include <iostream>
#include <type_traits>



namespace df {

enum class state : std::int8_t { clean = 0, dirty = 1 };

namespace flag_type {

struct default_storage {
    template <typename DirtyFlagValue>
    constexpr bool is_dirty(DirtyFlagValue&) const noexcept {
        return static_cast<bool>(state_);
    }
    template <typename DirtyFlagValue>
    constexpr void set_state(DirtyFlagValue&, state new_state) noexcept{
        state_ = new_state;
    }
    state state_ = state::dirty;
};

struct logging
{
    template <typename DirtyFlagValue>
    constexpr bool is_dirty(DirtyFlagValue&) const noexcept {
        return false;
    }
    template <typename DirtyFlagValue>
    constexpr void set_state(DirtyFlagValue& v,  state new_state) noexcept{
        if(new_state == state::dirty) log(v);
    }
    template <typename DirtyFlagValue>
    void log(DirtyFlagValue& v)
    {
        std::cout << "logging " << v;
    }

};

} // namespace flagtype


#pragma region internal
namespace _internal {

template<class T, class LogFunc, class FlagStorage = flag_type::default_storage>
struct _dirty_flag_base : _dirty_flag_base<T, void> {
    
    using Base = _dirty_flag_base<T, void>;
    constexpr explicit _dirty_flag_base(T object, const LogFunc& data, state start_state=state::dirty)
        : Base(object, start_state), logger_(data) {}
protected:
    LogFunc logger_;
};

template<class T, class FlagStorage>
struct _dirty_flag_base<T, void, FlagStorage > {
    
    constexpr explicit _dirty_flag_base(T object, state start_state=state::dirty) 
        : object_(object){}
protected:
    constexpr void logger_() const noexcept {} // empty stub
    T object_;
    mutable FlagStorage flag_;
    
};
} // namespace _internal

#pragma endregion internal

template<class T, class FlagStorage = flag_type::default_storage, class Log = void>
struct dirtyflag  final : private _internal::_dirty_flag_base<T, Log, FlagStorage> {
    using Base                  = _internal::_dirty_flag_base<T, Log, FlagStorage >;
    using Base::Base;
private:
public:
    constexpr dirtyflag(const dirtyflag & rhs )           noexcept(noexcept(logger_())) 
             : Base(rhs)           { mark(); };

    constexpr dirtyflag &operator=(const dirtyflag & rhs ) noexcept(noexcept(logger_()))  
             { if(rhs.object_ != this->object_) {
                  mark(); 
                  return Base::operator=(rhs);
                  }};

    [[nodiscard]] constexpr const T  get() const noexcept 
                requires ( std::is_pointer_v<T>) {         return object_;}
    [[nodiscard]] constexpr       T  pin()       noexcept 
                requires ( std::is_pointer_v<T>) { mark(); return object_;}
    [[nodiscard]] constexpr const T& get() const noexcept(noexcept(logger_())) 
                requires (!std::is_pointer_v<T>) {         return object_;}
    [[nodiscard]] constexpr       T& pin()       noexcept(noexcept(logger_()))  
                requires (!std::is_pointer_v<T>) { mark(); return object_;}

    constexpr void mark () noexcept(noexcept(logger_()))   { logger_(); flag_.set_state(object_, state::dirty) ; } 
    constexpr void clear() noexcept { flag_.set_state(object_, state::clean) ; }
    [[nodiscard]] constexpr bool is_dirty() const noexcept { return flag_.is_dirty(object_); }
private:
    using Base::object_;
    using Base::logger_;
    using Base::flag_;
}
#ifdef DF_PACK
__attribute__((__packed__))
#endif
;

} // namespace df