#pragma once

#include "rapid_type.hpp"
#include "rapid_fnv1a.hpp"
#include "rapid_convert.hpp"
#include <ranges>

namespace rapid {

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

struct RapidNumber final 
{
    Napi::Env& env;
    bool match{false};

    Napi::Value as_number(const rapidjson::Value& elem)
    {
        auto val = elem.GetDouble();
        if (!match) {
            return Napi::Number::New(env, val);
        } 
        else if (elem.IsLosslessDouble())
        {
            return (val < 0) ?
                Napi::BigInt::New(env, elem.GetInt64()) :
                Napi::BigInt::New(env, elem.GetUint64());
        }
    
        Napi::Error::New(env, "is number").ThrowAsJavaScriptException(); 
        return env.Undefined();
    }

    Napi::Value operator()(const rapidjson::Value& elem)
    {
        if (elem.IsUint64()) {
            auto val = elem.GetUint64();
            if (match)
                return Napi::BigInt::New(env, val);
            return Napi::Number::New(env, val);
        } else if (elem.IsInt64()) {
            auto val = elem.GetInt64();
            if (match)
                return Napi::BigInt::New(env, val);
            return Napi::Number::New(env, val);
        } else if (elem.IsUint()) {
            auto val = elem.GetUint();
            if (match)
                return Napi::BigInt::New(env, static_cast<std::uint64_t>(val));
            return Napi::Number::New(env, val);
        } else if (elem.IsInt()) {
            auto val = elem.GetInt();
            if (match)
                return Napi::BigInt::New(env, static_cast<std::int64_t>(val));
            return Napi::Number::New(env, val);
        }
        
        return as_number(elem);
    }
};

template<class F>
struct RapidConvert;

template<class F>
struct RapidObject final 
{
    Napi::Env& env;
    // каждый рекурсивный вызов будет добавлять к пути
    // /key и выссчитывать хеш
    fnv1a hf;
    // функтор который принимает путь и возвращает нужно сделать BigInt
    F& fn;
    Napi::Value operator()(const rapidjson::Value& elem) 
    {
        auto res = Napi::Object::New(env);
        for (auto&& [key, val] : elem.GetObject()) 
        {
            auto s = key.GetString();
            RapidConvert f{env, hf(s, key.GetStringLength()), fn};
            res.Set(s, f(val));
        }
        return res;        
    }
};

template<class F>
struct RapidArray final 
{
    Napi::Env& env;
    // каждый рекурсивный вызов будет добавлять к пути
    // /key и выссчитывать хеш, для массива используем "*"
    fnv1a hf;
    // функтор который принимает путь и возвращает нужно сделать BigInt
    F& fn;
    Napi::Value operator()(const rapidjson::Value& elem) const
    {
        using namespace std::string_view_literals;
        auto size = elem.Size();
        auto res = Napi::Array::New(env, size);
        auto hashval = hf("*");
        for (std::size_t i = 0; i < size; ++i) 
        {
            auto& val = elem[i];
            RapidConvert f{env, hashval, fn};
            res.Set(i, f(val));
        }

        return res;        
    }
};

// template<class F>
// struct RapidArray final 
// {
//     Napi::Env& env;
//     fnv1a hf;
//     F& fn;

//     Napi::Value operator()(const rapidjson::Value& elem) const
//     {
//         using namespace std::string_view_literals;
//         auto size = elem.Size();
//         auto res = Napi::Array::New(env, size);
//         auto hashval = hf("*");

//         // Создаем диапазон индексов от 0 до size
//         auto indexRange = std::ranges::iota_view(0u, size);

//         // Преобразуем каждый элемент с помощью std::views::transform
//         auto elements = indexRange | std::views::transform([&](std::size_t i) {
//             auto& val = elem[i];
//             RapidConvert f{env, hashval, fn};
//             return f(val);
//         });

//         // Заполняем Napi::Array через ranges::for_each
//         std::ranges::for_each(elements, [&](auto& napiValue, std::size_t i) {
//             res.Set(i, napiValue);
//         });

//         return res;
//     }
// };

struct RapidString final 
{
    Napi::Env& env;
    auto operator()(const rapidjson::Value& elem) const
    {
        return Napi::String::New(env, 
            elem.GetString(), elem.GetStringLength());               
    }    
};

template<class F>
struct RapidConvert final 
{
    Napi::Env& env;
    // каждый рекурсивный вызов будет добавлять к пути
    // /key при встрече целого числа
    // запрашивается функтор F fn который принимает путь
    // и возвращает нужо ли создавать BigInt
    fnv1a hf;
    // функтор который принимает путь и возвращает нужно сделать BigInt
    F& fn;

    Napi::Value operator()(const rapidjson::Value& value) const
    {
        switch (value.GetType()) {
            case rapidjson::kNullType:
                return env.Null();
            case rapidjson::kFalseType:
                return Napi::Boolean::New(env, false);
            case rapidjson::kTrueType:
                return Napi::Boolean::New(env, true);
            case rapidjson::kObjectType: {
                RapidObject f{env, hf("/"), fn};
                return f(value);
            };
            case rapidjson::kArrayType: {
                RapidArray f{env, hf("/"), fn};
                return f(value);
            };
            case rapidjson::kStringType: {
                RapidString f{env};
                return f(value);
            }
            case rapidjson::kNumberType: {
                RapidNumber f{env, fn(hf)};
                return f(value);
            }
            default: ;
        }

        return env.Undefined();
    }
};

template<class F>
RapidConvert<F> convert(Napi::Env& env, F& fn) {
    constexpr fnv1a hf;
    constexpr auto hash = hf("#");
    return RapidConvert<F>{env, hash, fn};
}


} // namespace rapid