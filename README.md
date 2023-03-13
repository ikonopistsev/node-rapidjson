# **node-rapidjson**

This module contains [rapidjson](https://github.com/Tencent/rapidjson) bindings.

The library was created to simplify working with BigInt in nodejs.

```js
const RapidJSON = require("@ikonopistsev/node-rapidjson");
const rapidJSON = new RapidJSON();
const bigintValue = BigInt(2600000000000698546n);
const array = rapidJSON.parse(rapidJSON.stringify([bigintValue]));
console.log(array[0])
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
npm i node-rapidjson
```