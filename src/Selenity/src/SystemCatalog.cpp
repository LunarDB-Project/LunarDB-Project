#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <random>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Common/CppExtensions/Testing/LunarTestGuard.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_SELENITY);

// #define LUNAR_ENABLE_TESTING

namespace LunarDB::Selenity::API {

LUNAR_SINGLETON_INIT_IMPL(SystemCatalog)
{
    loadConfiguration();
}

SystemCatalog::~SystemCatalog()
{
    saveConfigs();
}

std::filesystem::path SystemCatalog::getLunarHomePath() const
{
#ifdef LUNAR_ENABLE_TESTING
    std::filesystem::path lunar_home_path{LUNAR_TESTING_HOME_PATH};
#else
    auto const& lunar_home_path{m_config.home_path};
#endif

    if (!lunar_home_path.empty() && !std::filesystem::exists(lunar_home_path))
    {
        std::filesystem::create_directories(lunar_home_path);
    }

    return lunar_home_path;
}

std::filesystem::path SystemCatalog::getHomePath() const
{
    auto const c_data_home_path{getLunarHomePath() / "databases"};
    if (!m_config.home_path.empty() && !std::filesystem::exists(c_data_home_path))
    {
        std::filesystem::create_directories(c_data_home_path);
    }
    return c_data_home_path;
}

std::filesystem::path SystemCatalog::getDataHomePath() const
{
    return getHomePath();
}

std::filesystem::path SystemCatalog::getCatalogFilePath() const
{
    return getHomePath() / "system_catalog.ldb";
}

void SystemCatalog::loadConfiguration()
{
    m_database_in_use = std::nullopt;
    loadConfigs();
    loadSystemConfiguration();
}

void SystemCatalog::createDatabase(std::string const& name)
{
    if (m_catalog.name_to_config.contains(name))
    {
        throw std::runtime_error("Database already exists");
    }

    auto catalog_entry_ptr =
        m_catalog.name_to_config
            .emplace(
                name,
                std::make_shared<Managers::Configurations::DatabaseConfiguration>(
                    name, getDataHomePath() / name, Common::CppExtensions::UniqueID::generate()))
            .first->second;
    std::filesystem::create_directories(catalog_entry_ptr->home);

    auto const uid{catalog_entry_ptr->uid};
    std::ignore = m_catalog.id_to_manager.emplace(
        uid, std::make_shared<Managers::DatabaseManager>(catalog_entry_ptr));

    saveConfigs();
}

void SystemCatalog::dropDatabase(std::string const& name)
{
    auto const catalog_entry_it{m_catalog.name_to_config.find(name)};
    if (catalog_entry_it == m_catalog.name_to_config.end())
    {
        throw std::runtime_error("Database does not exist");
    }

    if (static_cast<bool>(m_database_in_use) && m_database_in_use->name == name)
    {
        m_database_in_use = std::nullopt;
    }

    std::filesystem::remove_all(catalog_entry_it->second->home);

    m_catalog.id_to_manager.erase(catalog_entry_it->second->uid);
    m_catalog.name_to_config.erase(catalog_entry_it);

    saveConfigs();
}

void SystemCatalog::useDatabase(std::string name)
{
    auto const catalog_entry_it{m_catalog.name_to_config.find(name)};
    if (catalog_entry_it == m_catalog.name_to_config.end())
    {
        throw std::runtime_error("Database does not exist");
    }

    m_database_in_use.emplace(std::move(name), catalog_entry_it->second->uid);
}

bool SystemCatalog::usingDatabase() const
{
    return static_cast<bool>(m_database_in_use);
}

std::shared_ptr<Managers::DatabaseManager> SystemCatalog::getDatabaseInUse()
{
    if (!static_cast<bool>(m_database_in_use))
    {
        throw std::runtime_error("No database in use");
    }

    return m_catalog.id_to_manager.find(m_database_in_use->uid)->second;
}

std::shared_ptr<Managers::DatabaseManager> const SystemCatalog::getDatabaseInUse() const
{
    if (!static_cast<bool>(m_database_in_use))
    {
        throw std::runtime_error("No database in use");
    }

    return m_catalog.id_to_manager.find(m_database_in_use->uid)->second;
}

std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> const& SystemCatalog::
    getCurrentSelection() const
{
    return m_current_selection;
}

std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>>&& SystemCatalog::extractCurrentSelection()
{
    return std::move(m_current_selection);
}

void SystemCatalog::setCurrentSelection(
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selection)
{
    m_current_selection = std::move(selection);
}

void SystemCatalog::clearCurrentSelection()
{
    m_current_selection.clear();
}

std::string generateHexString(size_t length = 32)
{
    // Ensure length is even, as each byte is represented by two hexadecimal digits.
    if (length % 2 != 0)
    {
        throw std::invalid_argument("Length must be an even number.");
    }

    // Create a random device and generator
    std::random_device rd{};
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 255);

