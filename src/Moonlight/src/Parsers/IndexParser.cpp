#include "QueryParsers.hpp"
#include "Errors.hpp"
#include "Utils.hpp"

namespace LunarDB::Moonlight::Implementation {

using namespace Utils;
using namespace std::literals;

namespace {

constexpr auto c_query_prefix{ "index" };

} // namespace

PROVIDE_QUERY_PARSER_IMPL(Index, c_query_prefix)
{
    DECLARE_PARSED_QUERY(Index);

    const auto [index, structure_name] = extractor.extractTuple<2>();

    Errors::assertKeywordEquals(index, "index");

    out.on_structure_name = Errors::assertNotEmpty(structure_name, "structure name");

    const auto unique_or_as = extractor.extractOne();
    if (Utils::equalsIgnoreCase(unique_or_as, "unique"))
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
            if (!Utils::isValidIdentifier(str)) { throw Errors::buildError("Invalid field identifiers found in index list"); }
            return std::move(str);});
        if (out.using_fields->empty()) { throw Errors::buildMissingError("index fields"); }
    }

    RETURN_PARSED_QUERY;
}

} // namespace LunarDB::Moonlight::Implementation
