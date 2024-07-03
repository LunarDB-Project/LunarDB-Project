#include <cassert>

#include "LunarDB/Astral/QueryExecutors.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

#include "LunarDB/Crescentum/Logger.hpp"
LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)

namespace LunarDB::Astral::Implementation {

namespace {

void createSingle(Common::QueryData::Create const& config)
{
    auto const& single{*config.single};
    auto database{Selenity::API::SystemCatalog::Instance().getDatabaseInUse()};

    if (single.blended)
    {
        // TODO: Provide implementation
        throw std::runtime_error{
            "[~/lunardb/src/Astral/src/Executors/CreateExecutor.cpp:createSingle::blended] Not "
            "implemented yet..."};
    }

    database->createCollection(
        single.structure_name, single.schema_name, config.structure_type, single.bindings);
}

void createMultiple(Common::QueryData::Create const& config)
{
    // TODO: Provide implementation
    throw std::runtime_error{
        "[~/lunardb/src/Astral/src/Executors/CreateExecutor.cpp:createMultiple(...)] Not "
        "implemented yet..."};
}

} // namespace

void Create::execute(Moonlight::API::ParsedQuery const& parsed_query)
{
    CLOG_VERBOSE("Create::execute()");

    auto const& query = parsed_query.get<Common::QueryData::Create>();

    assert(
        (static_cast<bool>(query.single) || static_cast<bool>(query.multiple)) &&
        "Invalid parsed query received");

    if (query.is_volatile)
    {
        // TODO: Provide implementation
        // throw std::runtime_error{
        //     "[~/lunardb/src/Astral/src/Executors/CreateExecutor.cpp:is_volatile] Not implemented
        //     " "yet..."};
    }

    static_cast<bool>(query.single) ? createSingle(query) : createMultiple(query);
}

} // namespace LunarDB::Astral::Implementation
