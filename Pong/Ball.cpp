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

void Ball::move(int elapsed_ns) {
    Entity::move(elapsed_ns);

    // Wall Reflection
    if (getY() <= 0) {
        setY(0);
        reverseVY();
    } else if (getY() >= GAME_HEIGHT - BALL_SIZE) {
        setY(GAME_HEIGHT - BALL_SIZE);
        reverseVY();
    };
};

void Ball::reset() {
    setX((GAME_WIDTH / 2.0) - (BALL_SIZE / 2.0));
    setY((GAME_HEIGHT / 2.0) - (BALL_SIZE / 2.0));
    setVX((rand() % 2 == 0 ? 1 : -1) * BALL_DEFAULT_SPEED);
    setVY((rand() % 2 == 0 ? 1 : -1) * BALL_DEFAULT_SPEED);
    _bounce_count = 0;
};