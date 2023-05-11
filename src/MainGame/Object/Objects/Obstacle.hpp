#ifndef SRC_MAINGAME_OBJECT_OBJECTS_OBJECT_HPP
#define SRC_MAINGAME_OBJECT_OBJECTS_OBJECT_HPP
#include "../Object.hpp"

class Obstacle : public Object {
public:
    Obstacle(Math::Rect<float> rect, Color color) : Object(rect), color(color) {}

#ifndef GAME_SERVER
    void Render() override {
        DrawRectangle(rect.x, rect.y, rect.w, rect.h, color);
    }
#endif

    void OnCollide(Player& player) override {
        player.rect.x = this->rect.x - player.rect.w;
        player.crashed = true;
        hasCollided = true;
    }

    Color color;
};

#endif /* SRC_MAINGAME_OBJECT_OBJECTS_OBJECT_HPP */
