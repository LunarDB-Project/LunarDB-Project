#pragma once

#include <filesystem>
#include <memory>
#include <string_view>

#include "LunarDB/Selenity/Managers/CatalogManager.hpp"
#include "LunarDB/Selenity/Managers/Collections/AbstractManager.hpp"
#include "LunarDB/Selenity/Managers/Configurations.hpp"

namespace LunarDB::Selenity::API::Managers {

class DatabaseManager
    : public CatalogManager<Configurations::DatabaseConfiguration, Configurations::CollectionConfiguration, Collections::AbstractManager>
{
public: // lifecycle
    DatabaseManager(std::shared_ptr<Configurations::DatabaseConfiguration> config);

    DatabaseManager() = default;
    ~DatabaseManager();
    DatabaseManager(DatabaseManager const&) = delete;
    DatabaseManager& operator=(DatabaseManager const&) = delete;
    DatabaseManager(DatabaseManager&&) noexcept = delete;
    DatabaseManager& operator=(DatabaseManager&&) noexcept = delete;

public: // Factory
    struct Factory
    {
        static std::shared_ptr<DatabaseManager> create(
            std::shared_ptr<Configurations::DatabaseConfiguration> config);
    };

public: // public API
    void createCollection(
        std::string const& name,
        std::string const& schema_name,
        Common::QueryData::Primitives::EStructureType type,
        std::vector<Common::QueryData::Create::Single::Binding> const& bindings);

    void dropCollection(std::string const& name, bool cascade);

    void rebind(
        std::string const& collection_name,
        std::string const& field_name,
        std::string const& to_collection_name,
        bool clean);

    Common::CppExtensions::UniqueID const& getUID() const;
    std::string_view getName() const;

    std::shared_ptr<Collections::AbstractManager> getCollection(std::string const& collection_name);
    std::shared_ptr<Collections::AbstractManager> getCollection(
        Common::CppExtensions::UniqueID const& collection_uid);

public: // overrides
    std::filesystem::path getHomePath() const override;
    std::filesystem::path getDataHomePath() const override;
    std::filesystem::path getCatalogFilePath() const override;
};

} // namespace LunarDB::Selenity::API::Managers
