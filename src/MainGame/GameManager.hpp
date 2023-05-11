#ifndef SRC_MAINGAME_GAMEMANAGER
#define SRC_MAINGAME_GAMEMANAGER
#include <iostream>
#include <vector>
#include <queue>
#include <cstring>
#ifndef GAME_SERVER
#include <raylib.h>
#endif

#include "Math.hpp"
#include "Player.hpp"
#include "Question.hpp"
#include "Object/Object.hpp"
#include "Object/Objects/Obstacle.hpp"

#ifdef GAME_SERVER
float previousTime = 0;
float GetFrameTime() {
    float currentTime = clock();
    float deltaTime = (currentTime - previousTime) / CLOCKS_PER_SEC;
    previousTime = currentTime;
    return deltaTime;
}
#endif

class GameManager {
public:
    GameManager() {}
    ~GameManager() {
        for (int i = 0; i < _objectsOne.size(); i++) delete _objectsOne[i];
        for (int i = 0; i < _objectsTwo.size(); i++) delete _objectsTwo[i];

#ifndef GAME_SERVER
        //CloseWindow();
#endif
    }

    void Initialize(std::string playerOneName, std::string playerTwoName, int playerOneGamesWon, int playerTwoGamesWon) {
        this->playerOneName = playerOneName;
        this->playerTwoName = playerTwoName;
        this->playerOneGamesWon = playerOneGamesWon;
        this->playerTwoGamesWon = playerTwoGamesWon;
#ifndef GAME_SERVER
        SetWindowState(FLAG_VSYNC_HINT + FLAG_MSAA_4X_HINT);
        InitWindow(1000, 800, "Runner!");
        //ToggleFullscreen();

        windowSize = { (float)GetScreenWidth(), (float)GetScreenHeight() };
#endif
        srand(time(0));

        _playerOne = Player({{ 0, 0 }, { 40, 100 }}, 300, true);
        _playerTwo = Player({{ 0, 0 }, { 40, 100 }}, 300, false);
        AddObject(new Obstacle({ 300, -50, 50, 50 }, PURPLE));
#ifdef GAME_SERVER
        previousTime = clock();
#endif
    }