    std::ostringstream oss{};

    for (auto const _ : std::ranges::iota_view{0u, static_cast<std::size_t>(length / 2)})
    {
        auto random_byte = distribution(generator);
        oss << std::hex << std::setw(2) << std::setfill('0') << random_byte;
    }

    return oss.str();
}

std::string generateRandomDigitString(size_t length = 16)
{
    std::random_device rd{};
    std::mt19937 generator(rd());
    std::uniform_int_distribution<int> distribution(0, 9);

    std::ostringstream oss{};

    for (auto const _ : std::ranges::iota_view{0u, length})
    {
        auto random_digit = distribution(generator);
        oss << random_digit;
    }

    return oss.str();
}

void SystemCatalog::loadSystemConfiguration()
{
    auto const config_file_path{std::filesystem::current_path() / "lunardb.config.json"};
    if (!std::filesystem::exists(config_file_path))
    {
        m_config = Config{};
        m_config.home_path = std::filesystem::current_path() / "lunardb";

        if (!std::filesystem::exists(m_config.home_path))
        {
            std::filesystem::create_directories(m_config.home_path);
        }

        nlohmann::json config{};

        config["home_path"] = m_config.home_path;
        config["encryption"]["key"] = generateRandomDigitString();
        config["encryption"]["iv"] = generateHexString();

        std::ofstream config_file(config_file_path);
        if (config_file.is_open())
        {
            config_file << config.dump(4);
            config_file.close();
        }
        else
        {
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not open", config_file_path)};
            CLOG_ERROR("SystemCatalog::loadSystemConfiguration(): Could not open", config_file_path);
        }
    }
    else
    {
        std::ifstream config_file(config_file_path);
        if (config_file.is_open())
        {
            nlohmann::json config{};
            config_file >> config;
            config_file.close();

            if (config.contains("home_path"))
            {
                m_config.home_path = std::string{config["home_path"]};
            }
            else
            {
                throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
                    "\"home_path\" key does not exist in ", config_file_path)};
            }

            if (config.contains("encryption"))
            {
                auto encryption = config["encryption"];

                if (encryption.contains("key"))
                {
                    m_config.encryption.key = std::string{encryption["key"]};
                }
                else
                {
                    throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
                        "\"encryption.key\" key does not exist in ", config_file_path)};
                }

                if (encryption.contains("iv"))
                {
                    m_config.encryption.iv = std::string{encryption["iv"]};
                }
                else
                {
                    throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
                        "\"encryption.iv\" key does not exist in ", config_file_path)};
                }
            }
            else
            {
                throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
                    "\"encryption\" key does not exist in ", config_file_path)};
            }
        }
        else
        {
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not open", config_file_path)};
            CLOG_ERROR("SystemCatalog::loadSystemConfiguration(): Could not open", config_file_path);
        }
    }
}

void SystemCatalog::setCurrentUser(std::string user)
{
    m_current_user = std::move(user);
}

std::string const& SystemCatalog::getCurrentUser() const
{
    return m_current_user;
}

SystemCatalog::Config const& SystemCatalog::getSystemConfiguration() const
{
    return m_config;
}

} // namespace LunarDB::Selenity::API
