#include "Entity.h"

Entity::Entity(int w, int h) {
    _w = w;
    _h = h;
};

Entity::~Entity() {};

int Entity::getW() const { return _w; };
int Entity::getH() const { return _h; };

float Entity::getX() const { return _x; };
void Entity::setX(float x) { _x = x; };

float Entity::getY() const { return _y; };
void Entity::setY(float y) { _y = y; };

float Entity::getVX() const { return _vx; };
void Entity::setVX(float vx) { _vx = vx; };

float Entity::getVY() const { return _vy; };
void Entity::setVY(float vy) { _vy = vy; };

void Entity::reverseVX() {
    _vx *= -1.0f;
};

void Entity::reverseVY() {
    _vy *= -1.0f;
};

void Entity::move(float elapsed) {
    _x += _vx * elapsed;
    _y += _vy * elapsed;
};

// AABB collision detection
bool Entity::check_collision(const Entity& entity) {
    return (
        entity.getX() < getX() + getW() &&
        entity.getX() + entity.getW() > getX() &&
        entity.getY() < getY() + getH() &&
        entity.getY() + entity.getH() > getY()
        );
};