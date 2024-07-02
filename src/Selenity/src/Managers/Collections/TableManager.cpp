#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <memory>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Common.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Common/CppExtensions/BinaryIO.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/Selenity/Managers/Collections/EvaluateWhereClause.hpp"
#include "LunarDB/Selenity/Managers/Collections/TableManager.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_SELENITY);

using namespace std::string_literals;

namespace LunarDB::Selenity::API::Managers::Collections {

namespace {

// TODO: Find better approach
std::string removeSubstring(std::string_view str, const std::string& toRemove)
{
    std::string out{str};
    auto pos = out.find(toRemove);
    while (pos != std::string::npos)
    {
        out.erase(pos, toRemove.length());
        pos = out.find(toRemove);
    }
    return out;
}

void update(
    nlohmann::json& json,
    Configurations::CollectionConfiguration::Schema const& schema,
    std::string const& collection_name,
    std::vector<Common::QueryData::Update::Modify> const& modify_list)
{
    using namespace std::string_literals;

    auto database = LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse();

    for (auto const& modify : modify_list)
    {
        if (auto dot_it = modify.field.find('.'); dot_it == std::string::npos)
        {
            auto const& field{schema.getField(modify.field)};
            switch (field->type)
            {
            case Configurations::EFieldDataType::Rid:
                throw std::runtime_error("Cannot set reserved _rid field");
            case Configurations::EFieldDataType::DateTime:
                json[modify.field] = modify.expression;
                break;
            case Configurations::EFieldDataType::String:
                json[modify.field] = modify.expression;
                break;
            case Configurations::EFieldDataType::Boolean:
                if (modify.expression == "true" || modify.expression == "false")
                {
                    json[modify.field] = modify.expression;
                }
                else
                {
                    throw std::runtime_error("Invalid boolean value '"s + modify.expression + "'");
                }
            case Configurations::EFieldDataType::Integer: {
                auto& value{json[modify.field]};
                auto const value_str{value.template get<std::string>()};
                try
                {
                    auto const value_int{std::stoi(value_str)};
                    auto const solved{Calculus::solveNumericExpression(
                        modify.expression, modify.field, value_int)};
                    value = std::to_string(solved);
                }
                catch (std::invalid_argument const& e)
                {
                    throw std::runtime_error(
                        "Cannot convert '"s + value_str + "' to Integer; " + e.what());
                }
                catch (std::out_of_range const& e)
                {
                    throw std::runtime_error(
                        "Value '"s + value_str + "' out of Integer range; " + e.what());
                }
                break;
            }
            case Configurations::EFieldDataType::Float: {
                auto& value{json[modify.field]};
                auto const value_str{value.template get<std::string>()};
                try
                {
                    auto const value_float{std::stof(value_str)};
                    auto const solved{Calculus::solveNumericExpression(
                        modify.expression, modify.field, value_float)};
                    std::ostringstream oss{};
                    if (solved == int(solved))
                    {
                        oss << std::fixed << std::setprecision(0) << solved;
                    }
                    else
                    {
                        oss << std::fixed << std::setprecision(10) << solved;
                    }
                    value = oss.str();
                }
                catch (std::invalid_argument const& e)
                {
                    throw std::runtime_error(
                        "Cannot convert '"s + value_str + "' to Float; " + e.what());
                }
                catch (std::out_of_range const& e)
                {
                    throw std::runtime_error(
                        "Value '"s + value_str + "' out of Float range; " + e.what());
                }
                break;
            }
            case Configurations::EFieldDataType::None:
                [[fallthrough]];
            default:
                assert(false && "Encountered unknown field data type");
                break;
            }
        }
        else
        {
            std::string key{modify.field.begin(), modify.field.begin() + dot_it};
            auto const entry_collection_name{collection_name + "_" + key};
            auto collection = database->getCollection(entry_collection_name);

            std::string dotted_key{key + '.'};

            std::vector<Common::QueryData::Update::Modify> entry_modify{};
            entry_modify.emplace_back(
                Collections::removeSubstring(modify.field, dotted_key),
                Collections::removeSubstring(modify.expression, dotted_key));

            Collections::update(
                json[key], collection->getConfig()->schema, entry_collection_name, entry_modify);
        }
    }
}

} // namespace

TableManager::TableManager(std::shared_ptr<Configurations::CollectionConfiguration> config)
    : AbstractManager(config)
{
}

// TODO: Refactor duplicate code
Common::CppExtensions::UniqueID TableManager::insert(Common::QueryData::Insert::Object const& object)
{
    auto const data_home_path{getDataHomePath()};
    auto const table_file_path{data_home_path / "data.ldbtbl"};
    auto const metadata_file_path{data_home_path / "metadata.ldb"};

    if (!std::filesystem::exists(metadata_file_path))
    {
        std::ofstream metadata_file(metadata_file_path, std::ios::out | std::ios::binary);
        LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(metadata_file, 0);
        metadata_file.close();
    }

    std::ofstream table_file(table_file_path, std::ios::out | std::ios::app | std::ios::binary);

    if (!table_file.is_open())
    {
        CLOG_ERROR("TableManager::insert(object): Could not open file", table_file_path);
        throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
            "TableManager::insert(object): Could not open file", table_file_path)};
    }

    std::size_t entries_count{0};
    {
        std::ifstream metadata_file(metadata_file_path, std::ios::in | std::ios::binary);
        if (!metadata_file.is_open())
        {
            CLOG_ERROR("TableManager::insert(object): Could not open file to read", metadata_file_path);
            throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
                "TableManager::insert(object): Could not open file", metadata_file_path)};
        }
        LunarDB::Common::CppExtensions::BinaryIO::Deserializer::deserialize(
            metadata_file, entries_count);
    }

    std::ofstream metadata_file(metadata_file_path, std::ios::out | std::ios::binary);

    if (!metadata_file.is_open())
    {
        CLOG_ERROR("TableManager::insert(object): Could not open file to write", metadata_file_path);
        throw std::runtime_error{Common::CppExtensions::StringUtils::stringify(
            "TableManager::insert(object): Could not open file to write", metadata_file_path)};
    }

    auto const rid{Common::CppExtensions::UniqueID::generate()};
    auto const rid_str{rid.toString()};

    nlohmann::json json{};
    json["_rid"] = rid_str;
    json["_del"] = "0";
    jsonify(
        object,
        json,
        m_collection_config->name,
        m_collection_config->schema,
        Common::QueryData::Primitives::EStructureType::Table);

    LunarDB::BrightMoon::API::Transactions::InsertTransactionData wal_data{};
    wal_data.database =
        LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
    wal_data.collection = m_collection_config->name;
    wal_data.json = json.dump();
    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

    auto bson{nlohmann::json::to_bson(json)};
    LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

    ++entries_count;
    metadata_file.seekp(0, std::ios::beg);
    LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(metadata_file, entries_count);
    metadata_file.flush();

    table_file.flush();

    metadata_file.close();
    table_file.close();

    return rid;
}

