#ifndef SRC_COMMON_PACKET
#define SRC_COMMON_PACKET
#include <cstring>
#include <string>
#include <array>
#include "../MainGame/GameManager.hpp"

enum PacketType {
    ACKNOWLEDGEMENT = 0,
    LOGIN = 1,
    QUEUE = 2,
    ECHOSEND = 3,
    ECHORESPONSE = 4,
    GAMEUPDATE = 5,
    GAMESTART = 6,
    GAMEEND = 7, // TODO
    LOGINRESPONSE = 8,
    CHECKQUEUE = 9,
    QUEUERESPONSE = 10,
};

class Packet {
public:
    Packet(PacketType type) : type_(type) {}
    virtual ~Packet() = default;
    virtual std::array<char, 1024> Serialize() = 0;
    virtual void Deserialize(std::array<char, 1024>& buf) = 0;
    
protected:
    PacketType type_;
};

class AcknowledgementPacket : public Packet {
public:
    AcknowledgementPacket() : Packet(ACKNOWLEDGEMENT) {}

    AcknowledgementPacket(std::array<char, 1024> buf) : Packet(ACKNOWLEDGEMENT) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
    }
};

class LoginPacket : public Packet {
public:
    LoginPacket(std::string hash, std::string groupCode) : Packet(LOGIN), hash(hash), groupCode(groupCode) {}

    LoginPacket(std::array<char, 1024> buf) : Packet(LOGIN) {
        Deserialize(buf);
    }
    
    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), hash.data(), hash.size());
        memcpy(buf.data() + sizeof(PacketType) + hash.size(), groupCode.data(), groupCode.size());

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        hash.resize(64);
        memcpy(hash.data(), buf.data() + sizeof(PacketType), 64);
        groupCode.resize(32);
        memcpy(groupCode.data(), buf.data() + sizeof(PacketType) + 64, 32);
        groupCode.resize(strlen(groupCode.data()));
    }

    std::string hash;
    std::string groupCode;
};

class QueuePacket : public Packet {
public:
    QueuePacket() : Packet(QUEUE) {}

    QueuePacket(std::array<char, 1024> buf) : Packet(QUEUE) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
    }
};

class EchoSendPacket : public Packet {
public:
    EchoSendPacket(std::string message) : Packet(ECHOSEND), message(message) {
        length = message.size();
    }

    EchoSendPacket(std::array<char, 1024>& buf) : Packet(ECHOSEND) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), &length, sizeof(length));
        memcpy(buf.data() + sizeof(PacketType) + sizeof(length), message.data(), message.size());

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        memcpy(&length, buf.data() + sizeof(PacketType), sizeof(length));
        buf.at(sizeof(PacketType) + sizeof(length)+1) = '\0';
        message = (char*)(buf.data() + sizeof(PacketType) + sizeof(length));
    }

    int length;
    std::string message;
};

class EchoResponsePacket : public Packet {
public:
    EchoResponsePacket(std::string message) : Packet(ECHORESPONSE), message(message) {
        length = message.size();
    }

    EchoResponsePacket(std::array<char, 1024>& buf) : Packet(ECHOSEND) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), &length, sizeof(length));
        memcpy(buf.data() + sizeof(PacketType) + sizeof(length), message.data(), message.size());

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        memcpy(&length, buf.data() + sizeof(PacketType), sizeof(length));
        buf.at(sizeof(PacketType) + sizeof(length) + length + 1) = '\0';
        message = (char*)(buf.data() + sizeof(PacketType) + sizeof(length));
    }

    int length;
    std::string message;
};

class GameUpdatePacket : public Packet {
public:
    GameUpdatePacket(Player player, float countdown) : Packet(GAMEUPDATE), player(player), countdown(countdown) {}

    GameUpdatePacket(std::array<char, 1024>& buf) : Packet(GAMEUPDATE) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        int dataOffset = 0;

        memcpy(buf.data(), &type_, sizeof(PacketType));
        dataOffset += sizeof(PacketType);
        memcpy(buf.data() + dataOffset, &player, (sizeof(player.streak)*5)+(sizeof(player.baseSpeed)*4));
        dataOffset += (sizeof(player.streak)*5)+(sizeof(player.baseSpeed)*4);
        int speedMultSize = player.speedMultipliers.size();
        memcpy(buf.data() + dataOffset, &speedMultSize, sizeof(speedMultSize));
        dataOffset += sizeof(speedMultSize);
        memcpy(buf.data() + dataOffset, player.speedMultipliers.data(), sizeof(player.speedMultipliers[0])*speedMultSize);
        dataOffset += sizeof(player.speedMultipliers[0])*speedMultSize;
        memcpy(buf.data() + dataOffset, player.lastQuestionAnswered.Serialize().data(), player.lastQuestionAnswered.DataSize());
        dataOffset += player.lastQuestionAnswered.DataSize();
        memcpy(buf.data() + dataOffset, &player.crashed, sizeof(player.crashed)*2+sizeof(player.rect));
        dataOffset += sizeof(player.crashed)*2+sizeof(player.rect);
        memcpy(buf.data() + dataOffset, &countdown, sizeof(countdown));
        
        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        int dataOffset = 0;
        
