#pragma once

#include <string>
#include <nlohmann/json.hpp>

#include "game/room.h"
#include "network/protocol.h"
#include "utils/logger.h"

namespace network {

// Handles a single parsed message from a player inside a room.
// Returns false if the message type is unrecognized (non-fatal).
inline bool handle_message(game::Room& room,
                           const std::string& player_id,
                           const nlohmann::json& msg) {
    std::string type = get_type(msg);
    if (type.empty()) {
        room.send_to(player_id, make_error(400, "Missing or invalid 'type' field"));
        return false;
    }

    // ── Heartbeat ───────────────────────────────
    if (type == "ping") {
        room.send_to(player_id, {{"type", "pong"}});
        return true;
    }

    // ── Lobby messages ────────────────────────────
    if (type == "player_ready") {
        bool ready = msg.value("ready", false);
        room.set_player_ready(player_id, ready);
        return true;
    }

    if (type == "chat_message") {
        std::string message = msg.value("message", "");
        if (message.empty()) {
            room.send_to(player_id, make_error(400, "Empty chat message"));
            return false;
        }
        if (message.size() > 200) {
            message = message.substr(0, 200);
        }
        room.handle_chat(player_id, message);
        return true;
    }

    // ── Gameplay messages ─────────────────────────
    if (type == "player_input") {
        int tick = msg.value("tick", 0);

        std::vector<std::string> actions;
        if (msg.contains("actions") && msg["actions"].is_array()) {
            for (const auto& a : msg["actions"]) {
                if (a.is_string()) {
                    actions.push_back(a.get<std::string>());
                }
            }
        }

        room.queue_input(player_id, tick, actions);
        return true;
    }

    if (type == "player_action") {
        // Phase 3+: use_item, etc.
        logger::debug("received player_action from " + player_id + " (Phase 3)");
        return true;
    }

    if (type == "buy_item") {
        // Phase 4: shop system
        logger::debug("received buy_item from " + player_id + " (Phase 4)");
        return true;
    }

    // Unknown message type — log but don't spam the client
    logger::warn("unknown message type '" + type + "' from player " + player_id);
    room.send_to(player_id, make_error(400, "Unknown message type: " + type));
    return false;
}

} // namespace network
