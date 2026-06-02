#pragma comment(lib, "ws2_32.lib")
#include "NetworkManager.h"
#include "Player.h"
#include "Ball.h"
#include "Constants.h"
#include <iostream>
#include <cstring>
#include <SDL3/SDL.h>

static void setNonBlocking(SOCKET s) {
    u_long mode = 1;
    ioctlsocket(s, FIONBIO, &mode);
}

static void setBroadcast(SOCKET s) {
    char opt = 1;
    setsockopt(s, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
}

static void setReuseAddr(SOCKET s) {
    char opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {
    cleanup();
}

bool NetworkManager::init() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        SDL_Log("Erreur lors de l'initialisation de Winsock.");
        return false;
    }
    return true;
}

void NetworkManager::cleanup() {
    if (_gameSocket != INVALID_SOCKET) {
        closesocket(_gameSocket);
        _gameSocket = INVALID_SOCKET;
    }
    if (_discoverySocket != INVALID_SOCKET) {
        closesocket(_discoverySocket);
        _discoverySocket = INVALID_SOCKET;
    }
    WSACleanup();
}

bool NetworkManager::startHost(int port) {
    _gameSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_gameSocket == INVALID_SOCKET) return false;
    setNonBlocking(_gameSocket);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_gameSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cleanup();
        return false;
    }

    _discoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_discoverySocket == INVALID_SOCKET) {
        cleanup();
        return false;
    }
    setReuseAddr(_discoverySocket);
    setBroadcast(_discoverySocket);
    setNonBlocking(_discoverySocket);

    sockaddr_in discAddr{};
    discAddr.sin_family = AF_INET;
    discAddr.sin_port = htons(DISCOVERY_PORT);
    discAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_discoverySocket, (sockaddr*)&discAddr, sizeof(discAddr)) == SOCKET_ERROR) {
        cleanup();
        return false;
    }

    _isHost = true;
    return true;
}

bool NetworkManager::startClient() {
    _gameSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_gameSocket == INVALID_SOCKET) return false;
    setNonBlocking(_gameSocket);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_gameSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cleanup();
        return false;
    }

    _discoverySocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_discoverySocket == INVALID_SOCKET) {
        cleanup();
        return false;
    }
    setReuseAddr(_discoverySocket);
    setBroadcast(_discoverySocket);
    setNonBlocking(_discoverySocket);

    sockaddr_in discAddr{};
    discAddr.sin_family = AF_INET;
    discAddr.sin_port = htons(0);
    discAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_discoverySocket, (sockaddr*)&discAddr, sizeof(discAddr)) == SOCKET_ERROR) {
        cleanup();
        return false;
    }

    _isHost = false;
    return true;
}

void NetworkManager::sendDiscoveryRequest() {
    if (_discoverySocket == INVALID_SOCKET) return;

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(DISCOVERY_PORT);
    dest.sin_addr.s_addr = inet_addr("255.255.255.255");

    GamePacket packet;
    packet.type = PACKET_TYPE_DISCOVERY_REQUEST;

    int result = sendto(_discoverySocket, (const char*)&packet, sizeof(GamePacket), 0, (sockaddr*)&dest, sizeof(dest));

    if (result == SOCKET_ERROR) {
        SDL_Log("Erreur envoi de decouverte : %d", WSAGetLastError());
    } else {
        SDL_Log("Requete de decouverte envoyée !");
    }
}

//void NetworkManager::updateHost(Player& p1, Player& p2, Ball& ball) {
void NetworkManager::updateHost() {
    if (_gameSocket == INVALID_SOCKET) return;

    sockaddr_in discRemote{};
    int discRemoteLen = sizeof(discRemote);
    GamePacket discPacket;

    while (recvfrom(_discoverySocket, (char*)&discPacket, sizeof(GamePacket), 0, (sockaddr*)&discRemote, &discRemoteLen) > 0) {
        if (discPacket.type == PACKET_TYPE_DISCOVERY_REQUEST) {
            GamePacket response;
            response.type = PACKET_TYPE_DISCOVERY_RESPONSE;
            sendto(_discoverySocket, (const char*)&response, sizeof(GamePacket), 0, (sockaddr*)&discRemote, sizeof(discRemote));
        }
    }

    
    sockaddr_in gameRemote{};
    int gameRemoteLen = sizeof(gameRemote);
    GamePacket gamePacket;

    while (recvfrom(_gameSocket, (char*)&gamePacket, sizeof(GamePacket), 0, (sockaddr*)&gameRemote, &gameRemoteLen) > 0) {
        _remoteAddr = gameRemote;
        _hasRemote = true;
        //handlePacket(gamePacket, p1, p2, ball);
    }

    /*if (_hasRemote) {
        GamePacket state;
        state.type = PACKET_TYPE_PADDLE_UPDATE;
        state.playerId = 1;
        state.y = p1.getY();
        sendPacket(state);

        state.type = PACKET_TYPE_BALL_UPDATE;
        state.x = ball.getX();
        state.y = ball.getY();
        state.vx = ball.getVX();
        state.vy = ball.getVY();
        sendPacket(state);

        state.type = PACKET_TYPE_SCORE_UPDATE;
        //state.score1 = p1.getScore();
        //state.score2 = p2.getScore();
        sendPacket(state, true);

        if (_gameStarted) {
            state.type = PACKET_TYPE_GAME_START;
            sendPacket(state, true);
        }
    }*/
}

