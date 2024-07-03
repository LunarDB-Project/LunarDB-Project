#pragma once

#include <functional>
#include <nlohmann/json.hpp>
#include <ranges>
#include <sstream>

#include "LunarDB/Astral/QueryExecutor.hpp"
#include "LunarDB/BrightMoon/WriteAheadLogger.hpp"
#include "LunarDB/Common/CppExtensions/Timer.hpp"
#include "LunarDB/Common/CppExtensions/UniqueID.hpp"
#include "LunarDB/Moonlight/QueryParser.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

#include "LunarDB/LunarDB/Common/QueryHandlingUtils.hpp"

namespace LunarDB::Common::QueryHandlingUtils {

std::string getSuccessMessage(LunarDB::Moonlight::API::ParsedQuery const& parsed_query);

template <typename OnSuccess, typename OnSelection, typename OnError, typename AfterParsing>
void handleQuery(
    std::string_view query,
    LunarDB::Crescentum::API::ELunarModule lunar_logger_module,
    OnSuccess&& on_success,
    OnSelection&& on_selection,
    OnError&& on_error,
    AfterParsing&& after_parsing)
{
    auto& wal{LunarDB::BrightMoon::API::WriteAheadLogger::Instance()};
    auto const get_query_identifier = []() {
        std::ostringstream oss{};
        auto const uid{LunarDB::Common::CppExtensions::UniqueID::generate()};
        oss << "QueryID(" << uid << ")";
        return oss.str();
    };

    auto const query_identifier{get_query_identifier()};
    LunarDB::Common::CppExtensions::Timer query_timer{};

    CLOG_VERBOSE(query_identifier, "-> Processing started");

    try
    {
        LunarDB::Common::CppExtensions::Timer timer{};

        CLOG_VERBOSE("Starting query parsing...");
        timer.reset();
        auto const parsed_query = LunarDB::Moonlight::API::parseQuery(query);
        CLOG_VERBOSE("Finished query parsing, elapsed", timer.elapsedExtended());

        after_parsing(parsed_query);

        CLOG_VERBOSE("Starting query execution...");
        wal.openTransaction(parsed_query.type());
        timer.reset();
        LunarDB::Astral::API::executeQuery(parsed_query);
        wal.closeTransaction(parsed_query.type());
        CLOG_VERBOSE("Finished query execution, elapsed", timer.elapsedExtended());

        if (parsed_query.type() != LunarDB::Common::QueryData::Primitives::EQueryType::Select)
        {
            auto const message{LunarDB::Common::QueryHandlingUtils::getSuccessMessage(parsed_query)};
            CLOG_VERBOSE("Execution finished, sending success message:", message);
            on_success(std::move(message));
            CLOG_VERBOSE("Success message sent");
        }
        else
        {
            auto query_fields{parsed_query.get<LunarDB::Common::QueryData::Select>().fields};

            auto& system_catalog{LunarDB::Selenity::API::SystemCatalog::Instance()};
            auto const& current_selection{system_catalog.getCurrentSelection()};

            CLOG_VERBOSE("Jsonifying current selection...");
            timer.reset();
            std::stringstream oss{};
            oss << R"({ "selection": [ )";
            if (!current_selection.empty())
            {
                auto const select_fields = [&query_fields](auto const& obj) -> nlohmann::json {
                    auto const& obj_json{obj->getJSON()};
                    nlohmann::json out{};

                    for (auto const& field : query_fields)
                    {
                        if (field.find('.') != std::string::npos)
                        {
                            std::istringstream iss{field};

                            auto follow_path =
                                [&iss](auto& json, auto&& follow_path) mutable -> std::string {
                                std::string token{};

                                auto dummy = json.dump();

                                if (std::getline(iss, token, '.'))
                                {
                                    if (json.find(token) != json.end())
                                    {
                                        return follow_path(json[token], follow_path);
                                    }
                                    else
                                    {
                                        return "NULL";
                                    }
                                }

                                return json;
                            };

                            out[field] = follow_path(obj_json, follow_path);
                        }
                        else
                        {
                            if (obj_json.find(field) != obj_json.end())
                            {
                                out[field] = obj_json[field];
                            }
                            else
                            {
                                out[field] = "NULL";
                            }
                        }
                    }

                    return out;
                };

                oss << select_fields(current_selection[0]).dump();
                for (auto const index : std::ranges::iota_view{1u, current_selection.size()})
                {
                    oss << ", " << select_fields(current_selection[index]).dump();
                }
            }
            oss << "] }";
            auto current_selection_str{std::move(oss.str())};
            CLOG_VERBOSE("Jsonifying current selection finished, elapsed", timer.elapsedExtended());

            CLOG_VERBOSE("Sending current selection:", current_selection_str);
            timer.reset();
            on_selection(std::move(current_selection_str), std::move(query_fields));
            CLOG_VERBOSE("Sending current selection finished, elapsed", timer.elapsedExtended());

            CLOG_VERBOSE("Clearing current selection...");
            system_catalog.clearCurrentSelection();
            CLOG_VERBOSE("Current selection cleared");
        }
    }
    catch (std::exception const& e)
    {
        CLOG_VERBOSE("Failed to handle query, cause:", e.what());
        on_error(std::string(e.what()));
    }

    CLOG_VERBOSE(query_identifier, "-> Processing finished, elapsed", query_timer.elapsedExtended());
}

} // namespace LunarDB::Common::QueryHandlingUtils
