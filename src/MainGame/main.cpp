#include <iostream>
#include <sstream>

#include "GameManager.hpp"

int main() {
    GameManager gm;
    gm.Initialize("", "", 0, 0);
    for (int i = 0; i < 20; i++) {
        int x = (rand() % 12)+1;
        int y = (rand() % 12)+1;

        std::stringstream ss; ss << x << 'x' << y;
        float answer = x*y;

        std::vector<float> dummyAnswers;
        for (int i = 0; i < 4; i++)
            dummyAnswers.push_back(answer + (rand()%20) - 10);

        gm.QueueQuestion(Question(ss.str(), answer, dummyAnswers));
    }

    while (true) {
        if (gm.Tick()) break;
        gm.Render();
    }
}