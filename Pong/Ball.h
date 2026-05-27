#ifndef BALL_H
#define BALL_H
#include "Entity.h"
#include "Player.h"

const int BALL_SIZE = 4;

class Ball : public Entity {
    private:
        int _bounce_count{0};

    public:
        Ball();
        ~Ball();

        int getBounceCount() const;

        void incrementBounceCount();
        void move(float elapsed) override;
        void reset();
};

#endif // BALL_H