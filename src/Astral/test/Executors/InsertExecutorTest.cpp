#include <gtest/gtest.h>

#include "LunarDB/Celestial/UsersCatalog.hpp"
#include "LunarDB/Common/CppExtensions/Testing/LunarTestGuard.hpp"
#include "LunarDB/Common/QueryData/helpers/Init.hpp"
#include "LunarDB/Common/QueryData/helpers/Operators.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"
#include "QueryExecutors.hpp"

using namespace std::string_literals;

namespace LunarDB::Astral::Tests {

TEST(Astral_InsertExecutorTest, document_insert)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};

    ASSERT_NO_THROW({ system_catalog.createDatabase(c_database_name); });
    ASSERT_NO_THROW({ system_catalog.useDatabase(c_database_name); });

    std::shared_ptr<Selenity::API::Managers::DatabaseManager> database{nullptr};
    ASSERT_NO_THROW({ database = system_catalog.getDatabaseInUse(); });

    // create collection
    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema =
        Init::SchemaInit{}
            .name("SomeSchema")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("salary").type("float").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("name").type("string").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("birth_date").type("datetime").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema.name); });

    auto const c_collection_name{"SomeCollection"s};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Collection};
    auto const c_bindings{std::vector<Common::QueryData::Create::Single::Binding>{}};

    ASSERT_NO_THROW({
        database->createCollection(c_collection_name, c_schema.name, c_structure_type, c_bindings);
    });

    // objects
    std::vector<Common::QueryData::Insert::Object> objects{};

    auto& obj1 = objects.emplace_back();
    obj1.entries.emplace("salary", "4000");
    obj1.entries.emplace("name", "Bob");
    obj1.entries.emplace("birth_date", "09/10/1985");

    auto& obj2 = objects.emplace_back();
    obj2.entries.emplace("salary", "4000");
    obj2.entries.emplace("name", "Bob");
    obj2.entries.emplace("birth_date", "09/10/1985");

    auto& obj3 = objects.emplace_back();
    obj3.entries.emplace("salary", "4000");
    obj3.entries.emplace("name", "Bob");
    obj3.entries.emplace("birth_date", "09/10/1985");

    auto& obj4 = objects.emplace_back();
    obj4.entries.emplace("salary", "4000");
    obj4.entries.emplace("name", "Bob");
    obj4.entries.emplace("birth_date", "09/10/1985");

    auto& obj5 = objects.emplace_back();
    obj5.entries.emplace("salary", "4000");
    obj5.entries.emplace("name", "Bob");
    obj5.entries.emplace("birth_date", "09/10/1985");

    std::shared_ptr<Selenity::API::Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    auto parsed_query = Moonlight::API::ParsedQuery::make<Common::QueryData::Insert>();
    parsed_query.get<Common::QueryData::Insert>() =
        Common::QueryData::Init::InsertInit{}.into(c_collection_name).objects(objects);
    ASSERT_NO_THROW({ Astral::Implementation::Insert::execute(parsed_query); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Selenity::API::Managers::Collections::AbstractManager::ICollectionEntry>>
        selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            return name < rhs.name && salary < rhs.salary && birth_date < rhs.birth_date;
        }
    };

    std::vector<Object> inserted_objects{};
    inserted_objects.reserve(objects.size());
    for (auto const& obj : objects)
    {
        inserted_objects.emplace_back(
            std::get<std::string>(obj.entries.at("name")),
            std::get<std::string>(obj.entries.at("salary")),
            std::get<std::string>(obj.entries.at("birth_date")));
    }

    std::vector<Object> selected_objects{};
    selected_objects.reserve(selected_entries.size());
    for (auto const& obj : selected_entries)
    {
        auto const& json{obj->getJSON()};
        selected_objects.emplace_back(json["name"], json["salary"], json["birth_date"]);
    }

    std::sort(inserted_objects.begin(), inserted_objects.end());
    std::sort(selected_objects.begin(), selected_objects.end());

    for (auto const index : std::ranges::iota_view{0u, inserted_objects.size()})
    {
        auto const& inserted{inserted_objects[index]};
        auto const& selected{selected_objects[index]};

        EXPECT_EQ(inserted.name, selected.name);
        EXPECT_EQ(inserted.salary, selected.salary);
        EXPECT_EQ(inserted.birth_date, selected.birth_date);
    }
}

