#include <sstream>
#include <string>

#include "LunarDB/BrightMoon/TransactionData.hpp"
#include "LunarDB/Common/CppExtensions/UniqueID.hpp"

namespace LunarDB::BrightMoon::API::Transactions {

std::string OpenTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "OPEN_TRANSACTION  " << uid;

    return oss.str();
}

std::string CloseTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "CLOSE_TRANSACTION " << uid;

    return oss.str();
}

std::string MigrateTransactionData::toString() const
{
    return "Migrate ";
}

std::string TruncateTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Truncate " << structure_name;

    return oss.str();
}

std::string RenameTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Rename " << Common::QueryData::Primitives::RenameType::toString(rename.type) << " "
        << rename.old_name << " " << rename.new_name;

    return oss.str();
}

std::string InsertTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Insert " << collection << " -> json: " << json;

    return oss.str();
}

std::string UpdateTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Update " << collection << " -> old: " << old_json;

    return oss.str();
}

std::string DeleteTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Delete " << collection << " -> old: " << old_json;

    return oss.str();
}

std::string CommitTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Commit";

    return oss.str();
}

std::string RollbackTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "Rollback";

    return oss.str();
}

std::string SavePointTransactionData::toString() const
{
    std::ostringstream oss{};

    oss << "SavePoint";

    return oss.str();
}

} // namespace LunarDB::BrightMoon::API::Transactions
