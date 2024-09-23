// Импортируем ваш N-API модуль
const nativeModule = require('./build/Release/node-rapidjson.node');
// Добавляем JavaScript класс к экспортам N-API модуля
nativeModule.FNV1a = require('./fnv1a.js');
nativeModule.pointerMap = (items) => {
    const hf = new nativeModule.FNV1a();
    return items.map(id => hf.hash(id)).sort();
};  
// Экспортируем объединенный модуль
module.exports = nativeModule;
