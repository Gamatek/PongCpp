#pragma once

#include "Entity.h"
#include <SDL3/SDL_timer.h>

const int BALL_SIZE = 4;
const double BALL_DEFAULT_SPEED = 150.0 / SDL_NS_PER_SECOND; // px/s

class Ball : public Entity {
    private:
        int _bounce_count{0};

    public:
        Ball();
        ~Ball();

        int getBounceCount() const;

        void incrementBounceCount();
        void move(int elapsed_ns) override;
        void reset();
};