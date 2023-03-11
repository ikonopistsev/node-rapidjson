#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/allocators.h"
#include <napi.h>
#include <string_view>
#include <vector>

using namespace Napi;

struct rapid_arrray final {
    Napi::Env& env;
    Napi::Array base;
    Napi::Value operator()(const rapidjson::Value& elem);
};

struct rapid_object final {
    Napi::Env& env;
    Napi::Object base;
    Napi::Value operator()(const rapidjson::Value& elem);
};

struct rapid_string final {
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) {
        return Napi::String::New(env, elem.GetString(), 
            elem.GetStringLength());               
    }    
};

Napi::Value rapid_arrray::operator()(const rapidjson::Value& elem)
{
    auto size = elem.Size();
    for (std::size_t i = 0; i < size; ++i) {
        auto& val = elem[i];
        auto type = val.GetType();
        switch (type) {
            case rapidjson::kNullType:  {
                base.Set(i, env.Null());
            } break;
            case rapidjson::kFalseType: {
                base.Set(i, false);
            } break;
            case rapidjson::kTrueType: {
                base.Set(i, true);
            } break;
            case rapidjson::kObjectType: {
                rapid_object f{env, Napi::Object::New(env)};
                base.Set(i, f(val));
            } break;
            case rapidjson::kArrayType: {
                rapid_arrray f{env, Napi::Array::New(env, val.Size())};
                base.Set(i, f(val));
            } break;
            case rapidjson::kStringType: {
                rapid_string s{env};
                base.Set(i, s(val));
            } break;
            default: {
                if (val.IsInt()) {
                    base.Set(i, Napi::Number::New(env, val.GetInt()));
                } else if (val.IsUint()) {
                    base.Set(i, Napi::Number::New(env, val.GetUint()));
                } else if (val.IsInt64()) {
                    base.Set(i, Napi::BigInt::New(env, val.GetInt64()));
                } else if (val.IsUint64()) {
                    base.Set(i, Napi::BigInt::New(env, val.GetUint64()));
                } else {
                    base.Set(i, Napi::Number::New(env, val.GetDouble()));
                }
            };
        }
    }
    return base;
}

Napi::Value rapid_object::operator()(const rapidjson::Value& elem)
{
    for (auto& [name, val] : elem.GetObject()) {
        auto key = name.GetString();
        switch (val.GetType()) {
            case rapidjson::kNullType:  {
                base.Set(key, env.Null());
            } break;
            case rapidjson::kFalseType: {
                base.Set(key, false);
            } break;
            case rapidjson::kTrueType: {
                base.Set(key, true);
            } break;
            case rapidjson::kObjectType: {
                rapid_object f{env, Napi::Object::New(env)};
                base.Set(key, f(val));
            } break;
            case rapidjson::kArrayType: {
                rapid_arrray f{env, Napi::Array::New(env, val.Size())};
                base.Set(key, f(val));
            } break;
            case rapidjson::kStringType: {
                rapid_string s{env};
                base.Set(key, s(val));
            } break;
            default: {
                if (val.IsInt()) {
                    base.Set(key, Napi::Number::New(env, val.GetInt()));
                } else if (val.IsUint()) {
                    base.Set(key, Napi::Number::New(env, val.GetUint()));
                } else if (val.IsInt64()) {
                    base.Set(key, Napi::BigInt::New(env, val.GetInt64()));
                } else if (val.IsUint64()) {
                    base.Set(key, Napi::BigInt::New(env, val.GetUint64()));
                } else {
                    base.Set(key, Napi::Number::New(env, val.GetDouble()));
                }
            };
        }
    }
    return base;
}

class RapidJSON final
    : public ObjectWrap<RapidJSON>
{
    char buffer_[16 * 1024];
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> alloc_{buffer_, sizeof(buffer_)};
public:
    RapidJSON(const Napi::CallbackInfo &callbackInfo)
        : ObjectWrap(callbackInfo)
    {   

    }

    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        auto func = DefineClass(env, "RapidJSON", {
            InstanceMethod("parse", &RapidJSON::parse)
        });

        auto constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("RapidJSON", func);
        return exports;
    }

private:
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
        if (arg0.IsUndefined())
            return arg0;

        if (arg0.IsNull())
            return arg0;
        
        if (arg0.IsNumber())
            return arg0;

        if (!arg0.IsString())
        {
            Napi::TypeError::New(env, "Wrong arguments")
                .ThrowAsJavaScriptException();
            return env.Null();
        }

        alloc_.Clear();
        rapidjson::Document d(&alloc_);
        d.Parse(arg0.ToString());
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
                rapid_object f{env, Napi::Object::New(env)};
                return f(d);
            };
            case rapidjson::kArrayType: {
                rapid_arrray f{env, Napi::Array::New(env, d.Size())};
                return f(d);
            };
            case rapidjson::kStringType: {
                rapid_string s{env};
                return s(d);
            }
            default:
                if (d.IsInt()) {
                    return Napi::Number::New(env, d.GetInt());
                } else if (d.IsUint()) {
                    return Napi::Number::New(env, d.GetUint());
                } else if (d.IsInt64()) {
                    return Napi::BigInt::New(env, d.GetInt64());
                } else if (d.IsUint64()) {
                    return Napi::BigInt::New(env, d.GetUint64());
                } else {
                    return Napi::Number::New(env, d.GetDouble());
                };
        }

        return env.Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return RapidJSON::Init(env, exports);
}

NODE_API_MODULE(addon, Init)