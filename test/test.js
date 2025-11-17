let MemManager = require("./db.js");
let path =require("path");

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
// if(root.testObj3==null){
//     root.testObj3 = {
//         "text":"test text 5",
//         "numkey":8765.42123
//     }
//     let key = MemManager.Append("/", "testObj3", root.testObj3);
//     console.log("added",key);
// }

let root2 = MemManager.Read( "/");
console.log(root2);

process.on('exit', ()=>{
    MemManager.Close();
    console.log("exiting...");
})

const express = require('express');
const app = express();
const port = 3000;

app.use(express.json());
app.use(express.urlencoded());

let objectView = require('./views/object.js');
app.get('/get/*getObj', objectView );
app.get('/get/', objectView);

app.post('/post', (req,res,next)=>{
    console.log(req.body);
    res.redirect('/get'+req.body.objpath);
})

app.use(express.static("public"));

// Start the server and listen on the specified port
app.listen(port, () => {
    console.log(`Example app listening on port ${port}`);
});

