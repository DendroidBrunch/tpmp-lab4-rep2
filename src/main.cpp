#include "database.h"
#include "auth.h"
#include "trip.h"
#include "statistics.h"
#include "bonus.h"
#include "utils.h"
#include <iostream>
#include <memory>

Database* g_db = nullptr;
Auth* g_auth = nullptr;

void showManagerMenu();
void showCrewMenu();
void handleManagerActions();
void handleCrewActions();

bool initializeDatabase() {
    if (!g_db->open()) return false;
    
    const char* create_tables = R"(
        CREATE TABLE IF NOT EXISTS Trawler (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            displacement REAL NOT NULL,
            build_date DATE NOT NULL
        );
        
        CREATE TABLE IF NOT EXISTS Bank (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );
        
        CREATE TABLE IF NOT EXISTS CrewMember (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            surname TEXT NOT NULL,
            position TEXT NOT NULL,
            hire_date DATE NOT NULL,
            birth_year INTEGER NOT NULL,
            trawler_id INTEGER,
            FOREIGN KEY (trawler_id) REFERENCES Trawler(id)
        );
        
        CREATE TABLE IF NOT EXISTS Trip (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            trawler_id INTEGER NOT NULL,
            bank_id INTEGER NOT NULL,
            departure_date DATE NOT NULL,
            return_date DATE,
            FOREIGN KEY (trawler_id) REFERENCES Trawler(id),
            FOREIGN KEY (bank_id) REFERENCES Bank(id)
        );
        
        CREATE TABLE IF NOT EXISTS FishCatch (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            trip_id INTEGER NOT NULL,
            fish_name TEXT NOT NULL,
            quantity REAL NOT NULL,
            quality TEXT NOT NULL,
            FOREIGN KEY (trip_id) REFERENCES Trip(id)
        );
        
        CREATE TABLE IF NOT EXISTS Statistics (
            trawler_id INTEGER PRIMARY KEY,
            total_catch REAL DEFAULT 0,
            FOREIGN KEY (trawler_id) REFERENCES Trawler(id)
        );
        
        CREATE TABLE IF NOT EXISTS Bonus (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            crew_id INTEGER NOT NULL,
            amount REAL NOT NULL,
            period_start DATE NOT NULL,
            period_end DATE NOT NULL,
            description TEXT,
            FOREIGN KEY (crew_id) REFERENCES CrewMember(id)
        );
        
        CREATE TABLE IF NOT EXISTS Users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password TEXT NOT NULL,
            role TEXT NOT NULL,
            crew_id INTEGER,
            FOREIGN KEY (crew_id) REFERENCES CrewMember(id)
        );
    )";
    
    if (!g_db->execute(create_tables)) return false;
    
    setupStatisticsTriggers(g_db->getHandle());
    
    g_db->execute("INSERT OR IGNORE INTO Trawler (id, name, displacement, build_date) VALUES "
                  "(1, 'Атлант', 2500.5, '2015-03-15'), "
                  "(2, 'Тихий Океан', 3100.0, '2018-07-22'), "
                  "(3, 'Северное Сияние', 1800.75, '2012-11-10');");
    
    g_db->execute("INSERT OR IGNORE INTO Bank (id, name) VALUES "
                  "(1, 'Северная банка'), "
                  "(2, 'Центральная банка'), "
                  "(3, 'Южная банка');");
    
    g_db->execute("INSERT OR IGNORE INTO CrewMember (id, surname, position, hire_date, birth_year, trawler_id) VALUES "
                  "(1, 'Иванов', 'капитан', '2010-01-15', 1975, 1), "
                  "(2, 'Петров', 'боцман', '2012-05-20', 1980, 1), "
                  "(3, 'Сидоров', 'капитан', '2015-03-10', 1970, 2), "
                  "(4, 'Козлов', 'матрос', '2018-06-01', 1985, 2), "
                  "(5, 'Михайлов', 'капитан', '2011-08-30', 1965, 3);");
    
    g_db->execute("INSERT OR IGNORE INTO Users (username, password, role, crew_id) VALUES "
                  "('manager', '1ae76917f6dc38', 'MANAGER', NULL), "
                  "('ivanov', '1ae76917f6dc38', 'CREW', 1), "
                  "('petrov', '1ae76917f6dc38', 'CREW', 2);");
    
    g_db->execute("INSERT OR IGNORE INTO Trip (id, trawler_id, bank_id, departure_date, return_date) VALUES "
                  "(1, 1, 1, '2024-01-10', '2024-01-25'), "
                  "(2, 1, 2, '2024-02-05', '2024-02-20'), "
                  "(3, 2, 2, '2024-01-15', '2024-01-30'), "
                  "(4, 2, 3, '2024-03-01', '2024-03-15'), "
                  "(5, 3, 1, '2024-02-10', '2024-02-25');");
    
    g_db->execute("INSERT OR IGNORE INTO FishCatch (trip_id, fish_name, quantity, quality) VALUES "
                  "(1, 'Треска', 5000, 'высокое'), "
                  "(1, 'Сельдь', 3000, 'среднее'), "
                  "(2, 'Минтай', 8000, 'высокое'), "
                  "(2, 'Камбала', 2000, 'низкое'), "
                  "(3, 'Треска', 6000, 'среднее'), "
                  "(3, 'Сельдь', 4000, 'высокое'), "
                  "(4, 'Минтай', 7000, 'низкое'), "
                  "(5, 'Камбала', 3500, 'среднее');");
    
    StatisticsService::updateStatistics(1);
    StatisticsService::updateStatistics(2);
    StatisticsService::updateStatistics(3);
    
    return true;
}