void TableManager::insert(std::vector<Common::QueryData::Insert::Object> const& objects)
{
    auto const data_home_path{getDataHomePath()};
    auto const table_file_path{data_home_path / "data.ldbtbl"};
    auto const metadata_file_path{data_home_path / "metadata.ldb"};

    if (!std::filesystem::exists(metadata_file_path))
    {
        std::ofstream metadata_file(metadata_file_path, std::ios::out | std::ios::binary);
        LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(metadata_file, 0);
        metadata_file.close();
    }

    std::ofstream table_file(table_file_path, std::ios::out | std::ios::app | std::ios::binary);

    if (!table_file.is_open())
    {
        CLOG_ERROR("TableManager::insert(): Could not open file", table_file_path);
        return;
    }

    std::size_t entries_count{0};
    {
        std::ifstream metadata_file(metadata_file_path, std::ios::in | std::ios::binary);
        if (!metadata_file.is_open())
        {
            CLOG_ERROR("TableManager::insert(): Could not open file to read", metadata_file_path);
            return;
        }
        LunarDB::Common::CppExtensions::BinaryIO::Deserializer::deserialize(
            metadata_file, entries_count);
    }

    std::ofstream metadata_file(metadata_file_path, std::ios::out | std::ios::binary);

    if (!metadata_file.is_open())
    {
        CLOG_ERROR("TableManager::insert(): Could not open file to write", metadata_file_path);
        return;
    }

    for (auto const& object : objects)
    {
        auto const rid{Common::CppExtensions::UniqueID::generate()};
        auto const rid_str{rid.toString()};

        nlohmann::json json{};
        json["_rid"] = rid_str;
        json["_del"] = "0";
        jsonify(
            object,
            json,
            m_collection_config->name,
            m_collection_config->schema,
            Common::QueryData::Primitives::EStructureType::Table);

        LunarDB::BrightMoon::API::Transactions::InsertTransactionData wal_data{};
        wal_data.database =
            LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
        wal_data.collection = m_collection_config->name;
        wal_data.json = json.dump();
        LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

        auto bson{nlohmann::json::to_bson(json)};
        LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

        ++entries_count;
        metadata_file.seekp(0, std::ios::beg);
        LunarDB::Common::CppExtensions::BinaryIO::Serializer::serialize(metadata_file, entries_count);
        metadata_file.flush();
    }

    metadata_file.flush();
    table_file.flush();

    metadata_file.close();
    table_file.close();
}

