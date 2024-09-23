// index.mjs
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
// Импортируем N-API модуль
const nativeModule = require('./build/Release/node-rapidjson.node');
// Добавляем JavaScript класс к экспортам N-API модуля
nativeModule.FNV1a = require('./fnv1a.js');
// Экспортируем объединенный модуль
export default nativeModule;