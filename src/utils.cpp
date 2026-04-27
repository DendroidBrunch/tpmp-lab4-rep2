#include "utils.h"
#include <iostream>
#include <limits>

namespace Utils {
    
std::string getCurrentDate() {
    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    char buffer[11];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
    return std::string(buffer);
}

bool isValidDate(const std::string& date) {
    if (date.length() != 10) return false;
    if (date[4] != '-' || date[7] != '-') return false;
    
    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    
    if (year < 1900 || year > 2100) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > 31) return false;
    
    return true;
}

int daysBetween(const std::string& d1, const std::string& d2) {
    struct tm tm1 = {}, tm2 = {};
    strptime(d1.c_str(), "%Y-%m-%d", &tm1);
    strptime(d2.c_str(), "%Y-%m-%d", &tm2);
    time_t t1 = mktime(&tm1);
    time_t t2 = mktime(&tm2);
    return static_cast<int>(std::difftime(t2, t1) / (60 * 60 * 24));
}

std::string inputString(const std::string& prompt) {
    std::string result;
    std::cout << prompt;
    std::getline(std::cin, result);
    return result;
}

int inputInt(const std::string& prompt, int min, int max) {
    int value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Ошибка: введите целое число!" << std::endl;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if ((min != -1 && value < min) || (max != -1 && value > max)) {
            std::cout << "Ошибка: значение должно быть от " << min << " до " << max << std::endl;
            continue;
        }
        return value;
    }
}

double inputDouble(const std::string& prompt, double min, double max) {
    double value;
    while (true) {
        std::cout << prompt;
        std::cin >> value;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Ошибка: введите число!" << std::endl;
            continue;
        }
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        if ((min != -1 && value < min) || (max != -1 && value > max)) {
            std::cout << "Ошибка: значение должно быть от " << min << " до " << max << std::endl;
            continue;
        }
        return value;
    }
}

char inputChar(const std::string& prompt) {
    std::string result;
    std::cout << prompt;
    std::getline(std::cin, result);
    if (result.empty()) return 0;
    return tolower(result[0]);
}

void printHeader(const std::string& title) {
    printSeparator();
    int padding = (80 - title.length()) / 2;
    std::cout << std::string(padding, ' ') << title << std::endl;
    printSeparator();
}

void printSeparator(int len) {
    std::cout << std::string(len, '=') << std::endl;
}

void clearScreen() {
    std::cout << "\033[2J\033[1;1H";
}

void waitForEnter() {
    std::cout << "\nНажмите Enter для продолжения...";
    std::cin.get();
}

void printTableRow(const std::vector<std::string>& cols, int widths[]) {
    for (size_t i = 0; i < cols.size(); i++) {
        std::cout << std::left << std::setw(widths[i]) << cols[i];
    }
    std::cout << std::endl;
}

}
