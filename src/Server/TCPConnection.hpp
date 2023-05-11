#ifndef SRC_SERVER_TCPCONNECTION
#define SRC_SERVER_TCPCONNECTION
#include <iostream>
#include <thread>
#include <unordered_map>
#include <queue>
#include <asio.hpp>
#include <sqlite3.h>
#define GAME_SERVER
#include "../Common/Packet.hpp"
#include "../MainGame/GameManager.hpp"
#include "DatabaseWrapper.hpp"

class TCPConnection;

DatabaseWrapper* db;
std::unordered_map<std::string, std::queue<TCPConnection*>*> queueingForGame;
std::queue<TCPConnection*> deadConnections;
std::unordered_map<std::string, GameManager*> games;

class TCPConnection {
public:
	TCPConnection(asio::io_context& io_context) : socket_(io_context) { }

	asio::ip::tcp::socket& socket() {
		return socket_;
	}

	void start() {
		started = true;
		gateway = socket_.remote_endpoint().address().to_string() + ':' + std::to_string(socket_.remote_endpoint().port());
		asio::error_code error;
		buf_ = std::array<char, 1024>();

		this->isDead = false;

		socket_.async_read_some(asio::buffer(buf_, 1024),
        	std::bind(&TCPConnection::HandleRead, this,
          		std::placeholders::_1));
	}

	void Print(std::string str) {
		outputQueue.push('[' + gateway + "] : " + str);
	}

	bool isDead = false, started = false, isQueuing = false, playerOne = false, gameSent = false, markedForDeletion = false;
	std::string gateway, gameID = "", userName, userHash, groupCode;
	int gamesPlayed, gamesWon, highScore;

private:
	void HandleWrite(const asio::error_code& error){
		if (!error) {
			socket_.async_read_some(asio::buffer(buf_, 1024),
				std::bind(&TCPConnection::HandleRead, this,
					std::placeholders::_1));
		} else ErrorHandler(error);
	}

	void HandleRead(const asio::error_code& error) {
		if (!error) {
			ParseData();
		} else ErrorHandler(error);

		asio::async_write(socket_, asio::buffer(buf_, 1024),
			std::bind(&TCPConnection::HandleWrite, this,
					std::placeholders::_1));
	}

	void ParseData() {
		PacketType packetType = *(PacketType*)buf_.data();
		socket_.wait(socket_.wait_write);
		Packet* response = new AcknowledgementPacket();

		switch (packetType) {
			case (ACKNOWLEDGEMENT): {
				Print("Acknowledgement recieved.");
			} break;

			case (LOGIN): {
				LoginPacket packet = LoginPacket(buf_);
				Print("Login packet recieved. Group code: " + packet.groupCode + " Hash: " + packet.hash);
				Student student = db->QueryStudent(packet.groupCode, packet.hash);
				if (student.success) {
					userName = student.name;
					userHash = packet.hash;
					groupCode = packet.groupCode;
					delete response;
					response = new LoginResponsePacket(true, (char*)student.name.c_str(), student.gamesPlayed, student.gamesWon, student.highScore);
				} else {
					delete response;
					response = new LoginResponsePacket(false, (char*)"", 0, 0, 0);
				}
			} break;

			case (QUEUE): {
				Print("Queue packet received.");
				queueingForGame[groupCode]->push(this);
				isQueuing = true;
				delete response;
				response = new QueuePacket();
			} break;

			case (ECHOSEND): {
				Print("Echo packet received. Message: " + EchoSendPacket(buf_).message);
				delete response;
				response = new EchoResponsePacket(EchoSendPacket(buf_).message);
			} break;

			case (GAMEUPDATE): {
				Print("Game update packet received.");
				GameUpdatePacket packet = GameUpdatePacket(buf_);
				GameManager* game;
				try {
					game = games.at(gameID);
				} catch (std::out_of_range) {
					delete response;
					response = new GameEndPacket(GAME_NOT_FOUND);
					break;
				}
				
				Player& player = playerOne ? game->GetPlayerOne() : game->GetPlayerTwo();
				player = packet.player;
				for (int i = 0; i < packet.player.correctSinceUpdate-1; i++) {
					if (playerOne) game->playerOneQuestions.pop();
					else game->playerTwoQuestions.pop();
				} 
				if (playerOne) { if (game->playerOneQuestions.size()) game->playerTwoQuestions.front() = packet.player.lastQuestionAnswered; }
				else if (game->playerTwoQuestions.size()) game->playerOneQuestions.front() = packet.player.lastQuestionAnswered;

				delete response;
				if (playerOne) response = new GameUpdatePacket(game->GetPlayerTwo(), game->countdownToStart);
				else response = new GameUpdatePacket(game->GetPlayerOne(), game->countdownToStart);
			} break;

			case (CHECKQUEUE):
				if (gameID != "" && !gameSent) {
					delete response;
	
					if (playerOne) response = new GameStartPacket((char*)games[gameID]->playerTwoName.c_str(), games[gameID]->playerTwoGamesWon, GenerateQuestions(10));
					else response = new GameStartPacket((char*)games[gameID]->playerOneName.c_str(), games[gameID]->playerOneGamesWon, GenerateQuestions(10));
					gameSent = true;
				}
				break;

			default: {
				Print("Unknown data received.");
			} break;
		}
		buf_.fill(0);
		memcpy(buf_.data(), response->Serialize().data(), 1024);
		delete response;
	}

	void ErrorHandler(const asio::error_code& error) {
		if (error == asio::error::eof) {
			Print("Client disconnected safely!");
		} else std::cout << error.message() << '\n';

		this->isDead = true;
		if (!markedForDeletion) deadConnections.push(this);
		markedForDeletion = true;
	}
	
	asio::ip::tcp::socket socket_;
	std::array<char, 1024> buf_;
};

#endif /* SRC_SERVER_TCPCONNECTION */
