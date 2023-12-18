#include "Primitives.hpp" 
#include "LiteralToStringMap.hpp"

#include <cctype>
#include <stdexcept>

#define DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(name, ...) \
namespace name { \
    namespace Internal { \
        const Implementation::LiteralToStringMap<Literal> map{ { \
            __VA_ARGS__ \
        } };\
    } \
    std::string_view toString(Literal literal) { return Internal::map.findByLiteral(literal); } \
    Literal toLiteral(std::string_view str) { return Internal::map.findByString(str); } \
    std::ostream& operator<<(std::ostream& os, const Literal& rhs) { return os << toString(rhs); } \
}

namespace LunarDB::Moonlight::QueryData::Primitives {

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(QueryType,
    { Literal::None, "None" },
    { Literal::Create, "Create" },
    { Literal::Drop, "Drop" },
    { Literal::Migrate, "Migrate" },
    { Literal::Truncate, "Truncate" },
    { Literal::Rename, "Rename" },
    { Literal::Select, "Select" },
    { Literal::Insert, "Insert" },
    { Literal::Update, "Update" },
    { Literal::Delete, "Delete" },
    { Literal::Lock, "Lock" },
    { Literal::Grant, "Grant" },
    { Literal::Revoke, "Revoke" },
    { Literal::Commit, "Commit" },
    { Literal::Rollback, "Rollback" },
    { Literal::SavePoint, "SavePoint" },
    { Literal::Index, "Index" },
    { Literal::Database, "Database" },
    { Literal::View, "View" },
    { Literal::Rebind, "Rebind" },
    { Literal::Schema, "Schema" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(StructureType,
    { Literal::None, "None" },
    { Literal::Table, "Table" },
    { Literal::Collection, "Collection" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(RenameType,
    { Literal::None, "None" },
    { Literal::Structure, "Structure" },
    { Literal::Field, "Field" },
    { Literal::Database, "Database" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(OrderType,
    { Literal::None, "None" },
    { Literal::Asc, "Asc" },
    { Literal::Desc, "Desc" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(UserPermissionType,
    { Literal::None, "None" },
    { Literal::Create, "Create" },
    { Literal::Update, "Update" },
    { Literal::Insert, "Insert" },
    { Literal::Delete, "Delete" },
    { Literal::Select, "Select" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(DatabaseOperationType,
    { Literal::None, "None" },
    { Literal::Create, "Create" },
    { Literal::Drop, "Drop" },
    { Literal::Backup, "Backup" },
    { Literal::Use, "Use" }
)

DECLARE_MOONLIGHT_PRIMITIVE_CONVERTOR(DataType,
    { Literal::None, "None" },
    { Literal::Rid, "Rid" },
    { Literal::String, "String" },
    { Literal::Boolean, "Boolean" },
    { Literal::DateTime, "DateTime" },
    { Literal::Integer, "Integer" },
    { Literal::Integer8, "Integer8" },
    { Literal::Integer16, "Integer16" },
    { Literal::Integer32, "Integer32" },
    { Literal::Integer64, "Integer64" },
    { Literal::Float, "Float" },
    { Literal::Float32, "Float32" },
    { Literal::Float64, "Float64" },
    { Literal::Schema, "Schema" }
)

} // namespace LunarDB::Moonlight::QueryData::Primitives
