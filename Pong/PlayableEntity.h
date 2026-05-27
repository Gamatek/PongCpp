#ifndef PLAYABLE_ENTITY_H
#define PLAYABLE_ENTITY_H
#include "Entity.h"

class PlayableEntity : public Entity {
private:
    int _score{ 0 };

public:
    PlayableEntity(int w, int h);
    ~PlayableEntity();

    int getScore() const;

    void addScore(int a);
    void resetScore();
};

#endif // PLAYABLE_ENTITY_H