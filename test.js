const RapidJSON = require("./index.js");
const RapidParser = RapidJSON.RapidParser;

// DEMO1

const JSONR = new RapidParser();
const example1 = {
    "hello": "world",
    "t": true ,
    "f": false,
    "n": null,
    "i": 123,
    "pi": 3.1416,
    "a": [1, 2, 3, 4]
};

const example1text = JSONR.stringify(example1);
console.log(example1text);
const example1parsed = JSONR.parse(example1text);
console.log(example1parsed)

// DEMO2

const RapidDocument = RapidJSON.Document;
const RapidSchema = RapidJSON.Schema;
const schema = new RapidSchema();
const document = new RapidDocument();

const schemaExample = Buffer.from(JSON.stringify({
    type: "object",
    required: ["numbers"],
    properties: {
      numbers: {
        type: "array",
        minItems: 5,
        items: { types: "number", format: "bigint" }
      }    
    }
}));

if (!schema.parse(schemaExample)) {
    throw new Error(`schema: ${schema.parseMessage()} offset:${schema.parseOffset()}`);
}

const example2 = Buffer.from(JSON.stringify({
    numbers: [1, 2, 3]
}));

if (!document.parse(example2)) {
    throw new Error(`document: ${document.parseMessage()} offset:${document.parseOffset()}`);
}

if (!schema.validate(document)) {
    console.log(`check: ${schema.validateKeyword()} (${schema.documentPointer()})`);
} else {
    console.log("schema validate ok");
}
  
const example3 = Buffer.from(JSON.stringify({
    number: [1, 2, 3]
}));

if (!document.parse(example3)) {
    throw new Error(`document: ${document.parseMessage()} offset:${document.parseOffset()}`);
}

if (!schema.validate(document)) {
    console.log(`check: ${schema.validateKeyword()} (${schema.documentPointer()})`);
} else {
    console.log("schema validate ok");
}

const example4 = Buffer.from(JSON.stringify({
    numbers: [1, 2, 3, 4, 5, 6]
}));

if (!document.parse(example4)) {
    throw new Error(`document: ${document.parseMessage()} offset:${document.parseOffset()}`);
} else {
    console.log("schema validate ok");
}

if (!schema.validate(document)) {
    console.log(`check: ${schema.validateKeyword()} (${schema.documentPointer()})`);
}

// DEMO3

const makeRapidPointer = RapidJSON.makeRapidPointer;

const example5 = Buffer.from(JSONR.stringify({
    regularNumber: 42,
    iWillBigInt: BigInt(9223372036854775801n),
    iWillNumber: BigInt(9223372036854775801n),
    someArray: [
        { someId: BigInt(9223372036854775801n), someNumber: BigInt(9223372036854775801n) },
        { someId: BigInt(2600000000000698546n), someNumber: BigInt(9223372036854775801n) },
        { someId: BigInt(9223372036854775801n), someNumber: BigInt(9223372036854775801n) },
        { someId: BigInt(2600000000000698546n), someNumber: BigInt(9223372036854775801n) }
    ],
}));

// only obj.bigInt and someId will be BigInt
const pointer = makeRapidPointer(["#/iWillBigInt", "#/someArray/*/someId"]);
console.log(JSONR.parse(example5, pointer));
  
// const RapidJSON = require("@ikonopistsev/node-rapidjson");
// const RapidParser = RapidJSON.RapidParser;
// const makeRapidPointer = RapidJSON.makeRapidPointer;
// const JSONR = new RapidParser();
// const bigintValue = BigInt(2600000000000698546n);
// // parsed as BigInt
// const array1 = JSONR.parse(JSONR.stringify([bigintValue]), makeRapidPointer(["#/*"]));
// console.log(array1[0]);
// // parsed as Number
// const array2 = JSONR.parse(JSONR.stringify([bigintValue]));
// console.log(array2[0]);


















// const arrayPointer = makeRapidPointer(["#/*"]);
// const bigintValue = BigInt(2600000000000698546n);
// const array = JSONMOU.parse(JSONMOU.stringify([bigintValue]), arrayPointer);
// console.log(array[0]);

// console.log(JSONMOU.stringify([0.0, 5, 4.9999, -3.00001, -0.23234234e-32, Number.MAX_SAFE_INTEGER, 
//         -1.0000000000000002, 2600000000000698546n, -2600000000000698546n]));
// console.log(JSONMOU.stringify([0.0, 5, 4.9999, -3.00001, -0.23234234e-32, Number.MAX_SAFE_INTEGER, -1.0000000000000002]));

// const int64max = 9007199254740991;
// console.log(JSONMOU.stringify(int64max));
// console.log(JSON.stringify(int64max));

// let json = {
//     "firstName": "Иван",
//     "lastName": "Иванов",
//     "address": {
//         "streetAddress": "Московское ш., 101, кв.101",
//         "city": "Ленинград",
//         "postalCode": 101101
//     },
//     "phoneNumbers": [
//         "812 123-1234",
//         "916 123-4567"
//     ],
//     someNumbers:[
//         0, 12,34.5,643.00000001, 0.000000312e-2, -Infinity
//     ]
// }

// const t1 = JSONMOU.stringify(JSONMOU.parse(JSONMOU.stringify(json)));
// const t2 = JSONMOU.stringify(JSON.parse(JSONMOU.stringify(json)));
// console.log(t1);
// console.log(t2);
// console.log(t2 == t1);
