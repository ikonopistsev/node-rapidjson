const RapidJSON = require("./index.js");
const PapidPointer = RapidJSON.RapidPointer;

const p = ["#/iWillBigInt", "#/someArray/*/additionNumber", "#/someArray/*", "#/someArray/*/someId/*/id", "#/someArray/*/someNumber",  "#/regularNumber", "#/*"];
const pointer = new PapidPointer(p);
console.log(p, 'to', JSON.stringify(pointer));