#include <algorithm>

#include "LunarDB/Selenity/Managers/Configurations.hpp"
#include "LunarDB/Selenity/Managers/DatabaseManager.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

namespace LunarDB::Selenity::API::Managers {

DatabaseManager::DatabaseManager(std::shared_ptr<Configurations::DatabaseConfiguration> config)
    : CatalogManager(config)
{
    loadConfigs();
}

DatabaseManager::~DatabaseManager()
{
    saveConfigs();
}

std::filesystem::path DatabaseManager::getHomePath() const
{
    return m_catalog.config->home;
}

std::filesystem::path DatabaseManager::getDataHomePath() const
{
    auto const c_data_home_path{getHomePath() / "collections"};
    if (!std::filesystem::exists(c_data_home_path))
    {
        std::filesystem::create_directories(c_data_home_path);
    }
    return c_data_home_path;
}

std::filesystem::path DatabaseManager::getCatalogFilePath() const
{
    return getHomePath() / "database_catalog.ldb";
}

Common::CppExtensions::UniqueID const& DatabaseManager::getUID() const
{
    return m_catalog.config->uid;
}

std::string_view DatabaseManager::getName() const
{
    return m_catalog.config->name;
}

void DatabaseManager::createCollection(
    std::string const& name,
    std::string const& schema_name,
    Common::QueryData::Primitives::EStructureType collection_type,
    std::vector<Common::QueryData::Create::Single::Binding> const& bindings)
{
    if (m_catalog.name_to_config.contains(name))
    {
        throw std::runtime_error("Collection already exists");
    }

    std::vector<Common::QueryData::Create::Single::Binding> updated_bindings{bindings};

    auto const& schema = SchemasCatalog::Instance().getSchema(schema_name);

    for (auto const& field : schema.fields)
    {
        try
        {
            std::ignore = Managers::Configurations::FieldDataType::toLiteral(field.type);
        }
        catch (std::exception const& e)
        {
            auto new_collection_name{name + "_" + field.name};

            createCollection(new_collection_name, field.type, collection_type, {});

            updated_bindings.emplace_back(field.name, std::move(new_collection_name));
        }
    }

    auto catalog_entry_ptr = m_catalog.name_to_config
                                 .emplace(
                                     name,
                                     std::make_shared<Configurations::CollectionConfiguration>(
                                         name,
                                         getDataHomePath() / name,
                                         Common::CppExtensions::UniqueID::generate(),
                                         collection_type,
                                         schema_name,
                                         updated_bindings))
                                 .first->second;
    std::filesystem::create_directories(catalog_entry_ptr->home);

    auto uid{catalog_entry_ptr->uid};
    std::ignore = m_catalog.id_to_manager.emplace(
        std::move(uid), Collections::AbstractManager::Factory::create(catalog_entry_ptr));

    saveConfigs();
}

void DatabaseManager::dropCollection(std::string const& name, bool cascade)
{
    auto const catalog_entry_it{m_catalog.name_to_config.find(name)};
    if (catalog_entry_it == m_catalog.name_to_config.end())
    {
        throw std::runtime_error(LunarDB::Common::CppExtensions::StringUtils::stringify(
            "Collection '", name, "' does not exist"));
    }

    if (cascade)
    {
        for (auto const& [_, collection_uid] : catalog_entry_it->second->schema.bindings)
        {
            dropCollection(std::string{getCollection(collection_uid)->getName()}, true);
        }
    }

    std::filesystem::remove_all(catalog_entry_it->second->home);

    m_catalog.id_to_manager.erase(catalog_entry_it->second->uid);
    m_catalog.name_to_config.erase(catalog_entry_it);

    saveConfigs();
}

std::shared_ptr<Collections::AbstractManager> DatabaseManager::getCollection(std::string const& name)
{
    auto const catalog_entry_it = m_catalog.name_to_config.find(name);
    if (catalog_entry_it == m_catalog.name_to_config.end())
    {
        throw std::runtime_error(LunarDB::Common::CppExtensions::StringUtils::stringify(
            "Collection '", name, "' does not exist"));
    }

    assert(
        m_catalog.name_to_config.size() == m_catalog.id_to_manager.size() &&
        "Collection sizes differ, some data was lost");

    auto const manager_it = m_catalog.id_to_manager.find(catalog_entry_it->second->uid);

    assert(manager_it != m_catalog.id_to_manager.end() && "Collection manager not found");

    return manager_it->second;
}

std::shared_ptr<Collections::AbstractManager> DatabaseManager::getCollection(
    Common::CppExtensions::UniqueID const& collection_uid)
{
    auto const manager_it = m_catalog.id_to_manager.find(collection_uid);

    assert(manager_it != m_catalog.id_to_manager.end() && "Collection manager not found");

    return manager_it->second;
}

void DatabaseManager::rebind(
    std::string const& collection_name,
    std::string const& field_name,
    std::string const& to_collection_name,
    bool clean)
{
    auto collection_config{getCollection(collection_name)->getConfig()};

    if (collection_config->collection_type !=
        LunarDB::Selenity::API::Managers::Configurations::Implementation::CollectionConfiguration::ECollectionType::Table)
    {
        throw std::runtime_error{"Rebind is designed for tables only"};
    }

    auto& bindings{collection_config->schema.bindings};
    auto& fields{collection_config->schema.fields};

    auto field_it = std::find_if(fields.begin(), fields.end(), [&field_name](auto const& field) {
        return field.name == field_name;
    });

    if (field_it == fields.end())
    {
        throw std::runtime_error("Field does not exist");
    }

    if (field_it->type != Configurations::EFieldDataType::Rid)
    {
        throw std::runtime_error("Cannot rebind non record ID field");
    }

    auto binding_it = bindings.find(field_name);

    auto const& to_collection_uid{getCollection(to_collection_name)->getUID()};
    if (binding_it != bindings.end())
    {
        std::vector<std::string> collections{};
        if (clean)
        {
            dropCollection(std::string{getCollection(binding_it->second)->getName()}, true);
        }
        bindings.erase(binding_it);
    }
    std::ignore = bindings.emplace(field_name, to_collection_uid);

    saveConfigs();
}

std::shared_ptr<DatabaseManager> DatabaseManager::Factory::create(
    std::shared_ptr<Configurations::DatabaseConfiguration> config)
{
    return std::make_shared<DatabaseManager>(config);
}

} // namespace LunarDB::Selenity::API::Managers
