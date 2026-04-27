#include "trip.h"
#include "database.h"
#include "statistics.h"
#include "utils.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

bool TripService::addTrip(const Trip& trip) {
    if (!g_db->beginTransaction()) return false;
    
    std::string sql = "INSERT INTO Trip (trawler_id, bank_id, departure_date, return_date) "
                      "VALUES (" + std::to_string(trip.trawler_id) + ", " +
                      std::to_string(trip.bank_id) + ", '" +
                      trip.departure_date + "', '" +
                      trip.return_date + "');";
    
    if (!g_db->execute(sql)) {
        g_db->rollback();
        return false;
    }
    
    int trip_id = sqlite3_last_insert_rowid(g_db->getHandle());
    
    for (const auto& catch_ : trip.catches) {
        sql = "INSERT INTO FishCatch (trip_id, fish_name, quantity, quality) "
              "VALUES (" + std::to_string(trip_id) + ", '" +
              catch_.fish_name + "', " +
              std::to_string(catch_.quantity) + ", '" +
              catch_.quality + "');";
        
        if (!g_db->execute(sql)) {
            g_db->rollback();
            return false;
        }
    }
    
    StatisticsService::updateStatistics(trip.trawler_id);
    
    return g_db->commit();
}

std::vector<Trip> TripService::getTripsByTrawler(int trawler_id, 
                                                   const std::string& start_date,
                                                   const std::string& end_date) {
    std::vector<Trip> trips;
    
    std::string sql = "SELECT t.id, t.trawler_id, t.bank_id, t.departure_date, t.return_date, "
                      "fc.fish_name, fc.quantity, fc.quality "
                      "FROM Trip t "
                      "LEFT JOIN FishCatch fc ON t.id = fc.trip_id "
                      "WHERE t.trawler_id = " + std::to_string(trawler_id) +
                      " AND t.departure_date >= '" + start_date + 
                      "' AND t.return_date <= '" + end_date + "' "
                      "ORDER BY t.departure_date;";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* trips = static_cast<std::vector<Trip>*>(data);
        
        int trip_id = std::stoi(argv[0]);
        
        auto it = std::find_if(trips->begin(), trips->end(), 
                               [trip_id](const Trip& t) { return t.id == trip_id; });
        
        if (it == trips->end()) {
            Trip newTrip;
            newTrip.id = trip_id;
            newTrip.trawler_id = std::stoi(argv[1]);
            newTrip.bank_id = std::stoi(argv[2]);
            newTrip.departure_date = argv[3];
            newTrip.return_date = argv[4];
            trips->push_back(newTrip);
            it = trips->end() - 1;
        }
        
        if (argc > 5 && argv[5]) {
            FishCatch fc;
            fc.fish_name = argv[5];
            fc.quantity = std::stod(argv[6]);
            fc.quality = argv[7];
            it->catches.push_back(fc);
        }
        
        return 0;
    };
    
    g_db->query(sql, callback, &trips);
    return trips;
}

void TripService::printTrawlerTrips(int trawler_id, const std::string& start, const std::string& end) {
    auto trips = getTripsByTrawler(trawler_id, start, end);
    
    Utils::printHeader("РЕЙСЫ ТРАУЛЕРА #" + std::to_string(trawler_id));
    std::cout << "Период: " << start << " - " << end << std::endl;
    Utils::printSeparator();
    
    double total_fish = 0;
    
    for (const auto& trip : trips) {
        std::cout << "\nРейс ID: " << trip.id 
                  << " | Банка: " << trip.bank_id
                  << " | " << trip.departure_date << " -> " << trip.return_date << std::endl;
        std::cout << "Уловы:" << std::endl;
        
        double trip_total = 0;
        for (const auto& catch_ : trip.catches) {
            std::cout << "  - " << catch_.fish_name 
                      << ": " << catch_.quantity << " кг (" << catch_.quality << ")" << std::endl;
            trip_total += catch_.quantity;
        }
        std::cout << "  Итого за рейс: " << trip_total << " кг" << std::endl;
        total_fish += trip_total;
    }
    
    Utils::printSeparator();
    std::cout << "ОБЩИЙ УЛОВ: " << total_fish << " кг" << std::endl;
    Utils::printSeparator();
}

