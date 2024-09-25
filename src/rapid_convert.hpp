#pragma once

#include "rapid_type.hpp"
#include "rapid_fnv1a.hpp"
#include "rapid_convert.hpp"
#include <ranges>
#include <iostream>

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

struct RapidString final 
{
    Napi::Env& env;
    Napi::Value operator()(const rapidjson::Value& elem) const
    {
        return Napi::String::New(env, 
            elem.GetString(), elem.GetStringLength());               
    }    
};

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

struct RapidConvert final 
{
    Napi::Env& env;
    Napi::Array& pointer;
    std::size_t level;
    fnv1a hf;

    bool match() const;
    Napi::Value operator()(const rapidjson::Value& value) const;
};

struct RapidObject final 
{
    Napi::Env& env;
    Napi::Array& pointer;
    std::size_t level;
    fnv1a hf;

    Napi::Value operator()(const rapidjson::Value& elem) 
    {
        auto res = Napi::Object::New(env);
        for (auto&& [key, val] : elem.GetObject()) 
        {
            auto s = key.GetString();
            //std::cout << "RapidObject " << std::string_view{s, key.GetStringLength()} << "=" << hf(s, key.GetStringLength()) << std::endl;
            RapidConvert f{env, pointer, level + 1, hf(s, key.GetStringLength())};
            res.Set(s, f(val));
        }
        return res;
    }
};

struct RapidArray final 
{
    Napi::Env& env;
    Napi::Array& pointer;
    std::size_t level;
    fnv1a hf;
    Napi::Value operator()(const rapidjson::Value& elem) const
    {
        using namespace std::string_view_literals;
        auto size = elem.Size();
        //std::cout << "RapidArray " << size << std::endl;
        auto res = Napi::Array::New(env, size);
        auto hashval = hf("*");
        for (std::size_t i = 0; i < size; ++i) 
        {
            auto& val = elem[i];
            //std::cout << "RapidArray " << i << std::endl;
            RapidConvert f{env, pointer, level + 1, hashval};
            res.Set(i, f(val));
        }
        return res;        
    }
};

bool RapidConvert::match() const 
{
    auto length = pointer.Length();
    if (level < length)
    {
        //std::cout << "RapidConvert::match" << level << " " << length << std::endl;
        // получаем уровень
        auto val = pointer.Get(level);
        // кастуем к массиву
        auto arr = val.As<Napi::Array>();
        if (arr.Length()) 
        {
            // Создаем диапазон индексов от 0 до Length
            auto idx = std::views::iota(0u, arr.Length());
            // Создаем трансформированный диапазон значений
            auto pointer = idx | std::views::transform([&arr](auto index) {
                auto number = arr.Get(index).ToNumber();
                return number.Uint32Value();
            });
            // находим вхождения элементов
            return std::ranges::binary_search(pointer, hf);
        }
    } 

    //std::cout << "RapidConvert::match " << level << " false" << std::endl;
    return false;
}

Napi::Value RapidConvert::operator()(const rapidjson::Value& value) const
{
    switch (value.GetType()) {
        case rapidjson::kNullType:
            return env.Null();
        case rapidjson::kFalseType:
            return Napi::Boolean::New(env, false);
        case rapidjson::kTrueType:
            return Napi::Boolean::New(env, true);
        case rapidjson::kObjectType: {
            //std::cout << "/{} " << level << std::endl;
            RapidObject f{env, pointer, level, hf("/")};
            return f(value);
        };
        case rapidjson::kArrayType: {
            //std::cout << "/[] " << level << std::endl;
            RapidArray f{env, pointer, level, hf("/")};
            return f(value);
        };
        case rapidjson::kStringType: {
            RapidString f{env};
            return f(value);
        }
        case rapidjson::kNumberType: {
            RapidNumber f{env, match()};
            return f(value);
        }
        default: ;
    }

    return env.Undefined();
}

auto convert(Napi::Env& env, Napi::Array& pointer, std::size_t level = 0) {
    constexpr fnv1a hf;
    constexpr auto hash = hf("#");
    //std::cout << "# " << level << std::endl;
    return RapidConvert{env, pointer, level, hash};
}

} // namespace rapid