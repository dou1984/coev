#pragma once
#include <string>
#include <memory>
#include "gssapi_kerberos.h"
struct PrincipalName;

struct KerberosGoKrb5Client
{
    void *krb5Client;

    std::string Domain();
    PrincipalName CName();
};

std::shared_ptr<KerberosClient> NewKerberosClient(const std::shared_ptr<GSSAPIConfig> &config, int &);
std::shared_ptr<KerberosClient> createClient(const std::shared_ptr<GSSAPIConfig> &config, void *cfg);
