#include "PlayableEntity.h"
#include "Player.h"
#include "Utils.h"
#include "Constants.h"

Player::Player(int number) : PlayableEntity(PADDLE_WIDTH, PADDLE_HEIGHT) {
    _number = number;
    setVY(PADDLE_SPEED);
};

Player::~Player() {};

void Player::move(int elapsed_ns, int direction) {
    Entity::reverseVY(direction == 1);
    Entity::move(elapsed_ns);
    setY(clampd(getY(), 0.0, (double)GAME_HEIGHT - PADDLE_HEIGHT));
};