# **node-rapidjson**

This module contains [rapidjson](https://github.com/Tencent/rapidjson) bindings.

The library was created to simplify working with BigInt in nodejs. Support 32-bit/64-bit signed/unsigned integer and double for JSON number type. See rapidjson [features](https://rapidjson.org/md_doc_features.html).

## Example

```js
const RapidJSON = require("@ikonopistsev/node-rapidjson");
const RapidParser = RapidJSON.RapidParser;
const makeRapidPointer = RapidJSON.makeRapidPointer;
const JSONR = new RapidParser();
const bigintValue = BigInt(2600000000000698546n);
const rapidPointer = makeRapidPointer(['#/*']);
// parsed as BigInt // 2600000000000698546n
const array1 = JSONR.parse(JSONR.stringify([bigintValue]), rapidPointer);
console.log(bigintValue, 'as BigInt', array1[0]);
// parsed as Number // 2600000000000698400
const array2 = JSONR.parse(JSONR.stringify([bigintValue]));
console.log(bigintValue, 'as Number', array2[0]);

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
const pointer = makeRapidPointer(['#/iWillBigInt', '#/someArray/*/someId']);
console.log(JSONR.parse(example5, pointer));
```
## Schema example

See rapidjson [schema](https://rapidjson.org/md_doc_schema.html).

```js
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
```

## Supported platforms

- Linux
- Windows

## Requirements

On Linux:
- CMake
- gcc

On Windows:
- [CMake](https://cmake.org/download/)
- [Build tools for Visual Studio 2019](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=BuildTools&rel=16)

## Installation

```bash
npm i @ikonopistsev/node-rapidjson
```
