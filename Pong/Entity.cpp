#include "Entity.h"

Entity::Entity(int w, int h) {
    _w = w;
    _h = h;
};

Entity::~Entity() {};

int Entity::getW() const { return _w; };
int Entity::getH() const { return _h; };

double Entity::getX() const { return _x; };
void Entity::setX(double x) { _x = x; };

double Entity::getY() const { return _y; };
void Entity::setY(double y) { _y = y; };

double Entity::getVX() const { return _vx; };
void Entity::setVX(double vx) { _vx = vx; };

double Entity::getVY() const { return _vy; };
void Entity::setVY(double vy) { _vy = vy; };

void Entity::reverseVX() {
    _vx *= -1.0;
};

void Entity::reverseVX(bool force) {
    if (force && _vy < 0) {
        _vx *= -1.0;
    } else if (!force && _vy > 0) {
        _vx *= -1.0;
    };
};

void Entity::reverseVY() {
    _vy *= -1.0;
};

void Entity::reverseVY(bool force) {
    if (force && _vy < 0) {
        _vy *= -1.0;
    } else if (!force && _vy > 0) {
        _vy *= -1.0;
    };
};

void Entity::move(int elapsed_ns) {
    _x += _vx * elapsed_ns;
    _y += _vy * elapsed_ns;
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