#include "kerberos_client.h"
#include <stdexcept>

std::string KerberosGoKrb5Client::Domain()
{
    // This would call into the real gokrb5 client's Domain method in a real binding.
    throw std::runtime_error("KerberosGoKrb5Client::Domain not implemented in C++ stub");
}

PrincipalName KerberosGoKrb5Client::CName()
{
    // This would call into the real gokrb5 client's CName method in a real binding.
    throw std::runtime_error("KerberosGoKrb5Client::CName not implemented in C++ stub");
    return PrincipalName{};
}

std::shared_ptr<KerberosClient> NewKerberosClient(const std::shared_ptr<GSSAPIConfig> &config, int &)
{
    // In real usage, this would load Kerberos config and call createClient.
    // Since gokrb5 is Go-only, this is a stub.
    throw std::runtime_error("NewKerberosClient not implemented in C++");
    return {nullptr};
}

std::shared_ptr<KerberosClient> createClient(const std::shared_ptr<GSSAPIConfig> &config, void *cfg)
{
    // Stub implementation — actual logic depends on Go runtime or native Kerberos library.
    throw std::runtime_error("createClient not implemented in C++");
    return {nullptr};
}