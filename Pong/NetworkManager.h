#pragma once

#include <vector>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
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
        //void updateHost(Player& p1, Player& p2, Ball& ball);
        void updateHost();
        //void updateClient(Player& p1, Player& p2, Ball& ball);
        void updateClient();

        bool connectTo(const NetAddress& address);
        void sendPacket(const GamePacket& packet, bool reliable = false);

        bool isHost() const { return _isHost; }
        bool isGameStarted() const { return _gameStarted; }
        void setGameStarted(bool val) { _gameStarted = val; }
        void startGame() { _gameStarted = true; }
        const std::vector<RoomInfo>& getDiscoveredRooms() const { return _discoveredRooms; }

    private:
        void handlePacket(const GamePacket& packet, Player& p1, Player& p2, Ball& ball);

        SOCKET _gameSocket = INVALID_SOCKET;
        SOCKET _discoverySocket = INVALID_SOCKET;

        bool _isHost = false;
        bool _gameStarted = false;

        sockaddr_in _remoteAddr{};
        bool _hasRemote = false;

        std::vector<RoomInfo> _discoveredRooms;
};