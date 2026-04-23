#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class Database {
private:
    sqlite3* db;
    std::string db_path;
    
public:
    Database(const std::string& path);
    ~Database();
    
    bool open();
    void close();
    bool execute(const std::string& sql);
    bool executeScript(const std::string& scriptPath);  
    bool query(const std::string& sql, 
               int (*callback)(void*, int, char**, char**), 
               void* data);
    
    class Statement {
    private:
        sqlite3_stmt* stmt;
    public:
        Statement(sqlite3* db, const std::string& sql);
        ~Statement();
        bool step();
        void bind(int pos, int value);
        void bind(int pos, const std::string& value);
        void bind(int pos, double value);
        void bind(int pos, const char* value);
        int getInt(int col);
        double getDouble(int col);
        std::string getString(int col);
        bool isValid() const { return stmt != nullptr; }
    };
    
    std::unique_ptr<Statement> prepare(const std::string& sql);
    
    bool beginTransaction();
    bool commit();
    bool rollback();
    
    sqlite3* getHandle() { return db; }
};

extern Database* g_db;

#endif
