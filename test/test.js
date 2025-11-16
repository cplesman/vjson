let vjson = require("./wrap.js");

let MemManager = new vjson("vmemtest");
console.log("free bytes",MemManager.CalculateFree());

let root = MemManager.Read( "/");
console.log(root);

if(root.testObj==null){
    root.testObj = {
        "text":"test text",
        "numkey":1234567
    }
    let key = MemManager.Append("/", "testObj", root.testObj);
    console.log("added","key");
}

let root2 = MemManager.Read( "/");
console.log(root2);

MemManager.Close();

console.log("hello");

