#ifndef SRC_SERVER_DATABASEWRAPPER
#define SRC_SERVER_DATABASEWRAPPER
#include <iostream>
#include <string>
#include <sstream>
#include <queue>
#include <sqlite3.h>

std::queue<std::string> outputQueue;

struct Student {
    bool success = false;
    std::string name;
    int gamesPlayed, gamesWon, highScore;
    std::string groupID;
};

class DatabaseWrapper {
public:
    DatabaseWrapper(std::string databaseName) {
        if (sqlite3_open(databaseName.c_str(), &db)) std::cout << "Error opening database: " << sqlite3_errmsg(db) << '\n';

        if (sqlite3_exec(db,
            "CREATE TABLE IF NOT EXISTS groups ("
                "groupID CHAR(32) PRIMARY KEY UNIQUE"
            ");", NULL, NULL, NULL)) std::cout << "Error creating table: " << sqlite3_errmsg(db) << '\n';
    }

    ~DatabaseWrapper() {
        sqlite3_close(db);
    }

    bool CreateGroup(std::string groupID) {
        std::string query = "INSERT INTO groups (groupID) VALUES ('" + groupID + "');";
        outputQueue.push("Query: " + query);
        if (Execute(query)) return true;
        std::stringstream ss;
        ss << "CREATE TABLE " << groupID << "students" << " ("
                "userHash CHAR(64) PRIMARY KEY,"
                "userName CHAR(20) NOT NULL UNIQUE,"
                "gamesPlayed INT DEFAULT 0,"
                "gamesWon INT DEFAULT 0,"
                "highScore INT DEFAULT 0,"
                "groupID CHAR(32) DEFAULT " + groupID + ","
                "FOREIGN KEY (groupID) REFERENCES groups( " + groupID + ")"
                ");";

        return Execute(ss.str());
    }

    bool AddStudent(std::string groupID, std::string userHash, std::string userName) {
        std::string query = "INSERT INTO " + groupID + "students (userHash, userName) VALUES ('" + userHash + "', '" + userName + "');";
        return Execute(query);
    }

    bool DeleteStudent(std::string groupID, std::string userHash) {
        std::string query = "DELETE FROM " + groupID + "students WHERE userHash = '" + userHash + "';";
        return Execute(query);
    }

    Student QueryStudent(std::string groupID, std::string userHash) {
        std::string query = "SELECT * FROM " + groupID + "students WHERE userHash = '" + userHash + "';";
        outputQueue.push("Query: " + query);
        sqlite3_stmt* stmt;
        Student student;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL)) {
            outputQueue.push("Error preparing query: " + std::string(sqlite3_errmsg(db)));
            return student;
        }

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            student.success = true;
            student.name = (char*)sqlite3_column_text(stmt, 1);
            student.gamesPlayed = sqlite3_column_int(stmt, 2);
            student.gamesWon = sqlite3_column_int(stmt, 3);
            student.highScore = sqlite3_column_int(stmt, 4);
            student.groupID = (char*)sqlite3_column_text(stmt, 5);
        }

        sqlite3_finalize(stmt);
        return student;
    }

    std::vector<std::string> QueryGroups() {
        std::string query = "SELECT * FROM groups;";
        outputQueue.push("Query: " + query);
        sqlite3_stmt* stmt;
        std::vector<std::string> groups;
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, NULL)) {
            outputQueue.push("Error preparing query: " + std::string(sqlite3_errmsg(db)));
            return groups;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            groups.push_back((char*)sqlite3_column_text(stmt, 0));
        }

        sqlite3_finalize(stmt);
        return groups;
    }

    bool DeleteGroup(std::string groupID) {
        std::string query = "DELETE FROM groups WHERE groupID = '" + groupID + "';";
        return Execute(query);
    }

    bool GroupExists(std::string groupID) {
        std::string query = "SELECT * FROM groups WHERE groupID = '" + groupID + "';";
        return Execute(query);
    }

    

    bool Execute(std::string query) {
        if (sqlite3_exec(db, query.c_str(), NULL, NULL, NULL)) {
            std::cout << "Error executing query: " << sqlite3_errmsg(db) << '\n';
            return true;
        }
        return false;
    }
    
    sqlite3* db;
};

#endif /* SRC_SERVER_DATABASEWRAPPER */
