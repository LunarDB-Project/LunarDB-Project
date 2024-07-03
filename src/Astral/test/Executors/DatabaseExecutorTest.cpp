#include <gtest/gtest.h>

#include "LunarDB/Common/CppExtensions/Testing/LunarTestGuard.hpp"
#include "LunarDB/Common/QueryData/helpers/Init.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"
#include "QueryExecutors.hpp"

using namespace std::string_literals;

namespace LunarDB::Astral::Tests {

TEST(Astral_DatabaseExecutorTest, create_drop_use_backup)
{
    Common::Testing::LunarTestGuard _{};

    // 1. setup
    auto& catalog{Selenity::API::SystemCatalog::Instance()};
    auto const c_database_name{"some_database"s};

    // 2. create database
    auto parsed_query = Moonlight::API::ParsedQuery::make<Common::QueryData::Database>();
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name(c_database_name)
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Create);
    ASSERT_NO_THROW({ Astral::Implementation::Database::execute(parsed_query); });
    EXPECT_THROW({ Astral::Implementation::Database::execute(parsed_query); }, std::runtime_error);

    ASSERT_NO_THROW({ catalog.useDatabase(c_database_name); });
    ASSERT_TRUE(catalog.getDatabaseInUse() != nullptr);
    auto const database_home_path = catalog.getDatabaseInUse()->getHomePath();
    EXPECT_TRUE(std::filesystem::exists(database_home_path));

    // 3. use existing database
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name(c_database_name)
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Use);
    Astral::Implementation::Database::execute(parsed_query);
    EXPECT_TRUE(catalog.usingDatabase());

    // 4. backup database
    auto const c_backup_path{LUNAR_TESTING_HOME_PATH "/backup"};
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name(c_database_name)
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Backup)
            .backup_path(c_backup_path);
    EXPECT_NO_THROW({ Astral::Implementation::Database::execute(parsed_query); });
    ASSERT_TRUE(std::filesystem::exists(c_backup_path));
    EXPECT_TRUE(std::filesystem::is_directory(c_backup_path));
    EXPECT_THROW({ Astral::Implementation::Database::execute(parsed_query); }, std::runtime_error);

    // 5. use non existing database
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name("some_non_existing_database")
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Use);
    EXPECT_THROW({ Astral::Implementation::Database::execute(parsed_query); }, std::runtime_error);

    // 6. load catalog from disk, check if configs saved
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name(c_database_name)
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Create);
    EXPECT_THROW({ Astral::Implementation::Database::execute(parsed_query); }, std::runtime_error);

    // 7. drop database
    parsed_query.get<Common::QueryData::Database>() =
        Common::QueryData::Init::DatabaseInit{}
            .name(c_database_name)
            .operation_type(Common::QueryData::Primitives::EDatabaseOperationType::Drop);
    ASSERT_NO_THROW({ Astral::Implementation::Database::execute(parsed_query); });
    EXPECT_THROW({ Astral::Implementation::Database::execute(parsed_query); }, std::runtime_error);
    EXPECT_THROW({ catalog.useDatabase(c_database_name); }, std::runtime_error);
    EXPECT_FALSE(std::filesystem::exists(database_home_path));
}

} // namespace LunarDB::Astral::Tests
