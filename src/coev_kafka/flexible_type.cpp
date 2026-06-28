#include "flexible_type.h"

namespace coev::kafka
{

    bool FlexibleType::__is_fixed()
    {
        return m_flexible == 0;
    }
    bool FlexibleType::__is_flexible()
    {
        return m_flexible > 0;
    }
    void FlexibleType::__push_flexible()
    {
        m_flexible++;
    }
    void FlexibleType::__pop_flexible()
    {
        assert(m_flexible > 0);
        m_flexible--;
    }

}