#include "rapid_basic_document.hpp"

namespace rapid {

std::size_t getSizeDefault(const Napi::CallbackInfo& i)
{
    int size = 0;
    auto env = i.Env();
    if (i.Length()) 
    {
        auto& arg0 = i[0];
        if (arg0.IsNumber()) {
            size = arg0.As<Napi::Number>().Int32Value();
        }
    }
    constexpr static auto sizeDefault = std::size_t{4 * 1024u};
    return (size < sizeDefault) ? sizeDefault : static_cast<std::size_t>(size);
}

void BasicDocument::create(std::size_t chunkSize)
{   
    chunk_.resize(chunkSize);
    // создаем аллокатор
    mem_.reset(new PoolAllocatorType{chunk_.data(), chunk_.size()});
    // создаем документ
    self_.reset(new rapidjson::Document{mem_.get()});
}

bool BasicDocument::parse(const char* json, std::size_t size)
{
    mem_->Clear();
    // парсим json
    self_->Parse(json, size);
    // возвращаем результат парсинга
    return !self_->HasParseError();
}

} // namespace rapid