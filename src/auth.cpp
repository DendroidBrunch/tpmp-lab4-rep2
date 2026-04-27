#include "auth.h"
#include "database.h"
#include <iostream>
#include <cstring>
#include <iomanip>
#include <sstream>

static std::string simpleHash(const std::string& password) {
    unsigned long hash = 5381;
    for (char c : password) {
        hash = ((hash << 5) + hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

Auth::Auth() : authenticated(false) {
    current_user.id = -1;
    current_user.crew_id = -1;
}

bool Auth::verifyPassword(const std::string& input, const std::string& hash) {
    return simpleHash(input) == hash;
}

std::string Auth::hashPassword(const std::string& password) {
    return simpleHash(password);
}

bool Auth::login(const std::string& username, const std::string& password) {
    std::string sql = "SELECT id, password, role, crew_id FROM Users WHERE username = '" + username + "';";
    
    bool found = false;
    int userId = -1;
    std::string pwdHash;
    std::string roleStr;
    int crewId = -1;
    
    auto callback = [](void* data, int argc, char** argv, char** colName) -> int {
        auto* result = static_cast<std::tuple<bool*, int*, std::string*, std::string*, int*>*>(data);
        auto* found = std::get<0>(*result);
        auto* id = std::get<1>(*result);
        auto* hash = std::get<2>(*result);
        auto* role = std::get<3>(*result);
        auto* crew = std::get<4>(*result);
        
        *found = true;
        *id = std::stoi(argv[0]);
        *hash = argv[1];
        *role = argv[2];
        *crew = argv[3] ? std::stoi(argv[3]) : -1;
        return 0;
    };
    
    auto data = std::make_tuple(&found, &userId, &pwdHash, &roleStr, &crewId);
    
    if (!g_db->query(sql, callback, &data)) {
        return false;
    }
    
    if (!found) {
	std::cout << "Неверное имя полбзователя!" << std::endl;
        return false;
    }        
    
    if(!verifyPassword(password, pwdHash)){
	std::cout << "Неверный  пароль! " << pwdHash << " " << password<< " " <<simpleHash(password) << std::endl;
        return false;
    }	
    
    authenticated = true;
    current_user.id = userId;
    current_user.username = username;
    current_user.password_hash = pwdHash;
    current_user.role = (roleStr == "MANAGER") ? UserRole::MANAGER : UserRole::CREW;
    current_user.crew_id = crewId;
    
    std::cout << "Добро пожаловать, " << username << "!" << std::endl;
    return true;
}

void Auth::logout() {
    authenticated = false;
    current_user.id = -1;
    current_user.crew_id = -1;
    current_user.username = "";
    std::cout << "Вы вышли из системы." << std::endl;
}
