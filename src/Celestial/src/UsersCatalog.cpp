#include "LunarDB/Celestial/UsersCatalog.hpp"

#include <cassert>
#include <fstream>
#include <stdexcept>

#include "LunarDB/Common/CppExtensions/BinaryIO.hpp"
#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Common/Cryptography/AES256.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

namespace LunarDB::Celestial::API {

LUNAR_SINGLETON_INIT_IMPL(UsersCatalog)
{
    loadFromDisk();
}

Common::CppExtensions::UniqueID UsersCatalog::createUser(std::string username, std::string password)
{
    // 1. Check if username available
    if (m_name_to_uid.find(username) != m_name_to_uid.end())
    {
        throw std::runtime_error(Common::CppExtensions::StringUtils::stringify(
            "User with", std::move(username), "username already exists"));
    }

    // 2. Add user to internal data
    auto const new_id{Common::CppExtensions::UniqueID::generate()};

    std::ignore = m_name_to_uid.emplace(username, new_id);
    auto const map_result = m_uid_to_config_cache.emplace(
        new_id, Configuration::User{new_id, std::move(username), std::move(password), {}});

    assert(map_result.second && "[Create User] Map insert failed");

    // 3. Save user to disk
    saveUserToDisk(map_result.first->second);
    saveUserNamesToDisk();

    return new_id;
}

void UsersCatalog::removeUser(Common::CppExtensions::UniqueID const& user_uid)
{
    // 1. Get user data
    auto& user{getUserFromUID(user_uid)};

    // 2. Remove user from internal data
    std::ignore = m_uid_to_config_cache.erase(user_uid);
    std::ignore = m_name_to_uid.erase(user.name);

    // 3. Remove user from disk
    // ?MAYBE: Mark user for deletion?
    std::filesystem::remove(getUserConfigurationFilePath(user_uid));
    saveUserNamesToDisk();
}

void UsersCatalog::updatePassword(Common::CppExtensions::UniqueID const& user_uid, std::string new_password)
{
    // 1. Get user internal data
    auto& user{getUserFromUID(user_uid)};

    // 2. Update user internal data
    user.password = std::move(new_password);

    // 3. Save user to disk
    saveUserToDisk(user);
}

void UsersCatalog::updatePermission(
    Common::CppExtensions::UniqueID const& user_uid,
    Configuration::PermissionUpdate permission)
{
    assert(
        permission.update_type != Configuration::EPermissionUpdateType::Unknown &&
        "Error, 'Unknown' update type issued");

    // 1. Get user internal data
    auto& user{getUserFromUID(user_uid)};

    // 2. Update user internal data
    switch (permission.update_type)
    {
    case Configuration::EPermissionUpdateType::Grant:
        std::ignore = user.permissions.emplace(std::move(permission.data));
        break;
    case Configuration::EPermissionUpdateType::Revoke:
        std::ignore = user.permissions.erase(permission.data);
        break;
    default:
        break;
    }

    // 3. Save user to disk
    saveUserToDisk(user);
}

bool UsersCatalog::userHasPermission(
    Common::CppExtensions::UniqueID const& user_uid,
    Configuration::Permission const& permission)
{
    auto const& user{getUserFromUID(user_uid)};
    return user.permissions.find(permission) != user.permissions.end();
}

std::pair<Configuration::EAuthState, Authentication::AuthKey> UsersCatalog::authenticateUser(
    std::string const& username,
    std::string_view password)
{
    std::string_view correct_password{};

    if (username == "root")
    {
        correct_password = m_root_password;
    }
    else
    {
        try
        {
            correct_password = getUserConfiguration(username).password;
        }
        catch (std::exception const& e)
        {
            return std::make_pair(Configuration::EAuthState::UnknwonUser, Authentication::AuthKey{});
        }
    }

    return std::make_pair(
        password == correct_password ? Configuration::EAuthState::Authenticated
                                     : Configuration::EAuthState::WrongPassword,
        Authentication::AuthKey{});
}

bool UsersCatalog::isUserAuthenticated(
    Common::CppExtensions::UniqueID const& user_uid,
    Authentication::AuthKey const& auth_key) const
{
    auto const it = m_authenticated_users.find(user_uid);
    return it != m_authenticated_users.end() && it->second == auth_key;
}

void UsersCatalog::deauthenticateUser(Common::CppExtensions::UniqueID const& user_uid)
{
    std::ignore = m_authenticated_users.erase(user_uid);
}

std::filesystem::path UsersCatalog::getUsersHomePath() const
{
    auto const c_users_home_path{
        Selenity::API::SystemCatalog::Instance().getLunarHomePath() / "users"};

    if (!std::filesystem::exists(c_users_home_path))
    {
        std::filesystem::create_directories(c_users_home_path);
    }

    return c_users_home_path;
}

std::filesystem::path UsersCatalog::getUserConfigurationFilePath(Common::CppExtensions::UniqueID user_uid) const
{
    return getUsersHomePath() / (user_uid.toString() + ".cfg");
}

void UsersCatalog::saveUserToDisk(Configuration::User const& user) const
{
    namespace Serializer = Common::CppExtensions::BinaryIO::Serializer;

    std::ofstream user_file(getUserConfigurationFilePath(user.uid), std::ios::trunc | std::ios::binary);

    auto const encrypted_name{Common::Cryptography::AES256::Instance().encrypt(
        Common::Cryptography::AES256::ByteArray(user.name.begin(), user.name.end()))};

    auto const encrypted_password{Common::Cryptography::AES256::Instance().encrypt(
        Common::Cryptography::AES256::ByteArray(user.password.begin(), user.password.end()))};

    Serializer::serialize(user_file, encrypted_name);
    Serializer::serialize(user_file, encrypted_password);
    // Serializer::serialize(user_file, user.name);
    // Serializer::serialize(user_file, user.password);
    Serializer::serialize(user_file, user.permissions);
    Serializer::serialize(user_file, user.uid);

    user_file.close();
}

Configuration::User UsersCatalog::loadUserFromDisk(std::filesystem::path const& user_file_path)
{
    namespace Deserializer = Common::CppExtensions::BinaryIO::Deserializer;

    std::ifstream user_file(user_file_path, std::ios::binary);

    Configuration::User user{};

    auto encrypted_username{Common::Cryptography::AES256::ByteArray{}};
    auto encrypted_password{Common::Cryptography::AES256::ByteArray{}};

    Deserializer::deserialize(user_file, encrypted_username);
    Deserializer::deserialize(user_file, encrypted_password);
    // Deserializer::deserialize(user_file, user.name);
    // Deserializer::deserialize(user_file, user.password);
    Deserializer::deserialize(user_file, user.permissions);
    Deserializer::deserialize(user_file, user.uid);

    auto const decrypted_username{Common::Cryptography::AES256::Instance().decrypt(encrypted_username)};
    user.name = std::string(decrypted_username.begin(), decrypted_username.end());

    auto const decrypted_password{Common::Cryptography::AES256::Instance().decrypt(encrypted_password)};
    user.password = std::string(decrypted_password.begin(), decrypted_password.end());

    user_file.close();

    return user;
}

Configuration::User UsersCatalog::loadUserFromDisk(Common::CppExtensions::UniqueID user_uid)
{
    auto const user_file_path{getUserConfigurationFilePath(user_uid)};

    if (!std::filesystem::exists(user_file_path))
    {
        throw std::runtime_error(Common::CppExtensions::StringUtils::stringify(
            "Configuration file for user '", user_uid, "' missing"));
    }

    return loadUserFromDisk(user_file_path);
}

Configuration::User& UsersCatalog::getUserFromUID(Common::CppExtensions::UniqueID user_uid)
{
    if (auto user_it = m_uid_to_config_cache.find(user_uid); user_it != m_uid_to_config_cache.end())
    {
        return user_it->second;
    }
    else
    {
        return m_uid_to_config_cache.emplace(user_uid, loadUserFromDisk(user_uid)).first->second;
    }
}

Configuration::User& UsersCatalog::getUserFromName(std::string const& username)
{
    return getUserFromUID(getUserUID(username));
}

std::filesystem::path UsersCatalog::getUsernamesFilePath() const
{
    return getUsersHomePath() / "usernames.db";
}

void UsersCatalog::saveToDisk() const
{
    // save usernames
    saveUserNamesToDisk();
}

void UsersCatalog::loadFromDisk()
{
    // load usernames
    if (!std::filesystem::exists(getUsernamesFilePath()))
    {
        saveUserNamesToDisk();
    }
    else
    {
        m_name_to_uid.clear();
        m_uid_to_config_cache.clear();

        std::ifstream usernames_file(getUsernamesFilePath(), std::ios::binary);
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(usernames_file, m_name_to_uid);
        usernames_file.close();
    }
}

void UsersCatalog::saveUserNamesToDisk() const
{
    std::ofstream usernames_file(getUsernamesFilePath(), std::ios::trunc | std::ios::binary);
    Common::CppExtensions::BinaryIO::Serializer::serialize(usernames_file, m_name_to_uid);
    usernames_file.close();
}

UsersCatalog::~UsersCatalog()
{
    saveToDisk();
}

Common::CppExtensions::UniqueID UsersCatalog::getUserUID(std::string const& username) const
{
    auto it = m_name_to_uid.find(username);

    if (it == m_name_to_uid.end())
    {
        throw std::runtime_error{
            Common::CppExtensions::StringUtils::stringify("User", username, "does not exist")};
    }

    return it->second;
}

void UsersCatalog::setRootPassword(std::string root_password)
{
    m_root_password = std::move(root_password);
}

std::string_view UsersCatalog::getRootPassword() const
{
    return m_root_password;
}

Configuration::User const& UsersCatalog::getUserConfiguration(std::string const& username)
{
    return getUserFromName(username);
}

} // namespace LunarDB::Celestial::API
