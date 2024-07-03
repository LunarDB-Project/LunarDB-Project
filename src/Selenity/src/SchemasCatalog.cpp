#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>

#include "LunarDB/Common/CppExtensions/BinaryIO.hpp"
#include "LunarDB/Common/CppExtensions/StringUtils.hpp"
#include "LunarDB/Selenity/SchemasCatalog.hpp"
#include "LunarDB/Selenity/SystemCatalog.hpp"

namespace LunarDB::Selenity::API {

LUNAR_SINGLETON_INIT_IMPL(SchemasCatalog)
{
}

SchemasCatalog::~SchemasCatalog()
{
    saveToDisk();
}

std::filesystem::path SchemasCatalog::getSchemasHomePath() const
{
    auto const c_schemas_home{SystemCatalog::Instance().getLunarHomePath() / "schemas"};

    if (!std::filesystem::exists(c_schemas_home))
    {
        std::filesystem::create_directories(c_schemas_home);
    }

    return c_schemas_home;
}

std::filesystem::path SchemasCatalog::getSchemaFilePath(std::string schema_name) const
{
    return getSchemasHomePath() / (schema_name + ".schema");
}

void SchemasCatalog::saveSchemaToDisk(Common::QueryData::Schema const& schema) const
{
    std::ofstream schema_file(getSchemaFilePath(schema.name), std::ios::trunc | std::ios::binary);
    Common::CppExtensions::BinaryIO::Serializer::serialize(schema_file, schema);
    schema_file.close();
}

Common::QueryData::Schema SchemasCatalog::loadSchemaFromDisk(std::string_view schema_name) const
{
    auto const schema_file_path{getSchemaFilePath(std::string{schema_name})};
    if (!std::filesystem::exists(schema_file_path))
    {
        throw std::runtime_error{
            Common::CppExtensions::StringUtils::stringify("Schema", schema_name, "does not exist")};
    }

    Common::QueryData::Schema out_schema{};
    std::ifstream schema_file(schema_file_path, std::ios::binary);
    Common::CppExtensions::BinaryIO::Deserializer::deserialize(schema_file, out_schema);
    schema_file.close();

    return out_schema;
}

Common::QueryData::Schema const& SchemasCatalog::getSchema(std::string schema_name)
{
    if (auto const it = m_schemas_cache.find(schema_name); it != m_schemas_cache.end())
    {
        return it->second;
    }
    else
    {
        return m_schemas_cache.emplace(schema_name, loadSchemaFromDisk(schema_name)).first->second;
    }
}

void SchemasCatalog::createSchema(Common::QueryData::Schema schema)
{
    for (auto const& inheirted_schema_name : schema.inherits)
    {
        auto const& inherited_schema = SchemasCatalog::Instance().getSchema(inheirted_schema_name);
        for (auto const& field : inherited_schema.fields)
        {
            schema.fields.push_back(field);
        }
    }

    saveSchemaToDisk(schema);
    auto schema_name{schema.name};
    m_schemas_cache.emplace(std::move(schema_name), std::move(schema));
}

void SchemasCatalog::saveToDisk() const
{
    for (auto const& [schema_name, schema] : m_schemas_cache)
    {
        saveSchemaToDisk(schema);
    }
}

void SchemasCatalog::clearCache()
{
    m_schemas_cache.clear();
}

} // namespace LunarDB::Selenity::API
