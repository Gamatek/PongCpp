#include "Entity.h"
#include "Player.h"
#include "Utils.h"
#include "Constants.h"

Player::Player(int number) : Entity(PADDLE_WIDTH, PADDLE_HEIGHT) {
    _number = number;
    setVY(PADDLE_SPEED);
};

Player::~Player() {};

int Player::getNumber() { return _number; };

void Player::move(int elapsed_ns, int direction) {
    Entity::reverseVY(direction == 1);
    Entity::move(elapsed_ns);
    setY(clampd(getY(), 0.0, (double)GAME_HEIGHT - PADDLE_HEIGHT));
};

/*void Player::draw(SDL_Renderer* renderer) override {
    SDL_FRect rect = { getX(), getY(), PADDLE_WIDTH, PADDLE_HEIGHT };
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &rect);
}*/