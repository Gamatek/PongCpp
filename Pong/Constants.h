#ifndef CONSTANTS_H
#define CONSTANTS_H

const int DEBUG_CHAR_WIDTH = 8;
const int DEBUG_CHAR_HEIGHT = 8;

const int PIXEL_SIZE = 2;

const int GAME_WIDTH = 400;
const int GAME_HEIGHT = 300;
const int GAME_PADDING = 10;

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

#endif // CONSTANTS_H