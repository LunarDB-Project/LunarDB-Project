#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "LunarDB/Common/CppExtensions/BinaryIO.hpp"
#include "LunarDB/Common/CppExtensions/Singleton.hpp"
#include "LunarDB/Selenity/Managers/CatalogManager.hpp"
#include "LunarDB/Selenity/Managers/Configurations.hpp"
#include "LunarDB/Selenity/Managers/DatabaseManager.hpp"

#define LUNARDB_ROOT_USER "root"
#define LUNARDB_NO_USER ""

namespace LunarDB::Selenity::API {

class SystemCatalog
    : public Common::CppExtensions::DesignPatterns::Singleton<SystemCatalog>
    , public Managers::CatalogManager<
          Managers::Configurations::NoConfiguration,
          Managers::Configurations::DatabaseConfiguration,
          Managers::DatabaseManager>
{
public: // public API
    ///
    /// @brief Self explanatory
    /// @return LunarDB home path
    ///
    std::filesystem::path getLunarHomePath() const;

    ///
    /// @brief Create database config, store it to internal configs and save configuration to disk
    /// @param [in] name -> database name
    /// @throws std::runtime_error if such database config already exists
    ///
    void createDatabase(std::string const& name);

    ///
    /// @brief Remove database config from internal ocnfigs and save configuration to disk
    /// @param [in] name -> database name
    /// @throws std::runtime_error if such database config does not exist
    ///
    void dropDatabase(std::string const& name);

    ///
    /// @brief Choose which database config will be used as work database
    /// @param [in] name -> database name
    /// @throws std::runtime_error if such database config does not exist
    ///
    void useDatabase(std::string name);

    ///
    /// @brief Self explanatory
    /// @throw std::runtime_error if no database in use
    ///
    std::shared_ptr<Managers::DatabaseManager> getDatabaseInUse();

    ///
    /// @brief Self explanatory
    /// @throw std::runtime_error if no database in use
    ///
    std::shared_ptr<Managers::DatabaseManager> const getDatabaseInUse() const;

    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> const& getCurrentSelection()
        const;

    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>>&& extractCurrentSelection();

    void setCurrentSelection(
        std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selection);

    void clearCurrentSelection();

    void setCurrentUser(std::string user);
    std::string const& getCurrentUser() const;

    ///
    /// @return true if any database is used as work database
    ///
    bool usingDatabase() const;

protected: // lifecycle
    SystemCatalog() = default;

public: // lifecycle
    ~SystemCatalog();

private: // singleton
    LUNAR_SINGLETON_INIT(SystemCatalog);

private: // private API
    void loadConfiguration();

    void loadSystemConfiguration();

public: // data structures
    struct Config
    {
        std::filesystem::path home_path{};

        struct Encryption
        {
            std::string key{};
            std::string iv{};
        };
        Encryption encryption{};
    };

public: // overrides
    std::filesystem::path getHomePath() const override;
    std::filesystem::path getDataHomePath() const override;
    std::filesystem::path getCatalogFilePath() const override;
    Config const& getSystemConfiguration() const;

private: // data structures
    struct DatabaseInUse
    {
        std::string name;
        Common::CppExtensions::UniqueID uid;
    };

private: // fields
    std::optional<DatabaseInUse> m_database_in_use{std::nullopt};

    std::string m_current_user{LUNARDB_NO_USER};

    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>>
        m_current_selection{};

    Config m_config;
};

} // namespace LunarDB::Selenity::API
