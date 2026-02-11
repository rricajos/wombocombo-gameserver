#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <cstdint>
#include <ctime>
#include <nlohmann/json.hpp>
#include <openssl/hmac.h>
#include <openssl/evp.h>

#include "utils/logger.h"

namespace auth {

struct JwtPayload {
    std::string sub;        // player UUID
    std::string username;   // display name
    int64_t exp = 0;        // expiration timestamp
    int64_t iat = 0;        // issued at timestamp
};

namespace detail {

// Base64url alphabet → value lookup table
inline int b64_val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-' || c == '+') return 62;
    if (c == '_' || c == '/') return 63;
    return -1;
}

inline std::vector<uint8_t> base64url_decode(std::string_view input) {
    std::vector<uint8_t> out;
    out.reserve(input.size() * 3 / 4);

    uint32_t buf = 0;
    int bits = 0;

    for (char c : input) {
        if (c == '=' || c == ' ' || c == '\n') continue;
        int val = b64_val(c);
        if (val < 0) continue;
        buf = (buf << 6) | val;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((buf >> bits) & 0xFF));
        }
    }
    return out;
}

inline std::string base64url_decode_str(std::string_view input) {
    auto bytes = base64url_decode(input);
    return std::string(bytes.begin(), bytes.end());
}

inline std::vector<uint8_t> hmac_sha256(std::string_view key, std::string_view data) {
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int len = 0;

    HMAC(EVP_sha256(),
         key.data(), static_cast<int>(key.size()),
         reinterpret_cast<const unsigned char*>(data.data()), data.size(),
         result, &len);

    return std::vector<uint8_t>(result, result + len);
}

} // namespace detail

// Validate a JWT token against a secret key.
// Returns the payload if valid, nullopt if invalid/expired.
inline std::optional<JwtPayload> validate_jwt(const std::string& token,
                                               const std::string& secret) {
    // Split into header.payload.signature
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return std::nullopt;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return std::nullopt;

    std::string_view header_b64 = std::string_view(token).substr(0, dot1);
    std::string_view payload_b64 = std::string_view(token).substr(dot1 + 1, dot2 - dot1 - 1);
    std::string_view signature_b64 = std::string_view(token).substr(dot2 + 1);

    // Verify signature: HMAC-SHA256(header.payload, secret)
    std::string_view signed_part = std::string_view(token).substr(0, dot2);
    auto expected_sig = detail::hmac_sha256(secret, signed_part);
    auto actual_sig = detail::base64url_decode(signature_b64);

    if (expected_sig.size() != actual_sig.size()) return std::nullopt;

    // Constant-time comparison to prevent timing attacks
    unsigned char diff = 0;
    for (size_t i = 0; i < expected_sig.size(); ++i) {
        diff |= expected_sig[i] ^ actual_sig[i];
    }
    if (diff != 0) {
        logger::warn("JWT signature verification failed");
        return std::nullopt;
    }

    // Decode payload
    std::string payload_json = detail::base64url_decode_str(payload_b64);
    try {
        auto payload = nlohmann::json::parse(payload_json);

        JwtPayload result;
        result.sub = payload.value("sub", "");
        result.username = payload.value("username", "");
        result.exp = payload.value("exp", int64_t(0));
        result.iat = payload.value("iat", int64_t(0));

        if (result.sub.empty()) {
            logger::warn("JWT missing 'sub' claim");
            return std::nullopt;
        }

        // Check expiration
        auto now = static_cast<int64_t>(std::time(nullptr));
        if (result.exp > 0 && now > result.exp) {
            logger::warn("JWT expired for player " + result.sub);
            return std::nullopt;
        }

        return result;
    } catch (const nlohmann::json::exception& e) {
        logger::warn("JWT payload parse error: " + std::string(e.what()));
        return std::nullopt;
    }
}

} // namespace auth
