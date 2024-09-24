# **node-rapidjson**

This module contains [rapidjson](https://github.com/Tencent/rapidjson) bindings.

The library was created to simplify working with BigInt in nodejs. Support 32-bit/64-bit signed/unsigned integer and double for JSON number type. See rapidjson [features](https://rapidjson.org/md_doc_features.html).

```js
const RapidJSON = require("@ikonopistsev/node-rapidjson");
const RapidParser = RapidJSON.RapidParser;
const makeRapidPointer = RapidJSON.makeRapidPointer;
const JSONR = new RapidParser();
const bigintValue = BigInt(2600000000000698546n);
const rapidPointer = makeRapidPointer(["#/*"]);
// parsed as BigInt // 2600000000000698546n
const array1 = JSONR.parse(JSONR.stringify([bigintValue]), rapidPointer);
console.log(array1[0]);
// parsed as Number // 2600000000000698400
const array2 = JSONR.parse(JSONR.stringify([bigintValue]));
console.log(array2[0]);
```

Supported platforms:
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
