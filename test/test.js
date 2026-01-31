let MemManager = require("./db.js");
let multer = require('multer');
let upload = multer();

const MAXUPDATEARRAYINCREMENTSIZE = 10000;

console.log("free bytes",MemManager.CalculateFree());

let root = MemManager.Read( "/");
console.log(root);

if(root.testObj==null){
    root.testObj = {
        "text":"test text",
        "numkey":1234567
    }
    let key = MemManager.Append("/", "testObj", root.testObj);
    console.log("added",key);
}
if(root.testObj2==null){
    root.testObj2 = {
        "text":"test text 2",
        "numkey":1234.567
    }
    let key = MemManager.Append("/", "testObj2", root.testObj2);
    console.log("added",key);
}
if(root.testObj3==null){
    root.testObj3 = {
        "text":"test text 5",
        "numkey":8765.42123
    }
    let key = MemManager.Append("/", "testObj3", root.testObj3);
    console.log("added",key);
}
if(root.testCollection==null){
    root.testCollection = [
        {
            "text":"collection item 1",
            "value":111
        },
        {
            "text":"collection item 2",
            "value":222
        },
        {
            "text":"collection item 3",
            "value":333
        }   
    ];
    let key = MemManager.Append("/", "testCollection", root.testCollection);
    console.log("added",key);
}

console.log("free bytes",MemManager.CalculateFree());

let results = MemManager.Find("/testCollection", "value == 222" , 3);
console.log("find results:",results);

MemManager.Close();
console.log("closed db");
