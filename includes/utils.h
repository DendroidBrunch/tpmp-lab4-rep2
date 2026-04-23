#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Utils {
    std::string getCurrentDate();
    bool isValidDate(const std::string& date);
    int daysBetween(const std::string& d1, const std::string& d2);
    
    std::string inputString(const std::string& prompt);
    int inputInt(const std::string& prompt, int min = -1, int max = -1);
    double inputDouble(const std::string& prompt, double min = -1, double max = -1);
    char inputChar(const std::string& prompt);
    
    void printHeader(const std::string& title);
    void printSeparator(int len = 80);
    void clearScreen();
    void waitForEnter();
    void printTableRow(const std::vector<std::string>& cols, int widths[]);
}

#endif
