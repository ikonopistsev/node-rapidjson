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
console.log(example1parsed);

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
        items: { types: "number" }
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
const p = ["#/iWillBigInt", "#/someArray/*/someId"];
const pointer = makeRapidPointer(p);
console.log(p, JSON.stringify(pointer));
console.log(JSONR.parse(example5, pointer));

const example6 = Buffer.from(JSONR.stringify([
    BigInt(9223372036854775801n),
    BigInt(2600000000000698546n)
]));
  
const p2 = ["#/*"];
const pointer2 = makeRapidPointer(p2);
console.log(p2, JSON.stringify(pointer2));
console.log(JSONR.parse(example6, pointer2));

const example7 = Buffer.from(JSONR.stringify(BigInt(9223372036854775801n)));
const p3 = ["#"];
const pointer3 = makeRapidPointer(p3);
console.log(p3, JSON.stringify(pointer3));
console.log(JSONR.parse(example7, pointer3));

// const RapidJSON = require("@ikonopistsev/node-rapidjson");
// const RapidParser = RapidJSON.RapidParser;
// const makeRapidPointer = RapidJSON.makeRapidPointer;
// const JSONR = new RapidParser();
// const bigintValue = BigInt(2600000000000698546n);
// const rapidPointer = makeRapidPointer(["#/*"]);
// // parsed as BigInt // 2600000000000698546n
// const array1 = JSONR.parse(JSONR.stringify([bigintValue]), rapidPointer);
// console.log(array1[0]);
// // parsed as Number // 2600000000000698400
// const array2 = JSONR.parse(JSONR.stringify([bigintValue]));
// console.log(array2[0]);
