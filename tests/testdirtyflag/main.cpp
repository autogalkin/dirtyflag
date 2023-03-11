#include <iostream>
#include "df/dirtyflag.h"

struct logging
{
    constexpr logging(df::state init_state){}
    template <typename DirtyFlagValue>
    constexpr void mark(DirtyFlagValue& v) noexcept{
        log(v);
    }
    template <typename DirtyFlagValue>
    constexpr void clear(DirtyFlagValue& v) noexcept{
    }
    template <typename DirtyFlagObj>
    void log(DirtyFlagObj& v)
    {
        std::cout << "logging example " << v << std::endl;
    }

};

int main()
{
    
    df::dirtyflag<char, logging> df{'f'};
    df::dirtyflag<char, df::flag_type::bool_flag> df2{'a'};
    df.pin() = 'a';
    bool b = df.get() == df.get();
    //std::cout << b;
    //std::cout << df.pin() << sizeof(df) << df2.is_dirty();
    return 0;
}
