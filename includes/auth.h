#ifndef AUTH_H
#define AUTH_H

#include <string>

enum class UserRole {
    MANAGER,
    CREW
};

struct User {
    int id;
    std::string username;
    std::string password_hash;
    UserRole role;
    int crew_id;
};

class Auth {
private:
    User current_user;
    bool authenticated;
    
    bool verifyPassword(const std::string& input, const std::string& hash);
   
    
public:
    std::string hashPassword(const std::string& password);

    Auth();
    
    bool login(const std::string& username, const std::string& password);
    void logout();
    bool isAuthenticated() const { return authenticated; }
    UserRole getRole() const { return current_user.role; }
    int getCurrentUserId() const { return current_user.id; }
    int getCurrentCrewId() const { return current_user.crew_id; }
    std::string getCurrentUsername() const { return current_user.username; }
};

extern Auth* g_auth;

#endif
