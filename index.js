const fs = require("node:fs");

let modulePath = "./build/Release/"
const moduleFile = "node-rapidjson"

if (fs.existsSync(moduleFile)) {
    modulePath = moduleFile;
} else {
    modulePath += moduleFile;
}

const { RapidJSON } = require(modulePath);
module.exports = RapidJSON;
