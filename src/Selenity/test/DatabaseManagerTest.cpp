#include <algorithm>
#include <fstream>
#include <gtest/gtest.h>

#include "LunarDB/Common/CppExtensions/BinaryIO.hpp"
#include "LunarDB/Common/CppExtensions/Testing/LunarTestGuard.hpp"
#include "LunarDB/Common/CppExtensions/UniqueID.hpp"
#include "LunarDB/Common/QueryData/helpers/Init.hpp"
#include "LunarDB/Common/QueryData/helpers/Operators.hpp"
#include "LunarDB/Selenity/Managers/DatabaseManager.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"

using namespace std::string_literals;

namespace LunarDB::Selenity::API::Managers::Tests {

TEST(Selenity_SystemCatalog_DatabaseManagerTest, binary_io)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};
    auto const c_home{"/tmp/lunardb/databases/somedatabase"s};

    auto const c_collection_type{Configurations::CollectionConfiguration::ECollectionType::Document};

    auto const database_config = std::make_shared<Configurations::DatabaseConfiguration>(
        c_database_name, c_home, LunarDB::Common::CppExtensions::UniqueID::generate());

    auto const c_collection_name{"SomeCollection"s};
    auto const c_schema1_name{"SomeSchema"};
    std::vector<Common::QueryData::Create::Single::Binding> c_bindings{};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Collection};
    auto const c_schema1{Common::QueryData::Init::SchemaInit{}
                             .name(c_schema1_name)
                             .fields({Common::QueryData::Init::SchemaInit::FieldInit{}
                                          .array(false)
                                          .name("Field1")
                                          .nullable(false)
                                          .type("String")})};
    Selenity::API::SystemCatalog::Instance().loadConfigs();
    ASSERT_NO_THROW({ Selenity::API::SchemasCatalog::Instance().createSchema(c_schema1); });
    ASSERT_NO_THROW({ Selenity::API::SystemCatalog::Instance().createDatabase(c_database_name); });
    ASSERT_NO_THROW({ Selenity::API::SystemCatalog::Instance().useDatabase(c_database_name); });

    DatabaseManager manager1{database_config};
    ASSERT_NO_THROW({
        manager1.createCollection(c_collection_name, c_schema1_name, c_structure_type, c_bindings);
    });
    EXPECT_THROW(
        {
            manager1.createCollection(c_collection_name, c_schema1_name, c_structure_type, c_bindings);
        },
        std::runtime_error);
    ASSERT_NO_THROW({ manager1.getCollection(c_collection_name); });
    EXPECT_THROW({ manager1.getCollection("random"); }, std::runtime_error);

    DatabaseManager manager2{database_config};
    EXPECT_EQ(manager1.getHomePath(), manager2.getHomePath());
    EXPECT_EQ(manager1.getUID(), manager2.getUID());
    EXPECT_EQ(manager1.getName(), manager2.getName());

    ASSERT_NO_THROW({ manager2.getCollection(c_collection_name); });
    EXPECT_THROW({ manager2.getCollection("random"); }, std::runtime_error);

    EXPECT_THROW(
        {
            manager2.createCollection(c_collection_name, c_schema1_name, c_structure_type, c_bindings);
        },
        std::runtime_error);
    ASSERT_NO_THROW({ manager2.dropCollection(c_collection_name, false); });
    EXPECT_THROW({ manager2.dropCollection(c_collection_name, false); }, std::runtime_error);

    DatabaseManager manager3{database_config};
    EXPECT_EQ(manager1.getHomePath(), manager3.getHomePath());
    EXPECT_EQ(manager1.getUID(), manager3.getUID());
    EXPECT_EQ(manager1.getName(), manager3.getName());

    EXPECT_THROW({ manager2.getCollection(c_collection_name); }, std::runtime_error);
    EXPECT_THROW({ manager2.getCollection("random"); }, std::runtime_error);

    EXPECT_THROW({ manager3.dropCollection(c_collection_name, false); }, std::runtime_error);
    ASSERT_NO_THROW({
        manager3.createCollection(c_collection_name, c_schema1_name, c_structure_type, c_bindings);
    });
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, collection)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};

    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema1 =
        Init::SchemaInit{}
            .name("SomeSchema")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("field_1").type("Rid").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_2").type("String").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_3").type("Boolean").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_4").type("DateTime").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_5").type("Integer").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_10").type("Float").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema1.name); });

    ASSERT_NO_THROW({ system_catalog.createDatabase(c_database_name); });
    ASSERT_NO_THROW({ system_catalog.useDatabase(c_database_name); });

    std::shared_ptr<Selenity::API::Managers::DatabaseManager> manager{nullptr};
    ASSERT_NO_THROW({ manager = system_catalog.getDatabaseInUse(); });

    auto const c_collection_name{"SomeCollection"s};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Collection};
    auto const c_bindings{std::vector<Common::QueryData::Create::Single::Binding>{}};
    ASSERT_NO_THROW({
        manager->createCollection(c_collection_name, c_schema1.name, c_structure_type, c_bindings);
    });
    EXPECT_THROW(
        {
            manager->createCollection(c_collection_name, c_schema1.name, c_structure_type, c_bindings);
        },
        std::runtime_error);
    ASSERT_NO_THROW({ manager->getCollection(c_collection_name); });

    manager->saveConfigs();

    auto const manager_copy{
        std::make_shared<Selenity::API::Managers::DatabaseManager>(manager->getConfig())};
    manager_copy->loadConfigs();
    ASSERT_NO_THROW({ manager_copy->getCollection(c_collection_name); });

    auto const& collection{manager->getCollection(c_collection_name)};
    auto const& collection_copy{manager_copy->getCollection(c_collection_name)};

    auto const& config{collection->getConfig()};
    auto const& config_copy{collection_copy->getConfig()};

    EXPECT_EQ(config->schema.bindings, config_copy->schema.bindings);
    EXPECT_EQ(config->collection_type, config_copy->collection_type);
    EXPECT_EQ(config->home, config_copy->home);
    EXPECT_EQ(config->name, config_copy->name);
    EXPECT_EQ(config->schema, config_copy->schema);
    EXPECT_EQ(config->uid, config_copy->uid);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, table)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};

    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema1 =
        Init::SchemaInit{}
            .name("SomeSchema")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("field_1").type("Rid").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_2").type("String").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_3").type("Boolean").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_4").type("DateTime").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_5").type("Integer").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_10").type("Float").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema1.name); });

    ASSERT_NO_THROW({ system_catalog.createDatabase(c_database_name); });
    ASSERT_NO_THROW({ system_catalog.useDatabase(c_database_name); });

    std::shared_ptr<Selenity::API::Managers::DatabaseManager> manager{nullptr};
    ASSERT_NO_THROW({ manager = system_catalog.getDatabaseInUse(); });

    auto const c_collection_name{"SomeCollection"s};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Table};
    auto const c_bindings{std::vector<Common::QueryData::Create::Single::Binding>{}};
    ASSERT_NO_THROW({
        manager->createCollection(c_collection_name, c_schema1.name, c_structure_type, c_bindings);
    });
    EXPECT_THROW(
        {
            manager->createCollection(c_collection_name, c_schema1.name, c_structure_type, c_bindings);
        },
        std::runtime_error);
    ASSERT_NO_THROW({ manager->getCollection(c_collection_name); });

    manager->saveConfigs();

    auto const manager_copy{
        std::make_shared<Selenity::API::Managers::DatabaseManager>(manager->getConfig())};
    manager_copy->loadConfigs();
    ASSERT_NO_THROW({ manager_copy->getCollection(c_collection_name); });

    auto const& collection{manager->getCollection(c_collection_name)};
    auto const& collection_copy{manager_copy->getCollection(c_collection_name)};

    auto const& config{collection->getConfig()};
    auto const& config_copy{collection_copy->getConfig()};

    EXPECT_EQ(config->schema.bindings, config_copy->schema.bindings);
    EXPECT_EQ(config->collection_type, config_copy->collection_type);
    EXPECT_EQ(config->home, config_copy->home);
    EXPECT_EQ(config->name, config_copy->name);
    EXPECT_EQ(config->schema, config_copy->schema);
    EXPECT_EQ(config->uid, config_copy->uid);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, rebind)
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

    // create collection 1
    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema1 =
        Init::SchemaInit{}
            .name("SomeSchema1")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("field_1").type("Rid").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_2").type("String").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_3").type("Boolean").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_4").type("DateTime").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_5").type("Integer").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_10").type("Float").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema1); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema1.name); });

    auto const c_collection_name{"SomeCollection1"s};
    auto const c_structure_type{Common::QueryData::Primitives::EStructureType::Collection};
    auto const c_bindings{std::vector<Common::QueryData::Create::Single::Binding>{}};

    ASSERT_NO_THROW({
        database->createCollection(c_collection_name, c_schema1.name, c_structure_type, c_bindings);
    });

    // create collection 2
    namespace Init = Common::QueryData::Init;
    Common::QueryData::Schema const c_schema2 =
        Init::SchemaInit{}
            .name("SomeSchema2")
            .fields(std::vector<Common::QueryData::Schema::Field>{
                Init::SchemaInit::FieldInit{}.name("field_1").type("Rid").nullable(false).array(false),
                Init::SchemaInit::FieldInit{}.name("field_10").type("Float").nullable(false).array(false)});
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema2); });
    ASSERT_NO_THROW({ schemas_catalog.createSchema(c_schema2); });
    ASSERT_NO_THROW({ schemas_catalog.getSchema(c_schema2.name); });

    auto const c_collection_name2{"SomeCollection2"s};
    auto const c_structure_type2{Common::QueryData::Primitives::EStructureType::Table};
    auto const c_bindings2{std::vector<Common::QueryData::Create::Single::Binding>{}};

    ASSERT_NO_THROW({
        database->createCollection(c_collection_name2, c_schema2.name, c_structure_type2, c_bindings2);
    });

    // rebind
    ASSERT_NO_THROW({ database->rebind(c_collection_name2, "field_1", c_collection_name, false); });
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, document_insert_select_truncate)
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, objects.size());

    ASSERT_NO_THROW({ collection->truncate(); });
    ASSERT_NO_THROW({ collection->truncate(); });
    ASSERT_NO_THROW({ collection->truncate(); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 0);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, document_insert_delete)
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, objects.size());

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 0);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, table_insert_delete)
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, table_insert_delete_enhanced)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};
    schemas_catalog.clearCache();

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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("salary")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("4000")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, _table_insert_delete_enhanced)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};
    schemas_catalog.clearCache();

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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("salary")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("4000")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, document_insert_delete_enhanced_more)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};
    schemas_catalog.clearCache();

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
    obj2.entries.emplace("salary", "4001");
    obj2.entries.emplace("name", "Andrey");
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("salary")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("4000"),
                     Common::QueryData::Primitives::EBooleanOperator::Or,
                     Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("name")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("Andrey")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, objects.size());

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 0);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, document_insert_select_update_select)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
    system_catalog.loadConfigs();
    auto& schemas_catalog{Selenity::API::SchemasCatalog::Instance()};
    schemas_catalog.clearCache();

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
    obj2.entries.emplace("salary", "4001");
    obj2.entries.emplace("name", "Andrey");
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("salary")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("4000"),
                     Common::QueryData::Primitives::EBooleanOperator::Or,
                     Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("name")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("Andrey")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, objects.size());

    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });
    ASSERT_NO_THROW({ collection->deleteWhere(select_config.where); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 0);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, table_insert_select_truncate)
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    std::size_t entries_count{0};
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 2);

    ASSERT_NO_THROW({ collection->truncate(); });
    ASSERT_NO_THROW({ collection->truncate(); });
    ASSERT_NO_THROW({ collection->truncate(); });

    entries_count = 0;
    for (auto const& entry : std::filesystem::directory_iterator(collection->getDataHomePath()))
    {
        if (std::filesystem::is_regular_file(entry))
        {
            ++entries_count;
        }
    }
    EXPECT_EQ(entries_count, 0);
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, table_insert_select_update_select)
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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });

    Common::QueryData::Select select_config =
        Init::SelectInit{}
            .from(c_collection_name)
            .where(Common::QueryData::Init::WhereClauseInit{}.expression(
                Common::QueryData::Init::WhereClauseInit::BooleanExpressionInit{}.negated(false).data(
                    {Common::QueryData::Init::WhereClauseInit::BinaryExpressionInit{}
                         .lhs("1")
                         .operation(Common::QueryData::Primitives::EBinaryOperator::Equals)
                         .rhs("1")})));
    std::vector<std::unique_ptr<Managers::Collections::AbstractManager::ICollectionEntry>> selected_entries{};
    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    struct Object
    {
        std::string name;
        std::string salary;
        std::string birth_date;

        bool operator<(Object const& rhs) const
        {
            if (name != rhs.name)
            {
                return name < rhs.name;
            }
            if (salary != rhs.salary)
            {
                return salary < rhs.salary;
            }
            return birth_date < rhs.birth_date;
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

    Common::QueryData::Update update_config =
        Common::QueryData::Init::UpdateInit{}
            .structure_name(c_collection_name)
            .where(select_config.where)
            .modify(std::vector<Common::QueryData::Update::Modify>{
                Common::QueryData::Init::UpdateInit::ModifyInit{}.field("salary").expression(
                    "4444"),
                Common::QueryData::Init::UpdateInit::ModifyInit{}.field("name").expression("ROB")});

    ASSERT_NO_THROW({ collection->update(update_config); });

    ASSERT_NO_THROW({ selected_entries = collection->select(select_config); });
    ASSERT_EQ(selected_entries.size(), objects.size());

    for (auto const& obj : selected_entries)
    {
        auto const& json{obj->getJSON()};
        EXPECT_EQ(json["name"], "ROB"s);
        EXPECT_EQ(json["salary"], "4444"s);
        EXPECT_EQ(json["birth_date"], "09/10/1985"s);
    }
}

TEST(Selenity_SystemCatalog_DatabaseManagerTest, insert_document_extended)
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

    // TODO: Fix insert extra nested objects on documents
    // auto dummy_obj{Common::QueryData::Insert::Object{}};
    // dummy_obj.entries.emplace("IQ", "101");
    // dummy_obj.entries.emplace("SKILL", "404");

    auto& obj1 = objects.emplace_back();
    obj1.entries.emplace("salary", "4000");
    obj1.entries.emplace("name", "Bob");
    obj1.entries.emplace("rank", "builder");
    obj1.entries.emplace("birth_date", "09/10/1985");
    // obj1.entries.emplace("stats", dummy_obj);

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

    std::shared_ptr<Managers::Collections::AbstractManager> collection{nullptr};
    ASSERT_NO_THROW({ collection = database->getCollection(c_collection_name); });

    ASSERT_NO_THROW({ collection->insert(objects); });
}

} // namespace LunarDB::Selenity::API::Managers::Tests
