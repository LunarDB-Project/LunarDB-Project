#include <algorithm>
#include <array>

#include "Common/CppExtensions/ItemArray.hpp"
#include "QueryExecutor.hpp"
#include "QueryExecutors.hpp"

namespace LunarDB::Astral::API {

void executeQuery(const Moonlight::API::ParsedQuery& parsed_query, const QueryExecutorConfig& config)
{
    using namespace Implementation;
    using namespace CppExtensions;

    static const DataStructures::ItemArray<ExecutorBundle, 20> s_executors{
        Create::makeExecutor(),   Drop::makeExecutor(),     Migrate::makeExecutor(),   Truncate::makeExecutor(),
        Rename::makeExecutor(),   Select::makeExecutor(),   Insert::makeExecutor(),    Update::makeExecutor(),
        Delete::makeExecutor(),   Lock::makeExecutor(),     Grant::makeExecutor(),     Revoke::makeExecutor(),
        Commit::makeExecutor(),   Rollback::makeExecutor(), SavePoint::makeExecutor(), Index::makeExecutor(),
        Database::makeExecutor(), View::makeExecutor(),     Rebind::makeExecutor(),    Schema::makeExecutor()};

    const auto executor_opt = s_executors.find_if(
        [&parsed_query](const ExecutorBundle& query_executor) { return parsed_query.type() == query_executor.first; });

    if (static_cast<bool>(executor_opt))
    {
        executor_opt.value()(parsed_query, config);
    }
    else
    {
        // TODO: Add throw statement...
    }
}

} // namespace LunarDB::Astral::API

#define PROVIDE_EXECUTOR_BUNDLER(Specialization)                                                           \
    ExecutorBundle Specialization::makeExecutor()                                                          \
    {                                                                                                      \
        return std::make_pair(QueryData::Primitives::EQueryType::Specialization, Specialization::execute); \
    }

namespace LunarDB::Astral::Implementation {

PROVIDE_EXECUTOR_BUNDLER(Create)
PROVIDE_EXECUTOR_BUNDLER(Drop)
PROVIDE_EXECUTOR_BUNDLER(Migrate)
PROVIDE_EXECUTOR_BUNDLER(Truncate)
PROVIDE_EXECUTOR_BUNDLER(Rename)
PROVIDE_EXECUTOR_BUNDLER(Select)
PROVIDE_EXECUTOR_BUNDLER(Insert)
PROVIDE_EXECUTOR_BUNDLER(Update)
PROVIDE_EXECUTOR_BUNDLER(Delete)
PROVIDE_EXECUTOR_BUNDLER(Lock)
PROVIDE_EXECUTOR_BUNDLER(Grant)
PROVIDE_EXECUTOR_BUNDLER(Revoke)
PROVIDE_EXECUTOR_BUNDLER(Commit)
PROVIDE_EXECUTOR_BUNDLER(Rollback)
PROVIDE_EXECUTOR_BUNDLER(SavePoint)
PROVIDE_EXECUTOR_BUNDLER(Index)
PROVIDE_EXECUTOR_BUNDLER(Database)
PROVIDE_EXECUTOR_BUNDLER(View)
PROVIDE_EXECUTOR_BUNDLER(Rebind)
PROVIDE_EXECUTOR_BUNDLER(Schema)

} // namespace LunarDB::Astral::Implementation
