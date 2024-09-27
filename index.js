// Импортируем ваш N-API модуль
const nativeModule = require('./build/Release/node-rapidjson.node');
// Добавляем JavaScript класс к экспортам N-API модуля
nativeModule.FNV1a = require('./fnv1a.js');
// массив уровней поиска
// описание поинтеров["#/iWillBigInt", "#/someArray/*/someId/*/id", "#/someArray/*/someNumber",  "#/regularNumber"];
// мы конвертируем в массивы хэшей по уровням
// [
//     [ fnva("#/iWillBigInt"), fnva("#/regularNumber") ],
//     [ ],
//     [ fnva("#/someArray/*/someNumber") ],
//     [ ],
//     [ fnva("#/someArray/*/someId/*/id") ]
// ]

class RapidPointer {
    constructor(items) {
        this.pointer = this.parsePointer(items);
    }

    parsePointer(items) {
        if (!Array.isArray(items)) {
            throw new Error("pointer must be an array");
        }
        const result = [];
        // переводим в объект где ключи это поинтер а значения это хэш
        const hf = new nativeModule.FNV1a();
        // хэши поинтеров
        const hashes = items.map(item => hf.hash(item));
        // обходим поинтеры и преобразовываем их в массивы хэшей
        items.forEach((item, index) => {
            // разбиваем на уровни
            const levels = item.split("/");
            // сколько элементов в массиве
            // на этом уровне он должен быть представлен
            // если на уровне нет элементов то добавляем пустой массив
            // сохраняем уровень на котором находится хэш
            const l = levels.length - 1;
            for (let i = 0; i < levels.length; i++) {
                if (result[i] === undefined) {
                    result[i] = [];
                }
                if (i === l) {
                    result[i].push(hashes[index]);
                }
            }

        });
        // нужно отсортировать эелменты на каждом уровне rc
        for (let i = 0; i < result.length; i++) {
            result[i].sort((a, b) => a - b);

        }
        return result;
    }
}

nativeModule.RapidPointer = RapidPointer
nativeModule.makeRapidPointer = (items) => new RapidPointer(items);

class RapidParser {   
    constructor(memorySize) {
        this.document = new nativeModule.Document(memorySize);
    }

    parse(json, pointer) {
        if (typeof json === "string") {
            json = Buffer.from(json);
        }
        const { document } = this;
        if (!document.parse(json)) {
            throw new Error(`${document.parseMessage()} offset:${document.parseOffset()}`);
        }
        return (pointer && (pointer instanceof RapidPointer)) ?
            document.getResult(pointer) :
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
