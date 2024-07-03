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

TEST(Astral_RebindExecutorTest, rebind)
{
    LunarDB::Common::Testing::LunarTestGuard _{};

    auto const c_database_name{"somedatabase"s};

    auto& system_catalog{Selenity::API::SystemCatalog::Instance()};
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

    auto const c_collection_name1{"SomeCollection1"s};
    auto const c_structure_type1{Common::QueryData::Primitives::EStructureType::Table};
    auto const c_bindings1{std::vector<Common::QueryData::Create::Single::Binding>{}};

    ASSERT_NO_THROW({
        database->createCollection(c_collection_name1, c_schema1.name, c_structure_type1, c_bindings1);
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

    // test rebind executor
    auto parsed_query = Moonlight::API::ParsedQuery::make<Common::QueryData::Rebind>();
    parsed_query.get<Common::QueryData::Rebind>() = Common::QueryData::Init::RebindInit{}
                                                        .structure_name(c_collection_name2)
                                                        .field("field_1")
                                                        .bind_structure_name(c_collection_name1)
                                                        .clean(std::nullopt);
    ASSERT_NO_THROW({ Astral::Implementation::Rebind::execute(parsed_query); });

    parsed_query.get<Common::QueryData::Rebind>() = Common::QueryData::Init::RebindInit{}
                                                        .structure_name(c_collection_name2)
                                                        .field("field_10")
                                                        .bind_structure_name(c_collection_name1)
                                                        .clean(std::nullopt);
    EXPECT_THROW({ Astral::Implementation::Rebind::execute(parsed_query); }, std::runtime_error);

    parsed_query.get<Common::QueryData::Rebind>() = Common::QueryData::Init::RebindInit{}
                                                        .structure_name("some_random_collection")
                                                        .field("field_10")
                                                        .bind_structure_name(c_collection_name1)
                                                        .clean(std::nullopt);
    EXPECT_THROW({ Astral::Implementation::Rebind::execute(parsed_query); }, std::runtime_error);
}

} // namespace LunarDB::Astral::Tests
