#pragma once

#include <memory>
#include <nlohmann/json.hpp>

#include "LunarDB/Common/QueryData/QueryData.hpp"
#include "LunarDB/Selenity/Managers/Configurations.hpp"

namespace LunarDB::Selenity::API::Managers::Collections {

class AbstractManager
{
public: // lifecycle
    AbstractManager(std::shared_ptr<Configurations::CollectionConfiguration> config);

public: // Factory
    struct Factory
    {
        static std::shared_ptr<AbstractManager> create(
            std::shared_ptr<Configurations::CollectionConfiguration> config);
    };

public: // CollectionEntry
    class ICollectionEntry
    {
    public: // lifecycle
        ICollectionEntry() = default;
        virtual ~ICollectionEntry() = default;
        ICollectionEntry(ICollectionEntry const&) = default;
        ICollectionEntry& operator=(ICollectionEntry const&) = default;
        ICollectionEntry(ICollectionEntry&&) noexcept = default;
        ICollectionEntry& operator=(ICollectionEntry&&) noexcept = default;

    public: // interface API
        virtual nlohmann::json const& getJSON() const = 0;
        virtual nlohmann::json& getJSON() = 0;
    };

public: // public API
    std::shared_ptr<Configurations::CollectionConfiguration> getConfig();
    Common::CppExtensions::UniqueID const& getUID() const;
    std::string_view getName() const;

    virtual void insert(std::vector<Common::QueryData::Insert::Object> const& objects) = 0;

    virtual std::vector<std::unique_ptr<ICollectionEntry>> select(
        Common::QueryData::Select const& config) const = 0;

    virtual void deleteWhere(Common::QueryData::WhereClause const& where) = 0;

    virtual void update(Common::QueryData::Update const& config) = 0;

    virtual void undoInsert(nlohmann::json json, bool is_last_call) = 0;
    virtual void undoUpdate(nlohmann::json json, bool is_last_call) = 0;
    virtual void undoDelete(nlohmann::json json, bool is_last_call) = 0;

    std::filesystem::path getDataHomePath() const;

    void truncate() const;

protected: // fields
    std::shared_ptr<Configurations::CollectionConfiguration> m_collection_config;
};

} // namespace LunarDB::Selenity::API::Managers::Collections