void TripService::printBankCatches(int bank_id) {
    std::string sql = "SELECT fc.fish_name, SUM(fc.quantity) as total_qty "
                      "FROM FishCatch fc "
                      "JOIN Trip t ON fc.trip_id = t.id "
                      "WHERE t.bank_id = " + std::to_string(bank_id) + " "
                      "GROUP BY fc.fish_name "
                      "ORDER BY total_qty DESC;";
    
    Utils::printHeader("УЛОВЫ НА БАНКЕ #" + std::to_string(bank_id));
    Utils::printSeparator();
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        std::cout << std::left << std::setw(20) << argv[0]
                  << std::right << std::setw(10) << argv[1] << " кг" << std::endl;
        return 0;
    };
    
    g_db->query(sql, callback, nullptr);
    Utils::printSeparator();
}

void TripService::printMaxLowQualityBank() {
    std::string sql = "SELECT b.id, b.name, SUM(fc.quantity) as low_qty "
                      "FROM Bank b "
                      "JOIN Trip t ON b.id = t.bank_id "
                      "JOIN FishCatch fc ON t.id = fc.trip_id "
                      "WHERE fc.quality = 'низкое' "
                      "GROUP BY b.id, b.name "
                      "ORDER BY low_qty DESC LIMIT 1;";
    
    int bank_id = -1;
    std::string bank_name;
    double low_qty = 0;
    
    auto callback1 = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* id = static_cast<int*>(data);
        auto* name = static_cast<std::string*>(data + sizeof(int));
        auto* qty = static_cast<double*>(data + sizeof(int) + sizeof(std::string));
        *id = std::stoi(argv[0]);
        *name = argv[1];
        *qty = std::stod(argv[2]);
        return 0;
    };
    
    struct CallbackData {
        int id;
        std::string name;
        double qty;
    } cbData;
    
    g_db->query(sql, callback1, &cbData);
    
    if (cbData.id != -1) {
        Utils::printHeader("БАНКА С МАКСИМАЛЬНЫМ УЛОВОМ НИЗКОКАЧЕСТВЕННОЙ РЫБЫ");
        std::cout << "Банка: " << cbData.name << " (ID: " << cbData.id << ")" << std::endl;
        std::cout << "Количество низкокачественной рыбы: " << cbData.qty << " кг" << std::endl;
        Utils::printSeparator();
        
        std::string sql2 = "SELECT t.departure_date, t.return_date, tr.name as trawler_name "
                           "FROM Trip t "
                           "JOIN Trawler tr ON t.trawler_id = tr.id "
                           "WHERE t.bank_id = " + std::to_string(cbData.id) + " "
                           "ORDER BY t.departure_date;";
        
        std::cout << "\nРейсы, посещавшие эту банку:" << std::endl;
        Utils::printSeparator();
        
        auto callback2 = [](void* data, int argc, char** argv, char** colName) -> int {
            std::cout << std::left << std::setw(15) << argv[0]
                      << std::setw(15) << argv[1]
                      << argv[2] << std::endl;
            return 0;
        };
        
        g_db->query(sql2, callback2, nullptr);
        Utils::printSeparator();
    } else {
        std::cout << "Нет данных о низкокачественной рыбе." << std::endl;
    }
}

