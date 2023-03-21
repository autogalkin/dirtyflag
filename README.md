# dirtyflag

## Table of Contents

- [Introduction](#intro)
- [About](#about)
- [Details](#details)
- [Usage](#usage)

Dirty flags are a useful tool for indicating when data has been modified. By using dirty flags, we can avoid the costly process of re-acquiring resources unnecessarily. In my pet project game, I utilized dirty flags to selectively rerender only those lines of text that had changed, as well as to update dynamic objects in real time. This technique proved to be highly effective and helped to optimize the overall performance of the game.

## Introduction <a name = "intro"></a>
Dirty flags are a useful tool for indicating when data has been modified. By using dirty flags, we can avoid the costly process of re-acquiring resources unnecessarily. In my pet project [game](https://github.com/autogalkin/notepadgame), I utilized dirty flags to selectively rerender only those lines of text that had changed by dynamic objects.

## About <a name = "about"></a>

I attempted to implement a C++ dirty flag without any memory overhead, and I tried several different methods that you'll see in [details](#details). This project consists of a single .hpp file that can be easily included in other projects.

## Details <a name = "details"></a>
I implemented a some flag storages to experiments how can use a bool flag within a structure:
- a simple bool flag (with memory overhead)
```cpp
df::dirtyflag<char, df::storages::bool_storage> bool_flag{'b'}; 
std::cout << "size of" << sizeof(bool_flag) << '\n'; // -> 2
assert(bool_flag.is_dirty() == false);
const char& value = bool_flag.get(); // get the const value
assert(bool_flag.is_dirty() == false);
bool_flag.pin() = 'f';              // get the non const ref of the object
assert(bool_flag.is_dirty() == true);

```
- A simple storage which has not a flag but calls a custom functor every time when we want to get a non const value
```cpp
static auto log = []{
            std::cout << "hello world from the logging storage!" << std::endl;
        };
df::dirtyflag<df::no_object_t, df::storages::logging<log>> log_flag{};
assert(sizeof(log_flag) == sizeof(df::storages::logging<log>));
log_flag.pin(); // -> Hello from the logging storage!
```



- To manage multiple dirty flags efficiently, I created a storage with an external static container. This approach helps to maintain the original size of the data and enables us to store the flags as a packed array of bits, minimizing memory usage:
```cpp
static std::vector<df::state> static_storage_{1}; // or boost::dynamic_bitset, std::bitset
size_t index_to_store = 0;
df::dirtyflag<char, df::storages::static_storage< static_storage_> > ststorage_flag{'a', /* start state*/df::state::clean, index_to_store};
assert(sizeof(ststorage_flag) == sizeof(char));
ststorage_flag.get(); // -> const & 'a'
ststorage_flag.pin(index_to_store) = 'q';
assert(ststorage_flag.is_dirty(index_to_store) == true);
    
```

- A dynamic storage can use a non static array as a flag storage. But we should pass a reference of this array every time to keep size of our data without store a pointer to a container
```cpp
std::vector<df::state> dynamic_storage_{1};
size_t index_to_store = 0;
df::dirtyflag<char, df::storages::dynamic_storage<  decltype(dynamic_storage_)> > dyn_storage{'g', df::state::clean, dynamic_storage_, index_to_store};
assert(sizeof(dyn_storage) == sizeof(char));
dyn_storage.pin(dynamic_storage_, index_to_store) = 't';
assert(dyn_storage.is_dirty(dynamic_storage_, index_to_store) == true);
```

- A tagged pointer storage is a method that utilizes a [tagged pointer]( https://en.wikipedia.org/wiki/Tagged_pointer) to store a flag in a free bit of the pointer to our custom data. To implement this approach, you can use a library such as https://github.com/marzer/tagged_ptr or another one that provides more details and ensures a cross-platform compatibility. However, it is important to note that this method may be unsafe for certain platforms or future use cases. Therefore, it's important to consider the potential risks before implementing this approach.

```cpp
df::dirtyflag<df::no_object_t, df::storages::tagged_ptr_storage<int>> ptr_storage{df::state::clean, new int{5}};
assert(sizeof(ptr_storage) == sizeof(int*));
ptr_storage.pin() = 3;
assert(ptr_storage.is_dirty() == true);
```
## Usage <a name = "usage"></a>
To implement a flag storage needs this two main methods(I not use a virtual table): 
```cpp
struct base_storage{
    // a mark method should use to change a flag value to true
    constexpr void mark(){}
    // a clear method should use to change a flag to false
    constexpr void clear(){} 
};
```
And then you can use it in dirty_flag< storage >. The template class 'dirtyflag' used as a simple template wrapper around many storages and help to keep unified api. 

