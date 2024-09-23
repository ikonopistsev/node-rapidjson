#pragma once

#include "rapid_type.hpp"

namespace rapid {

std::size_t getSizeDefault(const Napi::CallbackInfo& i);

class BasicDocument final
{
    std::vector<char> chunk_;
    DocumentAllocator mem_;
    DocumentPtr self_;
public:
    BasicDocument() = default;

    void create(std::size_t chunkSize);

    void clear();

    bool parse(const char* json, std::size_t size);

    bool accept(rapidjson::SchemaValidator& v) const
    {
        return self_->Accept(v);
    }

    rapidjson::Document& get() noexcept
    {
        return *self_;
    }

    const rapidjson::Document& get() const noexcept
    {
        return *self_;
    }    

    bool empty() const noexcept
    {
        return self_ == nullptr;
    }

    operator rapidjson::Document&() noexcept
    {
        return get();
    }

    operator const rapidjson::Document&() const noexcept
    {
        return get();
    }

    operator PoolAllocatorType& () noexcept
    {
        return *mem_;
    }
};

} // namespace rapid