#pragma once

#include <SDL3/SDL.h>

class Entity {
    private:
        int _w{0};
        int _h{0};

        double _x{0.0};
        double _y{0.0};

        double _vx{0.0};
        double _vy{0.0};

    public:
        Entity(int w, int h);
        ~Entity();

        int getW() const;
        int getH() const;
        double getX() const;
        void setX(double x);
        double getY() const;
        void setY(double y);

        double getVX() const;
        void setVX(double vx);
        double getVY() const;
        void setVY(double vy);

        void reverseVX();
        void reverseVX(bool force);
        void reverseVY();
        void reverseVY(bool force);

        virtual void move(int elapsed_ns);
        bool checkCollision(const Entity& entity);
        //virtual void draw(SDL_Renderer* renderer) = 0;
};