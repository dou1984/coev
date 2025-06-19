#pragma once
#include <nghttp2/nghttp2.h>
#include <vector>

namespace coev::nghttp2
{
    class header
    {
    public:
        header() = default;

        void push_back(const char *, size_t, const char *, size_t);
        void push_back(const char *, const char *);
        operator nghttp2_nv *();
        int size() const;

    protected:
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