#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include "version.h"
#include "packet_decoder.h"
#include "packet_encoder.h"
#include "protocol_body.h"
#include "real_decoder.h"

struct Broker;

inline constexpr const int32_t MaxRequestSize = 100 * 1024 * 1024;

struct request : versionedEncoder, IEncoder, IDecoder
{
    int32_t m_correlation_id = 0;
    std::string m_client_id;
    std::shared_ptr<protocol_body> m_body;
    int encode(packetEncoder &pe);
    int encode(packetEncoder &pe, int16_t version) override {
        // This method is inherited from versionedEncoder but not used directly
        // in the request struct. We delegate to the regular encode method.
        return encode(pe);
    }
    int decode(packetDecoder &pd);
    
    // Check if the request body is a flexible version
    bool is_flexible() const {
        if (m_body) {
            if (auto fv = std::dynamic_pointer_cast<flexible_version>(m_body)) {
                return fv->is_flexible();
            }
        }
        return false;
    }
};

coev::awaitable<int> decodeRequest(std::shared_ptr<Broker> &broker, int &req, request &size);
std::shared_ptr<protocol_body> allocateBody(int16_t key, int16_t version);
