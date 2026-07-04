const fs = require('fs');
let MemManager = global.DataServerMemManager;

if (!global.DataServerMemManager) {
    const dbPath = 'dbdata';
    fs.mkdirSync(dbPath, { recursive: true });

    let vjson = require('../src/vjson');
    MemManager = global.DataServerMemManager = new vjson(dbPath);
}

module.exports = MemManager;
