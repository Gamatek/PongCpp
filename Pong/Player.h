#pragma once

#include "Entity.h"
#include <SDL3/SDL_timer.h>

const int PADDLE_WIDTH = 8;
const int PADDLE_HEIGHT = 80;
const double PADDLE_SPEED = 600.0 / SDL_NS_PER_SECOND; // px/s

class Player : public Entity {
    private:
        int _number;

    public:
        Player(int number);
        ~Player();

        int getNumber();

        void move(int elapsed_ns, int direction);
};