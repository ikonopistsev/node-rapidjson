// Импортируем ваш N-API модуль
const nativeModule = require('./build/Release/node-rapidjson.node');
// Добавляем JavaScript класс к экспортам N-API модуля
nativeModule.FNV1a = require('./fnv1a.js');

nativeModule.pointerMap = (items) => {
    const hf = new nativeModule.FNV1a();
    return items.map(id => hf.hash(id)).sort();
};

class RapidPointer {
    constructor(items) {
        this.items = nativeModule.pointerMap(items);
    }
};

nativeModule.RapidPointer = RapidPointer
nativeModule.makeRapidPointer = (items) => new RapidPointer(items);

class RapidParser {   
    constructor() {
        this.document = new nativeModule.Document();
    }

    parse(json, pointer) {
        const { document } = this;
        if (typeof json !== "buffer") {
            json = Buffer.from(json);
        }
        
        document.parse(json);

        if (document.hasParseError()) {
            throw new Error(`${document.parseMessage()} offset:${document.parseOffset()}`);
        }

        return (pointer instanceof RapidPointer) ?
            document.getResult(pointer.items) :
            document.getResult();
    }

    stringify(value) { 
        return JSON.stringify(value, (_, value) => {
            return typeof value === "bigint" ? JSON.rawJSON(value) : value;
        });
    }
}

nativeModule.RapidParser = RapidParser

// Экспортируем объединенный модуль
module.exports = nativeModule;
