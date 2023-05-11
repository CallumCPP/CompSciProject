#ifndef SRC_MAINGAME_OBJECT_OBJECT_HPP
#define SRC_MAINGAME_OBJECT_OBJECT_HPP
#include <cstring>
#ifndef GAME_SERVER
#include <raylib.h>
#endif

#include "../Math.hpp"
#include "../Player.hpp"
#include "../Question.hpp"

class Object {
public:
    Object(Math::Rect<float> rect) : rect(rect) {}

    virtual void Tick(float deltaTime) {}
#ifndef GAME_SERVER
    virtual void Render() {}
#endif
    virtual void OnCollide(Player& player) {}

    Math::Rect<float> rect;
    bool hasCollided = false;
};

#endif /* SRC_MAINGAME_OBJECT_OBJECT_HPP */
