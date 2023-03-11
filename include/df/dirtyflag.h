#pragma once
#include <cstdint>



namespace df {

enum class state : std::int8_t { clean = 0, dirty = 1 };

namespace _internal {

template<class T, class Log>
struct _dirty_flag_base : _dirty_flag_base<T, void> {
    
    using Base = _dirty_flag_base<T, void>;
    explicit _dirty_flag_base(T object, const Log& data, state start_state=state::dirty)
        : Base(object, start_state), logger_(data) {}
protected:
    Log logger_;

};

template<class T>
struct _dirty_flag_base<T, void> {
    
    explicit _dirty_flag_base(T object, state start_state=state::dirty) 
        : object_(object), flag_(start_state) {}
protected:
    void logger_() const noexcept {} // empty
    T object_;
    mutable state flag_;
};
} // namespace _internal

template<class T, class Log = void>
struct dirtyflag  final : private _internal::_dirty_flag_base<T, Log> {
    using Base                  = _internal::_dirty_flag_base<T, Log>;
    using Base::Base;
private:
public:
    constexpr dirtyflag(const dirtyflag & rhs )           noexcept(noexcept(logger_())) 
             : Base(rhs)           { mark(); };

    constexpr dirtyflag(dirtyflag && rhs )     noexcept 
             : Base(std::move(rhs)){};

    constexpr dirtyflag &operator=(const dirtyflag & rhs ) noexcept(noexcept(logger_()))  
             { if(rhs.object_ != this->object_) {
                  mark(); 
                  return Base::operator=(rhs);
                  }};

    constexpr dirtyflag &operator=(dirtyflag && rhs )      noexcept(noexcept(logger_())) 
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

    constexpr void mark () noexcept(noexcept(logger_()))   { logger_(); flag_ = state::dirty ; } 
    constexpr void clear() noexcept { flag_ = state::clean ; }
    [[nodiscard]] constexpr bool is_dirty() const noexcept { return static_cast<bool>(flag_); }
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