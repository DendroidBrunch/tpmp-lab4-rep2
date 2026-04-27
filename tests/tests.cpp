#include <gtest/gtest.h>
#include "auth.h"
#include "database.h"
#include "bonus.h"
#include "statistics.h"
#include "trip.h"
#include "utils.h"
#include <string>

Database* g_db = nullptr;
Auth* g_auth = nullptr;

class UtilsTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(UtilsTest, GetCurrentDateReturnsValidFormat) {
    std::string date = Utils::getCurrentDate();
    EXPECT_EQ(10, date.length());
    EXPECT_EQ('-', date[4]);
    EXPECT_EQ('-', date[7]);
}

TEST_F(UtilsTest, IsValidDateValidatesCorrectly) {
    EXPECT_TRUE(Utils::isValidDate("2024-01-15"));
    EXPECT_FALSE(Utils::isValidDate("2024-13-01"));
    EXPECT_FALSE(Utils::isValidDate("2024-01-32"));
    EXPECT_FALSE(Utils::isValidDate("24-01-01"));
    EXPECT_FALSE(Utils::isValidDate("2024/01/01"));
}

TEST_F(UtilsTest, DaysBetweenCalculatesCorrectDifference) {
    int days = Utils::daysBetween("2024-01-01", "2024-01-31");
    EXPECT_EQ(30, days);
    
    int days2 = Utils::daysBetween("2024-01-01", "2024-02-01");
    EXPECT_EQ(31, days2);
}

class TripServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_db = new Database(":memory:");
        g_db->open();
        
        std::string createTrawlerTable = 
            "CREATE TABLE IF NOT EXISTS Trawler ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL, "
            "displacement REAL NOT NULL, "
            "build_date TEXT NOT NULL);";
        g_db->execute(createTrawlerTable);
        
        std::string createBankTable = 
            "CREATE TABLE IF NOT EXISTS Bank ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL);";
        g_db->execute(createBankTable);
        
        std::string createCrewTable = 
            "CREATE TABLE IF NOT EXISTS CrewMember ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "surname TEXT NOT NULL, "
            "position TEXT NOT NULL, "
            "hire_date TEXT NOT NULL, "
            "birth_year INTEGER NOT NULL, "
            "trawler_id INTEGER NOT NULL);";
        g_db->execute(createCrewTable);
        
        std::string createTripTable = 
            "CREATE TABLE IF NOT EXISTS Trip ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trawler_id INTEGER NOT NULL, "
            "bank_id INTEGER NOT NULL, "
            "departure_date TEXT NOT NULL, "
            "return_date TEXT NOT NULL);";
        g_db->execute(createTripTable);
        
        std::string createFishCatchTable = 
            "CREATE TABLE IF NOT EXISTS FishCatch ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trip_id INTEGER NOT NULL, "
            "fish_name TEXT NOT NULL, "
            "quantity REAL NOT NULL, "
            "quality TEXT NOT NULL);";
        g_db->execute(createFishCatchTable);
        
        std::string createStatisticsTable = 
            "CREATE TABLE IF NOT EXISTS Statistics ("
            "trawler_id INTEGER PRIMARY KEY, "
            "total_catch REAL NOT NULL);";
        g_db->execute(createStatisticsTable);
        
        g_db->execute("INSERT INTO Trawler (id, name, displacement, build_date) VALUES (1, 'Sea Hunter', 5000.0, '2010-01-01');");
        g_db->execute("INSERT INTO Trawler (id, name, displacement, build_date) VALUES (2, 'Ocean King', 6000.0, '2015-03-15');");
        
        g_db->execute("INSERT INTO Bank (id, name) VALUES (1, 'North Bank');");
        g_db->execute("INSERT INTO Bank (id, name) VALUES (2, 'South Bank');");
        
        g_db->execute("INSERT INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES (1, 'Ivanov', 'captain', '2010-01-01', 1960, 1);");
        g_db->execute("INSERT INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES (2, 'Petrov', 'fisherman', '2015-03-15', 1990, 1);");
        
        g_db->execute("INSERT INTO Statistics (trawler_id, total_catch) VALUES (1, 0);");
        g_db->execute("INSERT INTO Statistics (trawler_id, total_catch) VALUES (2, 0);");
    }
    
    void TearDown() override {
        g_db->execute("DROP TABLE IF EXISTS Statistics;");
        g_db->execute("DROP TABLE IF EXISTS FishCatch;");
        g_db->execute("DROP TABLE IF EXISTS Trip;");
        g_db->execute("DROP TABLE IF EXISTS CrewMember;");
        g_db->execute("DROP TABLE IF EXISTS Bank;");
        g_db->execute("DROP TABLE IF EXISTS Trawler;");
        g_db->close();
        delete g_db;
        g_db = nullptr;
    }
};

