#include "Ball.h"
#include "Player.h"
#include "Constants.h"
#include <cstdlib>

Ball::Ball() : Entity(BALL_SIZE, BALL_SIZE) { };
Ball::~Ball() { };

int Ball::getBounceCount() const { return _bounce_count; };

void Ball::incrementBounceCount() {
    _bounce_count++;
};

void Ball::move(float elapsed) {
    Entity::move(elapsed);

    // Wall Reflection
    if (getY() <= 0) {
        setY(0);
        reverseVY();
    }
    else if (getY() >= GAME_HEIGHT - BALL_SIZE) {
        setY(GAME_HEIGHT - BALL_SIZE);
        reverseVY();
    };
};

void Ball::reset() {
    setX((GAME_WIDTH / 2.0f) - (BALL_SIZE / 2.0f));
    setY((GAME_HEIGHT / 2.0f) - (BALL_SIZE / 2.0f));
    setVX((rand() % 2 == 0 ? 1 : -1) * 150.0f); // 150.0 px/s
    setVY((rand() % 2 == 0 ? 1 : -1) * 150.0f); // 150.0 px/s
    _bounce_count = 0;
};