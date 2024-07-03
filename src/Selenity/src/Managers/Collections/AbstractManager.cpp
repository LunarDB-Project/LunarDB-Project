#include "LunarDB/Selenity/Managers/Collections/AbstractManager.hpp"
#include "LunarDB/Selenity/Managers/Collections/DocumentManager.hpp"
#include "LunarDB/Selenity/Managers/Collections/TableManager.hpp"

namespace LunarDB::Selenity::API::Managers::Collections {

AbstractManager::AbstractManager(std::shared_ptr<Configurations::CollectionConfiguration> config)
    : m_collection_config{config}
{
}

std::shared_ptr<AbstractManager> AbstractManager::Factory::create(
    std::shared_ptr<Configurations::CollectionConfiguration> config)
{
    switch (config->collection_type)
    {
    case Configurations::CollectionConfiguration::ECollectionType::Table:
        return std::make_shared<Managers::Collections::TableManager>(config);
    case Configurations::CollectionConfiguration::ECollectionType::Document:
        return std::make_shared<Managers::Collections::DocumentManager>(config);
    case Configurations::CollectionConfiguration::ECollectionType::Undefined:
    default:
        throw std::runtime_error("Undefined collection type");
    }
}

Common::CppExtensions::UniqueID const& AbstractManager::getUID() const
{
    return m_collection_config->uid;
}

std::string_view AbstractManager::getName() const
{
    return m_collection_config->name;
}

std::shared_ptr<Configurations::CollectionConfiguration> AbstractManager::getConfig()
{
    return m_collection_config;
}

std::filesystem::path AbstractManager::getDataHomePath() const
{
    auto const data_home_path{m_collection_config->home / "data"};
    if (!std::filesystem::exists(data_home_path))
    {
        std::filesystem::create_directories(data_home_path);
    }
    return data_home_path;
}

void AbstractManager::truncate() const
{
    auto const data_path{getDataHomePath()};
    if (std::filesystem::exists(data_path))
    {
        std::filesystem::remove_all(data_path);
        std::filesystem::create_directories(data_path);
    }
}

} // namespace LunarDB::Selenity::API::Managers::Collections
