#pragma once

#include <string>
#include <unordered_map>
#include <memory>

#include "utils/config.h"
#include "game/room.h"
#include "storage/redis_client.h"

namespace server {

// Per-socket data attached to each WebSocket connection
struct PerSocketData {
    std::string player_id;
    std::string player_name;
    std::string room_id;
};

class WebSocketServer {
public:
    explicit WebSocketServer(const config::ServerConfig& cfg);

    // Start listening — blocks the calling thread
    void run();

    // Called by the game loop timer every tick
    void tick();

private:
    // Room management
    game::Room* get_or_create_room(const std::string& room_id);
    game::Room* get_room(const std::string& room_id);
    void cleanup_empty_rooms();

    // Setup broadcast callback for a room
    void setup_room_broadcast(game::Room* room);

    // Parse query string params from URL
    static std::unordered_map<std::string, std::string> parse_query(std::string_view url);

    config::ServerConfig cfg_;
    std::unordered_map<std::string, std::unique_ptr<game::Room>> rooms_;

    // Map player_id → their raw WebSocket pointer (void* to avoid template in header)
    std::unordered_map<std::string, void*> player_sockets_;

    // Redis for JWT secret and room config
    storage::RedisClient redis_;
    std::string jwt_secret_;

    // Game loop state
    int tick_count_ = 0;
    float tick_dt_ = 0.05f;  // 1/20 = 50ms
};

} // namespace server
