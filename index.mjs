// index.mjs
import { createRequire } from 'module';
const require = createRequire(import.meta.url);
// Импортируем N-API модуль
const nativeModule = require('./index.js');
// Экспортируем объединенный модуль
export default nativeModule;