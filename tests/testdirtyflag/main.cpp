#include <atomic>
#include <concepts>
#include <ios>
#include <iostream>
#include "df/dirtyflag.h"
#include <type_traits>
#include <vector>
#include <cassert>

struct test_function_specialization{
   test_function_specialization(df::state init_state){
    }
    template<typename Object>
    void mark(Object obj) noexcept{
        std::cout << "Hello World" << obj << std::endl;
   }
    void mark() noexcept{ // will newer call if specialization for dirty flag object exists
        std::cout << "newer calls " << std::endl;
    }
    constexpr void clear() noexcept{
    }
    constexpr bool is_dirty(size_t index) const noexcept { return false; }
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
        df::dirtyflag<char, df::storages::logging<log>> log_flag{'l'};
        assert(sizeof(log_flag) == sizeof(char)); // logging must be without a virtual table
        std::cout << "size " << sizeof(log_flag) << '\n';
        std::cout << "pin(): ";
        log_flag.pin() = 'd';
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
        dynamic_storage_[0] = df::state::clean;
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
        std::cout << "check copy constructors: \n";
        df::dirtyflag<char> original{'b'};
        // copy constructors not affect on the flag, because we not change an original value
        std::cout << "original, is dirty? - " << original.is_dirty() << " value = " << original.get() << '\n';
        auto copy = original;
        std::cout << "copy, is dirty? - " << original.is_dirty() << " value = " << original.get() << '\n';
        auto copy_2{original};  
        std::cout << "copy_2, is dirty? - " << original.is_dirty() << " value = " << original.get() << '\n'; 

    }

    
    
    
   // std::cout << df3.is_dirty(0);
   // df3.pin( 0) = 'f';
   // df3.mark(0);
    
    //df4.pin() = 'g';
   // df3.mark(0 );
    //df3.pin (0 ) = 'f';
    //df3.clear(0);
    //df2.pin() = 'f';
   // std::cout << df3.get() << df3.is_dirty(0);

    //df3.mark();
    //df.pin() = 'a';
    //bool b = df.get() == df.get();
    //std::cout << b;
    //std::cout << df.pin() << sizeof(df) << df2.is_dirty();
    return 0;
}
