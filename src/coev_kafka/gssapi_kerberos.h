#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <coev.h>
#include "broker.h"
#include "sasl_authenticate_request.h"
#include "sasl_authenticate_response.h"
#include "undefined.h"

struct PrincipalName;
struct GSSAPIConfig
{
    int AuthType;
    std::string KeyTabPath;
    std::string CCachePath;
    std::string KerberosConfigPath;
    std::string ServiceName;
    std::string Username;
    std::string Password;
    std::string Realm;
    bool DisablePAFXFAST;
    std::function<std::string(const std::string &, const std::string &)> BuildSpn;
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

    std::shared_ptr<GSSAPIConfig> Config;
    Ticket ticket;
    EncryptionKey encKey;
    std::function<std::shared_ptr<KerberosClient>(std::shared_ptr<GSSAPIConfig>, int &)> NewKerberosClientFunc;
    int step;

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