//void NetworkManager::updateClient(Player& p1, Player& p2, Ball& ball) {
void NetworkManager::updateClient() {
    if (_gameSocket == INVALID_SOCKET) return;

    sockaddr_in discRemote{};
    int discRemoteLen = sizeof(discRemote);
    GamePacket discPacket;

    while (recvfrom(_discoverySocket, (char*)&discPacket, sizeof(GamePacket), 0, (sockaddr*)&discRemote, &discRemoteLen) > 0) {
        if (discPacket.type == PACKET_TYPE_DISCOVERY_RESPONSE) {
            char ipStr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &discRemote.sin_addr, ipStr, INET_ADDRSTRLEN);

            bool found = false;
            for (auto& room : _discoveredRooms) {
                if (room.address.ip == ipStr) {
                    room.lastSeen = SDL_GetTicks();
                    found = true;
                    break;
                }
            }
            if (!found) {
                RoomInfo info;
                info.name = "Pong Room";
                info.address.ip = ipStr;
                info.address.port = GAME_PORT;
                info.lastSeen = SDL_GetTicks();
                _discoveredRooms.push_back(info);
                SDL_Log("Salle trouvee a l'adresse : %s", ipStr);
            }
        }
    }

    sockaddr_in gameRemote{};
    int gameRemoteLen = sizeof(gameRemote);
    GamePacket gamePacket;

    while (recvfrom(_gameSocket, (char*)&gamePacket, sizeof(GamePacket), 0, (sockaddr*)&gameRemote, &gameRemoteLen) > 0) {
        //handlePacket(gamePacket, p1, p2, ball);
    }

    /*if (_hasRemote) {
        GamePacket update;
        update.type = PACKET_TYPE_PADDLE_UPDATE;
        update.playerId = 2;
        update.y = p2.getY();
        sendPacket(update);
    }*/
}

void NetworkManager::handlePacket(const GamePacket& packet, Player& p1, Player& p2, Ball& ball) {
    switch (packet.type) {
    case PACKET_TYPE_GAME_START:
        if (!_isHost) {
            _gameStarted = true;
        }
        break;
    case PACKET_TYPE_PADDLE_UPDATE:
        if (_isHost && packet.playerId == 2) {
            p2.setY(packet.y);
        }
        else if (!_isHost && packet.playerId == 1) {
            p1.setY(packet.y);
        }
        break;
    case PACKET_TYPE_BALL_UPDATE:
        if (!_isHost) {
            ball.setX(packet.x);
            ball.setY(packet.y);
            ball.setVX(packet.vx);
            ball.setVY(packet.vy);
        }
        break;
    case PACKET_TYPE_SCORE_UPDATE:
        if (!_isHost) {
            //p1.resetScore();
            //p2.resetScore();
            //p1.addScore(packet.score1);
            //p2.addScore(packet.score2);
        }
        break;
    default:
        break;
    }
}

bool NetworkManager::connectTo(const NetAddress& address) {
    _remoteAddr.sin_family = AF_INET;
    _remoteAddr.sin_port = htons(address.port);
    inet_pton(AF_INET, address.ip.c_str(), &_remoteAddr.sin_addr);
    _hasRemote = true;
    _gameStarted = true;
    return true;
}

void NetworkManager::sendPacket(const GamePacket& packet, bool reliable) {
    if (_gameSocket == INVALID_SOCKET || !_hasRemote) return;

    sendto(_gameSocket, (const char*)&packet, sizeof(GamePacket), 0, (sockaddr*)&_remoteAddr, sizeof(_remoteAddr));
}