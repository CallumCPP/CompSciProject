#include <iostream>
#include <queue>
#include <chrono>
#include <asio.hpp>

#include "TCPClient.hpp"
#include "../Common/SHA256.hpp"
#include "../Common/Packet.hpp"

std::queue<std::string> responseQueue;
TCPClient* client;
GameManager* gm;
int loginResult = 0;
char userName[20], opponentName[20];
int gamesPlayed, gamesWon, highScore, opponentGamesWon = 0;
bool queueResponse = false, shouldDeleteGame = false;
GameEndReason gameEndReason = NONE;

void GameLoop() {
    gm->Initialize(std::string(userName), std::string(opponentName), gamesWon, opponentGamesWon);
    int x = (rand() % 12)+1;
    int y = (rand() % 12)+1;

    std::stringstream ss; ss << x << 'x' << y;
    float answer = x*y;

    std::vector<float> dummyAnswers;
    for (int i = 0; i < 4; i++)
        dummyAnswers.push_back(answer + (rand()%20) - 10);

    gm->QueueQuestion(Question(ss.str(), answer, dummyAnswers));
    float timeSinceUpdateSent = 0;
    while (true) {
        if (gm != nullptr) {
            if (gm->Tick()) {
                delete gm;
                gm = nullptr;
            } else gm->Render();
        } else break;

        timeSinceUpdateSent += gm->deltaTime;
        if (timeSinceUpdateSent > 0.05) {
            timeSinceUpdateSent = 0;
            client->Write(GameUpdatePacket(gm->GetPlayerOne(), 0).Serialize());
        }
    }

    delete gm;
    gm = nullptr;
}

void ReadCallback(std::array<char, 1024>& buf) {
    std::array<char, 1024> tmpBuf;
    switch (*(PacketType*)buf.data()) {
        case ACKNOWLEDGEMENT:
            //responseQueue.push("Acknowldgement packet received.");
            break;

        case (ECHOSEND):
            responseQueue.push("Echo send packet received. Message: " + EchoSendPacket(buf).message);
            client->Write(EchoResponsePacket(EchoSendPacket(buf).message).Serialize());
            break;

        case (ECHORESPONSE):
            responseQueue.push("Echo response packet received. Message: " + EchoResponsePacket(buf).message);
            break;

        case (GAMEUPDATE):
            responseQueue.push("Game update packet received.");
            if (gm != nullptr) {
                GameUpdatePacket packet(buf);
                gm->GetPlayerTwo() = packet.player;
                for (int i = 0; i < packet.player.correctSinceUpdate-1; i++)
                    gm->playerTwoQuestions.pop();

                if (packet.player.correctSinceUpdate) 
                    gm->playerTwoQuestions.front() = packet.player.lastQuestionAnswered;

                gm->countdownToStart = packet.countdown;
            }
            break;

        case (GAMESTART): {
            responseQueue.push("Game start packet received.");
            GameStartPacket packet(buf);
            strcpy(opponentName, packet.opponentName);
            opponentGamesWon = packet.opponentGamesWon;
            gm = new GameManager();
            for (auto question : packet.questions)
                gm->QueueQuestion(question);
            std::thread* gameThread = new std::thread(GameLoop);

        } break;

        case (GAMEEND): {
            responseQueue.push("Game end packet received.");
            GameEndPacket packet(buf);
            switch (packet.reason) {
                case WON:
                    responseQueue.push("You won.");
                case LOST:
                    responseQueue.push("You lost.");
                case GAME_NOT_FOUND:
                    responseQueue.push("You tied.");
            }
            
            if (gm != nullptr) {
                shouldDeleteGame = true;
            }
        } break;

        case (LOGINRESPONSE): {
            responseQueue.push("Login response packet received.");
            LoginResponsePacket packet(buf);
            if (packet.success) {
                responseQueue.push("Login successful.");
                loginResult = 1;
                strcpy(userName, packet.username);
                gamesPlayed = packet.gamesPlayed;
                gamesWon = packet.gamesWon;
                highScore = packet.highScore;
            } else {
                responseQueue.push("Login failed.");
                loginResult = -1;
            }
            break;
        }

        case (QUEUE):
            responseQueue.push("Queue packet received.");
            queueResponse = true;
            break;

        
        default:
            responseQueue.push("Unknown packet received.");
            break;
    }
}

std::string Input(std::string prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

int main(int argc, char** argv) {
    try {
        // if (argc != 3) {
        //     std::cerr << "Usage: client <host> <port>\n";
        //     return 1;
        // }
        
        asio::io_context io_context;
        client = new TCPClient(io_context, (char*)"127.0.0.1", (char*)"4347", &ReadCallback);
        //client = new TCPClient(io_context, argv[1], argv[2], &ReadCallback);
        client->start();
        std::thread t([&io_context]() { io_context.run(); });

        std::array<char, 1024> buf;
        while (loginResult < 1) {
            std::string groupCode = Input("Enter your group code: ");
            std::string hashedPassword = sha256(Input("Enter your password: "));
            LoginPacket loginPacket(hashedPassword, groupCode);
            client->Write(loginPacket.Serialize());
            while (loginResult == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (loginResult == -1) {
                std::cout << "Login failed. Try again.\n";
                loginResult = 0;
            }
        }

        //client->Write(QueuePacketInstance.Serialize().data());

        float timeSinceUpdateSent = 0;
        bool queueing = false;
        while (true) {
            while (responseQueue.size()) {
                std::cout << responseQueue.front() << '\n';
                responseQueue.pop();
            }

            Packet* packetToSend = nullptr;
            if (gm != nullptr) {
                
            } else if (!queueing) {
                std::string input = Input("Enter a message to send (\"queue\" to queue): ");
                if (input == "queue") {
                    packetToSend = new QueuePacket();
                    queueing = true;
                } else packetToSend = new EchoSendPacket(input);
            } else if (queueResponse) {
                packetToSend = new CheckQueuePacket();
            }
            
            if (packetToSend) {
                client->Write(packetToSend->Serialize());
                delete packetToSend;
            }
            if (gm == nullptr)
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            else
                std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
