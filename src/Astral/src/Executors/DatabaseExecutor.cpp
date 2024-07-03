#include <filesystem>

#include "LunarDB/Astral/QueryExecutors.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)
namespace LunarDB::Astral::Implementation {

void Database::execute(Moonlight::API::ParsedQuery const& parsed_query)
{
    CLOG_VERBOSE("Database::execute()");

    auto const& query = parsed_query.get<Common::QueryData::Database>();

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};

    switch (query.operation_type)
    {
    case Common::QueryData::Primitives::EDatabaseOperationType::Create: {
        system_catalog.createDatabase(query.name);
        break;
    }
    case Common::QueryData::Primitives::EDatabaseOperationType::Drop: {
        system_catalog.dropDatabase(query.name);
        break;
    }
    case Common::QueryData::Primitives::EDatabaseOperationType::Use: {
        system_catalog.useDatabase(query.name);
        break;
    }
    case Common::QueryData::Primitives::EDatabaseOperationType::Backup: {
        CLOG_INFO("Database::execute(): Starting backup of", query.name, "to", *query.backup_path);

        std::optional<std::string> current_db_name{};
        try
        {
            current_db_name.emplace(system_catalog.getDatabaseInUse()->getName());
        }
        catch (std::exception const& e)
        {
            current_db_name = std::nullopt;
        }

        std::filesystem::path backup_path{*query.backup_path};

        if (std::filesystem::exists(backup_path))
        {
            throw std::runtime_error{"Backup location is not empty, please change it."};
        }
        else
        {
            std::filesystem::create_directories(backup_path);
            if (!std::filesystem::is_directory(backup_path))
            {
                std::filesystem::remove(backup_path);
                throw std::runtime_error{"Backup path is not a directory"};
            }
        }

        system_catalog.useDatabase(query.name);

        std::filesystem::copy(
            system_catalog.getDatabaseInUse()->getHomePath(),
            backup_path,
            std::filesystem::copy_options::recursive);

        CLOG_INFO(
            "Database::execute(): Backup of",
            query.name,
            "to",
            *query.backup_path,
            "finished successfully");

        if (current_db_name.has_value())
        {
            system_catalog.useDatabase(*current_db_name);
        }

        break;
    }
    default:
        throw std::logic_error{"Cannot execute unknown query operation"};
    }
}

} // namespace LunarDB::Astral::Implementation