void TripService::printTopTrawlerInfo() {
    std::string sql = "SELECT tr.id, tr.name, s.total_catch "
                      "FROM Trawler tr "
                      "JOIN Statistics s ON tr.id = s.trawler_id "
                      "ORDER BY s.total_catch DESC LIMIT 1;";
    
    int trawler_id = -1;
    std::string trawler_name;
    double total_catch = 0;
    
    struct CallbackData {
        int id;
        std::string name;
        double catch_;
    } cbData = {-1, "", 0};
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* d = static_cast<CallbackData*>(data);
        d->id = std::stoi(argv[0]);
        d->name = argv[1];
        d->catch_ = std::stod(argv[2]);
        return 0;
    };
    
    g_db->query(sql, callback, &cbData);
    
    if (cbData.id != -1) {
        Utils::printHeader("ЛУЧШИЙ ТРАУЛЕР ПО УЛОВУ");
        std::cout << "Траулер: " << cbData.name << " (ID: " << cbData.id << ")" << std::endl;
        std::cout << "Общий улов: " << cbData.catch_ << " кг" << std::endl;
        Utils::printSeparator();
        
        std::string sql2 = "SELECT cm.surname, cm.position, t.departure_date, t.return_date, b.name as bank_name "
                           "FROM CrewMember cm "
                           "JOIN Trip t ON cm.trawler_id = t.trawler_id "
                           "JOIN Bank b ON t.bank_id = b.id "
                           "WHERE cm.trawler_id = " + std::to_string(cbData.id) + " "
                           "AND cm.position = 'капитан' "
                           "ORDER BY t.departure_date;";
        
        std::cout << "\nКапитан и посещенные банки:" << std::endl;
        Utils::printSeparator();
        
        auto callback2 = [](void* data, int argc, char** argv, char** colName) -> int {
            static bool captainPrinted = false;
            if (!captainPrinted && argv[0]) {
                std::cout << "Капитан: " << argv[0] << " (" << argv[1] << ")" << std::endl;
                std::cout << "\nПосещенные банки:" << std::endl;
                std::cout << std::left << std::setw(15) << "Дата выхода"
                          << std::setw(15) << "Дата возврата"
                          << "Название банки" << std::endl;
                captainPrinted = true;
            }
            std::cout << std::left << std::setw(15) << (argv[2] ? argv[2] : "")
                      << std::setw(15) << (argv[3] ? argv[3] : "")
                      << (argv[4] ? argv[4] : "") << std::endl;
            return 0;
        };
        
        g_db->query(sql2, callback2, nullptr);
        Utils::printSeparator();
    }
}

void TripService::printCrewRetirementDue(const std::string& date) {
    std::string sql = "SELECT id, surname, position, hire_date, birth_year "
                      "FROM CrewMember;";
    
    std::vector<CrewMember> members;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* members = static_cast<std::vector<CrewMember>*>(data);
        CrewMember m;
        m.id = std::stoi(argv[0]);
        m.surname = argv[1];
        m.position = argv[2];
        m.hire_date = argv[3];
        m.birth_year = std::stoi(argv[4]);
        members->push_back(m);
        return 0;
    };
    
    g_db->query(sql, callback, &members);
    
    Utils::printHeader("ЧЛЕНЫ КОМАНДЫ, ПОДЛЕЖАЩИЕ ПЕНСИИ НА " + date);
    Utils::printSeparator();
    
    bool found = false;
    for (const auto& m : members) {
        if (m.isRetirementDue(date)) {
            std::cout << std::left << std::setw(20) << m.surname
                      << std::setw(20) << m.position
                      << std::setw(15) << std::to_string(m.getAge()) + " лет"
                      << std::setw(15) << m.hire_date << std::endl;
            found = true;
        }
    }
    
    if (!found) {
        std::cout << "Нет членов команды, подлежащих пенсии на указанную дату." << std::endl;
    }
    Utils::printSeparator();
}

bool TripService::insertTrawler(const Trawler& trawler) {
    std::string sql = "INSERT INTO Trawler (name, displacement, build_date) "
                      "VALUES ('" + trawler.name + "', " + 
                      std::to_string(trawler.displacement) + ", '" +
                      trawler.build_date + "');";
    if (!g_db->execute(sql)) return false;
    
    int new_id = sqlite3_last_insert_rowid(g_db->getHandle());
    sql = "INSERT INTO Statistics (trawler_id, total_catch) VALUES (" + 
          std::to_string(new_id) + ", 0);";
    return g_db->execute(sql);
}

