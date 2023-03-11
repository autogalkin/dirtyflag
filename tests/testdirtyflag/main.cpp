#include <iostream>
#include "df/dirtyflag.h"
int main()
{
    
    df::dirtyflag<char, df::flag_type::logging> df{'f'};
    df.mark();
    df.clear();
    std::cout << df.is_dirty() << sizeof(df);

    return 0;
}
