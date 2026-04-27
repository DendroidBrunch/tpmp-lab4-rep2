#include "statistics.h"
#include "database.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

double StatisticsService::calculateTotalCatch(int trawler_id) {
    std::string sql = "SELECT SUM(fc.quantity) as total "
                      "FROM FishCatch fc "
                      "JOIN Trip t ON fc.trip_id = t.id "
                      "WHERE t.trawler_id = " + std::to_string(trawler_id) + ";";
    
    double total = 0;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        double* total = static_cast<double*>(data);
        if (argv[0]) {
            *total = std::stod(argv[0]);
        }
        return 0;
    };
    
    g_db->query(sql, callback, &total);
    return total;
}

bool StatisticsService::updateStatistics(int trawler_id) {
    double total = calculateTotalCatch(trawler_id);
    
    std::string check = "SELECT COUNT(*) FROM Statistics WHERE trawler_id = " + std::to_string(trawler_id) + ";";
    int exists = 0;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        int* exists = static_cast<int*>(data);
        *exists = std::stoi(argv[0]);
        return 0;
    };
    
    g_db->query(check, callback, &exists);
    
    std::string sql;
    if (exists) {
        sql = "UPDATE Statistics SET total_catch = " + std::to_string(total) +
              " WHERE trawler_id = " + std::to_string(trawler_id) + ";";
    } else {
        sql = "INSERT INTO Statistics (trawler_id, total_catch) VALUES (" +
              std::to_string(trawler_id) + ", " + std::to_string(total) + ");";
    }
    
    return g_db->execute(sql);
}

double StatisticsService::getTotalCatch(int trawler_id) {
    std::string sql = "SELECT total_catch FROM Statistics WHERE trawler_id = " + std::to_string(trawler_id) + ";";
    
    double total = 0;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        double* total = static_cast<double*>(data);
        if (argv[0]) {
            *total = std::stod(argv[0]);
        }
        return 0;
    };
    
    g_db->query(sql, callback, &total);
    return total;
}

void StatisticsService::printAllStatistics() {
    std::string sql = "SELECT tr.name, s.total_catch "
                      "FROM Statistics s "
                      "JOIN Trawler tr ON s.trawler_id = tr.id "
                      "ORDER BY s.total_catch DESC;";
    
    Utils::printHeader("СТАТИСТИКА УЛОВОВ ПО ТРАУЛЕРАМ");
    Utils::printSeparator();
    std::cout << std::left << std::setw(30) << "Название траулера"
              << std::right << std::setw(15) << "Общий улов (кг)" << std::endl;
    Utils::printSeparator();
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        std::cout << std::left << std::setw(30) << (argv[0] ? argv[0] : "")
                  << std::right << std::setw(15) << (argv[1] ? argv[1] : "0") << std::endl;
        return 0;
    };
    
    g_db->query(sql, callback, nullptr);
    Utils::printSeparator();
}

void setupStatisticsTriggers(sqlite3* db) {
    const char* trigger_sql = 
        "CREATE TRIGGER IF NOT EXISTS update_statistics_on_insert "
        "AFTER INSERT ON FishCatch "
        "BEGIN "
        "    UPDATE Statistics SET total_catch = ("
        "        SELECT SUM(fc.quantity) FROM FishCatch fc "
        "        JOIN Trip t ON fc.trip_id = t.id "
        "        WHERE t.trawler_id = (SELECT trawler_id FROM Trip WHERE id = NEW.trip_id)"
        "    ) WHERE trawler_id = (SELECT trawler_id FROM Trip WHERE id = NEW.trip_id); "
        "END;";
    
    char* errMsg;
    sqlite3_exec(db, trigger_sql, nullptr, nullptr, &errMsg);
    if (errMsg) {
        std::cerr << "Trigger error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}
