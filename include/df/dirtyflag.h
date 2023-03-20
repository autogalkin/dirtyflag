#pragma once
#include <concepts>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace df {

enum class state : std::int8_t { clean = false, dirty = true };

namespace storages {

struct base_storage{};

class bool_storage {
    state state_;
public:
    constexpr bool_storage(state init_state = state::dirty): state_(init_state){}
    constexpr bool is_dirty() const noexcept {
        return static_cast<bool>(state_);
    }
    constexpr void mark ()          noexcept{
        state_ = state::dirty;
    }
    constexpr void clear()          noexcept{
        state_ = state::clean;
    }  
};

template<auto& LogFuncPtr>
requires requires(){LogFuncPtr();}
struct logging{
    logging(df::state init_state){
        if(init_state == df::state::dirty) LogFuncPtr();
    }
    void mark() noexcept{ LogFuncPtr();}
    constexpr void clear() noexcept{}
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
    constexpr void mark(DynamicStorage& storage, size_t  index) noexcept{
        storage[index]  = df::state::dirty;
    }
    constexpr void clear(DynamicStorage& storage, size_t  index) noexcept{
        storage[index] = df::state::clean;
    }
    constexpr bool is_dirty(const DynamicStorage& storage, size_t index) const noexcept {
        return static_cast<bool>(storage[index]);
    }
};

} // namespace storage



template<class T, class FlagStorage = storages::bool_storage>
struct dirtyflag  final : public FlagStorage  {

    T object_;
    
public:

    template <typename... Args>
    constexpr dirtyflag(T &&object, state init_state = state::clean,
                        Args... args) noexcept
        : FlagStorage(init_state, args...), object_(std::forward<T>(object)) {}

    using FlagStorage::mark;
    using FlagStorage::clear;

    [[nodiscard]] constexpr
        typename std::conditional<std::is_trivial_v<T>, T, const T &>::type
        get() const noexcept {
            return object_; }

    using ptr_or_ref_to_obj = typename std::conditional<std::is_pointer_v<T>, T,  T& >::type;
    template<typename ... Args>
    
    [[nodiscard]] constexpr       ptr_or_ref_to_obj pin(Args&... args ) noexcept 
                requires requires ( Args ...args) {{ mark( args... ) }; }
    {   
        mark(args...);
        assert(FlagStorage::is_dirty(args...) == true && "a 'mark' function of FlagStorage is invalid a flag != df::state::dirty"); 
        return object_;
    }

    [[nodiscard]] constexpr       ptr_or_ref_to_obj pin()              noexcept
                  requires requires (T obj) {{ mark( obj ) }; } || requires{{ mark( ) }; }
    { 
        if constexpr(requires(){mark(object_);}){
            mark(object_);
        }
        else{
            mark();       
        }       
#ifndef DEBUG
        // check result
        constexpr auto error_message = "a 'mark' function of FlagStorage is invalid a flag != df::state::dirty";
        if constexpr(requires(){FlagStorage::is_dirty(object_);}){
            assert( FlagStorage::is_dirty(object_) == true && error_message); 
        }
        else if constexpr (requires(){FlagStorage::is_dirty();})  {
            assert( FlagStorage::is_dirty()        == true && error_message); 
        }     
#endif
        return object_;
    }              
};

} // namespace df