bool TripService::updateTrawler(int id, const Trawler& trawler) {
    std::string sql = "UPDATE Trawler SET name = '" + trawler.name + 
                      "', displacement = " + std::to_string(trawler.displacement) +
                      ", build_date = '" + trawler.build_date + 
                      "' WHERE id = " + std::to_string(id) + ";";
    return g_db->execute(sql);
}

bool TripService::deleteTrawler(int id) {
    if (!g_db->beginTransaction()) return false;
    
    std::string sql = "DELETE FROM FishCatch WHERE trip_id IN "
                      "(SELECT id FROM Trip WHERE trawler_id = " + std::to_string(id) + ");";
    g_db->execute(sql);
    
    sql = "DELETE FROM Trip WHERE trawler_id = " + std::to_string(id) + ";";
    g_db->execute(sql);
    
    sql = "DELETE FROM Statistics WHERE trawler_id = " + std::to_string(id) + ";";
    g_db->execute(sql);
    
    sql = "DELETE FROM Trawler WHERE id = " + std::to_string(id) + ";";
    if (!g_db->execute(sql)) {
        g_db->rollback();
        return false;
    }
    
    return g_db->commit();
}

bool TripService::insertCrewMember(const CrewMember& member) {
    std::string sql = "INSERT INTO CrewMember (surname, position, hire_date, birth_year, trawler_id) "
                      "VALUES ('" + member.surname + "', '" + member.position + "', '" +
                      member.hire_date + "', " + std::to_string(member.birth_year) + ", " +
                      std::to_string(member.trawler_id) + ");";
    return g_db->execute(sql);
}

bool TripService::updateCrewMember(int id, const CrewMember& member) {
    std::string sql = "UPDATE CrewMember SET surname = '" + member.surname +
                      "', position = '" + member.position +
                      "', hire_date = '" + member.hire_date +
                      "', birth_year = " + std::to_string(member.birth_year) +
                      ", trawler_id = " + std::to_string(member.trawler_id) +
                      " WHERE id = " + std::to_string(id) + ";";
    return g_db->execute(sql);
}

bool TripService::deleteCrewMember(int id) {
    std::string sql = "DELETE FROM CrewMember WHERE id = " + std::to_string(id) + ";";
    return g_db->execute(sql);
}

bool TripService::insertBank(const Bank& bank) {
    std::string sql = "INSERT INTO Bank (name) VALUES ('" + bank.name + "');";
    return g_db->execute(sql);
}

bool TripService::updateBank(int id, const Bank& bank) {
    std::string sql = "UPDATE Bank SET name = '" + bank.name + "' WHERE id = " + std::to_string(id) + ";";
    return g_db->execute(sql);
}

bool TripService::deleteBank(int id) {
    std::string sql = "DELETE FROM Bank WHERE id = " + std::to_string(id) + ";";
    return g_db->execute(sql);
}

std::vector<Trawler> TripService::getAllTrawlers() {
    std::vector<Trawler> trawlers;
    std::string sql = "SELECT id, name, displacement, build_date FROM Trawler;";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* t = static_cast<std::vector<Trawler>*>(data);
        Trawler trawler;
        trawler.id = std::stoi(argv[0]);
        trawler.name = argv[1];
        trawler.displacement = std::stod(argv[2]);
        trawler.build_date = argv[3];
        t->push_back(trawler);
        return 0;
    };
    
    g_db->query(sql, callback, &trawlers);
    return trawlers;
}

std::vector<CrewMember> TripService::getAllCrewMembers() {
    std::vector<CrewMember> members;
    std::string sql = "SELECT id, surname, position, hire_date, birth_year, trawler_id FROM CrewMember;";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* m = static_cast<std::vector<CrewMember>*>(data);
        CrewMember member;
        member.id = std::stoi(argv[0]);
        member.surname = argv[1];
        member.position = argv[2];
        member.hire_date = argv[3];
        member.birth_year = std::stoi(argv[4]);
        member.trawler_id = std::stoi(argv[5]);
        m->push_back(member);
        return 0;
    };
    
    g_db->query(sql, callback, &members);
    return members;
}

