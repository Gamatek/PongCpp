#include "PlayableEntity.h"

PlayableEntity::PlayableEntity(int w, int h) : Entity(w, h) { };

PlayableEntity::~PlayableEntity() {};

int PlayableEntity::getScore() const { return _score; };

void PlayableEntity::addScore(int a) {
    if (a > 0) _score += a;
};

void PlayableEntity::resetScore() {
    _score = 0;
};