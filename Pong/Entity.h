#ifndef ENTITY_H
#define ENTITY_H

class Entity {
    private:
        int _w{0};
        int _h{0};

        float _x{0.0f};
        float _y{0.0f};

        float _vx{0.0f};
        float _vy{0.0f};

    public:
        Entity(int w, int h);
        ~Entity();

        int getW() const;
        int getH() const;
        float getX() const;
        void setX(float x);
        float getY() const;
        void setY(float y);

        float getVX() const;
        void setVX(float vx);
        float getVY() const;
        void setVY(float vy);

        void reverseVX();
        void reverseVY();

        virtual void move(float elapsed);

        bool check_collision(const Entity& entity);
};

#endif // ENTITY_H