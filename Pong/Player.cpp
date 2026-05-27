#include "PlayableEntity.h"
#include "Player.h"
#include "Constants.h"
#include <algorithm>

Player::Player(int number) : PlayableEntity(PADDLE_WIDTH, PADDLE_HEIGHT) {
    _number = number;
};

Player::~Player() {};

void Player::move(float deltaTime, int direction) {
    setVY(direction * PADDLE_SPEED);
    Entity::move(deltaTime);
    setY(std::clamp(getY(), 0.0f, (float)GAME_HEIGHT - PADDLE_HEIGHT));
};