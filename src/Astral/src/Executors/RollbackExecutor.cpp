#include "LunarDB/Astral/QueryExecutors.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Crescentum/Logger.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)

namespace LunarDB::Astral::Implementation {

void Rollback::execute(Moonlight::API::ParsedQuery const& parsed_query)
{
    CLOG_VERBOSE("Rollback::execute()");

    // TODO: Provide implementation
    auto const& query = parsed_query.get<Common::QueryData::Rollback>();

    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(
        LunarDB::BrightMoon::API::Transactions::RollbackTransactionData{});
}

} // namespace LunarDB::Astral::Implementation
