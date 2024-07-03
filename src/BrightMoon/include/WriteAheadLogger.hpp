#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <stack>
#include <string>
#include <unordered_set>

#include "LunarDB/BrightMoon/TransactionData.hpp"
#include "LunarDB/Common/CppExtensions/DefinePrimitive.hpp"
#include "LunarDB/Common/CppExtensions/Singleton.hpp"
#include "LunarDB/Common/CppExtensions/UniqueID.hpp"
#include "LunarDB/Common/QueryData/QueryData.hpp"

namespace LunarDB::BrightMoon::API {

// clang-format off
DEFINE_LUNAR_PRIMITIVE(RecoveryFlag,
    Recover = 0,
    NoRecover = 1
)
// clang-format on

class WriteAheadLogger : public Common::CppExtensions::DesignPatterns::Singleton<WriteAheadLogger>
{
public: // public API
    void openTransaction(LunarDB::Common::QueryData::Primitives::EQueryType const type);
    void closeTransaction(LunarDB::Common::QueryData::Primitives::EQueryType const type);
    void log(Transactions::TransactionData const& data);
    void onNaturalSystemExit();
    void recover();

    ///
    /// @brief Reads the recovery flag from disk.
    /// @return saved ERecoveryFlag
    ///
    ERecoveryFlag getRecoveryFlag() const;

    void commit();
    void savepoint(std::optional<std::string> const hash = std::nullopt);
    void rollback(std::optional<std::string> const hash = std::nullopt);

private: // singleton
    LUNAR_SINGLETON_INIT(WriteAheadLogger);

private: // methods
    std::filesystem::path getHomePath() const;
    std::filesystem::path getRecoveryFlagFilePath() const;

    ///
    /// @brief Saves the recovery flag to disk.
    /// @param[in] flag
    ///
    void setRecoveryFlag(ERecoveryFlag flag) const;

    void saveRollbackDataToDisk() const;
    void loadRollbackDataFromDisk();

private: // fields
    std::ofstream m_log{};
    std::optional<std::filesystem::path> m_recovery_file_path{};
    std::optional<Common::CppExtensions::UniqueID> m_current_transaction_uid{};

    std::stack<std::string> m_savepoints_history{};
    std::unordered_set<std::string> m_rollbacked_hashes{};
};

} // namespace LunarDB::BrightMoon::API
