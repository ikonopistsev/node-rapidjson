const RapidJSON = require("./index.js");
const rapidJSON = new RapidJSON();
const bigintValue = BigInt(2600000000000698546n);
const array = rapidJSON.parse(rapidJSON.stringify([bigintValue]));
console.log(array[0])

console.log(rapidJSON.stringify([0.0, 5, 4.9999, -3.00001, -0.23234234e-32, Number.MAX_SAFE_INTEGER, -1.0000000000000002, 2600000000000698546n, -2600000000000698546n]));
console.log(JSON.stringify([0.0, 5, 4.9999, -3.00001, -0.23234234e-32, Number.MAX_SAFE_INTEGER, -1.0000000000000002]));

const int64max = 9007199254740991;
console.log(rapidJSON.stringify(int64max));
console.log(JSON.stringify(int64max));

let json = {
    "firstName": "Иван",
    "lastName": "Иванов",
    "address": {
        "streetAddress": "Московское ш., 101, кв.101",
        "city": "Ленинград",
        "postalCode": 101101
    },
    "phoneNumbers": [
        "812 123-1234",
        "916 123-4567"
    ],
    someNumbers:[
        0, 12,34.5,643.00000001, 0.000000312e-2, -Infinity
    ]
}

const t1 = rapidJSON.stringify(json);
const t2 = JSON.stringify(json);
console.log(t1);
console.log(t2);
console.log(t2 == t1);
json.bigNumbers = [2600000000000698546n, -2600000000000698546n, NaN];
const t3 = rapidJSON.stringify(json);
console.log(t3);

rapidJSON.forceBigInt(["id", "time"]);
