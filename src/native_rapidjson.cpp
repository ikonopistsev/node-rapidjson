#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/allocators.h"
#include <napi.h>
#include <string_view>
#include <vector>

using namespace Napi;

namespace {

// strange usage :(
struct rapid_number final {
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) {
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
};

struct rapid_bigint final {
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) {
        if (elem.IsInt64()) {
            return Napi::BigInt::New(env, elem.GetInt64());
        } else if (elem.IsUint64()) {
            return Napi::BigInt::New(env, elem.GetUint64());
        }
        return Napi::Number::New(env, elem.GetDouble());
    }    
};

struct rapid_string final {
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) {
        return Napi::String::New(env, elem.GetString(), 
            elem.GetStringLength());               
    }    
};

struct rapid_object final {
    Napi::Env& env;

    template<class F>
    Napi::Value parseArray(const rapidjson::Value& elem, F num_fn) {
        auto size = elem.Size();
        auto res = Napi::Array::New(env, size);
        for (std::size_t i = 0; i < size; ++i) {
            auto& val = elem[i];
            auto type = val.GetType();
            switch (type) {
                case rapidjson::kNullType:  {
                    res.Set(i, env.Null());
                } break;
                case rapidjson::kFalseType: {
                    res.Set(i, false);
                } break;
                case rapidjson::kTrueType: {
                    res.Set(i, true);
                } break;
                case rapidjson::kObjectType: {
                    rapid_object f{env};
                    res.Set(i, f.parseObject(val, num_fn));
                } break;
                case rapidjson::kArrayType: {
                    rapid_object f{env};
                    res.Set(i, f.parseArray(val, num_fn));                    
                } break;
                case rapidjson::kStringType: {
                    rapid_string s{env};
                    res.Set(i, s(val));
                } break;
                default:
                    res.Set(i, num_fn(val));
            }
        }
        return res;
    }

    template<class F>
    Napi::Value parseObject(const rapidjson::Value& elem, F num_fn) {
        auto res = Napi::Object::New(env);
        for (auto& [name, val] : elem.GetObject()) {
            auto key = name.GetString();
            switch (val.GetType()) {
                case rapidjson::kNullType:  {
                    res.Set(key, env.Null());
                } break;
                case rapidjson::kFalseType: {
                    res.Set(key, false);
                } break;
                case rapidjson::kTrueType: {
                    res.Set(key, true);
                } break;
                case rapidjson::kObjectType: {
                    rapid_object f{env};
                    res.Set(key, f.parseObject(val, num_fn));
                } break;
                case rapidjson::kArrayType: {
                    rapid_object f{env};
                    res.Set(key, f.parseArray(val, num_fn));
                } break;
                case rapidjson::kStringType: {
                    rapid_string s{env};
                    res.Set(key, s(val));
                } break;
                default:
                    res.Set(key, num_fn(val));               
            }
        }
        return res;        
    }
};

}

class RapidJSON final
    : public ObjectWrap<RapidJSON>
{
    using RapidAllocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;
    static constexpr auto alloc_min = std::size_t{2048u};
    static constexpr auto alloc_def = std::size_t{128u * alloc_min};
    std::size_t alloc_size_{alloc_def};
    std::unique_ptr<char[]> buffer_{};
    std::unique_ptr<RapidAllocator> allocator_{};
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
            InstanceMethod("parseMixed", &RapidJSON::parseMixed),
            InstanceMethod("parseInt64", &RapidJSON::parseInt64),
            InstanceMethod("forceInt64", &RapidJSON::forceInt64),
            InstanceMethod("forceNumber", &RapidJSON::forceNumber),
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

        switch (d.GetType()) {
            case rapidjson::kNullType:
                return env.Null();
            case rapidjson::kFalseType:
                return Napi::Boolean::New(env, false);
            case rapidjson::kTrueType:
                return Napi::Boolean::New(env, true);
            case rapidjson::kObjectType: {
                rapid_object f{env};
                return f.parseObject(d, number);
            };
            case rapidjson::kArrayType: {
                rapid_object f{env};
                return f.parseArray(d, number);
            };
            case rapidjson::kStringType: {
                rapid_string s{env};
                return s(d);
            }
            default: 
                return number(d);
        }

        return env.Undefined();
    }

    Napi::Value parse(const Napi::CallbackInfo &info)
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

        return parse(arg0.ToString(), env, rapid_bigint{env});
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

        return parse(arg0.ToString(), env, rapid_number{env});
    }

    Napi::Value parseInt64(const Napi::CallbackInfo &info)
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

        return parse(arg0.ToString(), env, rapid_bigint{env});
    }

    Napi::Value forceInt64(const Napi::CallbackInfo &info) 
    {
        auto env = info.Env();
        return env.Undefined();
    }

    Napi::Value forceNumber(const Napi::CallbackInfo &info) 
    {
        auto env = info.Env();
        return env.Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return RapidJSON::Init(env, exports);
}

NODE_API_MODULE(addon, Init)