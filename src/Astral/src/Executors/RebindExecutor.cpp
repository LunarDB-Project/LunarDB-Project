#include "LunarDB/Astral/QueryExecutors.hpp"

#include "LunarDB/Selenity/SystemCatalog.hpp"

#include "LunarDB/Crescentum/Logger.hpp"
LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)

namespace LunarDB::Astral::Implementation {

void Rebind::execute(Moonlight::API::ParsedQuery const& parsed_query)
{
    CLOG_VERBOSE("Rebind::execute()");

    auto const& query = parsed_query.get<Common::QueryData::Rebind>();

    bool clean = static_cast<bool>(query.clean) ? *query.clean : false;

    Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->rebind(
        query.structure_name, query.field, query.bind_structure_name, clean);
}

} // namespace LunarDB::Astral::Implementation
