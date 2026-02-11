#include "storage/redis_client.h"
#include "utils/logger.h"

#include <hiredis/hiredis.h>

namespace storage {

RedisClient::~RedisClient() {
    if (ctx_) {
        redisFree(static_cast<redisContext*>(ctx_));
        ctx_ = nullptr;
    }
}

bool RedisClient::connect(const std::string& host, int port, const std::string& password) {
    auto* c = redisConnect(host.c_str(), port);
    if (!c) {
        logger::error("redis: failed to allocate context");
        return false;
    }
    if (c->err) {
        logger::error("redis: connection error: " + std::string(c->errstr));
        redisFree(c);
        return false;
    }

    // Authenticate if password provided
    if (!password.empty()) {
        auto* reply = static_cast<redisReply*>(
            redisCommand(c, "AUTH %s", password.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            logger::error("redis: auth failed: " +
                (reply ? std::string(reply->str) : "no reply"));
            if (reply) freeReplyObject(reply);
            redisFree(c);
            return false;
        }
        freeReplyObject(reply);
    }

    // Test connection
    auto* reply = static_cast<redisReply*>(redisCommand(c, "PING"));
    if (!reply || reply->type != REDIS_REPLY_STATUS) {
        logger::error("redis: ping failed");
        if (reply) freeReplyObject(reply);
        redisFree(c);
        return false;
    }
    freeReplyObject(reply);

    ctx_ = c;
    logger::info("redis: connected to " + host + ":" + std::to_string(port));
    return true;
}

std::optional<std::string> RedisClient::get(const std::string& key) {
    if (!ctx_) return std::nullopt;
    auto* c = static_cast<redisContext*>(ctx_);

    auto* reply = static_cast<redisReply*>(
        redisCommand(c, "GET %s", key.c_str()));
    if (!reply) return std::nullopt;

    std::optional<std::string> result;
    if (reply->type == REDIS_REPLY_STRING) {
        result = std::string(reply->str, reply->len);
    }
    freeReplyObject(reply);
    return result;
}

bool RedisClient::set(const std::string& key, const std::string& value) {
    if (!ctx_) return false;
    auto* c = static_cast<redisContext*>(ctx_);

    auto* reply = static_cast<redisReply*>(
        redisCommand(c, "SET %s %s", key.c_str(), value.c_str()));
    if (!reply) return false;

    bool ok = (reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return ok;
}

bool RedisClient::set_ex(const std::string& key, const std::string& value, int ttl_seconds) {
    if (!ctx_) return false;
    auto* c = static_cast<redisContext*>(ctx_);

    auto* reply = static_cast<redisReply*>(
        redisCommand(c, "SET %s %s EX %d", key.c_str(), value.c_str(), ttl_seconds));
    if (!reply) return false;

    bool ok = (reply->type == REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return ok;
}

} // namespace storage
