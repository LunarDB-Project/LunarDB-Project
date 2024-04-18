#include <set>

#include "Errors.hpp"
#include "QueryParsers.hpp"
#include "Utils.hpp"

namespace LunarDB::Moonlight::Implementation {

using namespace CppExtensions;

namespace {

constexpr auto c_query_prefix{"grant"};

} // namespace

API::ParsedQuery Grant::parse(QueryExtractor extractor)
{
    auto out_parsed_query = API::ParsedQuery::make<QueryData::Grant>();
    auto& out = out_parsed_query.get<QueryData::Grant>();

    const auto grant = extractor.extractOne();
    Errors::assertKeywordEquals(grant, "grant");

    // parse permissions
    using namespace QueryData::Primitives;
    out.permissions = extractor.extractUniqueList<EUserPermissionType>(UserPermissionType::toLiteral);
    if (out.permissions.empty())
    {
        throw Errors::buildMissingError("permissions");
    }

    // parse user and structure names
    const auto [to, user_name, on, structure_name] = extractor.extractTuple<4>();
    if (!extractor.empty())
    {
        throw Errors::buildInvalidQueryFormatError(c_query_prefix);
    }

    Errors::assertKeywordEquals(to, "to");
    out.to_user = Errors::assertNotEmpty(user_name, "user name");

    if (!on.empty())
    {
        Errors::assertKeywordEquals(on, "on");
        out.structure_name = Errors::assertNotEmpty(structure_name, "structure name");
    }

    return out_parsed_query;
}

} // namespace LunarDB::Moonlight::Implementation
