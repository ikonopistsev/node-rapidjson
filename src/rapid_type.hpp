#pragma once

#include "rapidjson/allocators.h"
#include "rapidjson/document.h"
#include "rapidjson/schema.h"

#include <napi.h>
#include <memory>

namespace rapid {

constexpr auto number_max_safe = 9007199254740991u;

using PoolAllocatorType = 
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

using DocumentAllocator = std::unique_ptr<PoolAllocatorType>;
using DocumentPtr = std::unique_ptr<rapidjson::Document>;
using SchemaDocumentPtr = std::unique_ptr<rapidjson::SchemaDocument>;

} // namespace rapid