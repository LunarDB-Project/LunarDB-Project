#include "Common.hpp"

#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Selenity/Managers/Collections/DocumentManager.hpp"
#include "LunarDB/Selenity/Managers/Collections/TableManager.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

namespace LunarDB::Selenity::API::Managers::Collections {

// TODO: Split into separate function for sql and json
void jsonifyEntry(
    LunarDB::Common::QueryData::Insert::Object::type const& in_entry,
    nlohmann::json& out_json,
    Configurations::CollectionConfiguration::Schema::Fields::const_iterator entry_config,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema& collection_schema,
    Common::QueryData::Primitives::EStructureType collection_type)
{
    using Object = Common::QueryData::Insert::Object;

    if (std::holds_alternative<std::string>(in_entry))
    {
        auto value_str = std::get<std::string>(in_entry);

        switch (entry_config->type)
        {
        case Configurations::EFieldDataType::String:
            break;
        case Configurations::EFieldDataType::DateTime:
            break;
        case Configurations::EFieldDataType::Rid:
            break;
        case Configurations::EFieldDataType::None:
            break;
        case Configurations::EFieldDataType::Boolean: {
            if (!(LunarDB::Common::CppExtensions::StringUtils::equalsIgnoreCase(value_str, "true") ||
                  LunarDB::Common::CppExtensions::StringUtils::equalsIgnoreCase(value_str, "false")))
            {
                throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                    "Cannot set boolean field to '", value_str, "'")};
            }
            break;
        }
        case Configurations::EFieldDataType::Float: {
            try
            {
                std::ignore = std::stof(value_str);
            }
            catch (std::invalid_argument const& e)
            {
                throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                    "Cannot convert", value_str, "to Float", e.what())};
            }
            catch (std::out_of_range const& e)
            {
                throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                    "Value", value_str, "out of Float range", e.what())};
            }
            break;
        }
        case Configurations::EFieldDataType::Integer: {
            try
            {
                std::ignore = std::stoi(value_str);
            }
            catch (std::invalid_argument const& e)
            {
                throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                    "Cannot convert", value_str, "to Integer", e.what())};
            }
            catch (std::out_of_range const& e)
            {
                throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                    "Value", value_str, "out of Integer range", e.what())};
            }
            break;
        }
        default: {
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Invalid field, should be",
                Configurations::FieldDataType::toString(entry_config->type),
                "found object instead")};
            break;
        }
        }

        out_json = std::move(value_str);
    }
    else if (std::holds_alternative<Object>(in_entry))
    {
        auto const entry_collection_uid_it = collection_schema.bindings.find(entry_config->name);
        if (entry_collection_uid_it == collection_schema.bindings.end())
        {
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not access collection of '", entry_config->name, "'")};
        }

        auto collection_ptr{Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getCollection(
            entry_collection_uid_it->second)};

        switch (collection_type)
        {
        case Common::QueryData::Primitives::EStructureType::Table: {
            auto table_ptr =
                std::dynamic_pointer_cast<Selenity::API::Managers::Collections::TableManager>(
                    collection_ptr);

            out_json = table_ptr->insert(std::get<Object>(in_entry)).toString();
            break;
        }
        case Common::QueryData::Primitives::EStructureType::Collection: {
            jsonify(
                std::get<Object>(in_entry),
                out_json,
                std::string{collection_ptr->getName()},
                collection_ptr->getConfig()->schema,
                collection_type);
            break;
        }
        default:
            break;
        }
    }
    else if (std::holds_alternative<std::vector<std::string>>(in_entry))
    {
        if (!entry_config->array)
        {
            throw std::runtime_error{"Trying to set non-array field to array value"};
        }
        out_json = std::get<std::vector<std::string>>(in_entry);
    }
    else if (std::holds_alternative<std::vector<Object>>(in_entry))
    {
        if (!entry_config->array)
        {
            throw std::runtime_error{"Trying to set non-array field to array value"};
        }

        switch (collection_type)
        {
        case Common::QueryData::Primitives::EStructureType::Table: {
            // TODO: Provide implementation
            throw std::runtime_error{
                "[~/lunardb/src/Selenity/src/Managers/Collections/Common.cpp:insertEntry] Error "
                "X045"};
            break;
        }
        case Common::QueryData::Primitives::EStructureType::Collection: {
            nlohmann::json array = nlohmann::json::array();

            for (auto const& arr_object : std::get<std::vector<Object>>(in_entry))
            {
                auto& arr_json_item = array.emplace_back();
                jsonify(arr_object, arr_json_item, collection_name, collection_schema, collection_type);
            }

            out_json = std::move(array);

            break;
        }
        default:
            break;
        }
    }
    else
    {
        assert(false && "Object writing failure, unknown variant alternative");
    }
}

void jsonify(
    LunarDB::Common::QueryData::Insert::Object const& in_obj,
    nlohmann::json& out_json,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema& collection_schema,
    Common::QueryData::Primitives::EStructureType const collection_type)
{
    switch (collection_type)
    {
    case Common::QueryData::Primitives::EStructureType::Table: {
        for (auto const& [key, value] : in_obj.entries)
        {
            auto const& entry_config = collection_schema.getField(key);

            jsonifyEntry(
                value, out_json[key], entry_config, collection_name, collection_schema, collection_type);
        }
        break;
    }
    case Common::QueryData::Primitives::EStructureType::Collection: {
        for (auto const& [key, value] : in_obj.entries)
        {
            LunarDB::Selenity::API::Managers::Configurations::CollectionConfiguration::Schema::Fields::const_iterator
                entry_config{};

            try
            {
                entry_config = collection_schema.getField(key);
            }
            catch (std::exception const& e)
            {
                entry_config = collection_schema.addField(
                    key, LunarDB::Selenity::API::Managers::Configurations::EFieldDataType::String);
            }

            jsonifyEntry(
                value, out_json[key], entry_config, collection_name, collection_schema, collection_type);
        }
        break;
    }
    default:
        break;
    }
}

namespace Calculus {

std::uint8_t precedence(char operation)
{
    if (operation == '+' || operation == '-')
    {
        return 1;
    }
    if (operation == '*' || operation == '/')
    {
        return 2;
    }
    return 0;
}

} // namespace Calculus

} // namespace LunarDB::Selenity::API::Managers::Collections
