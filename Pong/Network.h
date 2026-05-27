#ifndef NETWORK_H
#define NETWORK_H

#include <string>
#include <vector>
#include <enet.h>
#include <SDL3/SDL.h>

enum PacketType {
    PACKET_TYPE_DISCOVERY_REQUEST,
    PACKET_TYPE_DISCOVERY_RESPONSE,
    PACKET_TYPE_PLAYER_JOIN,
    PACKET_TYPE_PADDLE_UPDATE,
    PACKET_TYPE_BALL_UPDATE,
    PACKET_TYPE_SCORE_UPDATE,
    PACKET_TYPE_GAME_START
};

struct GamePacket {
    PacketType type;
    int playerId; // 1 or 2
    float x, y;
    float vx, vy;
    int score1, score2;
};

struct RoomInfo {
    std::string name;
    ENetAddress address;
    Uint32 lastSeen;
};

class NetworkManager {
    public:
        NetworkManager();
        ~NetworkManager();

        bool init();
        void cleanup();

        // Host methods
        bool startHost(int port);
        void updateHost(class Player& p1, class Player& p2, class Ball& ball);

        // Client methods
        bool startClient();
        void updateClient(class Player& p1, class Player& p2, class Ball& ball);
        bool connectTo(const ENetAddress& address);
        void sendDiscoveryRequest();
        const std::vector<RoomInfo>& getDiscoveredRooms() const { return _discoveredRooms; }

        bool isHost() const { return _isHost; }
        bool isConnected() const { return _peer != nullptr; }
        bool isGameStarted() const { return _gameStarted; }
        void setGameStarted(bool started) { _gameStarted = started; }

        // Communication
        void sendPacket(const GamePacket& packet, bool reliable = false);

    private:
        void handlePacket(const GamePacket& packet, class Player& p1, class Player& p2, class Ball& ball);

        ENetHost* _host = nullptr;
        ENetPeer* _peer = nullptr;
        bool _isHost = false;
        bool _gameStarted = false;
        std::vector<RoomInfo> _discoveredRooms;

        ENetSocket _discoverySocket = ENET_SOCKET_NULL;
};

#endif // NETWORK_H