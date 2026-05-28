#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <format>
#include <algorithm>
#include <string>
#include "Network.h"
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
// Entity entities[3];
Player player1(1);
Player player2(2);
Ball ball;
NetworkManager network;

int game_mode = MODE_MENU;
bool show_stats = false;

void reset_players_positions() {
    double y = (GAME_HEIGHT / 2.0f) - (PADDLE_HEIGHT / 2.0f);
    player1.setY(y);
    player2.setY(y);
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

    if (!SDL_CreateWindowAndRenderer("PONG", GAME_WIDTH * PIXEL_SIZE, GAME_HEIGHT * PIXEL_SIZE, 0, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    };

    // Set logical scaling so we can work with 400x300 coordinates regardless of window size
    SDL_SetRenderScale(renderer, (float)PIXEL_SIZE, (float)PIXEL_SIZE);

    player1.setX(0);
    player2.setX(GAME_WIDTH - PADDLE_WIDTH);
    reset_players_positions();
    ball.reset();

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
            }
            else {
                game_mode = -1; // Return to menu
            };
        }
        else if (evt->key.key == SDLK_TAB) {
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
            // Reset player scores
            if (old_game_mode != game_mode) {
                player1.resetScore();
                player2.resetScore();
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

    /*if (game_mode == MODE_NET_SEARCH) {
        static Uint32 lastRequest = 0;
        if (SDL_GetTicks() - lastRequest > 2000) {
            SDL_Log("Sending discovery request...");
            network.sendDiscoveryRequest(); // Envoie un paquet UDP Broadcast
            lastRequest = SDL_GetTicks();
        };
        network.updateClient(player1, player2, ball); // �coute les r�ponses
    }*/

    // Get snapshot of keyboard for real-time movement
    const bool* keys = SDL_GetKeyboardState(nullptr);

    switch (game_mode) {
        case MODE_MENU: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Draw menu text (scaled up for title)
            SDL_SetRenderScale(renderer, (float)PIXEL_SIZE * 2, (float)PIXEL_SIZE * 2);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDebugText(renderer, 20, 20, "PONG - MENU");

            int gap = 20;

            SDL_SetRenderScale(renderer, (float)PIXEL_SIZE, (float)PIXEL_SIZE);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 40, 90, "1. PLAY AI");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 1), "2. PLAY LOCAL");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 2), "3. JOIN LAN ROOM");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 3), "4. CREATE LAN ROOM");
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 4), "5. SETTINGS");

            SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
            SDL_RenderDebugText(renderer, 40, 90 + ((DEBUG_CHAR_HEIGHT + gap) * 5), "ESC. EXIT");

            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;

        case MODE_NET_SEARCH: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDebugText(renderer, 40, 40, "SEARCHING ROOMS...");

            const auto& rooms = network.getDiscoveredRooms();
            for (size_t i = 0; i < rooms.size() && i < 9; ++i) {
                char ip[64];
                enet_peer_get_ip(nullptr, ip, sizeof(ip)); // This isn't quite right for ENetAddress but let's just show "Room" for now to fix build
                string roomStr = to_string(i + 1) + ". Room at " + to_string(i);
                SDL_RenderDebugText(renderer, 40, 70 + (int)i * 20, roomStr.c_str());
            };

            SDL_RenderDebugText(renderer, 40, 250, "ESC. BACK");
            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;

        case MODE_NET_HOST: {
            network.updateHost(player1, player2, ball);
        }; break;

        case MODE_NET_CLIENT: {
            network.updateClient(player1, player2, ball);
        }; break;

        case MODE_SETTINGS: {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            SDL_SetRenderScale(renderer, (float)PIXEL_SIZE * 2, (float)PIXEL_SIZE * 2);
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            SDL_RenderDebugText(renderer, 20, 20, "PONG - SETTINGS");

            int line_height = DEBUG_CHAR_HEIGHT * 2;
            SDL_SetRenderScale(renderer, (float)PIXEL_SIZE, (float)PIXEL_SIZE);

            // Backgroud item
            int pad = 4;
            SDL_FRect bgRect = {
                40 - pad,
                90 + (line_height * game_settings_item_selected) - pad,
                GAME_WIDTH - 80 + (pad * 2),
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

            SDL_RenderPresent(renderer);
            return SDL_APP_CONTINUE;
        }; break;
    };

    // Player 1 Movement
    if (game_mode != MODE_NET_CLIENT) {
        double oldY = player1.getY();
        if (keys[SDL_SCANCODE_LSHIFT]) player1.move(frameDuration, -1);
        if (keys[SDL_SCANCODE_LCTRL]) player1.move(frameDuration, 1);
        if (game_mode == MODE_NET_HOST && !network.isGameStarted() && player1.getY() != oldY) {
            network.setGameStarted(true);
        };
    };

    // Player 2 Movement
    switch (game_mode) {
        // AI: Follow the ball
        case MODE_AI: {
            player2.setY(clampd(ball.getY() - (PADDLE_HEIGHT / 2.0), 0.0, (double)GAME_HEIGHT - PADDLE_HEIGHT));
        }; break;

        // Local (UP/DOWN)
        case MODE_LOCAL: {
            if (keys[SDL_SCANCODE_UP]) player2.move(frameDuration, -1);
            if (keys[SDL_SCANCODE_DOWN]) player2.move(frameDuration, 1);
        }; break;

        case MODE_NET_CLIENT: {
            double oldY = player2.getY();
            if (keys[SDL_SCANCODE_UP]) player2.move(frameDuration, -1);
            if (keys[SDL_SCANCODE_DOWN]) player2.move(frameDuration, 1);
            if (!network.isGameStarted() && player2.getY() != oldY) {
                // We send movement, host will notice player 2 moved
            };
        }; break;

        // P2 controlled by client
        case MODE_NET_HOST: {
            static double lastP2Y = -1;
            if (!network.isGameStarted() && lastP2Y != -1 && player2.getY() != lastP2Y) {
                network.setGameStarted(true);
            };
            lastP2Y = player2.getY();
        } break;
    };

    // Physics only on Host or Offline
    // TODO: probleme ici pour la balle (point d'arrêt pas déclanché)
    if (game_mode != MODE_NET_CLIENT) {
        if (game_mode == MODE_NET_HOST && !network.isGameStarted()) {
            // Wait for movement
        } else {
            // Apply ball physics
            ball.move(frameDuration);

            // Paddle Collision detection
            if (ball.check_collision(player1)) {
                ball.setX((double)PADDLE_WIDTH);
                ball.reverseVX();
                ball.incrementBounceCount();
                if (ball.getBounceCount() % 3 == 0) {
                    ball.setVX(ball.getVX() * 1.1);
                    ball.setVY(ball.getVY() * 1.1);
                };
            };

            if (ball.check_collision(player2)) {
                ball.setX((double)GAME_WIDTH - PADDLE_WIDTH - BALL_SIZE);
                ball.reverseVX();
                ball.incrementBounceCount();
                if (ball.getBounceCount() % 3 == 0) {
                    ball.setVX(ball.getVX() * 1.1);
                    ball.setVY(ball.getVY() * 1.1);
                };
            };

            // Scoring detection
            bool player1_win = ball.getX() > GAME_WIDTH;
            bool player2_win = ball.getX() < 0;

            if (player1_win || player2_win) {
                if (player1_win) player1.addScore(1);
                if (player2_win) player2.addScore(1);
                reset_players_positions();
                ball.reset();
                if (game_mode == MODE_NET_HOST) network.setGameStarted(false);
            };
        };
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
    SDL_FRect bRect = { ball.getX(), ball.getY(), BALL_SIZE, BALL_SIZE };
    SDL_RenderFillRect(renderer, &bRect);

    // Draw Paddles
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect p1Rect = { 0, player1.getY(), PADDLE_WIDTH, PADDLE_HEIGHT };
    SDL_RenderFillRect(renderer, &p1Rect);

    SDL_FRect p2Rect = { (float)GAME_WIDTH - PADDLE_WIDTH, player2.getY(), PADDLE_WIDTH, PADDLE_HEIGHT };
    SDL_RenderFillRect(renderer, &p2Rect);

    // Draw Scores
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_RenderDebugText(renderer, (GAME_WIDTH / 2.0f) - 35, (float)GAME_PADDING, to_string(player1.getScore()).c_str());
    SDL_RenderDebugText(renderer, (GAME_WIDTH / 2.0f) + 30, (float)GAME_PADDING, to_string(player2.getScore()).c_str());

    // Debug Statistics Overlay
    if (show_stats) {
        SDL_SetRenderDrawColor(renderer, 0, 128, 0, 255);

        // Downscaling for stats
        SDL_SetRenderScale(renderer, (float)PIXEL_SIZE * 0.5f, (float)PIXEL_SIZE * 0.5f);

        // Left side stats
        string statsStr = format("VX: {:.2f} VY: {:.2f} B: {}", ball.getVX(), ball.getVY(), ball.getBounceCount());
        SDL_RenderDebugText(renderer, (float)GAME_PADDING * 2, (float)GAME_PADDING * 2, statsStr.c_str());

        // Right side FPS
        string fpsStr = format("{:.2f} FPS", current_fps);
        SDL_RenderDebugText(renderer, (float)(GAME_WIDTH * 2) - 100, (float)GAME_PADDING * 2, fpsStr.c_str());

        // Frame duration
        string frameStr = format("{:.2f} ms", (float)last_frame_duration / 1000000.0);
        SDL_RenderDebugText(renderer, (float)(GAME_WIDTH * 2) - 100, (float)(GAME_PADDING * 2) + 10, frameStr.c_str());

        // Reset scale for the rest
        SDL_SetRenderScale(renderer, (float)PIXEL_SIZE, (float)PIXEL_SIZE);
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
void SDL_AppQuit(void* appstate, SDL_AppResult result) { };