// Импортируем ваш N-API модуль
const nativeModule = require('./build/Release/node-rapidjson.node');
// Добавляем JavaScript класс к экспортам N-API модуля
nativeModule.FNV1a = require('./fnv1a.js');
// Экспортируем объединенный модуль
module.exports = nativeModule;