std::vector<Bank> TripService::getAllBanks() {
    std::vector<Bank> banks;
    std::string sql = "SELECT id, name FROM Bank;";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* b = static_cast<std::vector<Bank>*>(data);
        Bank bank;
        bank.id = std::stoi(argv[0]);
        bank.name = argv[1];
        b->push_back(bank);
        return 0;
    };
    
    g_db->query(sql, callback, &banks);
    return banks;
}

CrewMember TripService::getCrewMemberById(int id) {
    CrewMember member;
    member.id = -1;
    
    std::string sql = "SELECT id, surname, position, hire_date, birth_year, trawler_id "
                      "FROM CrewMember WHERE id = " + std::to_string(id) + ";";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* m = static_cast<CrewMember*>(data);
        m->id = std::stoi(argv[0]);
        m->surname = argv[1];
        m->position = argv[2];
        m->hire_date = argv[3];
        m->birth_year = std::stoi(argv[4]);
        m->trawler_id = std::stoi(argv[5]);
        return 0;
    };
    
    g_db->query(sql, callback, &member);
    return member;
}

Trawler TripService::getTrawlerById(int id) {
    Trawler trawler;
    trawler.id = -1;
    
    std::string sql = "SELECT id, name, displacement, build_date FROM Trawler WHERE id = " + std::to_string(id) + ";";
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* t = static_cast<Trawler*>(data);
        t->id = std::stoi(argv[0]);
        t->name = argv[1];
        t->displacement = std::stod(argv[2]);
        t->build_date = argv[3];
        return 0;
    };
    
    g_db->query(sql, callback, &trawler);
    return trawler;
}

void TripService::printTrawlersTable() {
    auto trawlers = getAllTrawlers();
    Utils::printHeader("СПИСОК ТРАУЛЕРОВ");
    Utils::printSeparator();
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(20) << "Название"
              << std::setw(12) << "Водоизмещение"
              << "Дата постройки" << std::endl;
    Utils::printSeparator();
    for (const auto& t : trawlers) {
        std::cout << std::left << std::setw(5) << t.id
                  << std::setw(20) << t.name
                  << std::setw(12) << t.displacement
                  << t.build_date << std::endl;
    }
    Utils::printSeparator();
}

void TripService::printCrewTable() {
    auto crew = getAllCrewMembers();
    Utils::printHeader("СПИСОК КОМАНДЫ");
    Utils::printSeparator();
    std::cout << std::left << std::setw(5) << "ID"
              << std::setw(20) << "Фамилия"
              << std::setw(20) << "Должность"
              << std::setw(12) << "Год рожд."
              << std::setw(12) << "Траулер"
              << "Дата найма" << std::endl;
    Utils::printSeparator();
    for (const auto& m : crew) {
        std::cout << std::left << std::setw(5) << m.id
                  << std::setw(20) << m.surname
                  << std::setw(20) << m.position
                  << std::setw(12) << m.birth_year
                  << std::setw(12) << m.trawler_id
                  << m.hire_date << std::endl;
    }
    Utils::printSeparator();
}

void TripService::printBanksTable() {
    auto banks = getAllBanks();
    Utils::printHeader("СПИСОК БАНОК");
    Utils::printSeparator();
    std::cout << std::left << std::setw(5) << "ID"
              << "Название банки" << std::endl;
    Utils::printSeparator();
    for (const auto& b : banks) {
        std::cout << std::left << std::setw(5) << b.id << b.name << std::endl;
    }
    Utils::printSeparator();
}

int CrewMember::getAge() const {
    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    int current_year = now->tm_year + 1900;
    return current_year - birth_year;
}

bool CrewMember::isRetirementDue(const std::string& date) const {
    int age = getAge();
    return age >= 65;
}
