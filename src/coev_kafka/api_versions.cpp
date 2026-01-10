#include "api_versions.h"
#include <algorithm>
#include <stdexcept>

void restrictApiVersion(std::shared_ptr<protocol_body> pb, const ApiVersionMap &brokerVersions)
{
    if (pb)
    {

        int16_t key = pb->key();
        int16_t clientMax = pb->version();

        auto it = brokerVersions.find(key);
        if (it != brokerVersions.end())
        {
            const ApiVersionRange &range = it->second;

            int16_t selected = std::min(clientMax, std::max(range.m_min_version, std::min(clientMax, range.m_max_version)));
            pb->set_version(selected);
        }
    }
}