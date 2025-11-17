let MemManager = global.MemManager;// = require('../src/wrap');

if(!global.MemManager){
    let vjson = require('../src/wrap');
    MemManager = global.MemManager = new vjson("vmemtest");
}

module.exports = MemManager;