TEST_F(TripServiceTest, InsertAndGetTrawler) {
    TripService service;
    Trawler newTrawler;
    newTrawler.name = "Test Trawler";
    newTrawler.displacement = 7000.0;
    newTrawler.build_date = "2020-01-01";
    
    bool inserted = service.insertTrawler(newTrawler);
    EXPECT_TRUE(inserted);
    
    auto trawlers = service.getAllTrawlers();
    EXPECT_EQ(3, trawlers.size());
    
    Trawler retrieved = service.getTrawlerById(3);
    EXPECT_EQ(3, retrieved.id);
    EXPECT_EQ("Test Trawler", retrieved.name);
    EXPECT_DOUBLE_EQ(7000.0, retrieved.displacement);
}

TEST_F(TripServiceTest, InsertAndGetCrewMember) {
    TripService service;
    CrewMember newMember;
    newMember.surname = "Sidorov";
    newMember.position = "mechanic";
    newMember.hire_date = "2020-06-01";
    newMember.birth_year = 1995;
    newMember.trawler_id = 1;
    
    bool inserted = service.insertCrewMember(newMember);
    EXPECT_TRUE(inserted);
    
    auto crew = service.getAllCrewMembers();
    EXPECT_EQ(3, crew.size());
    
    CrewMember retrieved = service.getCrewMemberById(3);
    EXPECT_EQ(3, retrieved.id);
    EXPECT_EQ("Sidorov", retrieved.surname);
    EXPECT_EQ(1995, retrieved.birth_year);
}

TEST_F(TripServiceTest, InsertAndGetBank) {
    TripService service;
    Bank newBank;
    newBank.name = "East Bank";
    
    bool inserted = service.insertBank(newBank);
    EXPECT_TRUE(inserted);
    
    auto banks = service.getAllBanks();
    EXPECT_EQ(3, banks.size());
    
    bool updated = service.updateBank(3, newBank);
    EXPECT_TRUE(updated);
    
    bool deleted = service.deleteBank(3);
    EXPECT_TRUE(deleted);
    
    banks = service.getAllBanks();
    EXPECT_EQ(2, banks.size());
}

class StatisticsTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_db = new Database(":memory:");
        g_db->open();
        
        std::string createTrawlerTable = 
            "CREATE TABLE IF NOT EXISTS Trawler ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "name TEXT NOT NULL);";
        g_db->execute(createTrawlerTable);
        
        std::string createTripTable = 
            "CREATE TABLE IF NOT EXISTS Trip ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trawler_id INTEGER NOT NULL, "
            "departure_date TEXT NOT NULL, "
            "return_date TEXT NOT NULL, "
            "captain_id INTEGER NOT NULL);";
        g_db->execute(createTripTable);
        
        std::string createFishCatchTable = 
            "CREATE TABLE IF NOT EXISTS FishCatch ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trip_id INTEGER NOT NULL, "
            "fish_type TEXT NOT NULL, "
            "quantity REAL NOT NULL);";
        g_db->execute(createFishCatchTable);
        
        std::string createStatisticsTable = 
            "CREATE TABLE IF NOT EXISTS Statistics ("
            "trawler_id INTEGER PRIMARY KEY, "
            "total_catch REAL NOT NULL);";
        g_db->execute(createStatisticsTable);
        
        g_db->execute("INSERT INTO Trawler (id, name) VALUES (101, 'Sea Hunter');");
        
        g_db->execute("INSERT INTO Trip (id, trawler_id, departure_date, return_date, captain_id) VALUES (1, 101, '2024-01-01', '2024-01-15', 1);");
        g_db->execute("INSERT INTO Trip (id, trawler_id, departure_date, return_date, captain_id) VALUES (2, 101, '2024-02-01', '2024-02-15', 1);");
        
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (1, 1, 'Cod', 5000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (2, 1, 'Herring', 3000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (3, 2, 'Cod', 6000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (4, 2, 'Herring', 4000);");
    }
    
    void TearDown() override {
        g_db->execute("DROP TABLE IF EXISTS Statistics;");
        g_db->execute("DROP TABLE IF EXISTS FishCatch;");
        g_db->execute("DROP TABLE IF EXISTS Trip;");
        g_db->execute("DROP TABLE IF EXISTS Trawler;");
        g_db->close();
        delete g_db;
        g_db = nullptr;
    }
};

TEST_F(StatisticsTest, UpdateAndGetTotalCatch) {
    bool updated = StatisticsService::updateStatistics(101);
    EXPECT_TRUE(updated);
    
    double total = StatisticsService::getTotalCatch(101);
    EXPECT_DOUBLE_EQ(18000.0, total);
}

TEST_F(StatisticsTest, GetTotalCatchForNonExistentTrawler) {
    double total = StatisticsService::getTotalCatch(999);
    EXPECT_DOUBLE_EQ(0.0, total);
}

TEST_F(StatisticsTest, UpdateStatisticsForMultipleTrawlers) {
    bool updated1 = StatisticsService::updateStatistics(101);
    EXPECT_TRUE(updated1);
    
    double total = StatisticsService::getTotalCatch(101);
    EXPECT_DOUBLE_EQ(18000.0, total);
}

class BonusTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_db = new Database(":memory:");
        g_db->open();
        
        std::string createCrewTable = 
            "CREATE TABLE IF NOT EXISTS CrewMember ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "surname TEXT NOT NULL, "
            "position TEXT NOT NULL, "
            "hire_date TEXT NOT NULL, "
            "birth_year INTEGER NOT NULL, "
            "trawler_id INTEGER NOT NULL);";
        g_db->execute(createCrewTable);
        
        std::string createTripTable = 
            "CREATE TABLE IF NOT EXISTS Trip ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trawler_id INTEGER NOT NULL, "
            "departure_date TEXT NOT NULL, "
            "return_date TEXT NOT NULL, "
            "captain_id INTEGER NOT NULL);";
        g_db->execute(createTripTable);
        
        std::string createFishCatchTable = 
            "CREATE TABLE IF NOT EXISTS FishCatch ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "trip_id INTEGER NOT NULL, "
            "fish_type TEXT NOT NULL, "
            "quantity REAL NOT NULL);";
        g_db->execute(createFishCatchTable);
        
        std::string createBonusTable = 
            "CREATE TABLE IF NOT EXISTS Bonus ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "crew_id INTEGER NOT NULL, "
            "amount REAL NOT NULL, "
            "period_start TEXT NOT NULL, "
            "period_end TEXT NOT NULL, "
            "description TEXT NOT NULL);";
        g_db->execute(createBonusTable);
        
        g_db->execute("INSERT INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES (1, 'Ivanov', 'Captain', '2020-01-01', 1980, 101);");
        g_db->execute("INSERT INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES (2, 'Petrov', 'Fisherman', '2021-03-15', 1985, 101);");
        g_db->execute("INSERT INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES (3, 'Sidorov', 'Fisherman', '2021-05-20', 1990, 101);");
        
        g_db->execute("INSERT INTO Trip (id, trawler_id, departure_date, return_date, captain_id) VALUES (1, 101, '2024-01-01', '2024-01-15', 1);");
        g_db->execute("INSERT INTO Trip (id, trawler_id, departure_date, return_date, captain_id) VALUES (2, 101, '2024-02-01', '2024-02-15', 1);");
        
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (1, 1, 'Cod', 5000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (2, 1, 'Herring', 3000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (3, 2, 'Cod', 6000);");
        g_db->execute("INSERT INTO FishCatch (id, trip_id, fish_type, quantity) VALUES (4, 2, 'Herring', 4000);");
    }
    
    void TearDown() override {
        g_db->execute("DROP TABLE IF EXISTS Bonus;");
        g_db->execute("DROP TABLE IF EXISTS FishCatch;");
        g_db->execute("DROP TABLE IF EXISTS Trip;");
        g_db->execute("DROP TABLE IF EXISTS CrewMember;");
        g_db->close();
        delete g_db;
        g_db = nullptr;
    }
};

TEST_F(BonusTest, CalculateBonusesForAllCrewMembers) {
    int count = BonusCalculator::calculateBonuses("2024-01-01", "2024-02-15", 15000.0, 100.0);
    EXPECT_EQ(3, count);
    
    auto bonuses = BonusCalculator::getAllBonuses();
    EXPECT_EQ(3, bonuses.size());
}

TEST_F(BonusTest, GetAllBonusesReturnsEmptyVectorInitially) {
    auto bonuses = BonusCalculator::getAllBonuses();
    EXPECT_EQ(0, bonuses.size());
}

TEST_F(BonusTest, CalculateBonusesWithHighPlan) {
    int count = BonusCalculator::calculateBonuses("2024-01-01", "2024-02-15", 50000.0, 100.0);
    EXPECT_EQ(0, count);
    
    auto bonuses = BonusCalculator::getAllBonuses();
    EXPECT_EQ(0, bonuses.size());
}

class AuthTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_db = new Database(":memory:");
        g_db->open();
        g_auth = new Auth();
        
        std::string createTableSQL = 
            "CREATE TABLE IF NOT EXISTS Users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "username TEXT NOT NULL UNIQUE, "
            "password TEXT NOT NULL, "
            "role TEXT NOT NULL CHECK (role IN ('MANAGER', 'CREW')), "
            "crew_id INTEGER);";
        g_db->execute(createTableSQL);
        
        Auth tempAuth;
        std::string hashedPassword = tempAuth.hashPassword("password123");
        std::string sql = "INSERT INTO Users (username, password, role, crew_id) VALUES ('manager1', '" + hashedPassword + "', 'MANAGER', NULL);";
        g_db->execute(sql);
    }
    
    void TearDown() override {
        g_db->execute("DROP TABLE IF EXISTS Users;");
        g_db->close();
        delete g_db;
        delete g_auth;
        g_db = nullptr;
        g_auth = nullptr;
    }
};

TEST_F(AuthTest, LoginSuccessWithValidCredentials) {
    Auth auth;
    EXPECT_TRUE(auth.login("manager1", "password123"));
    EXPECT_TRUE(auth.isAuthenticated());
    EXPECT_EQ(UserRole::MANAGER, auth.getRole());
}

TEST_F(AuthTest, LoginFailsWithInvalidPassword) {
    Auth auth;
    EXPECT_FALSE(auth.login("manager1", "wrongpassword"));
    EXPECT_FALSE(auth.isAuthenticated());
}

TEST_F(AuthTest, LogoutClearsAuthenticationState) {
    Auth auth;
    auth.login("manager1", "password123");
    EXPECT_TRUE(auth.isAuthenticated());
    
    auth.logout();
    EXPECT_FALSE(auth.isAuthenticated());
    EXPECT_EQ(-1, auth.getCurrentUserId());
    EXPECT_EQ("", auth.getCurrentUsername());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}