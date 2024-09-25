const RapidJSON = require("./index.js");
const RapidDocument = RapidJSON.Document;
const document = new RapidDocument(16*1024);
const makeRapidPointer = RapidJSON.makeRapidPointer;

const testData = [];
const textData = [];
const testDataSize = 300;
for (let i = 0; i < testDataSize; ++i) {
    const json = JSON.stringify({
        regularNumber: BigInt(42 + i),
        iWillBigInt: BigInt(9223372036854775801n),
        iWillNumber: BigInt(9223372036854775801n),
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
let k = BigInt(0);

let t = new Date();

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
    k += rc.regularNumber;
}

console.log("JSON", (new Date() - t) / 1000.0, "ms");

const rapidPointer = makeRapidPointer(["#/iWillBigInt", "#/someArray/*/someId", "#/regularNumber"]).level;
let j = BigInt(0);

t = new Date();

for (let i = 0; i < count; ++i) {
    if (!document.parse(testData[i % testDataSize])) {
        throw new Error(`document: ${document.parseMessage()} offset:${document.parseOffset()}`);
    }
    const rc = document.getResult(rapidPointer);
    j += rc.regularNumber;
}

console.log("rapidjson", (new Date() - t) / 1000.0, "ms");

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

let r = BigInt(0);
t = new Date();

for (let i = 0; i < count; ++i) {
    const rc = parse64(textData[i % testDataSize], fields);
    r += rc.regularNumber;
}

console.log("RegExp", (new Date() - t) / 1000.0, "ms");
console.log(k, j, r);
