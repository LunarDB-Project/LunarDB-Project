#include "Errors.hpp"
#include "QueryParsers.hpp"
#include "Utils.hpp"

namespace LunarDB::Moonlight::Implementation {

using namespace CppExtensions;

namespace {

constexpr auto c_query_prefix{ "index" };

} // namespace

API::ParsedQuery Index::parse(QueryExtractor extractor)
{
    auto out_parsed_query = API::ParsedQuery::make<QueryData::Index>();
    auto& out = out_parsed_query.get<QueryData::Index>();

    const auto [index, structure_name] = extractor.extractTuple<2>();

    Errors::assertKeywordEquals(index, "index");

    out.on_structure_name = Errors::assertNotEmpty(structure_name, "structure name");

    const auto unique_or_as = extractor.extractOne();
    if (StringUtils::equalsIgnoreCase(unique_or_as, "unique"))
    {
        out.unique = true;
        Errors::assertKeywordEquals(extractor.extractOne(), "as");
    }
    else
    {
        out.unique = false;
        Errors::assertKeywordEquals(unique_or_as, "as");

    }

    const auto index_name = extractor.extractOne();
    out.name = Errors::assertNotEmpty(index_name, "index name");

    if (!extractor.empty())
    {
        const auto using_ = extractor.extractOne();
        Errors::assertKeywordEquals(using_, "using");

        out.using_fields = extractor.extractUniqueList<std::string>([](std::string_view sv) {
            auto str = std::string(sv);
            Errors::assertValidIdentifier(str);
            return std::move(str);});
        if (out.using_fields.empty()) { throw Errors::buildMissingError("index fields"); }
    }

    return out_parsed_query;
}

} // namespace LunarDB::Moonlight::Implementation