void showManagerMenu() {
    std::cout << "\n";
    Utils::printHeader("МЕНЮ УПРАВЛЯЮЩЕГО");
    std::cout << "1. Просмотр траулеров\n";
    std::cout << "2. Просмотр команды\n";
    std::cout << "3. Просмотр банок\n";
    std::cout << "4. Добавить траулер\n";
    std::cout << "5. Добавить члена команды\n";
    std::cout << "6. Добавить банку\n";
    std::cout << "7. Добавить рейс и улов\n";
    std::cout << "8. Отчеты\n";
    std::cout << "9. Начисление премий\n";
    std::cout << "10. Просмотр премий\n";
    std::cout << "11. Статистика уловов\n";
    std::cout << "0. Выход из системы\n";
    Utils::printSeparator();
}

void showCrewMenu() {
    std::cout << "\n";
    Utils::printHeader("МЕНЮ ЧЛЕНА КОМАНДЫ");
    std::cout << "1. Мои данные\n";
    std::cout << "2. Информация о пенсионном статусе\n";
    std::cout << "3. Просмотр моих премий\n";
    std::cout << "0. Выход из системы\n";
    Utils::printSeparator();
}

void handleManagerActions() {
    TripService service;
    int choice;
    
    do {
        showManagerMenu();
        choice = Utils::inputInt("Выберите действие: ", 0, 11);
        
        switch (choice) {
            case 1:
                service.printTrawlersTable();
                break;
            case 2:
                service.printCrewTable();
                break;
            case 3:
                service.printBanksTable();
                break;
            case 4: {
                Trawler t;
                t.name = Utils::inputString("Название траулера: ");
                t.displacement = Utils::inputDouble("Водоизмещение (т): ");
                t.build_date = Utils::inputString("Дата постройки (ГГГГ-ММ-ДД): ");
                if (service.insertTrawler(t)) {
                    std::cout << "Траулер добавлен!" << std::endl;
                }
                break;
            }
            case 5: {
                CrewMember m;
                m.surname = Utils::inputString("Фамилия: ");
                m.position = Utils::inputString("Должность: ");
                m.hire_date = Utils::inputString("Дата найма (ГГГГ-ММ-ДД): ");
                m.birth_year = Utils::inputInt("Год рождения: ");
                m.trawler_id = Utils::inputInt("ID траулера: ");
                if (service.insertCrewMember(m)) {
                    std::cout << "Член команды добавлен!" << std::endl;
                }
                break;
            }
            case 6: {
                Bank b;
                b.name = Utils::inputString("Название банки: ");
                if (service.insertBank(b)) {
                    std::cout << "Банка добавлена!" << std::endl;
                }
                break;
            }
            case 7: {
                Trip trip;
                trip.trawler_id = Utils::inputInt("ID траулера: ");
                trip.bank_id = Utils::inputInt("ID банки: ");
                trip.departure_date = Utils::inputString("Дата выхода (ГГГГ-ММ-ДД): ");
                trip.return_date = Utils::inputString("Дата возврата (ГГГГ-ММ-ДД): ");
                
                int numCatches = Utils::inputInt("Количество видов рыбы: ");
                for (int i = 0; i < numCatches; i++) {
                    FishCatch fc;
                    std::cout << "\nУлов #" << (i + 1) << ":\n";
                    fc.fish_name = Utils::inputString("  Название рыбы: ");
                    fc.quantity = Utils::inputDouble("  Количество (кг): ");
                    fc.quality = Utils::inputString("  Качество (высокое/среднее/низкое): ");
                    trip.catches.push_back(fc);
                }
                
                if (service.addTrip(trip)) {
                    std::cout << "Рейс успешно добавлен!" << std::endl;
                } else {
                    std::cout << "Ошибка при добавлении рейса!" << std::endl;
                }
                break;
            }
            case 8: {
                std::cout << "\n--- ОТЧЕТЫ ---\n";
                std::cout << "1. Рейсы траулера за период\n";
                std::cout << "2. Уловы на банке\n";
                std::cout << "3. Банка с макс. низкокачественной рыбой\n";
                std::cout << "4. Лучший траулер\n";
                std::cout << "5. Члены на пенсию\n";
                int reportChoice = Utils::inputInt("Выберите отчет: ", 1, 5);
                
                switch (reportChoice) {
                    case 1: {
                        int tid = Utils::inputInt("ID траулера: ");
                        std::string start = Utils::inputString("Начало периода (ГГГГ-ММ-ДД): ");
                        std::string end = Utils::inputString("Конец периода (ГГГГ-ММ-ДД): ");
                        service.printTrawlerTrips(tid, start, end);
                        break;
                    }
                    case 2: {
                        int bid = Utils::inputInt("ID банки: ");
                        service.printBankCatches(bid);
                        break;
                    }
                    case 3:
                        service.printMaxLowQualityBank();
                        break;
                    case 4:
                        service.printTopTrawlerInfo();
                        break;
                    case 5: {
                        std::string date = Utils::inputString("Дата проверки (ГГГГ-ММ-ДД): ");
                        service.printCrewRetirementDue(date);
                        break;
                    }
                }
                break;
            }
            case 9: {
                std::cout << "\n--- НАЧИСЛЕНИЕ ПРЕМИЙ ---\n";
                std::cout << "1. Всем членам команды\n";
                std::cout << "2. Конкретному члену команды\n";
                int bonusChoice = Utils::inputInt("Выберите: ", 1, 2);
                
                std::string start = Utils::inputString("Начало периода: ");
                std::string end = Utils::inputString("Конец периода: ");
                double plan = Utils::inputDouble("Плановое задание (кг): ");
                double price = Utils::inputDouble("Средняя цена за кг (руб): ");
                
                if (bonusChoice == 1) {
                    int count = BonusCalculator::calculateBonuses(start, end, plan, price);
                    std::cout << "Начислено " << count << " премий." << std::endl;
                } else {
                    int crew_id = Utils::inputInt("ID члена команды: ");
                    if (BonusCalculator::calculateBonusForCrew(crew_id, start, end, plan, price)) {
                        std::cout << "Премия начислена!" << std::endl;
                    }
                }
                break;
            }
            case 10:
                BonusCalculator::printBonuses();
                break;
            case 11:
                StatisticsService::printAllStatistics();
                break;
        }
        
        if (choice != 0) {
            Utils::waitForEnter();
        }
        
    } while (choice != 0);
}

