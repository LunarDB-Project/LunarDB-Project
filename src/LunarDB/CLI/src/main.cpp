#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>

#include "LunarDB/Astral/QueryExecutor.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Celestial/UsersCatalog.hpp"
#include "LunarDB/Common/CppExtensions/ConsoleColorOutput.hpp"
#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Common/CppExtensions/Timer.hpp"
#include "LunarDB/Crescentum/Logger.hpp"
#include "LunarDB/LunarDB/Common/QueryHandlingUtils.hpp"
#include "LunarDB/Moonlight/QueryParser.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"
#include "QueryPrompt.hpp"

LUNAR_DECLARE_LOGGER_MODULE(MODULE_LUNARDB_SRV);

#define LUNARDB_ADD_EXTRA_QUERY(Type)                                    \
    LunarDB::Moonlight::API::QueryParsersManager::Instance().addParser(  \
        LunarDB::Moonlight::Implementation::Type::makeParser());         \
    LunarDB::Astral::API::QueryExecutorsManager::Instance().addExecutor( \
        LunarDB::Astral::Implementation::Type::makeExecutor())

using namespace std::string_view_literals;

namespace LunarDB::CLI {
namespace EventHandlers {

auto const on_success = [](auto message) {
    std::cout << ccolor::dark_gray << "[" << ccolor::green << "Success" << ccolor::dark_gray << "] "
              << ccolor::lime << message << ccolor::reset << std::endl;
};

auto const on_selection = [](auto selection, auto fields) {
    std::cout << ccolor::dark_gray << "[" << ccolor::purple << "Selection" << ccolor::dark_gray
              << "] " << ccolor::pink << std::endl;

    nlohmann::json json{nlohmann::json::parse(selection)};
    auto& jsonArray{json[0]["selection"]};

    if (!jsonArray.is_array())
    {
        assert(false && "Received invalid JSON array");
    }

    // Determine column widths
    std::vector<size_t> column_widths{};
    column_widths.reserve(fields.size());
    for (auto const& field : fields)
    {
        column_widths.push_back(field.length());
    }

    // Update column widths based on the content in the JSON array
    for (auto const& obj : jsonArray)
    {
        for (auto const index : std::ranges::iota_view{0u, fields.size()})
        {
            if (obj.contains(fields[index]))
            {
                column_widths[index] =
                    std::max(column_widths[index], obj[fields[index]].dump().size());
            }
        }
    }

    // Print the header
    std::cout << "| ";
    for (auto const index : std::ranges::iota_view{0u, fields.size()})
    {
        std::cout << std::setw(column_widths[index]) << fields[index] << " | ";
    }
    std::cout << '\n';

    // Print a separator line
    std::cout << "|-";
    for (auto const index : std::ranges::iota_view{0u, fields.size()})
    {
        for (auto const _ : std::ranges::iota_view{0u, column_widths[index] + (index == 0 ? 1 : 2)})
        {
            std::cout << "-";
        }
        std::cout << "|";
    }
    std::cout << '\n';

    // Print each row
    for (auto const& obj : jsonArray)
    {
        std::cout << "| ";
        for (auto const index : std::ranges::iota_view{0u, fields.size()})
        {
            if (obj.contains(fields[index]))
            {
                auto const value{obj[fields[index]].dump()};
                std::string_view value_sv{value};
                value_sv.remove_prefix(1);
                value_sv.remove_suffix(1);
                std::cout << std::setw(column_widths[index]) << value_sv << " | ";
            }
            else
            {
                std::cout << std::setw(column_widths[index]) << " " << " | ";
            }
        }
        std::cout << '\n';
    }

    std::cout << std::flush;
};

auto const on_error = [](auto error) {
    std::cout << ccolor::dark_gray << "[" << ccolor::dark_red << "Error" << ccolor::dark_gray
              << "] " << ccolor::light_red << error << ccolor::reset << std::endl;
};

auto const after_parsing = [](LunarDB::Moonlight::API::ParsedQuery const& parsed_query) {
    auto& users_catalog{LunarDB::Celestial::API::UsersCatalog::Instance()};
    auto const& system_catalog{LunarDB::Selenity::API::SystemCatalog::Instance()};

    auto const current_user{system_catalog.getCurrentUser()};

    if (current_user == LUNARDB_ROOT_USER)
    {
        return;
    }

    switch (parsed_query.type())
    {
    case LunarDB::Common::QueryData::Primitives::EQueryType::Auth:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Commit:
    case LunarDB::Common::QueryData::Primitives::EQueryType::SavePoint:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Rollback:
        return;

    case LunarDB::Common::QueryData::Primitives::EQueryType::User:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Grant:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Revoke:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Schema:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Database:
    case LunarDB::Common::QueryData::Primitives::EQueryType::Create:
        if (current_user != LUNARDB_ROOT_USER)
        {
            throw std::runtime_error{"Permission denied. Only root user can issue this command"};
        }
        break;
    case LunarDB::Common::QueryData::Primitives::EQueryType::Migrate: {
        auto const& query = parsed_query.get<Common::QueryData::Migrate>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Migrate,
                current_database->getUID(),
                current_database->getCollection(query.structure_name)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Truncate: {
        auto const& query = parsed_query.get<Common::QueryData::Truncate>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Truncate,
                current_database->getUID(),
                current_database->getCollection(query.structure_name)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Rename: {
        auto const& query = parsed_query.get<Common::QueryData::Rename>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Rename,
                current_database->getUID(),
                current_database->getCollection(query.old_name)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Select: {
        auto const& query = parsed_query.get<Common::QueryData::Select>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Select,
                current_database->getUID(),
                current_database->getCollection(query.from)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Insert: {
        auto const& query = parsed_query.get<Common::QueryData::Insert>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Insert,
                current_database->getUID(),
                current_database->getCollection(query.into)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Update: {
        auto const& query = parsed_query.get<Common::QueryData::Update>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Update,
                current_database->getUID(),
                current_database->getCollection(query.structure_name)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    case LunarDB::Common::QueryData::Primitives::EQueryType::Delete: {
        auto const& query = parsed_query.get<Common::QueryData::Delete>();
        auto const& user_config = users_catalog.getUserConfiguration(current_user);
        auto const current_database = system_catalog.getDatabaseInUse();
        if (!user_config.permissions.contains(LunarDB::Celestial::API::Configuration::Permission{
                LunarDB::Common::QueryData::Primitives::EUserPermissionType::Delete,
                current_database->getUID(),
                current_database->getCollection(query.from)->getUID()}))
        {
            throw std::runtime_error{"Permission denied"};
        }
        break;
    }
    default:
        break;
    }
};

} // namespace EventHandlers

std::ostream& printPrompt(std::string_view user)
{
    if (user == "root" || user == "system")
    {
        std::cout << ccolor::dark_red << "root";
    }
    else
    {
        std::cout << ccolor::dark_aqua << user;
    }

    return std::cout << ccolor::dark_gray << "@" << ccolor::purple << "lunardb" << ccolor::dark_gray
                     << "$ " << ccolor::black;
}

std::string_view getRootPassword()
{
    auto const root_password_ptr{std::getenv("LUNARDB_ROOT_PASSWORD")};
    return root_password_ptr != nullptr ? root_password_ptr : "";
}

bool handleAuthentication(std::string_view root_password)
{
    auto auth_query = LunarDB::Moonlight::API::ParsedQuery::make<LunarDB::Common::QueryData::Auth>();
    auto& username = auth_query.get<LunarDB::Common::QueryData::Auth>().username;

    printPrompt("system") << std::endl;

    while (username.empty())
    {
        std::cout << "> Enter username: ";
        std::getline(std::cin, username);
    }

    try
    {
        LunarDB::Astral::Implementation::Auth::execute(auth_query);
        return true;
    }
    catch (std::exception const& e)
    {
        EventHandlers::on_error(e.what());
        return false;
    }
}

void handleCrashRecovery()
{
    // if (LunarDB::BrightMoon::API::WriteAheadLogger::Instance().getRecoveryFlag() ==
    //     LunarDB::BrightMoon::API::ERecoveryFlag::Recover)
    // {
    std::cout << ccolor::dark_gray << '[' << ccolor::dark_red << "BrightMoon" << ccolor::dark_gray
              << "] " << ccolor::light_red << "Crash recovery process started" << std::endl;
    LunarDB::Common::CppExtensions::Timer timer{};
    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().recover();
    std::cout << ccolor::dark_gray << '[' << ccolor::dark_red << "BrightMoon" << ccolor::dark_gray
              << "] " << ccolor::light_red << "Crash recovery process finished successfully"
              << std::endl;
    auto const elapsed{timer.elapsedExtended()};
    std::cout << ccolor::dark_gray << '[' << ccolor::light_blue << "Elapsed" << ccolor::dark_gray
              << "] " << ccolor::dark_aqua << elapsed << std::endl;
    // }
}

} // namespace LunarDB::CLI

int main(int argc, char const* argv[])
{
    // Setup singletons
    LunarDB::Selenity::API::SystemCatalog::Instance().loadConfigs();
    LunarDB::Selenity::API::SchemasCatalog::Instance().clearCache();

    LunarDB::CLI::handleCrashRecovery();

    LUNARDB_ADD_EXTRA_QUERY(Auth);

    // Setup root password
    auto const root_password{LunarDB::CLI::getRootPassword()};
    if (root_password.empty())
    {
        LunarDB::CLI::EventHandlers::on_error(
            "System stopped loading, reason: LUNARDB_ROOT_PASSWORD env variable was not set or is "
            "empty.");
        // return EXIT_FAILURE;
    }
    LunarDB::Celestial::API::UsersCatalog::Instance().setRootPassword(std::string(root_password));
    LunarDB::Celestial::API::UsersCatalog::Instance().setRootPassword("root");

    // Setup input stream
    std::istream* input_stream_ptr{&std::cin};
    std::ifstream input_file{};
    if (argc >= 2)
    {
        input_file.open(argv[1]);
        input_stream_ptr = &input_file;

        if (!input_file.is_open())
        {
            LunarDB::CLI::EventHandlers::on_error("System stopped, cannot load script file");
            return EXIT_FAILURE;
        }
    }

    // Setup authentication
    if (!LunarDB::CLI::handleAuthentication(root_password))
    {
        return EXIT_FAILURE;
    }

    // Process querries
    LunarDB::CLI::QueryPrompt prompt{*input_stream_ptr};
    do
    {
        LunarDB::CLI::printPrompt(LunarDB::Selenity::API::SystemCatalog::Instance().getCurrentUser());

        auto const query = prompt.readQuery();
        std::string_view query_sv{query};
        LunarDB::Common::CppExtensions::StringUtils::ltrim(query_sv);

        if (query_sv.empty())
        {
            continue;
        }

        if (LunarDB::Common::CppExtensions::StringUtils::startsWithIgnoreCase(query, "exit"))
        {
            static auto constexpr c_options{"ynYN"sv};

            std::string option{};
            do
            {
                std::cout << ccolor::dark_gray << "[" << ccolor::gold << "Exit" << ccolor::dark_gray
                          << "] " << ccolor::yellow
                          << "Are you sure you want to exit? (y/n): " << std::flush;
                std::getline(std::cin, option);
            } while (c_options.find(option.front()) == std::string_view::npos);

            if (option.front() == 'n' || option.front() == 'N')
            {
                continue;
            }

            break;
        }

        if (input_stream_ptr == &input_file)
        {
            std::string_view query_sv{query};
            LunarDB::Common::CppExtensions::StringUtils::trim(query_sv);
            std::cout << query_sv << std::endl;
        }

        LunarDB::Common::CppExtensions::Timer timer{};

        LunarDB::Common::QueryHandlingUtils::handleQuery(
            query,
            lunar_logger_module,
            LunarDB::CLI::EventHandlers::on_success,
            LunarDB::CLI::EventHandlers::on_selection,
            LunarDB::CLI::EventHandlers::on_error,
            LunarDB::CLI::EventHandlers::after_parsing);

        auto const elapsed{timer.elapsedExtended()};
        std::cout << ccolor::dark_gray << '[' << ccolor::light_blue << "Elapsed"
                  << ccolor::dark_gray << "] " << ccolor::dark_aqua << elapsed << std::endl;
    } while (!prompt.done());

    std::cout << ccolor::reset << std::endl;

    LunarDB::Selenity::API::SystemCatalog::Instance().saveConfigs();

    if (input_file.is_open())
    {
        input_file.close();
    }

    LunarDB::BrightMoon::API::WriteAheadLogger::Instance().onNaturalSystemExit();

    std::cout << ccolor::dark_gray << "[" << ccolor::green << "LunarDB" << ccolor::dark_gray << "] "
              << ccolor::lime << "Process ended gracefully.\n"
              << std::endl;

    return EXIT_SUCCESS;
}
