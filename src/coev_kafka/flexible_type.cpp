#include "flexible_type.h"

namespace coev::kafka
{

    bool flexible_type::__is_fixed()
    {
        return m_flexible == 0;
    }
    bool flexible_type::__is_flexible()
    {
        return m_flexible > 0;
    }
    void flexible_type::__push_flexible()
    {
        m_flexible++;
    }
    void flexible_type::__pop_flexible()
    {
        assert(m_flexible > 0);
        m_flexible--;
    }

}