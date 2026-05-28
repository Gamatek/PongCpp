#pragma once

#include "PlayableEntity.h"
#include <SDL3/SDL_timer.h>

const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 40;
const double PADDLE_SPEED = 300.0 / SDL_NS_PER_SECOND; // px/s

class Player : public PlayableEntity {
    private:
        int _number;

    public:
        Player(int number);
        ~Player();

        void move(int elapsed_ns, int direction);
};