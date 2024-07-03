#pragma once

#include <string>

#include "LunarDB/Common/CppExtensions/UniqueID.hpp"
#include "LunarDB/Common/QueryData/QueryData.hpp"

#define LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(StructName) \
    StructName() = default;                                  \
    StructName(StructName const&) = default;                 \
    StructName& operator=(StructName const&) = default;      \
    StructName(StructName&&) noexcept = default;             \
    StructName& operator=(StructName&&) noexcept = default;  \
    ~StructName() = default

namespace LunarDB::BrightMoon::API::Transactions {

struct TransactionData
{
    virtual std::string toString() const = 0;

    TransactionData() = default;
    TransactionData(TransactionData const&) = default;
    TransactionData& operator=(TransactionData const&) = default;
    TransactionData(TransactionData&&) noexcept = default;
    TransactionData& operator=(TransactionData&&) noexcept = default;
    virtual ~TransactionData() = default;
};

struct OpenTransactionData : public TransactionData
{
    Common::CppExtensions::UniqueID uid{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(OpenTransactionData);
};

struct CloseTransactionData : public TransactionData
{
    Common::CppExtensions::UniqueID uid{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(CloseTransactionData);
};

struct MigrateTransactionData : public TransactionData
{
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(MigrateTransactionData);
};

struct TruncateTransactionData : public TransactionData
{
    std::string database{};
    std::string structure_name{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(TruncateTransactionData);
};

struct RenameTransactionData : public TransactionData
{
    std::string database{};
    Common::QueryData::Rename rename{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(RenameTransactionData);
};

struct InsertTransactionData : public TransactionData
{
    std::string database{};
    std::string collection{};
    std::string json{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(InsertTransactionData);
};

struct UpdateTransactionData : public TransactionData
{
    std::string database{};
    std::string collection{};
    std::string old_json{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(UpdateTransactionData);
};

struct DeleteTransactionData : public TransactionData
{
    std::string database{};
    std::string collection{};
    std::string old_json{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(DeleteTransactionData);
};

struct RollbackTransactionData : public TransactionData
{
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(RollbackTransactionData);
};

struct SavePointTransactionData : public TransactionData
{
    std::string hash{};
    std::string toString() const override;
    LUNARDB_PROVIDE_STRUCT_DEFAULT_LIFECYCLE(SavePointTransactionData);
};

} // namespace LunarDB::BrightMoon::API::Transactions
