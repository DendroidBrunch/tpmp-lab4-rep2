#ifndef BONUS_H
#define BONUS_H

#include <string>
#include <vector>

struct Bonus {
    int id;
    int crew_id;
    double amount;
    std::string period_start;
    std::string period_end;
    std::string description;
};

class BonusCalculator {
public:
    static int calculateBonuses(const std::string& start_date,
                                const std::string& end_date,
                                double plan,
                                double price);
    
    static bool calculateBonusForCrew(int crew_id,
                                       const std::string& start_date,
                                       const std::string& end_date,
                                       double plan,
                                       double price);
    
    static std::vector<Bonus> getAllBonuses();
    static void printBonuses();
    
private:
    static double calculateCrewCatch(int crew_id, 
                                      const std::string& start_date,
                                      const std::string& end_date);
    static bool insertBonus(int crew_id, double amount, 
                            const std::string& start_date,
                            const std::string& end_date,
                            const std::string& description);
};

#endif
