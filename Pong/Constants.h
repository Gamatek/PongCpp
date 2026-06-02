#pragma once

const int DEBUG_CHAR_WIDTH = 8;
const int DEBUG_CHAR_HEIGHT = 8;

const int GAME_WIDTH = 800;
const int GAME_HEIGHT = 600;
const int GAME_PADDING = 20;

const int GAME_PORT = 8000;
const int DISCOVERY_PORT = 8001;

const enum GameMode {
    MODE_MENU = -1,
    MODE_AI = 0,
    MODE_LOCAL = 1,
    MODE_NET_HOST = 2,
    MODE_NET_CLIENT = 3,
    MODE_NET_SEARCH = 4,
    MODE_SETTINGS = 5
};

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
    int playerId; // 1 or 2
    double x, y;
    double vx, vy;
    int score1, score2;
};