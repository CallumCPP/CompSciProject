#ifndef SRC_MAINGAME_QUESTION
#define SRC_MAINGAME_QUESTION
#define ANSWER_FONT_SIZE 40
#define QUESTION_FONT_SIZE 50

#include <vector>
#include <sstream>
#include <array>
#include <math.h>
#include <raylib.h>
#include "Math.hpp"

#ifndef GAME_SERVER
void DrawTextCentered(const char* text, int posX, int posY, int fontSize, Color color) {
    DrawText(text, posX - MeasureText(text, fontSize)/2, posY - fontSize/2, fontSize, color);
}
#endif

Math::Vec2<float> windowSize;

class Question {
public:
    Question() {}
    Question(std::string equation, float answer, std::vector<float> dummyAnswers) : _possibleAnswers(dummyAnswers), _equationString(equation), answer(answer) {
        _ansPos = rand()%_possibleAnswers.size();
        if (_ansPos == _possibleAnswers.size()) _possibleAnswers.push_back(6);
        else _possibleAnswers.emplace(_possibleAnswers.begin()+(_ansPos), answer);
    }

    void Tick() {
#ifndef GAME_SERVER
        Math::Vec2<float> mousePos(GetMouseX(), GetMouseY());
        if (mousePos.y > windowSize.y/2) return;
        if (!hasCompleted && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && (Math::Rect<float>){ windowSize.x/2-windowSize.x/6, 60, (float)windowSize.x/3, 60 }.Contains({ mousePos.x, mousePos.y })) {
            _chosenAnswer = floor((mousePos.x-(windowSize.x/3))/(windowSize.x/3)/_possibleAnswers.size());
            if (_possibleAnswers[_chosenAnswer] == answer) wasCorrect = true;
            hasCompleted = true;
        }
#endif
    }

#ifndef GAME_SERVER
    void Render(bool isPlayerOne) {
        float farLeft = windowSize.x/2-windowSize.x/6;
        float step = (windowSize.x/3)/_possibleAnswers.size();
        float yOffset = isPlayerOne ? 0 : windowSize.y/2;
        DrawRectangle(farLeft, 10 + yOffset, windowSize.x/3, 110, { 128, 128, 128, 128 });
        DrawRectangleLinesEx({ farLeft, 7 + yOffset, (float)windowSize.x/3, 113 }, 3, { 32, 32, 32, 255 });
        DrawTextCentered(_equationString.c_str(), windowSize.x/2, 12 + yOffset + 30, QUESTION_FONT_SIZE, RED);
        for (int i = 0; i < _possibleAnswers.size(); i++) {
            if (hasCompleted) {
                if (i == _ansPos) DrawRectangle(farLeft+step*i, 60+yOffset, step, 60, { 32, 200, 32, 200 });
                if (!wasCorrect && _chosenAnswer == i) DrawRectangle(farLeft+step*i, 60+yOffset, step, 60, { 200, 32, 32, 200 });
            }
            
            DrawRectangleLinesEx({ farLeft+step*i, 60+yOffset, step, 60 }, 3, { 32, 32, 32, 255 });
            DrawTextCentered(std::to_string(_possibleAnswers[i]).substr(0, std::to_string(_possibleAnswers[i]).find('.')).c_str(), farLeft+step*i+step/2, 70 + yOffset+30, ANSWER_FONT_SIZE, BLACK);
        }
    }
#endif

    const int DataSize() {
        return 64;
    }

    std::array<char, 64> Serialize() {
        std::array<char, 64> buf;
        buf.fill(0);
        int dataOffset = 0;
        memcpy(buf.data(), &answer, sizeof(float)*2+sizeof(bool)*2);
        dataOffset += sizeof(float)*2+sizeof(bool)*2;
        int equationSize = _equationString.size();
        memcpy(buf.data()+dataOffset, &equationSize, sizeof(int));
        dataOffset += sizeof(int);
        memcpy(buf.data()+dataOffset, _equationString.c_str(), equationSize);
        dataOffset += equationSize;
        int possibleAnswersSize = _possibleAnswers.size();
        memcpy(buf.data()+dataOffset, &possibleAnswersSize, sizeof(int));
        dataOffset += sizeof(int);
        memcpy(buf.data()+dataOffset, _possibleAnswers.data(), sizeof(float)*possibleAnswersSize);
        dataOffset += sizeof(float)*possibleAnswersSize;
        memcpy(buf.data()+dataOffset, &_chosenAnswer, sizeof(int)*2);

        return buf;
    }

    void Deserialize(std::array<char, 64> buf) {
        _possibleAnswers.clear();
        int dataOffset = 0;
        memcpy(&answer, buf.data(), sizeof(float)*2+sizeof(bool)*2);
        dataOffset += sizeof(float)*2+sizeof(bool)*2;
        int equationSize;
        memcpy(&equationSize, buf.data()+dataOffset, sizeof(int));
        dataOffset += sizeof(int);
        _equationString = std::string(buf.data()+dataOffset, equationSize);
        dataOffset += equationSize;
        int possibleAnswersSize;
        memcpy(&possibleAnswersSize, buf.data()+dataOffset, sizeof(int));
        dataOffset += sizeof(int);
        _possibleAnswers.resize(possibleAnswersSize);
        memcpy(_possibleAnswers.data(), buf.data()+dataOffset, sizeof(float)*possibleAnswersSize);
        dataOffset += sizeof(float)*possibleAnswersSize;
        memcpy(&_chosenAnswer, buf.data()+dataOffset, sizeof(int)*2);

        return;
    }

    float answer = 0, lifetime = 0;
    bool hasCompleted = false, wasCorrect = false;
    
private:
    std::string _equationString;
    std::vector<float> _possibleAnswers;
    int _chosenAnswer, _ansPos;
};

#endif /* SRC_MAINGAME_QUESTION */
