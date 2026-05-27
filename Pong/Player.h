#ifndef PLAYER_H
#define PLAYER_H
#include "PlayableEntity.h"

const int PADDLE_WIDTH = 4;
const int PADDLE_HEIGHT = 40;
const int PADDLE_SPEED = 350;

class Player : public PlayableEntity {
    private:
        int _number;

    public:
        Player(int number);
        ~Player();

        void move(float deltaTime, int direction);
};

#endif // PLAYER_H