        memcpy(&type_, buf.data(), sizeof(PacketType));
        dataOffset += sizeof(PacketType);
        memcpy(&player, buf.data() + dataOffset, (sizeof(player.streak)*5)+(sizeof(player.baseSpeed)*4));
        dataOffset += (sizeof(player.streak)*5)+(sizeof(player.baseSpeed)*4);
        int speedMultSize;
        memcpy(&speedMultSize, buf.data() + dataOffset, sizeof(speedMultSize));
        dataOffset += sizeof(speedMultSize);
        player.speedMultipliers.resize(speedMultSize);
        memcpy(player.speedMultipliers.data(), buf.data() + dataOffset, sizeof(player.speedMultipliers[0])*speedMultSize);
        dataOffset += sizeof(player.speedMultipliers[0])*speedMultSize;
        player.lastQuestionAnswered = Question();
        std::array<char, 64> questionBuf;
        memcpy(questionBuf.data(), buf.data() + dataOffset, 64);
        player.lastQuestionAnswered.Deserialize(questionBuf);
        dataOffset += player.lastQuestionAnswered.DataSize();
        memcpy(&player.crashed, buf.data() + dataOffset, sizeof(player.crashed)*2+sizeof(player.rect));
        dataOffset += sizeof(player.crashed)*2+sizeof(player.rect);
        memcpy(&countdown, buf.data() + dataOffset, sizeof(countdown));
    }

    Player player;
    float countdown = 0;
};

class GameStartPacket : public Packet {
public:
    GameStartPacket(char opponentName[20], int opponentGamesWon, std::vector<Question> questions) : Packet(GAMESTART) {
        memcpy(this->opponentName, opponentName, sizeof(char)*20);
        this->opponentGamesWon = opponentGamesWon;
        this->questions = questions;
    }

    GameStartPacket(std::array<char, 1024>& buf) : Packet(GAMESTART) {
        Deserialize(buf);
    }
    
    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), &opponentName, sizeof(char)*20);
        memcpy(buf.data() + sizeof(PacketType) + sizeof(char)*20, &opponentGamesWon, sizeof(int));
        int dataOffset = sizeof(PacketType) + sizeof(char)*20 + sizeof(int);
        int numQuestions = questions.size();
        memcpy(buf.data() + dataOffset, &numQuestions, sizeof(numQuestions));
        dataOffset += sizeof(numQuestions);
        for (int i = 0; i < numQuestions; i++) {
            memcpy(buf.data() + dataOffset, questions[i].Serialize().data(), questions[i].DataSize());
            dataOffset += questions[i].DataSize();
        }

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        memcpy(&opponentName, buf.data() + sizeof(PacketType), sizeof(char)*20);
        memcpy(&opponentGamesWon, buf.data() + sizeof(PacketType) + sizeof(char)*20, sizeof(int));
        int dataOffset = sizeof(PacketType) + sizeof(char)*20 + sizeof(int);
        int numQuestions;
        memcpy(&numQuestions, buf.data() + dataOffset, sizeof(numQuestions));
        dataOffset += sizeof(numQuestions);
        for (int i = 0; i < numQuestions; i++) {
            Question question;
            std::array<char, 64> questionBuf;
            memcpy(questionBuf.data(), buf.data() + dataOffset, 64);
            question.Deserialize(questionBuf);
            questions.push_back(question);
            dataOffset += question.DataSize();
        }
    }

    char opponentName[20];
    int opponentGamesWon;
    std::vector<Question> questions;
};

enum GameEndReason {
    WON,
    LOST,
    GAME_NOT_FOUND,
    NONE // Only used to initialize and mark as not ended
};

class GameEndPacket : public Packet {
public:
    GameEndPacket(GameEndReason reason) : Packet(GAMEEND) {
        this->reason = reason;
    }

    GameEndPacket(std::array<char, 1024>& buf) : Packet(GAMEEND) {
        Deserialize(buf);
    }


    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), &reason, sizeof(bool));

        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        memcpy(&reason, buf.data() + sizeof(PacketType), sizeof(GameEndReason));
    }

    GameEndReason reason;
};

class LoginResponsePacket : public Packet {
public:
    LoginResponsePacket(bool success, char username[20], int gamesPlayed, int gamesWon, int highScore) : Packet(LOGINRESPONSE) {
        this->success = success;
        memcpy(this->username, username, sizeof(char)*20);
        this->gamesPlayed = gamesPlayed;
        this->gamesWon = gamesWon;
        this->highScore = highScore;
    }

    LoginResponsePacket(std::array<char, 1024>& buf) : Packet(LOGINRESPONSE) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        memcpy(buf.data() + sizeof(PacketType), &success, sizeof(bool)+sizeof(char)*20+sizeof(int)*3);
        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
        memcpy(&success, buf.data() + sizeof(PacketType), sizeof(bool)+sizeof(char)*20+sizeof(int)*3);
    }

    bool success;
    char username[20];
    int gamesPlayed = 0, gamesWon = 0, highScore = 0;
};

class CheckQueuePacket : public Packet {
public:
    CheckQueuePacket() : Packet(CHECKQUEUE) {}

    CheckQueuePacket(std::array<char, 1024>& buf) : Packet(CHECKQUEUE) {
        Deserialize(buf);
    }

    std::array<char, 1024> Serialize() override {
        std::array<char, 1024> buf;
        buf.fill(0);

        memcpy(buf.data(), &type_, sizeof(PacketType));
        return buf;
    }

    void Deserialize(std::array<char, 1024>& buf) override {
        memcpy(&type_, buf.data(), sizeof(PacketType));
    }
};

#endif /* SRC_COMMON_PACKET */
