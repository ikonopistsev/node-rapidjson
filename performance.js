const RapidJSON = require("./index.js");
const RapidDocument = RapidJSON.Document;
const RapidSchema = RapidJSON.Schema;
const document = new RapidDocument(16*1024);
const makeRapidPointer = RapidJSON.makeRapidPointer;
const RapidParser = RapidJSON.RapidParser;
const rapidParser = new RapidParser();
// const Ajv = require("ajv");

const jsonSchama = {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "properties": {
        "iWillNumber": {
            "type": "number"
        },
        "someArray": {
            "type": "array",
            "minItems": 1,
            "maxItems": 5,
            "items": {
                "type": "object",
                "required": ["someId", "someNumber"],
                "additionalProperties": true
            }
        }
    },
    "required": ["iWillNumber", "someArray", "regularNumber", "iWillBigInt"],
    "additionalProperties": true
};
const rapidSchema = new RapidSchema();
rapidSchema.parse(Buffer.from(JSON.stringify(jsonSchama)));
// const ajv = new Ajv();
// const ajValidate = ajv.compile(jsonSchama)

const testData = [];
const textData = [];
const testDataSize = 300;
for (let i = 0; i < testDataSize; ++i) {
    const json = JSON.stringify({
        regularNumber: BigInt(42 + i),
        iWillBigInt: BigInt(9223372036854775801n),
        iWillNumber: 720 + i,
        someArray: [
            { someId: BigInt(9223372036854775801n), someNumber: BigInt(9223372036854775801n) },
            { someId: BigInt(2600000000000698546n), someNumber: BigInt(9223372036854775801n) },
            { someId: BigInt(9223372036854775801n), someNumber: BigInt(9223372036854775801n) },
            { someId: BigInt(2600000000000698546n), someNumber: BigInt(9223372036854775801n) }
        ],
    }, (_, value) => {
        return typeof value === "bigint" ? JSON.rawJSON(value) : value;
    });
    textData.push(json);
    testData.push(Buffer.from(json));
};

let count = 1000000;
let j = BigInt(0);
const rapidPointer = makeRapidPointer(["#/iWillBigInt", "#/someArray/*/someId", "#/regularNumber"]);
//const rapidPointer = makeRapidPointer(["#/regularNumber"]);

let t = new Date();

for (let i = 0; i < count; ++i) {
    if (!document.parse(testData[i % testDataSize])) {
        throw new Error(`document: ${document.parseMessage()} offset:${document.parseOffset()}`);
    }
    if (!rapidSchema.validate(document)) {
        throw new Error(`rapidSchema: ${rapidSchema.validateKeyword()} (${rapidSchema.documentPointer()})`);
    }
    const rc = document.get(rapidPointer);
    j += rc.regularNumber;
}

console.log("rapidjson", (new Date() - t) / 1000.0, "ms");

let k = BigInt(0);

t = new Date();

for (let i = 0; i < count; ++i) {
    const rc = JSON.parse(textData[i % testDataSize], (key, value, context) => {
        switch (key) {
            case "iWillBigInt":
                 return BigInt(context.source);
            case "someId":
                 return BigInt(context.source);
            case "regularNumber":
                return BigInt(context.source);    
        }
        return value;
      });
    // if (!ajValidate(rc)) {
    //     throw new Error(`ajv: ${ajv.errorsText(ajValidate.errors)}`);
    // }
    k += rc.regularNumber;
}

console.log("JSON", (new Date() - t) / 1000.0, "ms");

function field64(input, fields) {
    let output = input;
    for (let x of fields) {
        let s = '"' + x + '"\\s*:\\s*(\\d+)';
        output = output.replace(new RegExp(s, "g"), '"' + x + '":"$1"');
    }
    return output;
}

function parse64(val, fields) {
    if (fields) {
        const f = field64(val, fields);
        return JSON.parse(f, (key, val) => {
            return (fields.indexOf(key) != -1) ? BigInt(val) : val;
        });
    }
    ;
    return JSON.parse(val);
}


const fields = [ "iWillBigInt", "someId", "regularNumber" ];
//const fields = [ "regularNumber" ];

let r = BigInt(0);
t = new Date();

for (let i = 0; i < count; ++i) {
    const rc = parse64(textData[i % testDataSize], fields);
    r += rc.regularNumber;
}

console.log("RegExp", (new Date() - t) / 1000.0, "ms");
console.log(k, j, r);

count = 10000000;
let d = Number(0);
t = new Date();

for (let i = 0; i < count; ++i) {
    const rc = JSON.parse(textData[i % testDataSize]);
    d += rc.iWillNumber;
}

console.log("js", (new Date() - t) / 1000.0, "ms");

let v = Number(0);
t = new Date();

for (let i = 0; i < count; ++i) {
    const rc = rapidParser.parse(testData[i % testDataSize]);
    v += rc.iWillNumber;
}

console.log("rapid", (new Date() - t) / 1000.0, "ms");
console.log(d, v);