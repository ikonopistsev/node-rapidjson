#pragma once

#include "rapid_basic_document.hpp"

namespace rapid {

class Document final
    : public Napi::ObjectWrap<Document>
{
    BasicDocument self_;

public:
    static Napi::FunctionReference ctor;

    Document(const Napi::CallbackInfo& i);

    Napi::Value hasParseError(const Napi::CallbackInfo& i);

    Napi::Value parseError(const Napi::CallbackInfo& i);

    Napi::Value parseOffset(const Napi::CallbackInfo& i);

    Napi::Value parseMessage(const Napi::CallbackInfo& i);

    Napi::Value parse(const Napi::CallbackInfo& i);

    bool empty() const noexcept
    {
        return self_.empty();
    }

    bool parserError() const noexcept
    {
        if (!empty())
        {
            auto& d = static_cast<const rapidjson::Document&>(self_);
            return d.HasParseError();
        }

        return false;
    }

    operator rapidjson::Document&() noexcept
    {
        return self_;
    }

    operator const rapidjson::Document&() const noexcept
    {
        return self_;
    }

    bool Accept(rapidjson::SchemaValidator& v) const
    {
        return self_.accept(v);
    }

    Napi::Value getResult(const Napi::CallbackInfo& i);

    Napi::Value getResult(Napi::Env& env, Napi::Array& pointer, std::size_t level) const;
    
    static void Init(Napi::Env env, Napi::Object exports);
};    

} // namespace rapid