void handleCrewActions() {
    TripService service;
    int crew_id = g_auth->getCurrentCrewId();
    int choice;
    
    do {
        showCrewMenu();
        choice = Utils::inputInt("Выберите действие: ", 0, 3);
        
        switch (choice) {
            case 1: {
                auto member = service.getCrewMemberById(crew_id);
                if (member.id != -1) {
                    Utils::printHeader("МОИ ДАННЫЕ");
                    std::cout << "Фамилия: " << member.surname << std::endl;
                    std::cout << "Должность: " << member.position << std::endl;
                    std::cout << "Дата найма: " << member.hire_date << std::endl;
                    std::cout << "Год рождения: " << member.birth_year << std::endl;
                    std::cout << "Возраст: " << member.getAge() << " лет" << std::endl;
                    std::cout << "ID траулера: " << member.trawler_id << std::endl;
                    
                    auto trawler = service.getTrawlerById(member.trawler_id);
                    if (trawler.id != -1) {
                        std::cout << "Траулер: " << trawler.name << std::endl;
                    }
                }
                break;
            }
            case 2: {
                std::string date = Utils::getCurrentDate();
                auto member = service.getCrewMemberById(crew_id);
                if (member.isRetirementDue(date)) {
                    std::cout << "ВНИМАНИЕ: Вы достигли пенсионного возраста!" << std::endl;
                } else {
                    std::cout << "Вы еще не достигли пенсионного возраста." << std::endl;
                    std::cout << "До пенсии осталось: " << (65 - member.getAge()) << " лет" << std::endl;
                }
                break;
            }
            case 3: {
                auto bonuses = BonusCalculator::getAllBonuses();
                bool found = false;
                Utils::printHeader("МОИ ПРЕМИИ");
                for (const auto& b : bonuses) {
                    if (b.crew_id == crew_id) {
                        std::cout << "Сумма: " << b.amount << " руб." << std::endl;
                        std::cout << "Период: " << b.period_start << " - " << b.period_end << std::endl;
                        std::cout << "Описание: " << b.description << std::endl;
                        std::cout << "---" << std::endl;
                        found = true;
                    }
                }
                if (!found) {
                    std::cout << "Премий не найдено." << std::endl;
                }
                break;
            }
        }
        
        if (choice != 0) {
            Utils::waitForEnter();
        }
        
    } while (choice != 0);
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  РЫБОЛОВНАЯ ФЛОТИЛИЯ - СИСТЕМА УПРАВЛЕНИЯ" << std::endl;
    std::cout << "========================================" << std::endl;
    
    g_db = new Database("fishing_fleet.db");
    g_auth = new Auth();
    
    if (!initializeDatabase()) {
        std::cerr << "Ошибка инициализации базы данных!" << std::endl;
        delete g_db;
        delete g_auth;
        return 1;
    }
    
    bool authenticated = false;
    do {
        std::cout << "\n--- АВТОРИЗАЦИЯ ---" << std::endl;
        std::string username = Utils::inputString("Логин: ");
        std::string password = Utils::inputString("Пароль: ");
        
        if (g_auth->login(username, password)) {
            authenticated = true;
        } else {
            std::cout << "Повторить попытку? (y/n): ";
            char retry;
            std::cin >> retry;
            std::cin.ignore();
            if (retry != 'y' && retry != 'Y') {
                break;
            }
        }
    } while (!authenticated);
    
    if (authenticated) {
        if (g_auth->getRole() == UserRole::MANAGER) {
            handleManagerActions();
        } else {
            handleCrewActions();
        }
        g_auth->logout();
    }
    
    g_db->close();
    delete g_db;
    delete g_auth;
    
    std::cout << "Работа программы завершена." << std::endl;
    return 0;
}
