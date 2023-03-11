#pragma once
#include <cstdint>
#include <type_traits>



namespace df {

enum class state : std::int8_t { clean = false, dirty = true };

template<typename T, typename Object>
concept is_dirtyflag_storage = requires(T t, Object obj, state new_state)
{
    {std::is_constructible_v<T, state> };
    { t.mark(obj)     };
    { t.clear(obj)    };
};

namespace flag_type {

struct bool_flag {
    constexpr bool_flag(state init_state = state::dirty): state_(init_state){}
    template <typename DirtyFlagValue>
    constexpr bool is_dirty(DirtyFlagValue&) const noexcept {
        return static_cast<bool>(state_);
    }
    template <typename DirtyFlagValue>
    constexpr void mark(DirtyFlagValue&) noexcept{
        state_ = state::dirty;
    }
    template <typename DirtyFlagValue>
    constexpr void clear(DirtyFlagValue&) noexcept{
        state_ = state::clean;
    }
private:
    state state_;
};

} // namespace flagtype

template<class T, class FlagStorage = flag_type::bool_flag>
requires is_dirtyflag_storage<FlagStorage, T>
struct dirtyflag  final : private FlagStorage  {

public:
    constexpr dirtyflag(T&& object, state init_state=state::dirty)        noexcept
             : FlagStorage(init_state), object_(std::forward<T>(object)) {}

    constexpr dirtyflag(const dirtyflag & rhs )            noexcept(noexcept(mark()))
             : FlagStorage(rhs)           { mark(); };

    constexpr dirtyflag &operator=(const dirtyflag & rhs ) noexcept(noexcept(mark()))  
             { if(rhs.object_ != this->object_) {
                  mark(); 
                  return FlagStorage::operator=(rhs);
                  }};
    [[nodiscard]] constexpr const T  get() const  noexcept 
                requires ( std::is_pointer_v<T>) {         return object_;}
    [[nodiscard]] constexpr       T  pin()        noexcept(noexcept(mark()))  
                requires ( std::is_pointer_v<T>) { mark(); return object_;}
    [[nodiscard]] constexpr const T& get() const  noexcept
                requires (!std::is_pointer_v<T>) {         return object_;}
    [[nodiscard]] constexpr       T& pin()        noexcept(noexcept(mark()))  
                requires (!std::is_pointer_v<T>) { mark(); return object_;}

    constexpr void mark ()                        noexcept(noexcept(FlagStorage::mark(object_)))
                    { FlagStorage::mark(object_) ;  } 
    constexpr void clear()                        noexcept(noexcept(FlagStorage::clear(object_)))
                    { FlagStorage::clear(object_) ; }
    [[nodiscard]] constexpr bool is_dirty() const noexcept(noexcept(FlagStorage::is_dirty(object_)))
                requires requires(T& dirtyflag_object)
                {  { FlagStorage::is_dirty(dirtyflag_object) } -> std::convertible_to<bool>;}
                    { return FlagStorage::is_dirty(object_); }
private:
    T object_;
};

} // namespace df