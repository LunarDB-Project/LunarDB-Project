#include "LunarDB/Selenity/Managers/Configurations.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"

namespace LunarDB::Selenity::API::Managers::Configurations {

// clang-format off
DEFINE_LUNAR_PRIMITIVE_IMPL(FieldDataType,
    {Literal::None, "none"},
    {Literal::Rid, "rid"},
    {Literal::String, "string"},
    {Literal::Boolean, "boolean"},
    {Literal::DateTime, "datetime"},
    {Literal::Integer, "integer"},
    {Literal::Float, "float"},
)
// clang-format on

namespace Implementation {

CollectionConfiguration::ECollectionType convert(Common::QueryData::Primitives::EStructureType type)
{
    switch (type)
    {
    case Common::QueryData::Primitives::EStructureType::Collection:
        return CollectionConfiguration::ECollectionType::Document;
    case Common::QueryData::Primitives::EStructureType::Table:
        return CollectionConfiguration::ECollectionType::Table;
    case Common::QueryData::Primitives::EStructureType::None:
        [[fallthrough]];
    default:
        assert(false && "Structure type not supported, logics implementation error");
    }
}

// std::vector<CollectionConfiguration::Binding> convert(
//     std::vector<Common::QueryData::Create::Single::Binding> const& bindings)
// {
//     std::vector<CollectionConfiguration::Binding> out{};
//     out.reserve(bindings.size());

//     auto const database{Selenity::API::SystemCatalog::Instance().getDatabaseInUse()};

//     for (auto const& binding : bindings)
//     {
//         out.emplace_back(binding.field, database->getCollection(binding.table)->getUID());
//     }

//     return out;
// }

CollectionConfiguration::CollectionConfiguration(
    std::string name,
    std::filesystem::path home,
    Common::CppExtensions::UniqueID uid,
    Common::QueryData::Primitives::EStructureType type,
    std::string const& schema_name,
    std::vector<Common::QueryData::Create::Single::Binding> const& bindings)
    : BaseManagerConfiguration{std::move(name), std::move(home), std::move(uid)}
    , collection_type{convert(type)}
    , schema{SchemasCatalog::Instance().getSchema(schema_name), bindings}
{
}

std::vector<CollectionConfiguration::Schema::Field> convert(
    std::vector<Common::QueryData::Schema::Field> const& fields)
{
    std::vector<CollectionConfiguration::Schema::Field> out{};

    for (auto const& field : fields)
    {
        EFieldDataType type{EFieldDataType::Rid};
        try
        {
            auto const in_type{FieldDataType::toLiteral(field.type)};
            type = in_type;
        }
        catch (std::exception const& e)
        {
        }

        out.emplace_back(field.name, type, field.nullable, field.array);
    }

    return out;
}

CollectionConfiguration::Schema::Schema(
    Common::QueryData::Schema const& schema,
    std::vector<Common::QueryData::Create::Single::Binding> const& bindings)
{
    fields.reserve(schema.fields.size() + 1);
    fields.emplace_back("_rid", EFieldDataType::Rid, false, false);
    for (auto const& field : schema.fields)
    {
        EFieldDataType type{EFieldDataType::Rid};
        try
        {
            auto const in_type{FieldDataType::toLiteral(field.type)};
            type = in_type;
        }
        catch (std::exception const& e)
        {
        }

        fields.emplace_back(field.name, type, field.nullable, field.array);
    }

    auto const& catalog{LunarDB::Selenity::API::SystemCatalog::Instance()};

    this->bindings.reserve(bindings.size());
    for (auto const binding : bindings)
    {
        this->bindings.emplace(
            binding.field, catalog.getDatabaseInUse()->getCollection(binding.table)->getUID());
    }
}

CollectionConfiguration::Schema::Fields::const_iterator CollectionConfiguration::Schema::getField(
    std::string_view name) const
{
    auto it = std::find_if(
        fields.begin(), fields.end(), [name](auto const& field) { return name == field.name; });

    using namespace std::string_literals;
    if (it == fields.end())
    {
        throw std::runtime_error("Field '"s + std::string(name) + "' does not exist");
    }

    return it;
}

CollectionConfiguration::Schema::Fields::const_iterator CollectionConfiguration::Schema::addField(
    std::string name,
    EFieldDataType type)
{
    fields.emplace_back(std::move(name), type, true, false);
    return std::prev(fields.end());
}

} // namespace Implementation

} // namespace LunarDB::Selenity::API::Managers::Configurations
