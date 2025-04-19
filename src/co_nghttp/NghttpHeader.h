#pragma once
#include <nghttp2/nghttp2.h>
#include <vector>

namespace coev
{
    class NghttpHeader
    {
    public:
        NghttpHeader() = default;

        void push_back(const char *, size_t, const char *, size_t);
        void push_back(const char *, const char *);
        operator nghttp2_nv *();
        int size() const;

    private:
        union __nv
        {
            nghttp2_nv nva;
            struct nv
            {
                const char *name;
                const char *value;
                size_t namelen;
                size_t valuelen;
                uint8_t flags;
            } h;
        };
        std::vector<__nv> m_headers;
    };

}