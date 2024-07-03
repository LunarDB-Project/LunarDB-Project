#include <cassert>
#include <queue>
#include <unordered_set>

#include "LunarDB/Astral/QueryExecutors.hpp"
#include "LunarDB/Common/CppExtensions/UniqueID.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/Selenity/Managers/Configurations.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_ASTRAL)

namespace LunarDB::Astral::Implementation {

namespace {

LunarDB::Common::QueryData::Schema makeBlendedFrom(
    LunarDB::Common::QueryData::Schema const& in_schema,
    LunarDB::Selenity::API::SchemasCatalog& schemas_catalog)
{
    LunarDB::Common::QueryData::Schema out{};
    out.name = in_schema.name + "_blended";

    std::queue<LunarDB::Common::QueryData::Schema::Field> fields_to_process{};
    for (auto const& field : in_schema.fields)
    {
        fields_to_process.emplace(field);
    }

    std::unordered_set<std::string> inherits_set{};
    inherits_set.reserve(in_schema.inherits.size());
    for (auto const& inherit : in_schema.inherits)
    {
        inherits_set.emplace(inherit);
    }

    out.fields.reserve(in_schema.fields.size());
    while (!fields_to_process.empty())
    {
        auto current_field = std::move(fields_to_process.front());
        fields_to_process.pop();

        try
        {
            std::ignore = LunarDB::Selenity::API::Managers::Configurations::FieldDataType::toLiteral(
                current_field.type);
            out.fields.emplace_back(std::move(current_field));
        }
        catch (std::exception const& e)
        {
            auto const& field_schema{schemas_catalog.getSchema(current_field.type)};

            for (auto const& field : field_schema.fields)
            {
                std::ignore = fields_to_process.emplace(field);
            }

            for (auto const& inherit : field_schema.inherits)
            {
                std::ignore = inherits_set.emplace(inherit);
            }
        }
    }
    out.inherits.assign(inherits_set.begin(), inherits_set.end());

    return out;
}

void createSingle(Common::QueryData::Create const& config)
{
    auto const& single{*config.single};
    auto database{Selenity::API::SystemCatalog::Instance().getDatabaseInUse()};

    if (single.blended)
    {
        auto& schemas_catalog{LunarDB::Selenity::API::SchemasCatalog::Instance()};
        auto const& not_blended_schema{schemas_catalog.getSchema(single.schema_name)};

        auto blended_schema{makeBlendedFrom(not_blended_schema, schemas_catalog)};
        schemas_catalog.createSchema(blended_schema);

        database->createCollection(
            single.structure_name, blended_schema.name, config.structure_type, {});
    }
    else
    {
        database->createCollection(
            single.structure_name, single.schema_name, config.structure_type, single.bindings);
    }
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

    static_cast<bool>(query.single) ? LunarDB::Astral::Implementation::createSingle(query)
                                    : LunarDB::Astral::Implementation::createMultiple(query);
}

} // namespace LunarDB::Astral::Implementation
