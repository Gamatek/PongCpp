#define ENET_IMPLEMENTATION
#include <iostream>
#include <cstring>
#include "Network.h"
#include "Player.h"
#include "Ball.h"
#include "Constants.h"

NetworkManager::NetworkManager() {}

NetworkManager::~NetworkManager() {
    cleanup();
}

bool NetworkManager::init() {
    if (enet_initialize() != 0) {
        SDL_Log("An error occurred while initializing ENet.");
        return false;
    }
    return true;
}

void NetworkManager::cleanup() {
    if (_host) {
        enet_host_destroy(_host);
        _host = nullptr;
    }
    if (_discoverySocket != ENET_SOCKET_NULL) {
        enet_socket_destroy(_discoverySocket);
        _discoverySocket = ENET_SOCKET_NULL;
    }
    enet_deinitialize();
}

bool NetworkManager::startHost(int port) {
    ENetAddress address = { ENET_HOST_ANY, port };

    _host = enet_host_create(&address, 1, 1, 0, 0);
    if (_host == nullptr) {
        SDL_Log("An error occurred while trying to create an ENet server host.");
        return false;
    }

    _isHost = true;

    _discoverySocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if (_discoverySocket != ENET_SOCKET_NULL) {
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_REUSEADDR, 1);
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_BROADCAST, 1);
        ENetAddress discoveryAddr;
        discoveryAddr.host = ENET_HOST_ANY;
        discoveryAddr.port = DISCOVERY_PORT;
        enet_socket_bind(_discoverySocket, &discoveryAddr);
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_NONBLOCK, 1);
    }

    return true;
}

bool NetworkManager::startClient() {
    _host = enet_host_create(nullptr, 1, 2, 0, 0);
    if (_host == nullptr) {
        SDL_Log("An error occurred while trying to create an ENet client host.");
        return false;
    }
    _isHost = false;

    _discoverySocket = enet_socket_create(ENET_SOCKET_TYPE_DATAGRAM);
    if (_discoverySocket != ENET_SOCKET_NULL) {
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_REUSEADDR, 1);
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_BROADCAST, 1);
        enet_socket_set_option(_discoverySocket, ENET_SOCKOPT_NONBLOCK, 1);

        ENetAddress clientAddr;
        clientAddr.host = ENET_HOST_ANY;
        clientAddr.port = ENET_PORT_ANY;
        enet_socket_bind(_discoverySocket, &clientAddr);
    }

    return true;
}

void NetworkManager::sendDiscoveryRequest() {
    if (_discoverySocket == ENET_SOCKET_NULL) return;

    ENetAddress address;
    address.host = enet_v4_noaddr; // Broadcast logic in ENet 2.6.5 IPv6
    address.port = DISCOVERY_PORT;

    GamePacket packet;
    packet.type = PACKET_TYPE_DISCOVERY_REQUEST;

    ENetBuffer buffer;
    buffer.data = &packet;
    buffer.dataLength = sizeof(GamePacket);

    enet_socket_send(_discoverySocket, &address, &buffer, 1);
}

void NetworkManager::updateHost(Player& p1, Player& p2, Ball& ball) {
    if (!_host) return;

    ENetAddress discoveryRemoteAddr;
    ENetBuffer discoveryBuffer;
    GamePacket receivedDiscoveryPacket;
    discoveryBuffer.data = &receivedDiscoveryPacket;
    discoveryBuffer.dataLength = sizeof(GamePacket);

    while (enet_socket_receive(_discoverySocket, &discoveryRemoteAddr, &discoveryBuffer, 1) > 0) {
        if (receivedDiscoveryPacket.type == PACKET_TYPE_DISCOVERY_REQUEST) {
            GamePacket response;
            response.type = PACKET_TYPE_DISCOVERY_RESPONSE;
            ENetBuffer responseBuffer;
            responseBuffer.data = &response;
            responseBuffer.dataLength = sizeof(GamePacket);
            enet_socket_send(_discoverySocket, &discoveryRemoteAddr, &responseBuffer, 1);
        }
    }

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
    state.score1 = p1.getScore();
    state.score2 = p2.getScore();
    sendPacket(state, true);

    if (_gameStarted) {
        state.type = PACKET_TYPE_GAME_START;
        sendPacket(state, true);
    }

    ENetEvent event;
    while (enet_host_service(_host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            SDL_Log("Client connected.");
            _peer = event.peer;
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            if (event.packet->dataLength == sizeof(GamePacket)) {
                handlePacket(*(GamePacket*)event.packet->data, p1, p2, ball);
            }
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            SDL_Log("Client disconnected.");
            _peer = nullptr;
            break;
        default:
            break;
        }
    }
}

void NetworkManager::updateClient(Player& p1, Player& p2, Ball& ball) {
    if (!_host) return;

    ENetAddress discoveryRemoteAddr;
    ENetBuffer discoveryBuffer;
    GamePacket receivedDiscoveryPacket;
    discoveryBuffer.data = &receivedDiscoveryPacket;
    discoveryBuffer.dataLength = sizeof(GamePacket);

    while (enet_socket_receive(_discoverySocket, &discoveryRemoteAddr, &discoveryBuffer, 1) > 0) {
        SDL_Log("Receive packet: %s", receivedDiscoveryPacket.type);
        if (receivedDiscoveryPacket.type == PACKET_TYPE_DISCOVERY_RESPONSE) {
            bool found = false;
            for (auto& room : _discoveredRooms) {
                if (in6_equal(room.address.host, discoveryRemoteAddr.host)) {
                    room.lastSeen = SDL_GetTicks();
                    found = true;
                    break;
                }
            }
            if (!found) {
                RoomInfo info;
                info.name = "Pong Room";
                info.address = discoveryRemoteAddr;
                info.address.port = GAME_PORT;
                info.lastSeen = SDL_GetTicks();
                _discoveredRooms.push_back(info);
            }
        }
    }

    if (_peer) {
        GamePacket update;
        update.type = PACKET_TYPE_PADDLE_UPDATE;
        update.playerId = 2;
        update.y = p2.getY();
        sendPacket(update);
    }

    ENetEvent event;
    while (enet_host_service(_host, &event, 0) > 0) {
        switch (event.type) {
        case ENET_EVENT_TYPE_CONNECT:
            SDL_Log("Connected to server.");
            break;
        case ENET_EVENT_TYPE_RECEIVE:
            if (event.packet->dataLength == sizeof(GamePacket)) {
                handlePacket(*(GamePacket*)event.packet->data, p1, p2, ball);
            }
            enet_packet_destroy(event.packet);
            break;
        case ENET_EVENT_TYPE_DISCONNECT:
            SDL_Log("Disconnected from server.");
            _peer = nullptr;
            break;
        default:
            break;
        }
    }
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
            p1.resetScore();
            p2.resetScore();
            p1.addScore(packet.score1);
            p2.addScore(packet.score2);
        }
        break;
    default:
        break;
    }
}

bool NetworkManager::connectTo(const ENetAddress& address) {
    _peer = enet_host_connect(_host, &address, 2, 0);
    return _peer != nullptr;
}

void NetworkManager::sendPacket(const GamePacket& packet, bool reliable) {
    if (!_host) return;

    ENetPacket* enetPacket = enet_packet_create(&packet, sizeof(GamePacket), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);

    if (_isHost) {
        enet_host_broadcast(_host, 0, enetPacket);
    }
    else if (_peer) {
        enet_peer_send(_peer, 0, enetPacket);
    }
    else {
        enet_packet_destroy(enetPacket);
    }
}