TEST(Astral_InsertExecutorTest, table_insert)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};

    ASSERT_NO_THROW({ system_catalog.createDatabase(c_database_name); });
    ASSERT_NO_THROW({ system_catalog.useDatabase(c_database_name); });

    std::shared_ptr<Selenity::API::Managers::DatabaseManager> database{nullptr};
    ASSERT_NO_THROW({ database = system_catalog.getDatabaseInUse(); });

    // create collection
    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema =
        Init::SchemaInit{}
            .name("SomeSchema")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("salary").type("float").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("name").type("string").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("birth_date").type("datetime").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema.name); });

    auto const c_collection_name{"SomeCollection"s};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Table};
    auto const c_bindings{std::vector<Common::QueryData::Create::Single::Binding>{}};

    ASSERT_NO_THROW({
        database->createCollection(c_collection_name, c_schema.name, c_structure_type, c_bindings);
    });

    // objects
    std::vector<Common::QueryData::Insert::Object> objects{};

    auto& obj1 = objects.emplace_back();
    obj1.entries.emplace("salary", "4000");
    obj1.entries.emplace("name", "Bob");
    obj1.entries.emplace("birth_date", "09/10/1985");

    auto& obj2 = objects.emplace_back();
    obj2.entries.emplace("salary", "4000");
    obj2.entries.emplace("name", "Bob");
    obj2.entries.emplace("birth_date", "09/10/1985");

    auto& obj3 = objects.emplace_back();
    obj3.entries.emplace("salary", "4000");
    obj3.entries.emplace("name", "Bob");
    obj3.entries.emplace("birth_date", "09/10/1985");

    auto& obj4 = objects.emplace_back();
    obj4.entries.emplace("salary", "4000");
    obj4.entries.emplace("name", "Bob");
    obj4.entries.emplace("birth_date", "09/10/1985");

    auto& obj5 = objects.emplace_back();
    obj5.entries.emplace("salary", "4000");
    obj5.entries.emplace("name", "Bob");
    obj5.entries.emplace("birth_date", "09/10/1985");

    std::shared_ptr<Selenity::API::Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    auto parsed_query = Moonlight::API::ParsedQuery::make<Common::QueryData::Insert>();
    parsed_query.get<Common::QueryData::Insert>() =
        Common::QueryData::Init::InsertInit{}.into(c_collection_name).objects(objects);
    ASSERT_NO_THROW({ Astral::Implementation::Insert::execute(parsed_query); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Selenity::API::Managers::Collections::AbstractManager::ICollectionEntry>>
        selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            return name < rhs.name && salary < rhs.salary && birth_date < rhs.birth_date;
        }
    };

    std::vector<Object> inserted_objects{};
    inserted_objects.reserve(objects.size());
    for (auto const& obj : objects)
    {
        inserted_objects.emplace_back(
            std::get<std::string>(obj.entries.at("name")),
            std::get<std::string>(obj.entries.at("salary")),
            std::get<std::string>(obj.entries.at("birth_date")));
    }

    std::vector<Object> selected_objects{};
    selected_objects.reserve(selected_entries.size());
    for (auto const& obj : selected_entries)
    {
        auto const& json{obj->getJSON()};
        selected_objects.emplace_back(json["name"], json["salary"], json["birth_date"]);
    }

    std::sort(inserted_objects.begin(), inserted_objects.end());
    std::sort(selected_objects.begin(), selected_objects.end());

    for (auto const index : std::ranges::iota_view{0u, inserted_objects.size()})
    {
        auto const& inserted{inserted_objects[index]};
        auto const& selected{selected_objects[index]};

        EXPECT_EQ(inserted.name, selected.name);
        EXPECT_EQ(inserted.salary, selected.salary);
        EXPECT_EQ(inserted.birth_date, selected.birth_date);
    }
}

} // namespace LunarDB::Astral::Tests
