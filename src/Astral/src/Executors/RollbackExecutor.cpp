#include "QueryExecutors.hpp"

namespace LunarDB::Astral::Implementation {

namespace name {

} // namespace name

void Rollback::execute(
    Moonlight::API::ParsedQuery const& parsed_query,
    Astral::API::SelenityDependencies const& config)
{
    // TODO: Provide implementation
    auto const& query = parsed_query.get<Common::QueryData::Rollback>();
}

} // namespace LunarDB::Astral::Implementation
