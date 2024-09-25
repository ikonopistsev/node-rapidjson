#include "rapid_schema.hpp"
//#include <iostream>

namespace rapid {

Napi::FunctionReference Schema::ctor{};

Schema::Schema(const Napi::CallbackInfo& i)
    : ObjectWrap{i}
{   
    auto env = i.Env();
    // создаем документ
    auto documentInstance = Document::ctor.New({});
    docRef_ = Napi::Persistent(documentInstance);
    docRef_.SuppressDestruct();
}    

Schema::~Schema()
{
    docRef_.Reset();
}

Napi::Value Schema::parse(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    try {
        auto& document = schemaDoc();
        auto result = document.parse(i);
        if (result) {
            self_.reset(new rapidjson::SchemaDocument{document});
            validator_.reset(new rapidjson::SchemaValidator{*self_});
        }
        return Napi::Boolean::New(env, result);
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    } catch (...) {
        Napi::Error::New(env, "Document::parse").ThrowAsJavaScriptException();
    }    

    // возвращаем результат парсинга
    return Napi::Boolean::New(env, false); 
}

void Schema::saveLastError(const rapidjson::SchemaValidator& validator, 
    const rapidjson::Document& scheme, const rapidjson::Document& document) 
{
    validateKeyword_ = validator.GetInvalidSchemaKeyword();

    rapidjson::StringBuffer sb;
    auto schemaPointer = validator.GetInvalidSchemaPointer();
    schemaPointer.StringifyUriFragment(sb);
    schemaPointer_ = std::string_view{sb.GetString(), sb.GetSize()};
    sb.Clear();

    auto documentPointer = validator.GetInvalidDocumentPointer();
    documentPointer.StringifyUriFragment(sb);
    documentPointer_ = std::string_view{sb.GetString(), sb.GetSize()};
}

Napi::Value Schema::validate(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    if (!(self_ && validator_))
    {
        Napi::TypeError::New(env, "missing schema")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    // получаем документ из первого аргумента
    if (i.Length() < 1)
    {
        Napi::TypeError::New(env, "missing argument")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto& arg0 = i[0];
    if (!arg0.IsObject())
    {
        Napi::TypeError::New(env, "argument must be an object")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    auto someObject = arg0.As<Napi::Object>();
    // проверяем что объект является документом
    if (!someObject.InstanceOf(Document::ctor.Value()))
    {
        Napi::TypeError::New(env, "argument must be a Document")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }

    validator_->Reset();
    // теперь мы точно знаем что это документ
    auto doc = Napi::ObjectWrap<Document>::Unwrap(someObject);
    auto result = doc->Accept(*validator_);
    if (!result)
        saveLastError(*validator_, schemaDoc(), *doc);

    return Napi::Boolean::New(env, result);
}

void Schema::Init(Napi::Env env, Napi::Object exports)
{
    auto className = "Schema";
    auto func = DefineClass(env, className, {
        InstanceMethod("hasParseError", &Schema::hasParseError),
        InstanceMethod("parseError", &Schema::parseError),
        InstanceMethod("parseOffset", &Schema::parseOffset),
        InstanceMethod("parseMessage", &Schema::parseMessage),
        InstanceMethod("schemaPointer", &Schema::schemaPointer),
        InstanceMethod("validateKeyword", &Schema::validateKeyword),
        InstanceMethod("documentPointer", &Schema::documentPointer),
        InstanceMethod("parse", &Schema::parse),
        InstanceMethod("validate", &Schema::validate)
    });

    ctor = Napi::Persistent(func);
    ctor.SuppressDestruct();
    exports.Set(className, func);
}

} // namespace rapid