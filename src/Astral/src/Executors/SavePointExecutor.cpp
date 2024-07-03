#include "LunarDB/Astral/QueryExecutors.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Crescentum/Logger.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)

namespace LunarDB::Astral::Implementation {

void SavePoint::execute(Moonlight::API::ParsedQuery const& parsed_query)
{
    CLOG_VERBOSE("SavePoint::execute()");

    auto const& query = parsed_query.get<Common::QueryData::SavePoint>();

    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().savepoint(query.hash);
}

} // namespace LunarDB::Astral::Implementation
