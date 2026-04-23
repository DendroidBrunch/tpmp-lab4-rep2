#ifndef TRIP_H
#define TRIP_H

#include <string>
#include <vector>
#include <ctime>

struct FishCatch {
    std::string fish_name;
    double quantity;
    std::string quality;
};

struct Trip {
    int id;
    int trawler_id;
    int bank_id;
    std::string departure_date;
    std::string return_date;
    std::vector<FishCatch> catches;
};

struct Trawler {
    int id;
    std::string name;
    double displacement;
    std::string build_date;
};

struct Bank {
    int id;
    std::string name;
};

struct CrewMember {
    int id;
    std::string surname;
    std::string position;
    std::string hire_date;
    int birth_year;
    int trawler_id;
    
    int getAge() const;
    bool isRetirementDue(const std::string& date) const;
};

class TripService {
public:
    bool addTrip(const Trip& trip);
    std::vector<Trip> getTripsByTrawler(int trawler_id, 
                                         const std::string& start_date,
                                         const std::string& end_date);
    
    void printTrawlerTrips(int trawler_id, const std::string& start, const std::string& end);
    void printBankCatches(int bank_id);
    void printMaxLowQualityBank();
    void printTopTrawlerInfo();
    void printCrewRetirementDue(const std::string& date);
    
    bool insertTrawler(const Trawler& trawler);
    bool updateTrawler(int id, const Trawler& trawler);
    bool deleteTrawler(int id);
    
    bool insertCrewMember(const CrewMember& member);
    bool updateCrewMember(int id, const CrewMember& member);
    bool deleteCrewMember(int id);
    
    bool insertBank(const Bank& bank);
    bool updateBank(int id, const Bank& bank);
    bool deleteBank(int id);
    
    std::vector<Trawler> getAllTrawlers();
    std::vector<CrewMember> getAllCrewMembers();
    std::vector<Bank> getAllBanks();
    CrewMember getCrewMemberById(int id);
    Trawler getTrawlerById(int id);

    void printTrawlersTable();
    void printCrewTable();
    void printBanksTable();
};

#endif
