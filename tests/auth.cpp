#include <gtest/gtest.h>
#include "auth.h"
#include "database.h"
#include <string>

Database* g_db = nullptr;
Auth* g_auth = nullptr;

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