void TableManager::selectEntry(std::string const& rid, nlohmann::json& out_json)
{
    auto const documents_path{getDataHomePath()};

    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        return;
    }

    std::uint64_t entries_count{0};
    auto const file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::ifstream table_file(table_file_path, std::ios::binary);
        if (table_file.is_open())
        {
            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                auto collection_entry_ptr = std::make_unique<TableManager::CollectionEntry>();
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                collection_entry_ptr->data = nlohmann::json::from_bson(bson);

                if (collection_entry_ptr->data["_del"] == 1 || collection_entry_ptr->data["_rid"] != rid)
                {
                    continue;
                }

                std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                    collection_entry_ptr.release()};

                auto database = Selenity::API::SystemCatalog::Instance().getDatabaseInUse();
                auto& json = icollection_entry_ptr->getJSON();
                for (auto const& [field, collection_uid] : m_collection_config->schema.bindings)
                {
                    auto collection =
                        std::reinterpret_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            database->getCollection(collection_uid));
                    std::string rid{json[field]};
                    collection->selectEntry(rid, json[field]);
                }

                out_json = std::move(icollection_entry_ptr->getJSON());

                return;
            }
            table_file.close();
        }
    }
}

// TODO: Refactor
std::vector<std::unique_ptr<AbstractManager::ICollectionEntry>> TableManager::select(
    Common::QueryData::Select const& config) const
{
    std::vector<std::unique_ptr<AbstractManager::ICollectionEntry>> out{};
    auto const documents_path{getDataHomePath()};

    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        return out;
    }

    std::uint64_t entries_count{0};
    auto const file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::ifstream table_file(table_file_path, std::ios::binary);
        if (table_file.is_open())
        {
            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                auto collection_entry_ptr = std::make_unique<TableManager::CollectionEntry>();
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                collection_entry_ptr->data = nlohmann::json::from_bson(bson);
                if (collection_entry_ptr->data["_del"] == "1")
                {
                    continue;
                }

                std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                    collection_entry_ptr.release()};

                auto database = Selenity::API::SystemCatalog::Instance().getDatabaseInUse();
                auto& json = icollection_entry_ptr->getJSON();
                for (auto const& [field, collection_uid] : m_collection_config->schema.bindings)
                {
                    auto collection =
                        std::reinterpret_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            database->getCollection(collection_uid));
                    std::string rid{json[field]};
                    collection->selectEntry(rid, json[field]);
                }

                if (WhereClause::evaluate(
                        icollection_entry_ptr,
                        m_collection_config->name,
                        m_collection_config->schema,
                        config.where))
                {
                    out.emplace_back(std::move(icollection_entry_ptr));
                }
            }
            table_file.close();
        }
    }

    return out;
}

nlohmann::json const& TableManager::CollectionEntry::getJSON() const
{
    return data;
}

nlohmann::json& TableManager::CollectionEntry::getJSON()
{
    return data;
}

