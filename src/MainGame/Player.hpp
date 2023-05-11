#ifndef SRC_MAINGAME_PLAYER_HPP
#define SRC_MAINGAME_PLAYER_HPP
#define G 1000
#include <math.h>
#include <queue>
#ifndef GAME_SERVER
#include <raylib.h>
#endif

#include "Math.hpp"
#include "Question.hpp"

class Player {
public:
    Player() {}
    Player(Math::Rect<float> rect, float baseSpeed, bool isPlayerOne) : rect(rect), baseSpeed(baseSpeed), floorLevel(-rect.h), isPlayerOne(isPlayerOne), lastQuestionAnswered() {}

    void Tick(float deltaTime) {
        if (!crashed) {
            float finalSpeed = baseSpeed;
            for (auto mult : speedMultipliers)
                finalSpeed *= mult.x;
            
            for (int i = 0; i < speedMultipliers.size(); i++) {
                speedMultipliers[i].y -= deltaTime;
                if (speedMultipliers[i].y <= 0) {
                    speedMultipliers.erase(speedMultipliers.cbegin()+i);
                }
            }

#ifndef GAME_SERVER
            if (isPlayerOne) {
                if (IsKeyDown(KEY_SPACE) && this->rect.y == -this->rect.h) {
                    yVelocity = 500;
                }
            }
#endif

            this->rect.x += finalSpeed*deltaTime;
        }

        yVelocity -= G*deltaTime;
        this->rect.y -= yVelocity*deltaTime;
        if (this->rect.y > floorLevel) {
            this->rect.y = floorLevel;
            this->yVelocity = 0;
        }

        if (invincibilityTime > 0) {
            invincibilityTime -= deltaTime;
            if (invincibilityTime < 0) invincibilityTime = 0;
        }
    }

#ifndef GAME_SERVER
    void Render() {
        if (!crashed) {
            DrawRectangle(rect.x, rect.y, rect.w, rect.h, RED);
            if (invincibilityTime)
                DrawRectangle(rect.x+rect.w-5, rect.y-5, 10, rect.h+5, BLUE);
        } else {
            DrawRectangle(rect.x + rect.w - rect.h, rect.y + rect.h - rect.w, rect.h, rect.w, RED);
        }
    }
#endif

    void AddSpeedMult(float mult, float time = MAXFLOAT) {
        speedMultipliers.push_back({ mult, time });
    }

    void SetInvincibility(float time) {
        invincibilityTime += time;
    }

    int streak = 0, highestStreak = 0, correctWhileCrashed = 0, totalCorect = 0, correctSinceUpdate = 0;
    float baseSpeed = 0, yVelocity = 0, invincibilityTime = 0, floorLevel = 0;
    std::vector<Math::Vec2<float>> speedMultipliers; // x = multiplier, y = timer
    Question lastQuestionAnswered;
    bool crashed = false, isPlayerOne;
    Math::Rect<float> rect;
#ifndef GAME_SERVER
    Camera2D camera = {{ 0, 0 }, { 0, 0 }, 0, 1};
#endif
};

#endif /* SRC_MAINGAME_PLAYER_HPP */
