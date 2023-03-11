const RapidJSON = require("./index.js");
const rapidJSON = new RapidJSON();

console.log("\RapidJSON\n");
console.log(rapidJSON.parse(null));
console.log(rapidJSON.parse(1));
console.log(rapidJSON.parse(1.3));
console.log(rapidJSON.parse("[]"));
console.log(rapidJSON.parse("{}"));

const text = "{\"val\":1, \"a\": 60000000000069853, \"b\": 2.2, \"c\": [null, 1, 600000000000698546,3,4,\"5\",[], {\"bignum\":1600000000000698546}]}";
console.log(rapidJSON.parse(text));

console.log("\nJSON\n");
console.log(JSON.parse(null));
console.log(JSON.parse(1));
console.log(JSON.parse(1.3));
console.log(JSON.parse("[]"));
console.log(JSON.parse("{}"));

console.log(JSON.parse(text));


let rc = 0;
const count = 10000000;
for (let i = 0; i < count; ++i) {
    rc += JSON.parse(text).val;
}
console.log(rc);