// TODO: Refactor
void TableManager::deleteWhere(Common::QueryData::WhereClause const& where)
{
    auto documents_path{getDataHomePath()};

    std::uint64_t entries_count{0};
    auto const file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                auto collection_entry_ptr = std::make_unique<TableManager::CollectionEntry>();
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                collection_entry_ptr->data = nlohmann::json::from_bson(bson);
                if (collection_entry_ptr->data["_del"] == 1)
                {
                    continue;
                }
                std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                    collection_entry_ptr.release()};

                auto database = Selenity::API::SystemCatalog::Instance().getDatabaseInUse();
                auto& json = icollection_entry_ptr->getJSON();
                std::unordered_map<std::string, std::string> rids{};
                for (auto const& [field, collection_uid] : m_collection_config->schema.bindings)
                {
                    auto collection =
                        std::reinterpret_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            database->getCollection(collection_uid));
                    std::string rid{json[field]};
                    collection->selectEntry(rid, json[field]);
                    rids.emplace(field, std::move(rid));
                }

                if (WhereClause::evaluate(
                        icollection_entry_ptr, m_collection_config->name, m_collection_config->schema, where))
                {
                    LunarDB::BrightMoon::API::Transactions::DeleteTransactionData wal_data{};
                    wal_data.database =
                        LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
                    wal_data.collection = m_collection_config->name;
                    wal_data.old_json = icollection_entry_ptr->getJSON().dump();
                    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

                    collection_entry_ptr.reset(dynamic_cast<TableManager::CollectionEntry*>(
                        icollection_entry_ptr.release()));
                    collection_entry_ptr->data["_del"] = "1";
                    for (auto const& [field, rid] : rids)
                    {
                        collection_entry_ptr->data[field] = rid;
                    }
                    bson = nlohmann::json::to_bson(collection_entry_ptr->data);

                    auto const current_pos = table_file.tellg();

                    table_file.seekp(
                        -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)),
                        std::ios::cur);
                    Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                    table_file.seekg(current_pos, std::ios::beg);
                }
            }
            table_file.close();
        }
    }
}

// TODO: Refactor
void TableManager::overwrite(nlohmann::json& updated_json)
{
    std::string const& overwrited_rid{updated_json["_rid"]};
    auto const documents_path{getDataHomePath()};

    std::uint64_t entries_count{0};
    auto const file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                auto collection_entry_ptr = std::make_unique<TableManager::CollectionEntry>();
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                collection_entry_ptr->data = nlohmann::json::from_bson(bson);
                if (collection_entry_ptr->data["_del"] == 1 ||
                    collection_entry_ptr->data["_rid"] != overwrited_rid)
                {
                    continue;
                }

                auto database = Selenity::API::SystemCatalog::Instance().getDatabaseInUse();

                std::unordered_map<std::string, std::string> rids{};
                for (auto const& [field, collection_uid] : m_collection_config->schema.bindings)
                {
                    auto collection =
                        std::reinterpret_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            database->getCollection(collection_uid));
                    nlohmann::json temp_json{};
                    collection->selectEntry(overwrited_rid, temp_json);
                    rids.emplace(field, temp_json["_rid"]);
                }

                LunarDB::BrightMoon::API::Transactions::UpdateTransactionData wal_data{};
                wal_data.database =
                    LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
                wal_data.collection = m_collection_config->name;
                wal_data.old_json = collection_entry_ptr->getJSON().dump();
                LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

                for (auto const& [field, rid] : rids)
                {
                    auto const field_collection_name{m_collection_config->name + "_" + field};
                    auto collection_ptr = database->getCollection(field_collection_name);
                    auto collection_table_ptr =
                        std::dynamic_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            collection_ptr);
                    collection_table_ptr->overwrite(updated_json[field]);
                    updated_json[field] = rid;
                }

                bson = nlohmann::json::to_bson(updated_json);

                auto const current_pos = table_file.tellg();

                table_file.seekp(
                    -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)), std::ios::cur);
                Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                table_file.seekg(current_pos, std::ios::beg);

                break;
            }
            table_file.close();
        }
    }
}

