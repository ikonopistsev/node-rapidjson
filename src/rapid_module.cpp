#include "rapid_schema.hpp"
#include "rapid_document.hpp"

// Инициализация модуля
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    rapid::Document::Init(env, exports);
    rapid::Schema::Init(env, exports);
    return exports;
}

NODE_API_MODULE(RapidJSON, InitAll)
