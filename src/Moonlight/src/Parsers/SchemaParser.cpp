#include "Errors.hpp"
#include "LunarDB/Moonlight/QueryParsers.hpp"
#include "Utils.hpp"

#include "LunarDB/Common/CppExtensions/StringUtils.hpp"

#include "LunarDB/Crescentum/Logger.hpp"
LUNAR_DECLARE_LOGGER_MODULE(MODULE_MOONLIGHT)

namespace LunarDB::Moonlight::Implementation {

namespace StringUtils = LunarDB::Common::CppExtensions::StringUtils;

namespace {

constexpr auto c_query_prefix{"schema"};

Common::QueryData::Schema::Field parseField(std::string_view str)
{
    using namespace std::literals;

    Common::QueryData::Schema::Field out{};

    auto const separator_pos = str.find_first_of(':');
    if (separator_pos == std::string_view::npos)
    {
        throw Errors::buildMissingError(":");
    }

    auto field_name = str.substr(0, separator_pos);
    StringUtils::trim(field_name);
    out.name = Errors::assertNotEmpty(field_name, "field name");
    Errors::assertValidIdentifier(out.name);

    auto type = str.substr(separator_pos + 1, str.length());

    StringUtils::trim(type);

    Errors::assertNotEmpty(type, "type");

    if (type.back() == '?')
    {
        out.nullable = true;
        type.remove_suffix(1);
        StringUtils::trim(type);
    }
    else
    {
        out.nullable = false;
    }

    if (StringUtils::startsWithIgnoreCase(type, "ArrayOf<"))
    {
        if (type.back() == '>')
        {
            type.remove_suffix(1);
            type.remove_prefix("ArrayOf<"sv.length());
        }
        else
        {
            throw Errors::buildMissingError(">");
        }
        out.array = true;
    }
    else if (type.back() == '>')
    {
        throw Errors::buildMissingError("ArrayOf<");
    }
    else
    {
        out.array = false;
    }

    StringUtils::trim(type);
    out.type = Errors::assertNotEmpty(type, "type");
    Errors::assertValidIdentifier(out.type);

    return out;
}

} // namespace

API::ParsedQuery Schema::parse(QueryExtractor extractor)
{
    CLOG_VERBOSE("Schema::parse()");

    auto out_parsed_query = API::ParsedQuery::make<Common::QueryData::Schema>();
    auto& out = out_parsed_query.get<Common::QueryData::Schema>();

    auto const [schema, schema_name] = extractor.extractTuple<2>();
    if (extractor.empty())
    {
        throw Errors::buildInvalidQueryFormatError(c_query_prefix);
    }

    Errors::assertKeywordEquals(schema, "schema");

    out.name = Errors::assertNotEmpty(schema_name, "schema name");

    if (Common::CppExtensions::StringUtils::startsWithIgnoreCase(extractor.data(), "inherits"))
    {
        auto const [inherits, schema] = extractor.extractTuple<2>();
        out.inherits.emplace_back(schema);
    }

    out.fields = extractor.extractList<Common::QueryData::Schema::Field>(
        parseField, ';', std::make_pair('{', '}'));

    if (!extractor.empty())
    {
        throw Errors::buildInvalidQueryFormatError(c_query_prefix);
    }

    if (out.fields.empty() && out.inherits.empty())
    {
        throw Errors::buildError("Missing fields nor inherited schema");
    }

    return out_parsed_query;
}

} // namespace LunarDB::Moonlight::Implementation
