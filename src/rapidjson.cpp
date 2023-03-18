#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/allocators.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/internal/ieee754.h"
#include <limits>
#include <unordered_map>
#include <cmath>
#include <napi.h>

using namespace Napi;

namespace {

constexpr auto number_max_safe = 9007199254740991u;
using RapidAllocator = rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>;

template<class A>
auto copyout(const Napi::Value& value, A& alloc) {
    auto env = value.Env();
    size_t length;
    napi_status status =
        napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    NAPI_THROW_IF_FAILED(env, status, "");
    auto buf = static_cast<char*>(alloc.Malloc(++length));
    status = napi_get_value_string_utf8(env, value, buf, length, &length);
    NAPI_THROW_IF_FAILED(env, status, "");
    return rapidjson::StringRef(buf, length);
}

struct rapid_number final {
    bool is_mixed{false};
    Napi::Value mixed(const rapidjson::Value& elem, Napi::Env& env) {
        if (elem.IsInt()) {
            return Napi::Number::New(env, elem.GetInt());
        } else if (elem.IsUint()) {
            return Napi::Number::New(env, elem.GetUint());
        } else if (elem.IsInt64()) {
            auto val = elem.GetInt64();
            if (val > number_max_safe)
                return Napi::BigInt::New(env, val);
        } else if (elem.IsUint64()) {
            auto val = elem.GetInt64();
            if (val > number_max_safe)
                return Napi::BigInt::New(env, val);
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
    auto operator()(const rapidjson::Value& elem, Napi::Env& env) {
        return is_mixed ? mixed(elem, env) : bigint(elem, env);
    }
};

struct rapid_string final {
    Napi::Env& env;
    auto operator()(const rapidjson::Value& elem) {
        return Napi::String::New(env, elem.GetString(), 
            elem.GetStringLength());               
    }    
};

using keyword_type = std::unordered_map<std::string, rapid_number>;

template<class F>
Napi::Value rapid_convert(const rapidjson::Value& value, 
    Napi::Env& env, keyword_type& keyword, F number);

struct rapid_object final {
    Napi::Env& env;
    keyword_type& keyword;

    template<class F>
    auto parseArray(const rapidjson::Value& elem, F num_fn) {
        auto size = elem.Size();
        auto res = Napi::Array::New(env, size);
        for (std::size_t i = 0; i < size; ++i) {
            auto& val = elem[i];
            res.Set(i, rapid_convert(val, env, keyword, num_fn));
        }
        return res;
    }

    template<class F>
    auto parseObject(const rapidjson::Value& elem, F num_fn) {
        auto res = Napi::Object::New(env);
        for (auto&& [name, val] : elem.GetObject()) {
            auto key = name.GetString();
            if (!keyword.empty()) {
                auto f = keyword.find(key);
                if (f != keyword.end()) {
                    num_fn = f->second;
                }
            }
            res.Set(key, rapid_convert(val, env, keyword, num_fn));
        }
        return res;        
    }
};

template<class F>
Napi::Value rapid_convert(const rapidjson::Value& value, 
    Napi::Env& env, keyword_type& keyword, F number) {
    switch (value.GetType()) {
        case rapidjson::kNullType:
            return env.Null();
        case rapidjson::kFalseType:
            return Napi::Boolean::New(env, false);
        case rapidjson::kTrueType:
            return Napi::Boolean::New(env, true);
        case rapidjson::kObjectType: {
            rapid_object f{env, keyword};
            return f.parseObject(value, number);
        };
        case rapidjson::kArrayType: {
            rapid_object f{env, keyword};
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

struct rapid_generate final {
    using RapidStringBuffer = rapidjson::GenericStringBuffer<rapidjson::UTF8<>, RapidAllocator>;
    rapidjson::Document& doc;
    std::size_t output_size{ 64 * 1024u };
    auto operator()(const Napi::Value& value) {
        switch (value.Type()) {
            case napi_boolean:
                if (value.As<Napi::Boolean>().Value())
                    return rapidjson::Value{rapidjson::kTrueType};
                return rapidjson::Value{rapidjson::kFalseType};
            case napi_number: {
                auto number = value.As<Napi::Number>();
                auto val = number.DoubleValue();
                if (std::isnan(val) || std::isinf(val))
                    return rapidjson::Value{rapidjson::kNullType};
                if (val < 0) 
                    val = -val;
                if (val <= static_cast<double>(number_max_safe)) {
                    // Split the value into an integer part and a fractional part.
                    double integer = std::floor(val);
                    double fraction = val - integer;
                    // We only compute fractional digits up to the input double's precision.
                    double delta = 0.5 * (rapidjson::internal::Double(val).NextPositiveDouble() - val);
                    delta = std::max(rapidjson::internal::Double(0.0).NextPositiveDouble(), delta);
                    if (fraction >= delta) {
                        return rapidjson::Value{number.DoubleValue()};
                    }
                    return rapidjson::Value{number.Int64Value()};
                }
                return rapidjson::Value{number.DoubleValue()};
            }
            case napi_string: {
                return rapidjson::Value{copyout(value, doc.GetAllocator())};
            }
            case napi_object: {
                if (value.IsArray())
                    return rapidArray(value.As<Napi::Array>());
                return rapidObject(value.As<Napi::Object>());
            }
            case napi_bigint: {
                int sign;
                constexpr auto size = 2u;
                std::size_t count{size};
                std::uint64_t val[size];
                auto bigint = value.As<Napi::BigInt>();
                bigint.ToWords(&sign, &count, val);
                if (count < size) {
                    if (sign)
                        return rapidjson::Value{-(*reinterpret_cast<std::int64_t*>(val))};
                    return rapidjson::Value{val[0]}; 
                }
                Napi::RangeError::New(value.Env(), "Only 64bit BigInt")
                    .ThrowAsJavaScriptException();
            }
            default:;
        }
        
        return rapidjson::Value{rapidjson::kNullType};
    }

    rapidjson::Value rapidArray(const Napi::Array& value) {
        auto size = value.Length();
        auto& alloc = doc.GetAllocator();
        rapidjson::Value arr{rapidjson::kArrayType};
        arr.Reserve(size, alloc);
        rapid_generate gen{doc};
        for (std::size_t i = 0; i < size; ++i)
            arr.PushBack(gen(value[i]), alloc);
        return arr;
    }

    auto rapidArrayDocument(Napi::Env& env, const Napi::Array& value) {
        auto size = value.Length();
        auto& alloc = doc.GetAllocator();
        doc.SetArray();
        doc.Reserve(size, alloc);
        rapid_generate gen{doc};
        for (std::size_t i = 0; i < size; ++i) {
            doc.PushBack(gen(value[i]), alloc);
        }

        RapidAllocator a{alloc.Malloc(output_size), output_size};
        RapidStringBuffer buffer{&a, output_size / 4};
        rapidjson::Writer<RapidStringBuffer, rapidjson::UTF8<>, 
            rapidjson::UTF8<>, RapidAllocator> writer{buffer, &alloc};
        doc.Accept(writer);

        return Napi::String::New(env, 
            buffer.GetString(), buffer.GetSize());
    }

    rapidjson::Value rapidObject(const Napi::Object& value) {
        auto& alloc = doc.GetAllocator();
        rapidjson::Value arr{rapidjson::kObjectType};
        rapid_generate gen{doc};
        for (auto&& elem: value) {
            arr.AddMember(rapidjson::Value{copyout(elem.first, alloc)}, 
                gen(elem.second), alloc);
        }
        return arr;
    }

    auto rapidObjectDocument(Napi::Env& env, const Napi::Object& value) {
        doc.SetObject();
        auto& alloc = doc.GetAllocator();
        rapid_generate gen{doc};
        for(auto&& elem: value) {
            doc.AddMember(rapidjson::Value{copyout(elem.first, alloc)}, 
                gen(elem.second), alloc);
        }
    
        RapidAllocator a{alloc.Malloc(output_size), output_size};
        RapidStringBuffer buffer{&a, output_size / 4};
        rapidjson::Writer<RapidStringBuffer, rapidjson::UTF8<>, 
            rapidjson::UTF8<>, RapidAllocator> writer{buffer, &alloc};
        doc.Accept(writer);

        return Napi::String::New(env, 
            buffer.GetString(), buffer.GetSize());
    }    

    auto rapidDocument(Napi::Env& env, const Napi::Value& value) {
        if (napi_object == value.Type()) {
            rapid_generate gen{doc};
            if (value.IsArray()) {
                return gen.rapidArrayDocument(env, value.As<Napi::Array>());
            }
            return gen.rapidObjectDocument(env, value.As<Napi::Object>());
        }
        return value.ToString();
    }
};

}

class RapidJSON final
    : public ObjectWrap<RapidJSON>
{
    static constexpr auto alloc_min = std::size_t{2048u}; // 256kb total
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
            InstanceMethod("forceBigInt", &RapidJSON::forceBigInt),
            InstanceMethod("stringify", &RapidJSON::stringify)
        });

        auto constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("RapidJSON", func);
        return exports;
    }

private:
    template<class F>
    auto parse(const Napi::Value& value, Napi::Env& env, F number)
    {        
        auto allocator = allocator_.get();
        allocator->Clear();
        
        rapidjson::Document d(allocator);
        auto json = copyout(value, d.GetAllocator());
        d.Parse(json, json.length);
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

        return rapid_convert(d, env, keyword_, number);
    }

    Napi::Value parse(const Napi::CallbackInfo &info)
    {
        return parseMixed(info);
    }

    Napi::Value parseMixed(const Napi::CallbackInfo &info)
    {
        auto env = info.Env();
        if (info.Length() != 1) 
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

        return parse(arg0, env, rapid_number{true});
    }

    Napi::Value parseBigInt(const Napi::CallbackInfo &info)
    {
        auto env = info.Env();
        if (info.Length() != 1) 
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

        return parse(arg0, env, rapid_number{});
    }

    void setupMixed(const Napi::Array& arr, bool is_mixed)
    {
        keyword_type k;
        for (std::size_t i = 0; i < arr.Length(); ++i) 
        {
            auto text = std::string{arr.Get(i).As<Napi::String>()};
            k.emplace(std::move(text), rapid_number{is_mixed});
        }

        keyword_ = std::move(k);
    }

    void forceBigInt(const Napi::CallbackInfo& info) 
    {
        if (info.Length() < 1) 
        {
            Napi::TypeError::New(info.Env(), "Wrong number of arguments")
                .ThrowAsJavaScriptException();
        }

        setupMixed(info[0].As<Napi::Array>(), false);
    }

    Napi::Value stringify(const Napi::CallbackInfo& info)
    {
        auto allocator = allocator_.get();
        allocator->Clear();

        auto env = info.Env();
        if (info.Length() != 1) 
        {
            Napi::TypeError::New(env, "Wrong number of arguments")
                .ThrowAsJavaScriptException();
            return env.Undefined();
        }

        auto value = info[0];
        if (!value.IsEmpty()) 
        {
            rapidjson::Document doc(allocator);
            rapid_generate gen{doc, 
                static_cast<std::size_t>(alloc_size_ * 1.618)};
            return gen.rapidDocument(env, value);
        }
        return env.Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    return RapidJSON::Init(env, exports);
}

NODE_API_MODULE(addon, Init)