#ifndef SRC_SERVER_TCPSERVER
#define SRC_SERVER_TCPSERVER
#include <unordered_map>
#include <thread>
#include <chrono>
#include <asio.hpp>

#include "../Common/SHA256.hpp"
#include "TCPConnection.hpp"

std::string Input(std::string prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input);
    return input;
}

class TCPServer {
public:
    TCPServer(asio::io_context& io_context) : io_context_(io_context), acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), 4347)) {
        db = new DatabaseWrapper("database.db");
        
        std::thread* queueThread = new std::thread([]() {
            for (auto group : db->QueryGroups()) 
                queueingForGame.insert(std::pair(group, new std::queue<TCPConnection*>()));

            while (true) {
                while (deadConnections.size()) {
                    deadConnections.front()->socket().close();
                    if (!deadConnections.front()->isQueuing)
                        queueingForGame[deadConnections.front()->groupCode]->push(deadConnections.front());
                    deadConnections.pop();
                }

                for (auto group : queueingForGame) {
                    std::queue<TCPConnection*>* currentQueue = group.second;
                    if (currentQueue->size()) {
                        if (currentQueue->front()->isDead) {
                            if (currentQueue->front()->gameID != "") {
                                games.erase(currentQueue->front()->gameID);
                            }
                            delete currentQueue->front();
                            currentQueue->pop();
                        }

                        if (currentQueue->size() > 1) {
                            TCPConnection& playerOne = *currentQueue->front();
                            currentQueue->pop();

                            TCPConnection& playerTwo = *currentQueue->front();
                            currentQueue->pop();

                            playerOne.isQueuing = false;
                            playerTwo.isQueuing = false;

                            std::string gameID = playerOne.userName + playerTwo.userName;
                            playerOne.gameID = gameID;
                            playerTwo.gameID = gameID;
                            playerOne.playerOne = true;

                            if (playerOne.isDead || playerTwo.isDead) {
                                currentQueue->push(&playerOne);
                                currentQueue->push(&playerTwo);
                            }

                            games.insert(std::pair(gameID, new GameManager()));

                            games[gameID]->Initialize(playerOne.userName, playerTwo.userName, playerOne.gamesWon, playerTwo.gamesWon);
                        }
                    }
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });

        std::thread* gameThread = new std::thread([]() {
            while (true) {
                for (auto game : games) {
                    game.second->Tick();
                    try {
                        games.at(game.first);
                    } catch (std::out_of_range) {
                        delete game.second;
                    }
                    outputQueue.push("Ticked game: " + game.first);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(3));
            }
        });

        std::thread* inputThread = new std::thread([]() {
            bool databaseMode = false;
            while (true) {
                std::string input = Input(">> ");
                if (!databaseMode) switch (input[0]) {
                    case 'h':
                        std::cout << "Commands:\n"
                            "h - help\n"
                            "q - query outputs\n"
                            "c - clear outputs\n"
                            "d - enter database mode\n"
                            "p - query queueing players\n";
                        break;

                    case 'q':
                        std::cout << "Queued outputs:\n";
                        while (outputQueue.size()) {
                            std::cout << outputQueue.front() << '\n';
                            outputQueue.pop();
                        }
                        break;

                    case 'c':
                        while (outputQueue.size()) outputQueue.pop();
                        break;

                    case 'd':
                        databaseMode = true;
                        break;

                    case 'p':
                        std::cout << "There are " << queueingForGame.size() << " group(s) queueing for a game.\n";
                        for (auto group : queueingForGame) {
                            std::cout << "  Group " << group.first << " has " << group.second->size() << " player(s) queueing.\n";
                        } break;

                    default:
                        break;
                } else switch (input[0]) {
                    case 'h':
                        std::cout << "Commands:\n"
                            "h - help\n"
                            "q - query outputs\n"
                            "c - clear outputs\n"
                            "e - exit database mode\n"
                            "g - create new group\n"
                            "u - create new user\n";
                        break;

                    case 'q':
                        std::cout << "Queued outputs:\n";
                        while (outputQueue.size()) {
                            std::cout << outputQueue.front() << '\n';
                            outputQueue.pop();
                        }
                        break;

                    case 'c':  
                        while (outputQueue.size()) outputQueue.pop();
                        break;

                    case 'e':
                        databaseMode = false;
                        break;

                    case 'g': {
                        std::string groupID = Input("Group ID: ");
                        queueingForGame.insert(std::pair(groupID, new std::queue<TCPConnection*>()));

                        db->CreateGroup(groupID);

                    }; break;

                    case 'u': {
                        std::string userGroup = Input("User group: ");
                        std::string userHash = sha256(Input("User password: "));
                        std::string userName = Input("User name: ");

                        db->AddStudent(userGroup, userHash, userName);
                    }; break;

                    default:
                        break;
                }
            }
        });

        StartAccept();
    }

    ~TCPServer() {
        delete db;
    }

private:
    void StartAccept() {
        TCPConnection* new_connection = new TCPConnection(io_context_);

        acceptor_.async_accept(new_connection->socket(),
            std::bind(&TCPServer::HandleAccept, this, new_connection,
            std::placeholders::_1));
        std::cout << "iusdfhio";
    }

    void HandleAccept(TCPConnection* new_connection, const asio::error_code& error) {
        if (!error)
            new_connection->start();

        StartAccept();
    }

    void close() {
        acceptor_.close();
        delete this;
    }

    asio::io_context& io_context_;
    asio::ip::tcp::acceptor acceptor_;
};

#endif /* SRC_SERVER_TCPSERVER */
