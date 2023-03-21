#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace df {

enum class state : std::int8_t { clean = false, dirty = true };

namespace storages {

struct base_storage{
    constexpr void mark () noexcept{}
    constexpr void clear() noexcept{} 
};

class bool_storage {
    state state_;
public:
    constexpr bool_storage(state init_state = state::dirty): state_(init_state){}
    constexpr bool is_dirty() const noexcept {
        return static_cast<bool>(state_);
    }
    constexpr void mark () noexcept{
        state_ = state::dirty;
    }
    constexpr void clear() noexcept{
        state_ = state::clean;
    }  
};

template<auto& LogFuncRef>
requires requires(){LogFuncRef();}
struct logging : base_storage{
    constexpr logging(df::state init_state) noexcept {
        if(init_state == df::state::dirty) LogFuncRef();
    }
    constexpr void mark()  noexcept{ LogFuncRef();}
};


template<auto& StaticStorageRef>
requires requires(size_t i){
    { StaticStorageRef[i] } -> std::convertible_to<df::state>;
}
struct static_storage {
    constexpr static_storage(df::state init_state, size_t index_to_store){
    StaticStorageRef[index_to_store] = init_state;
    }
    constexpr void mark(size_t index) noexcept{
        StaticStorageRef[index] = df::state::dirty;
    }
    constexpr void clear(size_t index) noexcept{
        StaticStorageRef[index] = df::state::clean;
    }
    constexpr bool is_dirty(size_t    index) const noexcept {
        return static_cast<bool>(StaticStorageRef[index]);
    }
};


template<typename DynamicStorage>
requires requires(DynamicStorage& storage, size_t i){
    { storage[i] } -> std::convertible_to<df::state>;
}
struct dynamic_storage {

    constexpr dynamic_storage(df::state init_state, DynamicStorage& storage, size_t index_to_store){
        storage[index_to_store] = init_state;
    }
    constexpr void mark(DynamicStorage& storage, size_t  index)  noexcept{
        storage[index]  = df::state::dirty;
    }
    constexpr void clear(DynamicStorage& storage, size_t  index) noexcept{
        storage[index] = df::state::clean;
    }
    constexpr bool is_dirty(const DynamicStorage& storage, size_t index) const noexcept {
        return static_cast<bool>(storage[index]);
    }
};

template<typename T, uint8_t Mask = alignof(T) - static_cast<uint8_t>(1)>
class tagged_ptr_storage {
    using ptr_type = uintptr_t;
    ptr_type ptr_;
    static_assert((static_cast<uint8_t>(1) & Mask) == 1  && "You shouldn't be using T* that doesn't satisfy alighof(T), because in this case you dont have a free space for store a flag. Minimus 'short' required");
public:
  tagged_ptr_storage(const tagged_ptr_storage &) = delete;
  tagged_ptr_storage &operator=(const tagged_ptr_storage &) = delete;

  tagged_ptr_storage(df::state init_state, T *ptr)
      : ptr_(set_flag(reinterpret_cast<uintptr_t>(ptr),
                      static_cast<bool>(init_state))) {}
  constexpr bool is_dirty() const noexcept {
        return static_cast<bool>(ptr_ & static_cast<uintptr_t>(Mask));
    }
    constexpr T& pin() { mark(); return *get_ptr(ptr_);}
    constexpr void mark () noexcept{
        ptr_ = set_flag(ptr_, true);
    }
    constexpr void clear() noexcept{
        ptr_ = set_flag(ptr_, false);
    }
    constexpr const T& get() const noexcept {
        return *get_ptr(ptr_);
    }
    ~tagged_ptr_storage(){ delete get_ptr(ptr_);}
private:
    static uintptr_t set_flag(const uintptr_t ptr, bool flag){
        return ptr | static_cast<uintptr_t>(static_cast<uint8_t>(flag) & Mask );
    }
    static T* get_ptr(uintptr_t ptr) {return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) & ~Mask);}
};

} // namespace storage


template<typename T> class _object_storage_base;
struct no_object {
    no_object() = default;
};

template<typename T>
struct _object_storage_base{
    _object_storage_base(T&& obj): object_(obj){}
protected:
    T object_;
};

template<> 
struct _object_storage_base<no_object>{
    _object_storage_base(){}
    [[no_unique_address]] no_object object_{};
};
template<typename T>
concept has_pin_func = requires(T& t){{ t.pin() }; };

template<typename T = no_object, typename FlagStorage = storages::bool_storage>
struct dirtyflag  final : public FlagStorage, private _object_storage_base<T>  {

    using Base_ = _object_storage_base<T> ;
public:

    template <typename... Args>
    requires( ! std::is_same_v<T, no_object>)
    constexpr dirtyflag(T &&object, state init_state = state::clean,
                        Args... args) noexcept
        : FlagStorage(init_state, args...), _object_storage_base<T>(std::forward<T>(object)){}

    template <typename... Args>
    requires(std::is_same_v<T, no_object>)
    constexpr dirtyflag(state init_state = state::clean,
                        Args... args) noexcept
        : FlagStorage(init_state, args...), _object_storage_base<T>(){}

    using FlagStorage::mark;
    using FlagStorage::clear;

    [[nodiscard]] constexpr
        const auto&
        get() const noexcept 
        requires requires(){ {Base_::object_} -> std::convertible_to<T>;} 
        {
            if constexpr (requires(){FlagStorage::get();}){
                return FlagStorage::get();
            }
            else return Base_::object_;
        }

    constexpr auto& get()
    requires requires(){{FlagStorage::get()};} && (!requires(){ std::is_pointer_v<decltype(FlagStorage::get())>;})
    {
        return FlagStorage::get();
    }

    using ptr_or_ref_to_obj = typename std::conditional<std::is_pointer_v<T>, T,  T& >::type;
    template<typename ... Args>
    constexpr ptr_or_ref_to_obj pin(Args&... args ) noexcept 
                requires requires ( Args ...args) {{ mark( args... ) }; }
                && (!requires(){{FlagStorage::pin(args...)};})
    {     
        mark(args...);
        assert(FlagStorage::is_dirty(args...) == true && "a 'mark' function of FlagStorage is invalid a flag != df::state::dirty"); 
        return Base_::object_;
    }

    constexpr auto& pin()
    requires requires(){{FlagStorage::pin()};}{
        return FlagStorage::pin();
    }
    template<typename ... Args>
    constexpr auto& pin(Args&... args )
    requires requires(Args&... args ){{FlagStorage::pin(args...)};}{
        return FlagStorage::pin(args...);
    }

    constexpr auto& pin()              noexcept
                  requires requires (T obj) {{ mark( obj ) }; } || requires{{ mark( ) }; }
                  && (!requires(){{FlagStorage::pin()};})
    { 
        if constexpr(requires(){mark(Base_::object_);}){
            mark(Base_::object_);
        }
        else{
            mark();       
        }       
#ifndef DEBUG
    // check result
        constexpr auto error_message = "a 'mark' function of FlagStorage is invalid a flag != df::state::dirty";
        if constexpr(requires(){FlagStorage::is_dirty(Base_::object_);}){
            assert( FlagStorage::is_dirty(Base_::object_) == true && error_message); 
        }
        else if constexpr (requires(){FlagStorage::is_dirty();})  {
            assert( FlagStorage::is_dirty()        == true && error_message); 
        }     
#endif
        return Base_::object_;
        }            
};

} // namespace df