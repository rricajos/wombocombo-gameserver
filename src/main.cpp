#include "utils/config.h"
#include "utils/logger.h"
#include "server/websocket_server.h"

int main() {
    auto cfg = config::ServerConfig::from_env();
    logger::set_level(cfg.log_level);

    logger::info("=== WomboCombo Game Server v0.2.0 (Phase 2) ===");
    logger::info("port=" + std::to_string(cfg.port)
                 + " tick_rate=" + std::to_string(cfg.tick_rate)
                 + " log_level=" + cfg.log_level);

    server::WebSocketServer ws_server(cfg);
    ws_server.run();

    logger::info("server stopped");
    return 0;
}
