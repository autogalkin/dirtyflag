#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <cassert>

namespace df {

enum class state : std::int8_t { clean = false, dirty = true };

namespace storages {

// this is a base class with empty methods to be compatible to dirtyflag<storage>
struct base_storage{
    // a mark method should use to change a flag value to true
    constexpr void mark () noexcept{}
    // a clear method should use to change a flag to false
    constexpr void clear() noexcept{} 
};

// a simple storage that aligns to x2 size because stores a bool member
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

// a simple storage which has not a flag but calls a custom functor every time when
// we get a non const value
template<auto& LogFuncRef>
requires requires(){LogFuncRef();}
struct logging : base_storage{
    constexpr logging(df::state init_state) noexcept {
        if(init_state == df::state::dirty) LogFuncRef();
    }
    constexpr void mark()  noexcept{ LogFuncRef();}
};

/*
To manage multiple dirty flags efficiently, I created a storage with an external static container.
This approach helps to maintain the original size of the data and enables us to
store the flags as a packed array of bits, minimizing memory usage
*/
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
/*
A dynamic storage can use a non static array as a flag storage. 
But we should pass a reference of this array every time
to keep size of our data without store a pointer to container
*/
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
/*
A tagged pointer storage is a method that utilizes a tagged pointer to store
a flag in a free bit of the pointer to our custom data. 
To implement this approach, you can use a library such 
as https://github.com/marzer/tagged_ptr or another one that provides more details and ensures 
a cross-platform compatibility. However, it is important to note that this method may be 
unsafe for certain platforms or future use cases. Therefore, it's important to consider the 
potential risks before implementing this approach.
*/
template<typename T, uint8_t Mask = alignof(T) - static_cast<uint8_t>(1)>
class tagged_ptr_storage {
    uintptr_t ptr_;
    static_assert((static_cast<uint8_t>(1) & Mask) == 1  && "You shouldn't be using T* that doesn't satisfy alighof(T), because in this case you dont have a free space for store a flag. Minimum 'short' required");
public:
    tagged_ptr_storage(const tagged_ptr_storage &)            = delete;
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

} // namespace storages


// an empty object that indicate to specialize a dirtyflag without object member
struct no_object_t {
    no_object_t() = default;
};

namespace __details {
// the base of dirtyflag with the object member
template<typename T>
struct _object_storage_base{
    _object_storage_base(T&& obj): object_(obj){}
protected:
    T object_;
};
// the base of dirtyflag without the object member
template<> 
struct _object_storage_base<no_object_t>{
    _object_storage_base(){}
};

template<typename T>
concept is_noobject = std::is_same_v<T, no_object_t>;

// a few concepts to avoid a virtual table and check existing methods in the compile time
template<typename T, typename ...Args>
concept has_get_func      = requires(T& t, Args... args) {{ t.get(args...) };};

template<typename T, typename ...Args>
concept has_pin_func      = requires(T& t, Args... args) {{ t.pin(args...) };};

template<typename T, typename ...Args>
concept has_mark_func     = requires(T& t, Args... args) {{ t.mark(args...) };};

template<typename T, typename ...Args>
concept has_is_dirty_func = requires(T& t, Args... args) {{ t.is_dirty(args...) };};

}

// a main wrapper for many flag storages that provides a standart api to use the dirty flag pattern
template<typename T = no_object_t, typename FlagStorage = storages::bool_storage>
struct dirtyflag  final : private FlagStorage, private __details::_object_storage_base<T>  {

    using Base_ = __details::_object_storage_base<T> ;
public:

    template <typename... Args>
    requires(not __details::is_noobject<T>)
    constexpr dirtyflag(T &&object, state init_state = state::clean, Args... args) noexcept
        : FlagStorage(init_state, args...), Base_ (std::forward<T>(object)){}

    // constructor for df::no_object
    template <typename... Args>
    requires __details::is_noobject<T>
    constexpr dirtyflag(state init_state = state::clean, Args... args) noexcept
        : FlagStorage(init_state, args...), Base_(){}

    // flag storage must implements this methods
    using FlagStorage::mark;
    using FlagStorage::clear;

    // return a const ref to object which we can't change
    [[nodiscard]] constexpr const auto& get() const noexcept
    requires (not __details::is_noobject<T>)  {
            return Base_::object_;
    }

    // if FlagStorage provides own get or pin functions
    ///////////////////////////////////////////
    [[nodiscard]] constexpr const auto& get() const noexcept
    requires __details::has_get_func<FlagStorage>{
        return FlagStorage::get();
    }
    template<typename ... Args>
    constexpr auto& pin(Args&... args ) noexcept
    requires __details::has_pin_func<FlagStorage, Args...>{
        return  FlagStorage::pin(args...);
    }
    ////////////////////////////////////////////

    // the pin method changes a flag and returns a non const ref to object
    template<typename ... Args>
    [[nodiscard]] constexpr typename std::conditional<std::is_pointer_v<T>, T,  T& >::type
    pin(Args&... args ) noexcept 
    requires __details::has_mark_func<FlagStorage, Args...> 
    && (not __details::has_pin_func<FlagStorage>) && (not __details::is_noobject<T>){     
        mark(args...);
        return Base_::object_;
    }

    // specialization for df::no_object
    template<typename ... Args>
    constexpr void pin(Args&... args ) noexcept 
    requires __details::has_mark_func<FlagStorage, Args...> 
    && (not __details::has_pin_func<FlagStorage>) 
    && __details::is_noobject<T> {
        mark(args...);
    }

    // the object of dirty flag have been changed or not
    template<typename ... Args>
    constexpr bool is_dirty(Args&... args) const noexcept
    requires __details::has_is_dirty_func<FlagStorage, Args...> {
        return FlagStorage::is_dirty(args...) ;
    }
          
};


} // namespace df