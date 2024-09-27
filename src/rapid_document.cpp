#include "rapid_document.hpp"
#include "rapid_convert.hpp"
#include "rapid_fnv1a.hpp"
#include "rapidjson/error/en.h"
#include <limits>
#include <cmath>
#include <ranges>
#include <napi.h>
//#include <iostream>

using namespace std::string_view_literals;

namespace rapid {

Napi::FunctionReference Document::ctor{};

Document::Document(const Napi::CallbackInfo& i)
    : ObjectWrap{i}
{   
    auto env = i.Env();
    try {
        self_.create(getSizeDefault(i));
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    }
}

Napi::Value Document::hasParseError(const Napi::CallbackInfo& i)
{
    auto& d = self_.get();
    return Napi::Boolean::New(i.Env(), d.HasParseError());
}

Napi::Value Document::parseError(const Napi::CallbackInfo& i)
{          
    auto env = i.Env();
    auto& d = self_.get();
    return Napi::Number::New(env, d.GetParseError());
}

Napi::Value Document::parseOffset(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    auto& d = self_.get();
    return Napi::Number::New(env, d.GetErrorOffset());    
}

Napi::Value Document::parseMessage(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    auto& d = self_.get();
    return Napi::String::New(env, 
        rapidjson::GetParseError_En(d.GetParseError()));
}

Napi::Value Document::parse(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    if (i.Length() != 1) 
    {
        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
    
    auto& arg0 = i[0];
    if (arg0.IsUndefined() || arg0.IsNull() || 
        arg0.IsNumber() || arg0.IsBoolean())
        return arg0;

    // аргумент должен быть строкой или буффером
    if (!arg0.IsBuffer())
    {
        Napi::TypeError::New(env, "argument must be a buffer")
            .ThrowAsJavaScriptException();
        return env.Null();
    }
    
    try {
        // вычитываем json из аргумента
        auto buffer = arg0.As<Napi::Buffer<char>>();
        return Napi::Boolean::New(env, self_.parse(buffer.Data(), buffer.Length()));
    } catch (const std::exception& e) {
        Napi::Error::New(env, e.what()).ThrowAsJavaScriptException();
    } catch (...) {
        Napi::Error::New(env, "Document::parse").ThrowAsJavaScriptException();
    }    

    // возвращаем результат парсинга
    return Napi::Boolean::New(env, false);
}

Napi::Value Document::getResult(const Napi::CallbackInfo& i)
{
    auto env = i.Env();
    // если нам передали массив поинтеров
    if (i.Length() == 1)
    {
        auto& arg0 = i[0];
        // arg0 это объект со свойстом level типа Array
        if (arg0.IsObject())
        {
            auto obj = arg0.As<Napi::Object>();
            auto pointer = obj.Get("pointer");
            auto arr = pointer.As<Napi::Array>();
            // аргумент должен быть массивом
            // массив должен быть отсортирован
            //std::cout << "Document::getResult" << std::endl;
            return getResult(env, arr, 0);
        }
    }

    auto arr = Napi::Array::New(env, 0);
    //std::cout << "Document::getResult -1 " << arr.Length() << std::endl;
    return getResult(env, arr, 1);
}

Napi::Value Document::getResult(Napi::Env& env, Napi::Array& pointer, std::size_t level) const
{
    auto f = convert(env, pointer, level);
    return f(self_.get());
}

void Document::Init(Napi::Env env, Napi::Object exports)
{
    auto className = "Document";
    auto func = DefineClass(env, className, {
        InstanceMethod("hasParseError", &Document::hasParseError),
        InstanceMethod("parseError", &Document::parseError),
        InstanceMethod("parseOffset", &Document::parseOffset),
        InstanceMethod("parseMessage", &Document::parseMessage),
        InstanceMethod("parse", &Document::parse),
        InstanceMethod("getResult", &Document::getResult),
        InstanceMethod("get", &Document::getResult)
    });
    ctor = Napi::Persistent(func);
    ctor.SuppressDestruct();
    exports.Set(className, func);
}

} // namespace rapid
