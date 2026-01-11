#include <cstdint>
#include <arpa/inet.h>
#include "block.h"

namespace coev::details
{

    void block::set_size(uint32_t val)
    {
        *u32 = htonl(val);
    }
    uint32_t block::get_size()
    {
        return *u32;
    }
    uint32_t block::size() const
    {
        return htonl(*u32);
    }
    char *block::body()
    {
        return str + sizeof(uint32_t);
    }
    char *block::next()
    {
        return body() + size();
    }

}