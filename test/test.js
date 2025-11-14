let addon = require('../build/Release/vjson.node');

let MemManager = addon.init("vmemtest");
console.log("free bytes",addon.calculateFree(MemManager));

console.log("hello");

