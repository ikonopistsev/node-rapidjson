#pragma once

#include "rapid_document.hpp"

namespace rapid {

class Schema final
    : public Napi::ObjectWrap<Schema>
{
    SchemaDocumentPtr self_;
    Napi::ObjectReference docRef_;
    std::string schemaPointer_{};
    std::string validateKeyword_{};
    std::string documentPointer_{};

    Document& schemaDoc() noexcept
    {
        return *Napi::ObjectWrap<Document>::Unwrap(docRef_.Value());
    }

    const Document& schemaDoc() const noexcept
    {
        return *Napi::ObjectWrap<Document>::Unwrap(docRef_.Value());
    }

    void saveLastError(const rapidjson::SchemaValidator& validator, 
        const rapidjson::Document& scheme, const rapidjson::Document& document);

public:
    static Napi::FunctionReference ctor;

    Schema(const Napi::CallbackInfo& info);

    ~Schema();

    Napi::Value hasParseError(const Napi::CallbackInfo& i)
    {
        return schemaDoc().hasParseError(i);
    }

    Napi::Value parseError(const Napi::CallbackInfo& i)
    {          
        return schemaDoc().parseError(i);
    }

    Napi::Value parseOffset(const Napi::CallbackInfo& i)
    {
        return schemaDoc().parseOffset(i);
    }

    Napi::Value parseMessage(const Napi::CallbackInfo& i)
    {
        return schemaDoc().parseMessage(i);
    }

    Napi::Value parse(const Napi::CallbackInfo& i);

    Napi::Value validate(const Napi::CallbackInfo& i);

    Napi::Value schemaPointer(const Napi::CallbackInfo& i)
    {
        return Napi::String::New(i.Env(), schemaPointer_);
    }

    Napi::Value validateKeyword(const Napi::CallbackInfo& i)
    {
        return Napi::String::New(i.Env(), validateKeyword_);
    }

    Napi::Value documentPointer(const Napi::CallbackInfo& i)
    {
        return Napi::String::New(i.Env(), documentPointer_);
    }

    static void Init(Napi::Env env, Napi::Object exports);
};

} // namespace rapid
