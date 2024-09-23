const RapidJSON = require("./index.js");

// Пример использования:
const fnv1a = new RapidJSON.FNV1a();

const JSG = (value) => {
  return JSON.stringify(value, (_, value) => {
    return typeof value === "bigint" ? JSON.rawJSON(value) : value;
  });
};

const JSP = (json, path) => {
  const doc = new RapidJSON.Document();
  if (typeof json !== "buffer") {
    json = Buffer.from(json);
  }
  doc.parse(json);
  if (doc.hasParseError()) {
    throw new Error(`${doc.parseMessage()} offset:${doc.parseOffset()}`);
  }
  return doc.getResult(path);
};

const doc1 = new RapidJSON.Document();
const doc2 = new RapidJSON.Document();
const schema = new RapidJSON.Schema();

const jsonSchemaText = JSON.stringify({
  type: "object",
  required: ["numbers"],
  properties: {
    numbers: {
      type: "array",
      minItems: 5,
      items: { types: "number", format: "bigint" }
    }    
  }
});

// пример объекта этой схемы
const jsonText = JSON.stringify({
  numbers: [1, 2, 3]
});

const jsonText2 = JSON.stringify({
  numbers1: [1, 2, 3]
});

schema.parse(Buffer.from(jsonSchemaText));
if (schema.hasParseError()) {
  console.log(`schema: ${doc.parseMessage()} offset:${doc.parseOffset()}`);
} else {
  console.log("schema noParseError");
}

doc1.parse(Buffer.from(jsonText));

if (doc1.hasParseError()) {
  console.log(`doc1: ${doc1.parseMessage()} offset:${doc1.parseOffset()}`);
} else {
  console.log("doc noParseError");
}

doc2.parse(Buffer.from(jsonText2));
if (doc2.hasParseError()) {
  console.log(`doc2: ${doc2.parseMessage()} offset:${doc2.parseOffset()}`);
} else {
  console.log("doc noParseError");
}

if (!schema.validate(doc1)) {
  console.log(`check: ${schema.validateKeyword()} (${schema.documentPointer()})`);
}

if (!schema.validate(doc2)) {
  console.log(`check: ${schema.validateKeyword()} (${schema.documentPointer()})`);
}
console.log(JSG(doc1.getResult([ "/numbers/*" ])));

const obj = {
  regularNumber: 42,
  bigInt: BigInt(9223372036854775801n),
  bigNumber: BigInt(9223372036854775801n)
};


console.log("final");
const jsonString = JSG(obj);
const ids = [ "#/bigInt", "#/bigNumber" ];
const idsmap = ids.map(id => fnv1a.hash(id)).sort();
console.log(JSG(JSP(jsonString, null, idsmap)));
