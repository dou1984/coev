#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <coev/coev.h>
#include "broker.h"
#include "sasl_authenticate_request.h"
#include "sasl_authenticate_response.h"
#include "undefined.h"

struct PrincipalName;
struct GSSAPIConfig
{
    int m_auth_type;
    std::string m_key_tab_path;
    std::string m_ccache_path;
    std::string m_kerberos_config_path;
    std::string m_service_name;
    std::string m_username;
    std::string m_password;
    std::string m_realm;
    bool m_disable_pafx_fast;
    std::function<std::string(const std::string &, const std::string &)> m_build_spn;
};

struct KerberosClient
{

    virtual ~KerberosClient() = default;
    virtual coev::awaitable<int> Login() = 0;
    virtual KError GetServiceTicket(const std::string &spn, std::shared_ptr<Ticket> &ticket, std::shared_ptr<EncryptionKey> &encKey) = 0;
    virtual std::string Domain() = 0;
    virtual PrincipalName CName() = 0;
    virtual int Destroy() = 0;
};

struct GSSAPIKerberosAuth
{

    std::shared_ptr<GSSAPIConfig> m_config;
    Ticket m_ticket;
    EncryptionKey m_enc_key;
    std::function<std::shared_ptr<KerberosClient>(std::shared_ptr<GSSAPIConfig>, int &)> m_new_kerberos_client_func;
    int m_step;

    GSSAPIKerberosAuth();

    coev::awaitable<int> WritePackage(std::shared_ptr<Broker> broker, const std::string &payload, int &bytes_written);
    coev::awaitable<int> ReadPackage(std::shared_ptr<Broker> broker, std::string &payload, int &bytes_read);
    std::string NewAuthenticatorChecksum();
    int CreateKrb5Token(const std::string &domain, const PrincipalName &cname, const Ticket &ticket, const EncryptionKey &sessionKey, std::string &token);
    int AppendGSSAPIHeader(const std::string &payload, std::string &result);
    int InitSecContext(std::shared_ptr<KerberosClient> client, const std::string &bytes, std::string &output);
    std::string Spn(std::shared_ptr<Broker> broker);
    coev::awaitable<int> Login(std::shared_ptr<KerberosClient> client, const std::string &spn, std::shared_ptr<Ticket> &ticket_out);
    coev::awaitable<int> Authorize(std::shared_ptr<Broker> broker);
    coev::awaitable<int> AuthorizeV2(std::shared_ptr<Broker> broker, AuthSendReceiver authSendReceiver);
};