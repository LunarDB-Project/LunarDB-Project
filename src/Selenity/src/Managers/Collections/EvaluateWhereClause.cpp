#include <algorithm>
#include <ranges>
#include <regex>
#include <string>
#include <variant>

#include "LunarDB/Moonlight/QueryExtractor.hpp"
#include "LunarDB/Selenity/Managers/Collections/EvaluateWhereClause.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

using namespace std::string_literals;

namespace LunarDB::Selenity::API::Managers::Collections::WhereClause {

namespace {

using FieldData = std::variant<std::string, bool, int, float>;

FieldData convert(std::string const& value, Configurations::EFieldDataType to_type)
{
    static std::regex s_between_rhs_regex{
        ".*and.*", std::regex_constants::ECMAScript | std::regex_constants::icase};

    if (std::regex_match(value, s_between_rhs_regex))
    {
        return value;
    }

    switch (to_type)
    {
    case Configurations::EFieldDataType::Rid:
    case Configurations::EFieldDataType::String:
    case Configurations::EFieldDataType::DateTime:
        return value;
    case Configurations::EFieldDataType::Boolean:
        if (value == "true")
        {
            return true;
        }
        else if (value == "false")
        {
            return false;
        }
        assert(false && "Trying to convert unknown boolean value");
    case Configurations::EFieldDataType::Integer:
        try
        {
            return std::stoi(value);
        }
        catch (std::invalid_argument const& e)
        {
            throw std::runtime_error("Cannot convert '"s + value + "' to Integer; " + e.what());
        }
        catch (std::out_of_range const& e)
        {
            throw std::runtime_error("Value '"s + value + "' out of Integer range; " + e.what());
        }
    case Configurations::EFieldDataType::Float:
        try
        {
            return std::stof(value);
        }
        catch (std::invalid_argument const& e)
        {
            throw std::runtime_error("Cannot convert '"s + value + "' to Float; " + e.what());
        }
        catch (std::out_of_range const& e)
        {
            throw std::runtime_error("Value '"s + value + "' out of Float range; " + e.what());
        }
    case Configurations::EFieldDataType::None:
        [[fallthrough]];
    default:
        assert(false && "Trying to convert not supported field data type");
        break;
    }
}

FieldData sameTypeAs(FieldData const& source, std::string const& what)
{
    if (std::holds_alternative<bool>(source))
    {
        if (what == "true")
        {
            return true;
        }
        else if (what == "false")
        {
            return false;
        }
    }
    else if (std::holds_alternative<int>(source))
    {
        try
        {
            return std::stoi(what);
        }
        catch (std::invalid_argument const& e)
        {
            throw std::runtime_error("Cannot convert '"s + what + "' to Integer; " + e.what());
        }
        catch (std::out_of_range const& e)
        {
            throw std::runtime_error("Value '"s + what + "' out of Integer range; " + e.what());
        }
    }
    else if (std::holds_alternative<float>(source))
    {
        try
        {
            return std::stof(what);
        }
        catch (std::invalid_argument const& e)
        {
            throw std::runtime_error("Cannot convert '"s + what + "' to Float; " + e.what());
        }
        catch (std::out_of_range const& e)
        {
            throw std::runtime_error("Value '"s + what + "' out of Float range; " + e.what());
        }
    }

    return what;
}

// TODO: Refactor execution paths
std::pair<FieldData, FieldData> getLhsRhs(
    nlohmann::json const& json,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema const& schema,
    Common::QueryData::WhereClause::BinaryExpression const& binary_expression)
{
    auto const json_end{json.end()};

    auto const contains_lhs = json.contains(binary_expression.lhs);
    auto const contains_rhs = json.contains(binary_expression.rhs);

    auto const get_type_and_value_from = [&schema](auto const& key, auto const& json)
        -> std::pair<LunarDB::Selenity::API::Managers::Configurations::EFieldDataType, std::string> {
        return std::make_pair(schema.getField(key)->type, json[key]);
    };

    // salary == salary
    if (contains_lhs && contains_rhs)
    {
        auto const& [lhs_type, lhs_value] = get_type_and_value_from(binary_expression.lhs, json);
        auto const& [rhs_type, rhs_value] = get_type_and_value_from(binary_expression.rhs, json);

        return {convert(lhs_value, lhs_type), convert(rhs_value, rhs_type)};
    }

    std::istringstream iss{binary_expression.lhs};

    auto const contains_dotted =
        [](std::istringstream& iss, auto const& json, auto&& contains_dotted) -> bool {
        std::string token{};
        if (std::getline(iss, token, '.'))
        {
            if (json.find(token) != json.end())
            {
                return contains_dotted(iss, json[token], contains_dotted);
            }
            else
            {
                return false;
            }
        }
        return true;
    };

    auto database = LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse();
    auto const get_dotted_type_and_value_from = [&database](
                                                    std::istringstream& iss,
                                                    std::string_view key,
                                                    std::string const& collection_name,
                                                    auto const& json,
                                                    auto&& get_dotted_type_and_value_from)
        -> std::pair<LunarDB::Selenity::API::Managers::Configurations::EFieldDataType, std::string> {
        std::vector<std::string> path{};
        std::string token{};
        while (std::getline(iss, token, '.'))
        {
            path.emplace_back(std::move(token));
        }
        auto* final_json{&json};
        std::ostringstream final_collection_name_oss{};
        final_collection_name_oss << collection_name;
        for (auto const index : std::ranges::iota_view{0u, path.size() - 1})
        {
            {
                auto const final_collection_name{final_collection_name_oss.str()};
            }

            final_json = &(*final_json)[path[index]];
            final_collection_name_oss << "_" << path[index];

            {
                auto const final_collection_name{final_collection_name_oss.str()};
            }
        }

        auto const final_collection_name{final_collection_name_oss.str()};
        auto collection = database->getCollection(final_collection_name);
        return std::make_pair(
            collection->getConfig()->schema.getField(path.back())->type, (*final_json)[path.back()]);
    };

    std::istringstream lhs_iss{binary_expression.lhs};
    auto const contains_dotted_lhs = contains_dotted(lhs_iss, json, contains_dotted);

    std::istringstream rhs_iss{binary_expression.rhs};
    auto const contains_dotted_rhs = contains_dotted(rhs_iss, json, contains_dotted);

    // obj.salary == obj.salary
    if (contains_dotted_lhs && contains_dotted_rhs)
    {
        std::istringstream iss_lhs{binary_expression.lhs};
        std::istringstream iss_rhs{binary_expression.rhs};

        auto const& [lhs_type, lhs_value] = get_dotted_type_and_value_from(
            iss_lhs, "", collection_name, json, get_dotted_type_and_value_from);
        auto const& [rhs_type, rhs_value] = get_dotted_type_and_value_from(
            iss_lhs, "", collection_name, json, get_dotted_type_and_value_from);

        return {convert(lhs_value, lhs_type), convert(rhs_value, rhs_type)};
    }

    // salary == 5000
    if (contains_lhs)
    {
        auto const& [lhs_type, lhs_value] = get_type_and_value_from(binary_expression.lhs, json);
        return {convert(lhs_value, lhs_type), convert(binary_expression.rhs, lhs_type)};
    }

    // obj.salary == 5000
    if (contains_dotted_lhs)
    {
        std::istringstream iss_lhs{binary_expression.lhs};
        auto const& [lhs_type, lhs_value] = get_dotted_type_and_value_from(
            iss_lhs, "", collection_name, json, get_dotted_type_and_value_from);
        return {convert(lhs_value, lhs_type), convert(binary_expression.rhs, lhs_type)};
    }

    // 5000 == salary
    if (contains_rhs)
    {
        auto const& [rhs_type, rhs_value] = get_type_and_value_from(binary_expression.rhs, json);
        return {convert(binary_expression.lhs, rhs_type), convert(binary_expression.rhs, rhs_type)};
    }

    // 5000 == obj.salary
    if (contains_dotted_rhs)
    {
        std::istringstream iss_rhs{binary_expression.rhs};
        auto const& [rhs_type, rhs_value] = get_dotted_type_and_value_from(
            iss_rhs, "", collection_name, json, get_dotted_type_and_value_from);
        return {convert(binary_expression.lhs, rhs_type), convert(binary_expression.rhs, rhs_type)};
    }

    // 5000 == 5000
    return {binary_expression.lhs, binary_expression.rhs};
}

bool evaluateBinaryExpression(
    nlohmann::json const& json_entry,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema const& schema,
    Common::QueryData::WhereClause::BinaryExpression const& binary_expression)
{
    auto const [lhs, rhs] = getLhsRhs(json_entry, collection_name, schema, binary_expression);

    switch (binary_expression.operation)
    {
    case Common::QueryData::Primitives::EBinaryOperator::Equals: {
        return lhs == rhs;
    }
    case Common::QueryData::Primitives::EBinaryOperator::GreaterThan: {
        return lhs > rhs;
    }
    case Common::QueryData::Primitives::EBinaryOperator::GreaterThanEqualTo: {
        return lhs >= rhs;
    }
    case Common::QueryData::Primitives::EBinaryOperator::LessThan: {
        return lhs < rhs;
    }
    case Common::QueryData::Primitives::EBinaryOperator::LessThanEqualTo: {
        return lhs <= rhs;
    }
    case Common::QueryData::Primitives::EBinaryOperator::In: {
        if (!std::holds_alternative<std::string>(rhs))
        {
            throw std::runtime_error{"Right operand is not a string"};
        }

        if (!std::holds_alternative<std::string>(lhs))
        {
            throw std::runtime_error{"Left  operand is not a string"};
        }

        auto const& lhs_str = std::get<std::string>(lhs);
        auto const& rhs_str = std::get<std::string>(rhs);

        static std::string s_last_list_string{};
        static std::vector<std::string> s_search_list{};

        if (s_last_list_string != lhs_str)
        {
            std::string_view rhs_sv{rhs_str};
            LunarDB::Moonlight::Implementation::QueryExtractor extractor{rhs_str};
            s_search_list = extractor.extractList<std::string>(
                [](std::string_view sv) { return std::string{sv}; });
        }

        return std::find_if(s_search_list.begin(), s_search_list.end(), [&lhs_str](auto const& str) {
                   return lhs_str == str;
               }) != s_search_list.end();
    }
    case Common::QueryData::Primitives::EBinaryOperator::Between: {
        if (!std::holds_alternative<std::string>(rhs))
        {
            throw std::runtime_error{"Right operand is not a string"};
        }

        auto const& rhs_str = std::get<std::string>(rhs);
        std::string_view rhs_sv{rhs_str};
        LunarDB::Moonlight::Implementation::QueryExtractor extractor{rhs_str};

        auto const [low, and_kw, high] = extractor.extractTuple<3>();

        return sameTypeAs(lhs, std::string{low}) <= lhs && lhs <= sameTypeAs(lhs, std::string{high});
    }
    case Common::QueryData::Primitives::EBinaryOperator::Like: {
        static std::string s_last_pattern_str{};
        static std::regex s_regex{};

        if (!std::holds_alternative<std::string>(rhs))
        {
            throw std::runtime_error{"Right operand is not a string"};
        }

        if (!std::holds_alternative<std::string>(lhs))
        {
            throw std::runtime_error{"Left  operand is not a string"};
        }

        auto const& lhs_str = std::get<std::string>(lhs);
        auto const& rhs_str = std::get<std::string>(rhs);

        if (s_last_pattern_str != rhs_str)
        {
            s_last_pattern_str = rhs_str;
            s_regex = std::regex{s_last_pattern_str, std::regex_constants::ECMAScript};
        }

        return std::regex_match(lhs_str, s_regex);
    }
    case Common::QueryData::Primitives::EBinaryOperator::None:
    default:
        break;
    }
    return true;
}

bool evaluateBooleanExpression(
    nlohmann::json const& json_entry,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema const& schema,
    Common::QueryData::WhereClause::BooleanExpression const& boolean_expression)
{
    auto const& data{boolean_expression.data};

    auto result{false};

    assert(!data.empty() && "Received empty where clause");

    using WhereClause = Common::QueryData::WhereClause;

    assert(
        (std::holds_alternative<WhereClause::BinaryExpression>(data[0]) ||
         std::holds_alternative<WhereClause::BooleanExpression>(data[0])) &&
        "Invalid Where clause beginning");

    if (std::holds_alternative<WhereClause::BinaryExpression>(data[0]))
    {
        result = evaluateBinaryExpression(
            json_entry, collection_name, schema, std::get<WhereClause::BinaryExpression>(data[0]));
    }
    else
    {
        result = evaluateBooleanExpression(
            json_entry, collection_name, schema, std::get<WhereClause::BooleanExpression>(data[0]));
    }

    for (std::size_t index = 2; index < data.size(); index += 3)
    {
        auto const& boolean_operator{data[index - 1]};
        auto const& expression{data[index]};

        assert(
            (std::holds_alternative<Common::QueryData::Primitives::EBooleanOperator>(boolean_operator)) &&
            "Invalid boolean operator found at some position in where clause");

        assert(
            (std::holds_alternative<WhereClause::BinaryExpression>(expression) ||
             std::holds_alternative<WhereClause::BooleanExpression>(expression)) &&
            "Invalid item found at some position in where clause for expression");

        bool boolean{false};
        if (std::holds_alternative<WhereClause::BinaryExpression>(expression))
        {
            boolean = evaluateBinaryExpression(
                json_entry,
                collection_name,
                schema,
                std::get<WhereClause::BinaryExpression>(data[index]));
        }
        else
        {
            result = evaluateBooleanExpression(
                json_entry,
                collection_name,
                schema,
                std::get<WhereClause::BooleanExpression>(data[index]));
        }

        switch (std::get<Common::QueryData::Primitives::EBooleanOperator>(boolean_operator))
        {
        case Common::QueryData::Primitives::EBooleanOperator::And:
            result = result && boolean;
            break;
        case Common::QueryData::Primitives::EBooleanOperator::Or:
            result = result || boolean;
            break;
        case Common::QueryData::Primitives::EBooleanOperator::None:
        default:
            assert(false && "Unknown boolean operator encountered");
            break;
        }
    }

    return result;
}

} // namespace

bool evaluate(
    std::unique_ptr<AbstractManager::ICollectionEntry> const& entry,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema const& schema,
    Common::QueryData::WhereClause const& where_clause)
{
    return evaluateBooleanExpression(
        entry->getJSON(), collection_name, schema, where_clause.expression);
}

} // namespace LunarDB::Selenity::API::Managers::Collections::WhereClause
