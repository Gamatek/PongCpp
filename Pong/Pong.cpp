#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <format>
#include <algorithm>
#include <string>
#include <vector>
#include <thread>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "NetworkManager.h"
#include "ScoreManager.h"
#include "Player.h"
#include "Ball.h"
#include "Utils.h"
#include "Constants.h"

using namespace std;

/* We will use this renderer to draw into this window every frame. */
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
int target_fps = 60;
int game_settings_item_selected = 0;

double current_fps = 0.0;
int last_frame_duration = 0;

// Global game state variables
vector<Entity*> entities;
Ball* ball;
NetworkManager network;
ScoreManager scores;

int game_mode = MODE_MENU;
bool show_stats = false;

void resetPlayersPositions() {
    double y = (GAME_HEIGHT / 2.0f) - (PADDLE_HEIGHT / 2.0f);
    for (Entity* e : entities) {
        if (Player* player = dynamic_cast<Player*>(e)) {
            e->setY(y);
        };
    };
};

void RenderDebugOverlay() {
    constexpr int ROW_HEIGHT = DEBUG_CHAR_HEIGHT + 4;

    // Left side (physic/network)
    if (network.isGameStarted() || network.isHost()) {
        const NetworkStats& net = network.getStats();

        SDL_SetRenderDrawColor(renderer, 255, net.ping > 100 ? 60 : 255, net.ping > 100 ? 60 : 255, 255);
        string pingStr = format("Ping: {:.0f}", net.ping);
        SDL_RenderDebugText(renderer, GAME_PADDING, GAME_PADDING + (0 * ROW_HEIGHT), pingStr.c_str());

        SDL_SetRenderDrawColor(renderer, 255, net.packetLoss > 1 ? 60 : 255, net.packetLoss > 1 ? 60 : 255, 255);
        int firstColumnX = GAME_PADDING;
        string upSpeedStr   = format("UP: {:.2f}KB/s", net.uploadRate);
        string upPacketsStr = format("    {} Packets/s", net.upPacketsPerSec);
        SDL_RenderDebugText(renderer, firstColumnX, GAME_PADDING + ROW_HEIGHT, upSpeedStr.c_str());
        SDL_RenderDebugText(renderer, firstColumnX, GAME_PADDING + (ROW_HEIGHT * 2), upPacketsStr.c_str());

        int secondColumnX = GAME_PADDING + 150;
        SDL_SetRenderDrawColor(renderer, 255, net.packetLoss > 1 ? 60 : 255, net.packetLoss > 1 ? 60 : 255, 255);
        string downSpeedStr   = format("DOWN: {:.2f}KB/s", net.downloadRate);
        string downPacketsStr = format("      {} Packets/s", net.downPacketsPerSec);
        string downLossStr    = format("      {:.0f}% Packet Loss", net.packetLoss);
        SDL_RenderDebugText(renderer, secondColumnX, GAME_PADDING + ROW_HEIGHT, downSpeedStr.c_str());
        SDL_RenderDebugText(renderer, secondColumnX, GAME_PADDING + (ROW_HEIGHT * 2), downPacketsStr.c_str());
        SDL_RenderDebugText(renderer, secondColumnX, GAME_PADDING + (ROW_HEIGHT * 3), downLossStr.c_str());
    };

    SDL_SetRenderDrawColor(renderer, 60, 220, 60, 255);

    // Right side (perf)
    int rightTextX = GAME_WIDTH - 80 - GAME_PADDING;

    // FPS
    string fpsStr = format("{:.1f} FPS", current_fps);
    SDL_RenderDebugText(renderer, rightTextX, GAME_PADDING, fpsStr.c_str());

    // Frame duration
    string frameStr = format("{:.2f} ms", last_frame_duration / 1000000.0);
    SDL_RenderDebugText(renderer, rightTextX, GAME_PADDING + ROW_HEIGHT, frameStr.c_str());
};

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    srand(time(0));

    SDL_SetAppMetadata("Pong", "1.0", "com.gamatek.pong");

    // Initialize SDL Video subsystem
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    };

    if (!SDL_CreateWindowAndRenderer("PONG", GAME_WIDTH, GAME_HEIGHT, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    };

    Player* newPlayer1 = new Player(1);
    Player* newPlayer2 = new Player(2);
    Ball* newBall = new Ball();
    entities.push_back(newPlayer1);
    entities.push_back(newPlayer2);
    entities.push_back(newBall);
    ball = newBall;

    for (Entity* e : entities) {
        if (Player* player = dynamic_cast<Player*>(e)) {
            player->setX(player->getNumber() % 2 ? 0 : GAME_WIDTH - PADDLE_WIDTH);
        };
    };

    newBall->reset();

    network.init();

    return SDL_APP_CONTINUE;
};

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* evt) {
    if (evt->type == SDL_EVENT_QUIT) return SDL_APP_SUCCESS;

    if (evt->type == SDL_EVENT_KEY_DOWN) {
        if (evt->key.key == SDLK_ESCAPE) {
            if (game_mode == -1) {
                return SDL_APP_SUCCESS;
            } else {
                game_mode = -1; // Return to menu
            };
        } else if (evt->key.key == SDLK_TAB) {
            show_stats = !show_stats;
        };

        // Menu selection
        switch (game_mode) {
            case MODE_MENU: {
                int old_game_mode = game_mode;
                if (evt->key.key == SDLK_1) game_mode = MODE_AI;
                if (evt->key.key == SDLK_2) game_mode = MODE_LOCAL;
                if (evt->key.key == SDLK_3) {
                    game_mode = MODE_NET_SEARCH;
                    network.startClient();
                };
                if (evt->key.key == SDLK_4) {
                    game_mode = MODE_NET_HOST;
                    network.startHost(GAME_PORT);
                    network.setGameStarted(false);
                };
                if (evt->key.key == SDLK_5) game_mode = MODE_SETTINGS;
                if (old_game_mode != game_mode) {
                    scores.resetAll();
                    resetPlayersPositions();
                    ball->reset();
                };
            }; break;

            case MODE_NET_SEARCH: {
                if (evt->key.key >= SDLK_1 && evt->key.key <= SDLK_9) {
                    int index = (int)(evt->key.key - SDLK_1);
                    const auto& rooms = network.getDiscoveredRooms();
                    if (index < (int)rooms.size()) {
                        if (network.connectTo(rooms[index].address)) {
                            game_mode = MODE_NET_CLIENT;
                        };
                    };
                };
            }; break;

            case MODE_SETTINGS: {
                switch (game_settings_item_selected) {
                    // Frame Rate Limit
                    case 0: {
                        if (evt->key.key == SDLK_LEFT) target_fps = max(target_fps - 10, 30);
                        if (evt->key.key == SDLK_RIGHT) target_fps = min(target_fps + 10, 360);
                    }; break;
                };
            }; break;
        };
    };

    return SDL_APP_CONTINUE;
};

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void* appstate) {
    Uint64 frameStart = SDL_GetTicksNS();
    int frameDuration = SDL_NS_PER_SECOND / target_fps;

    // Get snapshot of keyboard for real-time movement
    const bool* keys = SDL_GetKeyboardState(nullptr);

    switch (game_mode) {
        case MODE_MENU: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Draw menu text (scaled up for title)
            SDL_SetRenderScale(renderer, 4.0, 4.0);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDebugText(renderer, 20, 20, "PONG - MENU");

            int gap = 20;

            SDL_SetRenderScale(renderer, 2.0, 2.0);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 40, 90, "1. PLAY AI");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 1), "2. PLAY LOCAL");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 2), "3. JOIN LAN ROOM");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 3), "4. CREATE LAN ROOM");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 4), "5. SETTINGS");

            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 5), "ESC. EXIT");
            SDL_SetRenderScale(renderer, 1.0, 1.0);

            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;

        case MODE_NET_SEARCH: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 40, 40, "SEARCHING ROOMS...");

            static Uint32 lastRequest = 0;
            if (SDL_GetTicks() - lastRequest > 2000) {
                SDL_Log("Sending discovery request...");
                network.sendDiscoveryRequest();
                lastRequest = SDL_GetTicks();
            };
            network.updateClient(entities, scores);

            const auto& rooms = network.getDiscoveredRooms();
            for (size_t i = 0; i < rooms.size() && i < 9; ++i) {
                string roomStr = to_string(i + 1) + ". Room at " + rooms[i].address.ip;
                SDL_RenderDebugText(renderer, 40, 70 + (int)i * 20, roomStr.c_str());
            };

            SDL_RenderDebugText(renderer, 40, 250, "ESC. BACK");
            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;

        case MODE_NET_HOST: {
            network.updateHost(entities, scores);
        }; break;

        case MODE_NET_CLIENT: {
            network.updateClient(entities, scores);
        }; break;

        case MODE_SETTINGS: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderScale(renderer, 4.0, 4.0);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDebugText(renderer, 20, 20, "PONG - SETTINGS");

            int line_height = DEBUG_CHAR_HEIGHT * 2;
            SDL_SetRenderScale(renderer, 2.0, 2.0);

            // Backgroud item
            int pad = 4;
            SDL_FRect bgRect = {
                40 - pad,
                90 + (line_height * game_settings_item_selected) - pad,
                (GAME_WIDTH / 2) - 80 + (pad * 2),
                DEBUG_CHAR_HEIGHT + (pad * 2)
            };
            SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
            SDL_RenderFillRect(renderer, &bgRect);

            // Items
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

            // Item 1: Frame Rate Limit
            string frameRateLimitStr = format("FRAME RATE LIMIT: < {} >", target_fps);
            SDL_RenderDebugText(renderer, 40, 90, frameRateLimitStr.c_str());

            // Back
            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderDebugText(renderer, 40, 90 + (line_height * 5), "ESC. BACK");
            SDL_SetRenderScale(renderer, 1.0, 1.0);

            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;
    };

    // Physics only on Host or Offline
    if (game_mode != MODE_NET_CLIENT) {
        if (game_mode == MODE_NET_HOST && !network.isGameStarted()) {
            // Wait for movement
        } else {
            // Apply ball physics
            ball->move(frameDuration);

            // Paddle Collision detection
            for (Entity* e : entities) {
                if (Player* player = dynamic_cast<Player*>(e)) {
                    if (!ball->checkCollision(*player)) continue;
                    ball->reverseVX();
                    ball->incrementBounceCount();
                    if (ball->getBounceCount() % 3 == 0) {
                        ball->setVX(ball->getVX() * 1.1);
                        ball->setVY(ball->getVY() * 1.1);
                     };
                     thread([]() { Beep(800, 80); }).detach();
                 };
                 break;
            };
        };

        // Scoring detection
        bool team1_win = ball->getX() > GAME_WIDTH;
        bool team2_win = ball->getX() < 0;

        if (team1_win || team2_win) {
            scores.incrimentScore(team1_win ? 1 : 2);
            resetPlayersPositions();
            ball->reset();
            if (game_mode == MODE_NET_HOST) network.setGameStarted(false);
            thread([team1_win, team2_win]() {
                if (team1_win) {
                    Beep(523, 100); // Do
                    Beep(659, 100); // Mi
                    Beep(784, 200); // Sol
                };
                if (team2_win) {
                    Beep(440, 100); // La
                    Beep(587, 100); // Ré
                    Beep(740, 200); // Fa#
                };
             }).detach();
        };
    };

    // Player 1 Movement
    if (game_mode != MODE_NET_CLIENT) {
        for (Entity* e : entities) {
            if (Player* player = dynamic_cast<Player*>(e)) {
                if (player->getNumber() != 1) continue;
                double oldY = player->getY();
                if (keys[SDL_SCANCODE_LSHIFT]) player->move(frameDuration, -1);
                if (keys[SDL_SCANCODE_LCTRL]) player->move(frameDuration, 1);
                if (game_mode == MODE_NET_HOST && !network.isGameStarted() && player->getY() != oldY) {
                    network.setGameStarted(true);
                };
            };
        };
    };

    // Player 2 Movement
    switch (game_mode) {
        // AI: Follow the ball
        case MODE_AI: {
            for (Entity* e : entities) {
                if (Player* player = dynamic_cast<Player*>(e)) {
                    if (player->getNumber() != 2) continue;
                    player->setY(clampd(ball->getY() - (PADDLE_HEIGHT / 2.0), 0.0, GAME_HEIGHT - PADDLE_HEIGHT));
                };
            };
        }; break;

        // Local (UP/DOWN)
        case MODE_LOCAL: {
            for (Entity* e : entities) {
                if (Player* player = dynamic_cast<Player*>(e)) {
                    if (player->getNumber() != 2) continue;
                    if (keys[SDL_SCANCODE_UP]) player->move(frameDuration, -1);
                    if (keys[SDL_SCANCODE_DOWN]) player->move(frameDuration, 1);
                };
            };
        }; break;

        case MODE_NET_CLIENT: {
            for (Entity* e : entities) {
                if (Player* player = dynamic_cast<Player*>(e)) {
                    if (player->getNumber() != 2) continue;
                    if (keys[SDL_SCANCODE_UP]) player->move(frameDuration, -1);
                    if (keys[SDL_SCANCODE_DOWN]) player->move(frameDuration, 1);
                };
            };
        }; break;

        // P2 controlled by client
        case MODE_NET_HOST: {
            static double lastP2Y = -1;
            for (Entity* e : entities) {
                if (Player* player = dynamic_cast<Player*>(e)) {
                    if (player->getNumber() != 2) continue;
                    if (!network.isGameStarted() && lastP2Y != -1 && player->getY() != lastP2Y) {
                        network.setGameStarted(true);
                    };
                    lastP2Y = player->getY();
                };
            };
        }; break;
    };

    // Render Frame
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw middle net
    SDL_SetRenderDrawColor(renderer, 64, 64, 64, 255);
    for (int i = 0; i < (GAME_HEIGHT / PADDLE_WIDTH); i++) {
        SDL_FRect rect = { (GAME_WIDTH / 2.0f) - (PADDLE_WIDTH / 2.0f), (PADDLE_WIDTH * 2.0f) * i, PADDLE_WIDTH, PADDLE_WIDTH };
        SDL_RenderFillRect(renderer, &rect);
    };

    // Draw Ball
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_FRect bRect = { ball->getX(), ball->getY(), BALL_SIZE, BALL_SIZE };
    SDL_RenderFillRect(renderer, &bRect);

    // Draw Paddles
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (Entity* e : entities) {
        if (Player* player = dynamic_cast<Player*>(e)) {
            SDL_FRect rect = { player->getX(), player->getY(), PADDLE_WIDTH, PADDLE_HEIGHT };
            SDL_RenderFillRect(renderer, &rect);
        };
    };

    // Draw Scores
    SDL_SetRenderScale(renderer, 2.0, 2.0);
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_RenderDebugText(renderer, (GAME_WIDTH / 4) - 35, GAME_PADDING / 2, to_string(scores.getScore(1)).c_str());
    SDL_RenderDebugText(renderer, (GAME_WIDTH / 4) + 30, GAME_PADDING /2, to_string(scores.getScore(2)).c_str());
    SDL_SetRenderScale(renderer, 1.0, 1.0);

    // Debug Statistics Overlay
    if (show_stats) {
        RenderDebugOverlay();
    };

    SDL_RenderPresent(renderer);

    Uint64 frameEnd = SDL_GetTicksNS();
    Uint64 elapsed = frameEnd - frameStart;

    if (elapsed < frameDuration) {
        SDL_DelayNS((Uint64)(frameDuration - elapsed));
    };

    Uint64 finalFrameEnd = SDL_GetTicksNS();
    Uint64 finalElapsed = finalFrameEnd - frameStart;
    double instantFPS = (double)SDL_NS_PER_SECOND / finalElapsed;
    current_fps = current_fps * 0.8 + instantFPS * 0.2;
    last_frame_duration = finalElapsed;

    return SDL_APP_CONTINUE;
};

/* This function runs once at shutdown. */
void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    for (Entity* e : entities) {
        delete e;
    };
};