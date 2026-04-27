#include "database.h"
#include <iostream>
#include <fstream>

// Удалите эту строку:
// Database* g_db = nullptr;

Database::Database(const std::string& path) : db_path(path), db(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open() {
    int rc = sqlite3_open(db_path.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

void Database::close() {
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::executeScript(const std::string& scriptPath) {
    std::ifstream file(scriptPath);
    if (!file.is_open()) {
        std::cerr << "Cannot open script: " << scriptPath << std::endl;
        return false;
    }
    
    std::string sql((std::istreambuf_iterator<char>(file)), 
                     std::istreambuf_iterator<char>());
    file.close();
    
    return execute(sql);
}

bool Database::query(const std::string& sql, 
                     int (*callback)(void*, int, char**, char**), 
                     void* data) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), callback, data, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Query error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

Database::Statement::Statement(sqlite3* db, const std::string& sql) {
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
}

Database::Statement::~Statement() {
    if (stmt) sqlite3_finalize(stmt);
}

bool Database::Statement::step() {
    return sqlite3_step(stmt) == SQLITE_ROW;
}

void Database::Statement::bind(int pos, int value) {
    sqlite3_bind_int(stmt, pos, value);
}

void Database::Statement::bind(int pos, const std::string& value) {
    sqlite3_bind_text(stmt, pos, value.c_str(), -1, SQLITE_TRANSIENT);
}

void Database::Statement::bind(int pos, double value) {
    sqlite3_bind_double(stmt, pos, value);
}

void Database::Statement::bind(int pos, const char* value) {
    sqlite3_bind_text(stmt, pos, value, -1, SQLITE_TRANSIENT);
}

int Database::Statement::getInt(int col) {
    return sqlite3_column_int(stmt, col);
}

double Database::Statement::getDouble(int col) {
    return sqlite3_column_double(stmt, col);
}

std::string Database::Statement::getString(int col) {
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
}

std::unique_ptr<Database::Statement> Database::prepare(const std::string& sql) {
    return std::make_unique<Statement>(db, sql);
}

bool Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

bool Database::commit() {
    return execute("COMMIT;");
}

bool Database::rollback() {
    return execute("ROLLBACK;");
}
