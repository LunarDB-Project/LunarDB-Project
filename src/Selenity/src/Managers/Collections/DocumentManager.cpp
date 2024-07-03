#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stack>
#include <string>

#include "Common.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/Moonlight/QueryExtractor.hpp"
#include "LunarDB/Selenity/Managers/Collections/DocumentManager.hpp"
#include "LunarDB/Selenity/Managers/Collections/EvaluateWhereClause.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_SELENITY);

using namespace std::string_literals;

namespace LunarDB::Selenity::API::Managers::Collections {

namespace {

void insert(
    std::filesystem::path const& home,
    std::string const& collection_name,
    Configurations::CollectionConfiguration::Schema& collection_schema,
    Common::QueryData::Insert::Object const& object,
    std::shared_ptr<LunarDB::Selenity::API::Managers::Configurations::CollectionConfiguration> const& config)
{
    auto const rid{Common::CppExtensions::UniqueID::generate()};
    auto const rid_str{rid.toString()};

    nlohmann::json json{};
    json["_rid"] = rid_str;
    jsonify(
        object,
        json,
        collection_name,
        collection_schema,
        Common::QueryData::Primitives::EStructureType::Collection);
    LunarDB::BrightMoon::API::Transactions::InsertTransactionData wal_data{};
    wal_data.database =
        LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
    wal_data.collection = config->name;
    wal_data.json = json.dump();
    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

    auto const document_file_path{home / (rid_str + ".ldbdoc")};
    auto const parent_path{document_file_path.parent_path()};
    if (!std::filesystem::exists(parent_path))
    {
        std::filesystem::create_directories(parent_path);
    }

    std::ofstream document(document_file_path);
    if (document.is_open())
    {
        document << json.dump(4);
        document.close();
    }
    else
    {
        CLOG_ERROR("DocumentManager::namespace::insert(): Could not create file", document_file_path);
    }
}

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
                    oss << std::fixed << std::setprecision(10) << solved;
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

DocumentManager::DocumentManager(std::shared_ptr<Configurations::CollectionConfiguration> config)
    : AbstractManager(config)
{
}

void DocumentManager::insert(std::vector<Common::QueryData::Insert::Object> const& objects)
{
    auto const documents_path{getDataHomePath()};

    for (auto const& object : objects)
    {
        Collections::insert(
            documents_path,
            m_collection_config->name,
            m_collection_config->schema,
            object,
            m_collection_config);
    }
}

std::vector<std::unique_ptr<AbstractManager::ICollectionEntry>> DocumentManager::select(
    Common::QueryData::Select const& config) const
{
    std::vector<std::unique_ptr<AbstractManager::ICollectionEntry>> out{};
    auto const documents_path{getDataHomePath()};

    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        return out;
    }

    for (auto const& entry : std::filesystem::directory_iterator(documents_path))
    {
        if (!std::filesystem::is_regular_file(entry))
        {
            continue;
        }

        std::ifstream object_file(entry.path());
        if (object_file.is_open())
        {
            auto collection_entry_ptr = std::make_unique<DocumentManager::CollectionEntry>();
            object_file >> collection_entry_ptr->data;
            std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                collection_entry_ptr.release()};
            object_file.close();

            if (WhereClause::evaluate(
                    icollection_entry_ptr,
                    m_collection_config->name,
                    m_collection_config->schema,
                    config.where))
            {
                out.emplace_back(std::move(icollection_entry_ptr));
            }
        }
        else
        {
            CLOG_ERROR("DocumentManager::select() Could not open file", entry.path());
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not open file", entry.path())};
        }
    }

    return out;
}

nlohmann::json const& DocumentManager::CollectionEntry::getJSON() const
{
    return data;
}

nlohmann::json& DocumentManager::CollectionEntry::getJSON()
{
    return data;
}

void DocumentManager::deleteWhere(Common::QueryData::WhereClause const& where)
{
    auto const documents_path{getDataHomePath()};

    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        CLOG_ERROR("DocumentManager::deleteWhere(): Could not open", documents_path);
        throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
            "Could not open", documents_path)};
    }

    for (auto const& entry : std::filesystem::directory_iterator(documents_path))
    {
        if (!std::filesystem::is_regular_file(entry))
        {
            continue;
        }

        std::ifstream object_file(entry.path());
        if (object_file.is_open())
        {
            auto collection_entry_ptr = std::make_unique<DocumentManager::CollectionEntry>();
            object_file >> collection_entry_ptr->data;
            object_file.close();

            std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                collection_entry_ptr.release()};

            if (WhereClause::evaluate(
                    icollection_entry_ptr, m_collection_config->name, m_collection_config->schema, where))
            {
                LunarDB::BrightMoon::API::Transactions::DeleteTransactionData wal_data{};
                wal_data.database =
                    LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
                wal_data.collection = m_collection_config->name;
                wal_data.old_json = icollection_entry_ptr->getJSON().dump();
                LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

                std::filesystem::remove(entry.path());
            }
        }
        else
        {
            CLOG_ERROR("DocumentManager::deleteWhere(): Could not open", entry.path());
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not open", entry.path())};
        }
    }
}

