#include "QueryExecutors.hpp"

namespace LunarDB::Astral::Implementation {

namespace name {

} // namespace name

void Database::execute(const Moonlight::API::ParsedQuery& parsed_query, const Astral::API::QueryExecutorConfig& config)
{
    // TODO: Provide implementation
    const auto& query = parsed_query.get<QueryData::Database>();
}

} // namespace LunarDB::Astral::Implementation