// TODO: Refactor
void TableManager::update(Common::QueryData::Update const& config)
{
    auto documents_path{getDataHomePath()};

    std::uint64_t entries_count{0};
    auto const file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                auto collection_entry_ptr = std::make_unique<TableManager::CollectionEntry>();
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                collection_entry_ptr->data = nlohmann::json::from_bson(bson);
                if (collection_entry_ptr->data["_del"] == 1)
                {
                    continue;
                }
                std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                    collection_entry_ptr.release()};

                auto database = Selenity::API::SystemCatalog::Instance().getDatabaseInUse();
                auto& json = icollection_entry_ptr->getJSON();
                std::unordered_map<std::string, std::string> rids{};
                for (auto const& [field, collection_uid] : m_collection_config->schema.bindings)
                {
                    auto collection =
                        std::reinterpret_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                            database->getCollection(collection_uid));
                    std::string rid{json[field]};
                    collection->selectEntry(rid, json[field]);
                    rids.emplace(field, std::move(rid));
                }

                if (WhereClause::evaluate(
                        icollection_entry_ptr,
                        m_collection_config->name,
                        m_collection_config->schema,
                        config.where))
                {
                    collection_entry_ptr.reset(dynamic_cast<TableManager::CollectionEntry*>(
                        icollection_entry_ptr.release()));

                    auto original = collection_entry_ptr->getJSON();
                    for (auto const& [field, rid] : rids)
                    {
                        original[field] = rid;
                    }

                    LunarDB::BrightMoon::API::Transactions::UpdateTransactionData wal_data{};
                    wal_data.database =
                        LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
                    wal_data.collection = m_collection_config->name;
                    wal_data.old_json = original.dump();
                    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

                    Collections::update(
                        collection_entry_ptr->data,
                        m_collection_config->schema,
                        m_collection_config->name,
                        config.modify);

                    for (auto const& [field, rid] : rids)
                    {
                        auto const field_collection_name{m_collection_config->name + "_" + field};
                        auto collection_ptr = database->getCollection(field_collection_name);
                        auto collection_table_ptr =
                            std::dynamic_pointer_cast<LunarDB::Selenity::API::Managers::Collections::TableManager>(
                                collection_ptr);
                        collection_table_ptr->overwrite(json[field]);
                        json[field] = rid;
                    }

                    bson = nlohmann::json::to_bson(collection_entry_ptr->data);

                    auto const current_pos = table_file.tellg();

                    table_file.seekp(
                        -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)),
                        std::ios::cur);
                    Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                    table_file.seekg(current_pos, std::ios::beg);
                }
            }
            table_file.close();
        }
    }
}

void TableManager::undoInsert(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE(
        "TableManager::undoInsert(): table:", m_collection_config->name, "-> begin, is_last_call:", is_last_call);

    static std::unordered_set<std::string> s_inserted_rids{};

    std::ignore = s_inserted_rids.emplace(json["_rid"]);
    if (!is_last_call)
    {
        return;
    }
    CLOG_VERBOSE("TableManager::undoInsert(): last call, undo size:", s_inserted_rids.size());

    auto documents_path{getDataHomePath()};
    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        CLOG_ERROR("TableManager::undoInsert(): Could not open file:", documents_path);
        return;
    }

    std::uint64_t entries_count{0};
    auto const metadata_file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(metadata_file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            auto const undo_size = s_inserted_rids.size();
            std::size_t undo_number{0};

            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                CLOG_VERBOSE(">>>>>");
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                auto json_entry = nlohmann::json::from_bson(bson);
                CLOG_VERBOSE("TableManager::undoInsert(): index:", _, "json:", json_entry.dump());

                if (auto const rid_it = s_inserted_rids.find(json_entry["_rid"]);
                    rid_it != s_inserted_rids.end())
                {
                    auto const rid{*rid_it};
                    CLOG_INFO("TableManager::undoInsert(): start RID:", rid);
                    json_entry["_del"] = "1";
                    bson = nlohmann::json::to_bson(json_entry);

                    auto const current_pos = table_file.tellg();

                    table_file.seekp(
                        -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)),
                        std::ios::cur);
                    Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                    table_file.seekg(current_pos, std::ios::beg);

                    undo_number++;
                    CLOG_INFO(
                        "TableManager::undoInsert(): end  RID:",
                        rid,
                        "->",
                        undo_number,
                        "out of",
                        undo_size,
                        "changes undone");
                    s_inserted_rids.erase(rid_it);
                }
                CLOG_VERBOSE("<<<<");
            }
        }
        else
        {
            CLOG_ERROR("TableManager::undoInsert(): Could not open data file", table_file_path);
        }
    }
    else
    {
        CLOG_ERROR("TableManager::undoInsert(): Could not open metadata file", metadata_file_path);
    }

    s_inserted_rids.clear();
    CLOG_VERBOSE("TableManager::undoInsert(): end");
}

