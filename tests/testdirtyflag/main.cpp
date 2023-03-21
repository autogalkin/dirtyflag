

#include <iostream>
#include <vector>
#include <cassert>
#include <inttypes.h>
#include "df/dirtyflag.h"

struct test_function_specialization : df::storages::base_storage{
   test_function_specialization(df::state init_state){
    }
    template<typename Object>
    void mark(Object obj) noexcept{
        std::cout << "Hello World" << obj << std::endl;
   }
    void mark() noexcept{ // will newer call if specialization ~^ for dirty flag object exists
        std::cout << "newer calls " << std::endl;
    }
};

int main()
{
    {
        std::cout << "default bool flag storage: \n";
        df::dirtyflag<char, df::storages::bool_storage> bool_flag{'b'}; 
        std::cout << "size of a char dirty flag: " << sizeof(bool_flag) << '\n';
        assert(bool_flag.is_dirty() == false);
        std::cout << "start value: '" << bool_flag.get() << "' start dirty:" << std::boolalpha << bool_flag.is_dirty() << '\n';
        const char& value = bool_flag.get(); // get const value
        assert(bool_flag.is_dirty() == false);
        bool_flag.pin() = 'f';              // get non const ref
        assert(bool_flag.is_dirty() == true);
        std::cout << "after pin() the value is: '" << bool_flag.get()  << "' is dirty?- " << std::boolalpha << bool_flag.is_dirty() << "\n\n";
    }

    {
        std::cout << "logging: \n";
        static auto log = []{
            std::cout << "hello world from the logging storage!" << std::endl;
        };
        df::dirtyflag<df::no_object_t, df::storages::logging<log>> log_flag{};
        assert(sizeof(log_flag) == sizeof(df::storages::logging<log>));
        std::cout << "size " << sizeof(log_flag) << '\n';
        std::cout << "pin(): ";
        log_flag.pin();
        std::cout << "\n";
    }

    {
        std::cout << "static storage: \n";
        static std::vector<df::state> static_storage_{1}; // or boost::dynamic_bitset, std::bitset
        size_t index_to_store = 0;
        df::dirtyflag<char, df::storages::static_storage< static_storage_> > ststorage_flag{'a', df::state::clean, index_to_store};
        assert(sizeof(ststorage_flag) == sizeof(char));
        std::cout << "size " << sizeof(ststorage_flag) << '\n';
        std::cout << "start value: '" << ststorage_flag.get() << "' start dirty:" << std::boolalpha << ststorage_flag.is_dirty(index_to_store) << '\n';
        std::cout << "pin(): ";
        ststorage_flag.pin(index_to_store) = 'q';
        assert(ststorage_flag.is_dirty(index_to_store) == true);
        std::cout << "after pin() the value is: '" << ststorage_flag.get()  << "' is dirty?- " << std::boolalpha << ststorage_flag.is_dirty(index_to_store) << "\n\n";
    }
    {
        std::cout << "dynamic storage: \n";
        std::vector<df::state> dynamic_storage_{1};
        dynamic_storage_.push_back(df::state::clean);
        size_t index_to_store = 0;
        df::dirtyflag<char, df::storages::dynamic_storage<  decltype(dynamic_storage_)> > dyn_storage{'g', df::state::clean, dynamic_storage_, index_to_store};
        assert(sizeof(dyn_storage) == sizeof(char));
        std::cout << "size " << sizeof(dyn_storage) << '\n';
        std::cout << "start value: '" << dyn_storage.get() << "' start dirty:" << std::boolalpha << dyn_storage.is_dirty(dynamic_storage_, index_to_store) << '\n';
        std::cout << "pin(): ";
        dyn_storage.pin(dynamic_storage_, index_to_store) = 't';
        assert(dyn_storage.is_dirty(dynamic_storage_, index_to_store) == true);
        std::cout << "after pin() the value is: '" << dyn_storage.get()  << "' is dirty?- " << std::boolalpha << dyn_storage.is_dirty(dynamic_storage_, index_to_store) << "\n\n";
    }
    {
        std::cout << "tagged ptr storage: \n";
        df::dirtyflag<df::no_object_t, df::storages::tagged_ptr_storage<int>> ptr_storage{df::state::clean, new int{5}}; // minimum short available
        std::cout << "size " << sizeof(ptr_storage) << '\n';
        assert(sizeof(ptr_storage) == sizeof(int*));
        std::cout << "start value: '" << ptr_storage.get() << "' start dirty:" << std::boolalpha << ptr_storage.is_dirty() << '\n';
        std::cout << "pin(): ";
        ptr_storage.pin() = 3;
        std::cout << "after pin() the value is: '" << ptr_storage.get()  << "' is dirty?- " << std::boolalpha << ptr_storage.is_dirty() << "\n\n";
    }
    
    return 0;
}
