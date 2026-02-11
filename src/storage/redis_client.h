#pragma once

#include <string>
#include <optional>

namespace storage {

class RedisClient {
public:
    RedisClient() = default;
    ~RedisClient();

    // Non-copyable
    RedisClient(const RedisClient&) = delete;
    RedisClient& operator=(const RedisClient&) = delete;

    // Connect to Redis. Returns false on failure.
    bool connect(const std::string& host, int port, const std::string& password = "");

    // Key-value operations
    std::optional<std::string> get(const std::string& key);
    bool set(const std::string& key, const std::string& value);
    bool set_ex(const std::string& key, const std::string& value, int ttl_seconds);

    bool is_connected() const { return ctx_ != nullptr; }

private:
    void* ctx_ = nullptr;  // redisContext*, void to avoid hiredis include in header
};

} // namespace storage