void TableManager::undoUpdate(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE("TableManager::undoUpdate(): begin");

    static std::unordered_map<std::string, nlohmann::json> s_updated_rids{};

    std::string rid{json["_rid"]};
    s_updated_rids.emplace(std::move(rid), std::move(json));
    if (!is_last_call)
    {
        return;
    }

    auto documents_path{getDataHomePath()};
    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        CLOG_ERROR("TableManager::undoUpdate(): Could not open file:", documents_path);
        return;
    }

    std::uint64_t entries_count{0};
    auto const metadata_file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(metadata_file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            auto const undo_size = s_updated_rids.size();
            std::size_t undo_number{0};

            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                auto json_data = nlohmann::json::from_bson(bson);

                if (auto const rid_it = s_updated_rids.find(json_data["_rid"]);
                    rid_it != s_updated_rids.end())
                {
                    auto const rid{rid_it->first};
                    CLOG_INFO("TableManager::undoUpdate(): RID:", rid);
                    bson = nlohmann::json::to_bson(rid_it->second);

                    auto const current_pos = table_file.tellg();

                    table_file.seekp(
                        -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)),
                        std::ios::cur);
                    Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                    table_file.seekg(current_pos, std::ios::beg);

                    undo_number++;
                    CLOG_INFO(
                        "TableManager::undoInsert(): done RID:",
                        rid,
                        "->",
                        undo_number,
                        "out of",
                        undo_size,
                        "changes undone");
                    s_updated_rids.erase(rid_it);
                }
            }
            table_file.close();
        }
        else
        {
            CLOG_ERROR("TableManager::undoUpdate(): Could not open data file", table_file_path);
        }
        metadata_file.close();
    }
    else
    {
        CLOG_ERROR("TableManager::undoUpdate(): Could not open metadata file", metadata_file_path);
    }

    s_updated_rids.clear();
    CLOG_VERBOSE("TableManager::undoUpdate(): end");
}

void TableManager::undoDelete(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE("TableManager::undoDelete(): begin");

    static std::unordered_set<std::string> s_deleted_rids{};

    s_deleted_rids.emplace(json["_rid"]);
    if (!is_last_call)
    {
        return;
    }

    auto documents_path{getDataHomePath()};
    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        CLOG_ERROR("TableManager::undoDelete(): Could not open file:", documents_path);
        return;
    }

    std::uint64_t entries_count{0};
    auto const metadata_file_path{getDataHomePath() / "metadata.ldb"};
    std::fstream metadata_file{};
    metadata_file.open(metadata_file_path, std::ios::in | std::ios::binary);

    if (metadata_file.is_open())
    {
        Common::CppExtensions::BinaryIO::Deserializer::deserialize(metadata_file, entries_count);
        metadata_file.close();

        auto const table_file_path(documents_path / "data.ldbtbl");
        std::fstream table_file(table_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (table_file.is_open())
        {
            auto const undo_size = s_deleted_rids.size();
            std::size_t undo_number{0};

            for (auto const _ : std::ranges::iota_view{0u, entries_count})
            {
                std::vector<std::uint8_t> bson{};
                Common::CppExtensions::BinaryIO::Deserializer::deserialize(table_file, bson);
                auto json_data = nlohmann::json::from_bson(bson);

                if (auto const rid_it = s_deleted_rids.find(json_data["_rid"]);
                    rid_it != s_deleted_rids.end())
                {
                    auto const rid{*rid_it};
                    CLOG_INFO("TableManager::undoDelete(): RID:", rid);
                    json_data["_del"] = "0";
                    bson = nlohmann::json::to_bson(json_data);

                    auto const current_pos = table_file.tellg();

                    table_file.seekp(
                        -(static_cast<std::uint64_t>(bson.size()) + sizeof(std::size_t)),
                        std::ios::cur);
                    Common::CppExtensions::BinaryIO::Serializer::serialize(table_file, bson);

                    table_file.seekg(current_pos, std::ios::beg);

                    undo_number++;
                    CLOG_INFO(
                        "TableManager::undoInsert(): done RID:",
                        rid,
                        "->",
                        undo_number,
                        "out of",
                        undo_size,
                        "changes undone");
                    s_deleted_rids.erase(rid_it);
                }
            }
            table_file.close();
        }
        else
        {
            CLOG_ERROR("TableManager::undoDelete(): Could not open data file", table_file_path);
        }
        metadata_file.close();
    }
    else
    {
        CLOG_ERROR("TableManager::undoDelete(): Could not open metadata file", metadata_file_path);
    }

    s_deleted_rids.clear();
    CLOG_VERBOSE("TableManager::undoDelete(): end");
}

} // namespace LunarDB::Selenity::API::Managers::Collections
