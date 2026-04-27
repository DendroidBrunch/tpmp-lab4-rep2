#include "bonus.h"
#include "database.h"
#include "trip.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

double BonusCalculator::calculateCrewCatch(int crew_id, 
                                            const std::string& start_date,
                                            const std::string& end_date) {
    std::string crew_sql = "SELECT trawler_id FROM CrewMember WHERE id = " + std::to_string(crew_id) + ";";
    int trawler_id = -1;
    
    auto callback1 = [](void* data, int argc, char** argv, char** colName) -> int {
        int* tid = static_cast<int*>(data);
        if (argv[0]) *tid = std::stoi(argv[0]);
        return 0;
    };
    
    g_db->query(crew_sql, callback1, &trawler_id);
    
    if (trawler_id == -1) return 0;
    
    std::string sql = "SELECT SUM(fc.quantity) "
                      "FROM FishCatch fc "
                      "JOIN Trip t ON fc.trip_id = t.id "
                      "WHERE t.trawler_id = " + std::to_string(trawler_id) +
                      " AND t.departure_date >= '" + start_date + 
                      "' AND t.return_date <= '" + end_date + "';";
    
    double total = 0;
    
    auto callback2 = [](void* data, int argc, char** argv, char** colName) -> int {
        double* total = static_cast<double*>(data);
        if (argv[0]) *total = std::stod(argv[0]);
        return 0;
    };
    
    g_db->query(sql, callback2, &total);
    return total;
}

bool BonusCalculator::insertBonus(int crew_id, double amount, 
                                   const std::string& start_date,
                                   const std::string& end_date,
                                   const std::string& description) {
    std::string sql = "INSERT INTO Bonus (crew_id, amount, period_start, period_end, description) "
                      "VALUES (" + std::to_string(crew_id) + ", " +
                      std::to_string(amount) + ", '" +
                      start_date + "', '" +
                      end_date + "', '" +
                      description + "');";
    return g_db->execute(sql);
}

int BonusCalculator::calculateBonuses(const std::string& start_date,
                                       const std::string& end_date,
                                       double plan,
                                       double price) {
    std::string crew_sql = "SELECT id, surname, trawler_id FROM CrewMember;";
    
    std::vector<std::tuple<int, std::string, int>> crew;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* vec = static_cast<std::vector<std::tuple<int, std::string, int>>*>(data);
        if (argc >= 3 && argv[0] && argv[1] && argv[2]) {
            vec->push_back({std::stoi(argv[0]), argv[1], std::stoi(argv[2])});
        }
        return 0;
    };
    
    g_db->query(crew_sql, callback, &crew);
    
    int bonuses_given = 0;
    
    for (const auto& [crew_id, surname, trawler_id] : crew) {
        double catch_total = calculateCrewCatch(crew_id, start_date, end_date);
        
        if (catch_total > plan) {
            double over_plan = catch_total - plan;
            double bonus = over_plan * price * 0.05;
            
            std::string description = "Премия за внеплановый улов: " + 
                                      std::to_string(over_plan) + " кг сверх плана";
            
            if (insertBonus(crew_id, bonus, start_date, end_date, description)) {
                bonuses_given++;
                std::cout << "Начислена премия " << surname << ": " << bonus << " руб." << std::endl;
            }
        }
    }
    
    return bonuses_given;
}

bool BonusCalculator::calculateBonusForCrew(int crew_id,
                                             const std::string& start_date,
                                             const std::string& end_date,
                                             double plan,
                                             double price) {
    std::string sql = "SELECT id, surname, position, hire_date, birth_year, trawler_id "
                      "FROM CrewMember WHERE id = " + std::to_string(crew_id) + ";";
    
    int id = -1;
    std::string surname;
    std::string position;
    std::string hire_date;
    int birth_year = 0;
    int trawler_id = -1;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* id_ptr = static_cast<int*>(data);
        auto* surname_ptr = static_cast<std::string*>(data + sizeof(int));
        auto* position_ptr = static_cast<std::string*>(data + sizeof(int) + sizeof(std::string));
        auto* hire_date_ptr = static_cast<std::string*>(data + sizeof(int) + 2*sizeof(std::string));
        auto* birth_year_ptr = static_cast<int*>(data + sizeof(int) + 3*sizeof(std::string));
        auto* trawler_id_ptr = static_cast<int*>(data + 2*sizeof(int) + 3*sizeof(std::string));
        
        *id_ptr = std::stoi(argv[0]);
        *surname_ptr = argv[1];
        *position_ptr = argv[2];
        *hire_date_ptr = argv[3];
        *birth_year_ptr = std::stoi(argv[4]);
        *trawler_id_ptr = std::stoi(argv[5]);
        return 0;
    };
    
    // Выделяем память для данных
    struct CallbackData {
        int id;
        std::string surname;
        std::string position;
        std::string hire_date;
        int birth_year;
        int trawler_id;
    } cbData = {-1, "", "", "", 0, -1};
    
    g_db->query(sql, callback, &cbData);
    
    if (cbData.id == -1) {
        std::cout << "Член команды с ID " << crew_id << " не найден!" << std::endl;
        return false;
    }
    
    double catch_total = calculateCrewCatch(crew_id, start_date, end_date);
    
    if (catch_total > plan) {
        double over_plan = catch_total - plan;
        double bonus = over_plan * price * 0.05;
        
        std::string description = "Персональная премия за внеплановый улов: " + 
                                  std::to_string(over_plan) + " кг сверх плана";
        
        return insertBonus(crew_id, bonus, start_date, end_date, description);
    } else {
        std::cout << "Улов члена команды (" << catch_total << " кг) не превышает план (" 
                  << plan << " кг)" << std::endl;
        return false;
    }
}

std::vector<Bonus> BonusCalculator::getAllBonuses() {
    std::vector<Bonus> bonuses;
    std::string sql = "SELECT b.id, b.crew_id, b.amount, b.period_start, b.period_end, b.description "
                      "FROM Bonus b ORDER BY b.id;";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* vec = static_cast<std::vector<Bonus>*>(data);
        Bonus bonus;
        bonus.id = std::stoi(argv[0]);
        bonus.crew_id = std::stoi(argv[1]);
        bonus.amount = std::stod(argv[2]);
        bonus.period_start = argv[3] ? argv[3] : "";
        bonus.period_end = argv[4] ? argv[4] : "";
        bonus.description = argv[5] ? argv[5] : "";
        vec->push_back(bonus);
        return 0;
    };
    
    g_db->query(sql, callback, &bonuses);
    return bonuses;
}

void BonusCalculator::printBonuses() {
    auto bonuses = getAllBonuses();
    
    Utils::printHeader("СПИСОК ПРЕМИЙ");
    Utils::printSeparator();
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(8) << "Crew ID"
              << std::setw(12) << "Сумма"
              << std::setw(15) << "Период"
              << "Описание" << std::endl;
    Utils::printSeparator();
    
    for (const auto& b : bonuses) {
        std::string period = b.period_start + " - " + b.period_end;
        std::cout << std::left << std::setw(5) << b.id
                  << std::setw(8) << b.crew_id
                  << std::setw(12) << b.amount
                  << std::setw(15) << period.substr(0, 15)
                  << (b.description.length() > 40 ? b.description.substr(0, 40) + "..." : b.description) << std::endl;
    }
    Utils::printSeparator();
}
