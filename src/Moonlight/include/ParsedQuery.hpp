#pragma once

#include <variant>

#include "LunarDB/Common/QueryData/QueryData.hpp"

namespace LunarDB::Moonlight::API {

class ParsedQuery
{
public: // methods
    ///
    /// @brief Self explanatory.
    ///
    Common::QueryData::Primitives::EQueryType type() const;

    ///
    /// @brief Self explanatory.
    ///
    template <typename Data>
    Data& get();

    ///
    /// @brief Self explanatory.
    ///
    template <typename Data>
    Data const& get() const;

public: // helpers
    ///
    /// @brief Creates an empty ParsedQuery of given Common::QueryData::Type.
    ///
    template <typename Data>
    static ParsedQuery make();

private: // fields;
    Common::QueryData::Primitives::EQueryType m_type;

    std::variant<
        Common::QueryData::Create,
        Common::QueryData::Drop,
        Common::QueryData::Migrate,
        Common::QueryData::Truncate,
        Common::QueryData::Rename,
        Common::QueryData::Select,
        Common::QueryData::Insert,
        Common::QueryData::Update,
        Common::QueryData::Delete,
        Common::QueryData::Grant,
        Common::QueryData::Revoke,
        Common::QueryData::Commit,
        Common::QueryData::Rollback,
        Common::QueryData::SavePoint,
        Common::QueryData::Database,
        Common::QueryData::Rebind,
        Common::QueryData::Schema,
        Common::QueryData::User,
        Common::QueryData::Auth>
        m_data;

    ///
    /// @brief Used by @see ParsedQuery::make() to decide the Common::QueryData::Type.
    /// @tparam T Common::QueryData::Type
    ///
    template <typename T>
    struct QueryDataToTypeMap
    {
        static const Common::QueryData::Primitives::EQueryType value;
    };
};

template <typename Data>
inline Data& ParsedQuery::get()
{
    return std::get<Data>(m_data);
}

template <typename Data>
inline Data const& ParsedQuery::get() const
{
    return std::get<Data>(m_data);
}

template <typename Data>
ParsedQuery ParsedQuery::make()
{
    ParsedQuery query{};
    query.m_type = QueryDataToTypeMap<Data>::value;
    query.m_data = Data{};

    return query;
}

} // namespace LunarDB::Moonlight::API
