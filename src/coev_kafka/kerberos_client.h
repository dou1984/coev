/*
 *	coev - c++20 coroutine library
 *
 *	Copyright (c) 2023-2026, Zhao Yun Shan
 *
 */
#pragma once
#include <string>
#include <memory>
#include "gssapi_kerberos.h"

namespace coev::kafka
{
    struct PrincipalName;
    struct KerberosGoKrb5Client
    {
        void *krb5Client;

        std::string Domain();
        PrincipalName CName();
    };

    std::shared_ptr<KerberosClient> NewKerberosClient(const std::shared_ptr<GSSAPIConfig> &config, int &);
    std::shared_ptr<KerberosClient> CreateClient(const std::shared_ptr<GSSAPIConfig> &config, void *cfg);
}