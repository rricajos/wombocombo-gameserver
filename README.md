# WomboCombo Game Server (C++)

Real-time WebSocket game server for WomboCombo — 2D cooperative survival platformer.

## Phase 2 — Multiplayer Movement

- [x] **Phase 1**: WebSocket server, rooms, lobby, chat, broadcast
- [x] **Phase 2**: Game loop at 20 ticks/s, player physics, game_state broadcast, JWT validation, Redis

### What's new in Phase 2

- **Game loop**: Global timer runs at 20 ticks/s, ticking all active rooms
- **Player input → physics**: Clients send `player_input` with actions (`left`, `right`, `jump`), server updates position with gravity and ground collision
- **game_state broadcast**: Every tick, all players receive positions of all other players
- **JWT validation**: Tokens validated via HMAC-SHA256 using the secret from Redis (published by Go API)
- **Redis integration**: Reads `jwt:secret`, writes `server:status`
- **Countdown**: 5-second countdown before game starts when all players are ready

### Game flow

```
1. Players join room (WAITING state)
2. All players click READY
3. Server starts 5s countdown (COUNTDOWN state)
4. Server sends game_start with map_data and spawn_points
5. Game loop begins (PLAYING state) — 20 ticks/s
6. Clients send player_input → server updates physics → broadcasts game_state
```

## Quick Start

### Docker

```bash
docker build -t wombocombo-gameserver .
docker run -p 9001:9001 \
  -e REDIS_ADDR=redis:6379 \
  -e REDIS_PASSWORD=secret \
  wombocombo-gameserver
```

### Local Build

```bash
# Dependencies (Alpine)
apk add build-base cmake git zlib-dev openssl-dev hiredis-dev pkgconf

# Dependencies (Ubuntu)
apt install build-essential cmake git zlib1g-dev libssl-dev libhiredis-dev pkg-config

# Build
mkdir -p third_party
git clone --depth 1 --recurse-submodules \
    https://github.com/uNetworking/uWebSockets.git third_party/uWebSockets

cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)

# Run
REDIS_ADDR=localhost:6379 LOG_LEVEL=debug ./build/gameserver
```

## Environment Variables

| Variable | Default | Description |
|---|---|---|
| `PORT` | `9001` | WebSocket server port |
| `TICK_RATE` | `20` | Game loop ticks per second |
| `LOG_LEVEL` | `info` | `debug`, `info`, `warn`, `error` |
| `MAX_ROOMS` | `100` | Maximum concurrent rooms |
| `MAX_PLAYERS_PER_ROOM` | `4` | Max players per room |
| `REDIS_ADDR` | `localhost:6379` | Redis host:port |
| `REDIS_PASSWORD` | _(empty)_ | Redis auth password |

## Architecture

```
Client (Svelte + Phaser) ←→ Traefik ←→ Game Server (C++ / uWebSockets)
                                              ↓
                                           Redis ← Go API (JWT secret)
```

- Single-threaded event loop (uWebSockets)
- One global timer ticks all active rooms
- No threading needed — everything runs on the same loop
- JWT secret cached at startup from Redis
