#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ScoreManager.h"
#include "Player.h"
#include "Ball.h"
#include "Constants.h"

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

    private:
        void handlePacket(const GamePacket& packet, std::vector<Entity*>& entities, ScoreManager& scores);

        SOCKET _gameSocket = INVALID_SOCKET;
        SOCKET _discoverySocket = INVALID_SOCKET;

        bool _isHost = false;
        bool _gameStarted = false;

        sockaddr_in _remoteAddr{};
        bool _hasRemote = false;

        std::vector<RoomInfo> _discoveredRooms;
};