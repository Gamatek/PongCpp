#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ScoreManager.h"
#include "Player.h"
#include "Ball.h"
#include "Constants.h"

const enum PacketType {
    PACKET_TYPE_DISCOVERY_REQUEST,
    PACKET_TYPE_DISCOVERY_RESPONSE,
    PACKET_TYPE_PLAYER_JOIN,
    PACKET_TYPE_PADDLE_UPDATE,
    PACKET_TYPE_BALL_UPDATE,
    PACKET_TYPE_SCORE_UPDATE,
    PACKET_TYPE_GAME_START
};

const struct GamePacket {
    PacketType type;

    unsigned int sequence;
    unsigned int timestamp;
    unsigned int echoTimestamp;
    unsigned int echoSequence;

    int playerId; // 1 or 2
    double x, y;
    double vx, vy;
    int score1, score2;
};

struct NetworkStats {
    double ping = 0.0;
    double uploadRate = 0.0;
    double downloadRate = 0.0;
    double packetLoss = 0.0;
    int upPacketsPerSec = 0;
    int downPacketsPerSec = 0;
};

struct NetAddress {
    std::string ip;
    int port;
};

struct RoomInfo {
    std::string name;
    NetAddress address;
    unsigned int lastSeen;
};

class NetworkManager {
    private:
        void handlePacket(const GamePacket& packet, std::vector<Entity*>& entities, ScoreManager& scores);
        void updateStats(unsigned int currentTime);

        SOCKET _gameSocket = INVALID_SOCKET;
        SOCKET _discoverySocket = INVALID_SOCKET;

        bool _isHost = false;
        bool _gameStarted = false;

        sockaddr_in _remoteAddr{};
        bool _hasRemote = false;

        std::vector<RoomInfo> _discoveredRooms;

        // Stats
        NetworkStats _stats;
        unsigned int _lastStatsUpdateTime = 0;

        // Bandwidth
        unsigned int _bytesSentAccumulator = 0;
        unsigned int _bytesReceivedAccumulator = 0;

        // Ping
        unsigned int _lastReceivedTimestamp = 0;
        unsigned int _lastReceivedSequence = 0;
        unsigned int _lastCalculatedPingSequence = 0;

        // Packet
        unsigned int _packetsSentThisSecond = 0;
        unsigned int _packetsReceivedThisSecond = 0;

        // Packet loss
        unsigned int _nextSequenceToSend = 0;
        unsigned int _highestSequenceReceived = 0;
        unsigned int _packetsExpected = 0;
        unsigned int _packetsReceivedCount = 0;

    public:
        NetworkManager();
        ~NetworkManager();

        bool init();
        void cleanup();

        bool startHost(int port);
        bool startClient();

        void sendDiscoveryRequest();
        void updateHost(std::vector<Entity*>& entities, ScoreManager& scores);
        void updateClient(std::vector<Entity*>& entities, ScoreManager& scores);

        bool connectTo(const NetAddress& address);
        void sendPacket(const GamePacket& packet, bool reliable = false);

        bool isHost() const { return _isHost; }
        bool isGameStarted() const { return _gameStarted; }
        void setGameStarted(bool val) { _gameStarted = val; }
        void startGame() { _gameStarted = true; }
        const std::vector<RoomInfo>& getDiscoveredRooms() const { return _discoveredRooms; }

        const NetworkStats& getStats() const { return _stats; }
};