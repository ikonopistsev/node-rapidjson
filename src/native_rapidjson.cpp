#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/allocators.h"
#include <napi.h>
#include <string_view>
#include <unordered_map>

using namespace Napi;

namespace {

struct rapid_number final {
    bool is_mixed{false};
    Napi::Value mixed(const rapidjson::Value& elem, Napi::Env& env) {
        if (elem.IsInt()) {
            return Napi::Number::New(env, elem.GetInt());
        } else if (elem.IsUint()) {
            return Napi::Number::New(env, elem.GetUint());
        } else if (elem.IsInt64()) {
            return Napi::BigInt::New(env, elem.GetInt64());
        } else if (elem.IsUint64()) {
            return Napi::BigInt::New(env, elem.GetUint64());
        }
        return Napi::Number::New(env, elem.GetDouble());          
    }
    Napi::Value bigint(const rapidjson::Value& elem, Napi::Env& env) {
        if (elem.IsInt64()) {
            return Napi::BigInt::New(env, elem.GetInt64());
        } else if (elem.IsUint64()) {
            return Napi::BigInt::New(env, elem.GetUint64());
        }
        return Napi::Number::New(env, elem.GetDouble());
    }
    Napi::Value operator()(const rapidjson::Value& elem, Napi::Env& env) {
        return is_mixed ? 
            mixed(elem, env) : bigint(elem, env);
    }
};

struct rapid_string final {
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) {
        return Napi::String::New(env, elem.GetString(), 
            elem.GetStringLength());               
    }    
};

template<class F>
Napi::Value rapid_convert(const rapidjson::Value& value, Napi::Env& env, F number);

struct rapid_object final {
    Napi::Env& env;

    template<class F>
    Napi::Value parseArray(const rapidjson::Value& elem, F num_fn) {
        auto size = elem.Size();
        auto res = Napi::Array::New(env, size);
        for (std::size_t i = 0; i < size; ++i) {
            auto& val = elem[i];
            auto type = val.GetType();
            res.Set(i, rapid_convert(val, env, num_fn));
        }
        return res;
    }

    template<class F>
    Napi::Value parseObject(const rapidjson::Value& elem, F num_fn) {
        auto res = Napi::Object::New(env);
        for (auto& [name, val] : elem.GetObject()) {
            auto key = name.GetString();
            res.Set(key, rapid_convert(val, env, num_fn));
        }
        return res;        
    }
};

template<class F>
Napi::Value rapid_convert(const rapidjson::Value& value, Napi::Env& env, F number) {
    switch (value.GetType()) {
        case rapidjson::kNullType:
            return env.Null();
        case rapidjson::kFalseType:
            return Napi::Boolean::New(env, false);
        case rapidjson::kTrueType:
            return Napi::Boolean::New(env, true);
        case rapidjson::kObjectType: {
            rapid_object f{env};
            return f.parseObject(value, number);
        };
        case rapidjson::kArrayType: {
            rapid_object f{env};
            return f.parseArray(value, number);
        };
        case rapidjson::kStringType: {
            rapid_string s{env};
            return s(value);
        }
        default: ;
    }
    return number(value, env);
}

}

class RapidJSON final
    : public ObjectWrap<RapidJSON>
{
    using RapidAllocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;
    using keyword_type = std::unordered_map<std::string, rapid_number>;
    static constexpr auto alloc_min = std::size_t{2048u};
    static constexpr auto alloc_def = std::size_t{128u * alloc_min};
    std::size_t alloc_size_{alloc_def};
    std::unique_ptr<char[]> buffer_{};
    std::unique_ptr<RapidAllocator> allocator_{};
    keyword_type keyword_{};
public:
    RapidJSON(const Napi::CallbackInfo& info)
        : ObjectWrap(info)
    {   
        auto env = info.Env();
        if ((info.Length() >= 1) && info[0].IsNumber())
        {
            auto mem_size = info[0].As<Napi::Number>();
            auto val = mem_size.Uint32Value();
            alloc_size_ = (alloc_min < val) ? val : alloc_min;
        }

        buffer_.reset(new char[alloc_size_]);
        allocator_.reset(new RapidAllocator{buffer_.get(), alloc_size_});
    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        auto func = DefineClass(env, "RapidJSON", {
            InstanceMethod("parse", &RapidJSON::parse),
            InstanceMethod("parseBigInt", &RapidJSON::parseBigInt),
            InstanceMethod("forceKeyword", &RapidJSON::forceKeyword)
        });

        auto constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("RapidJSON", func);
        return exports;
    }

private:
    template<class F>
    Napi::Value parse(const std::string& text, Napi::Env& env, F number)
    {        
        auto allocator = allocator_.get();
        allocator->Clear();
        rapidjson::Document d(allocator);
        d.Parse(text);
        if (d.HasParseError()) 
        {
            std::string err("Error at offset ");
            err += std::to_string(d.GetErrorOffset());
            err += ": ";
            err += GetParseError_En(d.GetParseError());
            Napi::TypeError::New(env, err)
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        return rapid_convert(d, env, number);
    }

    Napi::Value parse(const Napi::CallbackInfo &info)
    {
        return parseMixed(info);
    }

    Napi::Value parseMixed(const Napi::CallbackInfo &info)
    {
        auto env = info.Env();
        if (1 != info.Length()) 
        {
            Napi::TypeError::New(env, "Wrong number of arguments")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        
        auto& arg0 = info[0];
        if (arg0.IsUndefined() || arg0.IsNull() || 
            arg0.IsNumber() || arg0.IsBoolean())
            return arg0;

        if (!arg0.IsString())
        {
            Napi::TypeError::New(env, "Wrong arguments")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        return parse(arg0.ToString(), env, rapid_number{true});
    }

    Napi::Value parseBigInt(const Napi::CallbackInfo &info)
    {
        auto env = info.Env();
        if (1 != info.Length()) 
        {
            Napi::TypeError::New(env, "Wrong number of arguments")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }
        
        auto& arg0 = info[0];
        if (arg0.IsUndefined() || arg0.IsNull() || 
            arg0.IsNumber() || arg0.IsBoolean())
            return arg0;

        if (!arg0.IsString())
        {
            Napi::TypeError::New(env, "Wrong arguments")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        return parse(arg0.ToString(), env, rapid_number{});
    }

    void setupMixed(Napi::Array arr, bool is_mixed)
    {
        keyword_type k;
        for (std::size_t i = 0; i < arr.Length(); ++i) 
        {
            auto text = std::string{arr.Get(i).As<Napi::String>()};
            k.emplace(std::move(text), rapid_number{is_mixed});
        }

        keyword_ = std::move(k);
    }

    Napi::Value forceKeyword(const Napi::CallbackInfo &info) 
    {
        auto env = info.Env();
        if (info.Length() >= 1)
        {
            setupMixed(info[0].As<Napi::Array>(), false);
            return env.Undefined();
        }        

        Napi::TypeError::New(env, "Wrong number of arguments")
            .ThrowAsJavaScriptException();
        return env.Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return RapidJSON::Init(env, exports);
}

NODE_API_MODULE(addon, Init)