    bool Tick() {
#ifndef GAME_SERVER
        if (WindowShouldClose()) return true;
        
        _playerOne.camera.offset = (Vector2){ windowSize.x/2-_playerOne.rect.w/2, windowSize.y/2-_playerOne.rect.h-2 };
        _playerOne.camera.target = (Vector2){ floor(_playerOne.rect.x), -_playerOne.rect.h };

        _playerTwo.camera.offset = (Vector2){ windowSize.x/2-_playerTwo.rect.w/2, windowSize.y-_playerTwo.rect.h-2 };
        _playerTwo.camera.target = (Vector2){ floor(_playerTwo.rect.x), -_playerTwo.rect.h };
#endif
        deltaTime = GetFrameTime();
        float tmpDeltaTime = deltaTime;
        if (countdownToStart > 0) {
            countdownToStart -= deltaTime;
            deltaTime = 0;
        }

        for (auto obj : _objectsOne) obj->Tick(deltaTime);
        for (auto obj : _objectsTwo) obj->Tick(deltaTime);

        if (countdownToStart <= 0) {
            if (!playerOneQuestions.empty()) {
                playerOneQuestions.front().Tick();
                if (!playerOneQuestions.front().lifetime) {
                    if (playerOneQuestions.front().hasCompleted) {
                        _playerOne.lastQuestionAnswered = playerOneQuestions.front();
                        if (playerOneQuestions.front().wasCorrect) {
                            _playerOne.AddSpeedMult(1.2, 1);
                            playerOneQuestions.front().lifetime = 0.5;
                            _playerOne.streak++;
                            _playerOne.totalCorect++;
                            _playerOne.correctSinceUpdate++;
                            if (_playerOne.streak > _playerOne.highestStreak) _playerOne.highestStreak = _playerOne.streak;
                            if (_playerOne.crashed) {
                                _playerOne.correctWhileCrashed++;
                                if (_playerOne.correctWhileCrashed >= 2) 
                                    _playerOne.crashed = false;
                            }
                        } else {
                            _playerOne.AddSpeedMult(0.8, 1);
                            playerOneQuestions.front().lifetime = 1;
                            _playerOne.correctWhileCrashed = 0;
                            _playerOne.streak = 0;
                        }
                    }
                } else {
                    playerOneQuestions.front().lifetime -= deltaTime;
                    if (playerOneQuestions.front().lifetime <= 0) {
                        playerOneQuestions.pop();
                    }
                }
            }

            if (countdownToStart <= 0 && !playerTwoQuestions.empty()) {
                
            }
            
            _playerOne.Tick(deltaTime);
            _playerTwo.Tick(deltaTime);

            if (!_playerOne.invincibilityTime) {
                for (auto obj : _objectsOne) {
                    if (!obj->hasCollided && obj->rect.IsCollidingWith(_playerOne.rect)) {
                        obj->OnCollide(_playerOne);
                    }
                }
            }

            if (!_playerTwo.invincibilityTime) {
                for (auto obj : _objectsTwo) {
                    if (!obj->hasCollided && obj->rect.IsCollidingWith(_playerTwo.rect)) {
                        obj->OnCollide(_playerTwo);
                    }
                }
            }
        } 

        deltaTime = tmpDeltaTime;

        return false;
    }

#ifndef GAME_SERVER
    void Render() {
        BeginDrawing(); {
        ClearBackground(WHITE);
            BeginMode2D(_playerOne.camera); {
                for (auto obj : _objectsOne) {
                    obj->Render();
                } _playerOne.Render();
            } EndMode2D();
            if (_playerOne.crashed) {
                DrawTextCentered((char*)"You have crashed!", windowSize.x/2, 150, 20, BLACK);
                DrawTextCentered((char*)"Get 2 answers in a row to continue.", windowSize.x/2, 170, 20, BLACK);
            }

            if (!playerOneQuestions.empty()) playerOneQuestions.front().Render(true);

            DrawRectangle(0, windowSize.y/2-2, windowSize.x, 2, BLACK);

            BeginMode2D(_playerTwo.camera); {
                for (auto obj : _objectsTwo) {
                    obj->Render();
                } _playerTwo.Render();
            } EndMode2D();
            if (_playerTwo.crashed) {  
                DrawTextCentered((char*)"They have crashed!", windowSize.x/2, 150+windowSize.y/2, 20, BLACK);
            }

            if (!playerTwoQuestions.empty()) playerTwoQuestions.front().Render(false);

            if (countdownToStart > 0) {
                DrawTextCentered((char*)"Game starting in...", windowSize.x/2, windowSize.y/2-20, 20, PURPLE);
                DrawTextCentered((char*)std::to_string((int)countdownToStart).c_str(), windowSize.x/2, windowSize.y/2+20, 20, PURPLE);
            }

            DrawRectangle(0, windowSize.y-2, windowSize.x, 2, BLACK);
        } EndDrawing();
    }
#endif
    
    void AddObject(Object* object) {
        _objectsOne.push_back(object);
        Object* tmp = (Object*)std::malloc(sizeof(*object));
        std::memcpy(tmp, object, sizeof(*object));
        _objectsTwo.push_back(tmp);
    }

    void QueueQuestion(Question question) {
        playerOneQuestions.push(question);
        playerTwoQuestions.push(question);
    }

    // Returns read/write reference to player two
    Player& GetPlayerOne() {
        return _playerOne;
    }

    // Returns read/write reference to player two
    Player& GetPlayerTwo() {
        return _playerTwo;
    }

    float deltaTime = 0, countdownToStart = 15;
    std::string playerOneName, playerTwoName;
    int playerOneGamesWon = 0, playerTwoGamesWon = 0;
    std::queue<Question> playerOneQuestions, playerTwoQuestions;

private:
    std::vector<Object*> _objectsOne, _objectsTwo;
    Player _playerOne, _playerTwo;
};

std::vector<Question> GenerateQuestions(int numQuestions) {
    srand(time(0));
    std::vector<Question> questions;
    for (int i = 0; i < numQuestions; i++) {
        int num1 = (rand() % 12)+1;
        int num2 = (rand() % 12)+1;
        int answer = num1 * num2;
        std::string questionStr = std::to_string(num1) + " x " + std::to_string(num2) + " = ?";
        std::vector<float> dummyAnswers;
        for (int i = 0; i < 3; i++) {
            int num = (rand() % 144)+1;
            while (num == answer) num = (rand() % 144)+1;
            dummyAnswers.push_back(num);
        }

        Question* question = new Question(questionStr, answer, dummyAnswers);
        questions.push_back(*question);
    } return questions;
}

#endif /* SRC_MAINGAME_GAMEMANAGER */
