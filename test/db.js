let MemManager = global.MemManager;// = require('../src/wrap');

if(!global.MemManager){
    let vjson = require('../src/vjson');
    MemManager = global.MemManager = new vjson("vmemtest");
}

module.exports = MemManager;
