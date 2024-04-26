#pragma once

#include "Moonlight/ParsedQuery.hpp"
#include "Selenity/SystemCatalog.hpp"

namespace LunarDB::Astral::API {

struct SelenityDependencies
{
    Selenity::API::SystemCatalog& db_catalog;
};

} // namespace LunarDB::Astral::API
