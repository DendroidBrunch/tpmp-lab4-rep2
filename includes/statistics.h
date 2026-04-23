#ifndef STATISTICS_H
#define STATISTICS_H

#include <string>

struct Statistics {
    int trawler_id;
    double total_catch;
};

class StatisticsService {
public:
    static bool updateStatistics(int trawler_id);
    double getTotalCatch(int trawler_id);
    void printAllStatistics();
    
private:
    static double calculateTotalCatch(int trawler_id);
};

void setupStatisticsTriggers(sqlite3* db);

#endif