void DocumentManager::update(Common::QueryData::Update const& config)
{
    auto documents_path{getDataHomePath()};

    if (!std::filesystem::exists(documents_path) || !std::filesystem::is_directory(documents_path))
    {
        CLOG_ERROR("DocumentManager::update(): Could not open", documents_path);
        throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
            "Could not open", documents_path)};
    }

    for (auto const& entry : std::filesystem::directory_iterator(documents_path))
    {
        if (!std::filesystem::is_regular_file(entry))
        {
            continue;
        }

        std::fstream object_file{};
        object_file.open(entry.path(), std::ios::in);
        if (object_file.is_open())
        {
            auto collection_entry_ptr = std::make_unique<DocumentManager::CollectionEntry>();
            object_file >> collection_entry_ptr->data;
            object_file.close();

            std::unique_ptr<AbstractManager::ICollectionEntry> icollection_entry_ptr{
                collection_entry_ptr.release()};

            if (WhereClause::evaluate(
                    icollection_entry_ptr,
                    m_collection_config->name,
                    m_collection_config->schema,
                    config.where))
            {
                object_file.open(entry.path(), std::ios::out | std::ios::trunc);
                if (object_file.is_open())
                {
                    collection_entry_ptr.reset(dynamic_cast<DocumentManager::CollectionEntry*>(
                        icollection_entry_ptr.release()));

                    LunarDB::BrightMoon::API::Transactions::UpdateTransactionData wal_data{};
                    wal_data.database =
                        LunarDB::Selenity::API::SystemCatalog::Instance().getDatabaseInUse()->getName();
                    wal_data.collection = m_collection_config->name;
                    wal_data.old_json = collection_entry_ptr->getJSON().dump();
                    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().log(wal_data);

                    Collections::update(
                        collection_entry_ptr->data,
                        m_collection_config->schema,
                        m_collection_config->name,
                        config.modify);
                    object_file << collection_entry_ptr->data;
                    object_file.close();
                }
                else
                {
                    CLOG_ERROR("DocumentManager::update(): Could not open", entry.path());
                    throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                        "Could not open", entry.path())};
                }
            }
        }
        else
        {
            CLOG_ERROR("DocumentManager::update(): Could not open", entry.path());
            throw std::runtime_error{LunarDB::Common::CppExtensions::StringUtils::stringify(
                "Could not open", entry.path())};
        }
    }
}

void DocumentManager::undoInsert(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE("DocumentManager::undoInsert(): begin");

    auto const uid = json["_rid"];
    auto const record_file_path{getDataHomePath() / (std::string(uid) + ".ldbdoc")};

    if (std::filesystem::exists(record_file_path))
    {
        CLOG_INFO("DocumentManager::undoInsert(): Removing file", record_file_path);
        std::filesystem::remove(record_file_path);
        CLOG_INFO("DocumentManager::undoInsert(): File", record_file_path, "successfully removed");
    }
    else
    {
        CLOG_INFO("DocumentManager::undoInsert(): File", record_file_path, "no longer exists");
    }

    CLOG_VERBOSE("DocumentManager::undoInsert(): end");
}

void DocumentManager::undoUpdate(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE("DocumentManager::undoUpdate(): begin");

    auto const uid = json["_rid"];
    auto const record_file_path{getDataHomePath() / (std::string(uid) + ".ldbdoc")};

    if (std::filesystem::exists(record_file_path))
    {
        CLOG_INFO("DocumentManager::undoUpdate(): File", record_file_path, "already exists.");
        CLOG_INFO("DocumentManager::undoUpdate(): Removing file", record_file_path);
        std::filesystem::remove(record_file_path);
        CLOG_INFO("DocumentManager::undoUpdate(): File", record_file_path, "successfully removed");
    }

    CLOG_INFO("DocumentManager::undoUpdate(): Creating file", record_file_path);

    std::ofstream document(record_file_path);
    if (document.is_open())
    {
        document << json.dump(4);
        document.close();
        CLOG_INFO("DocumentManager::undoUpdate(): File", record_file_path, "created successfully");
    }
    else
    {
        CLOG_ERROR("DocumentManager::undoUpdate(): Could not create file", record_file_path);
    }

    CLOG_VERBOSE("DocumentManager::undoUpdate(): end");
}

void DocumentManager::undoDelete(nlohmann::json json, bool is_last_call)
{
    CLOG_VERBOSE("DocumentManager::undoDelete(): begin");

    auto const uid = json["_rid"];
    auto const record_file_path{getDataHomePath() / (std::string(uid) + ".ldbdoc")};

    if (std::filesystem::exists(record_file_path))
    {
        CLOG_INFO("DocumentManager::undoDelete(): File", record_file_path, "already exists.");
        CLOG_INFO("DocumentManager::undoDelete(): Removing file", record_file_path);
        std::filesystem::remove(record_file_path);
        CLOG_INFO("DocumentManager::undoDelete(): File", record_file_path, "successfully removed");
    }

    CLOG_INFO("DocumentManager::undoDelete(): Creating file", record_file_path);

    std::ofstream document(record_file_path);
    if (document.is_open())
    {
        document << json.dump(4);
        document.close();
        CLOG_INFO("DocumentManager::undoDelete(): File", record_file_path, "created successfully");
    }
    else
    {
        CLOG_ERROR("DocumentManager::undoDelete(): Could not create file", record_file_path);
    }

    CLOG_VERBOSE("DocumentManager::undoDelete(): end");
}

} // namespace LunarDB::Selenity::API::Managers::Collections
