let MemManager = require("./db.js");
let multer = require('multer');
let upload = multer();

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

let root2 = MemManager.Read( "/");
console.log(root2);

process.on('exit', ()=>{
    MemManager.Close();
    console.log("exiting...");
})

const express = require('express');
const app = express();
const port = 3000;

app.use(express.urlencoded());
app.use(express.json());

app.get("/", (res,req)=>{
    req.redirect('/get/');
})

let objectView = require('./views/object.js');
app.get('/get/*getObj', objectView );
app.get('/get/', objectView);

app.post('/post', upload.none(), (req,res,next)=>{
    console.log(req.body);
    try{
        const {objPath,objIsArray} = req.body;
        let keys = Object.keys(req.body);
        if(typeof(objIsArray)=='undefined' || typeof(objPath)=='undefined'){
            throw new Error("invalid form data");
        }
        let obj = objIsArray=='true'?[]:{};
        //todo: it would be better to parse keys on the client side and send a structured object
        for(let i=0;i<keys.length;i++){
            let k = keys[i];
            let lastIdx_ = keys[i].lastIndexOf("_");
            if(lastIdx_<0) continue;
            let field = keys[i].slice(lastIdx_+1);
            let keyname = keys[i].slice(0,lastIdx_);
            if(field=='type'){ //ignore value fields, they will be processed together with type fields
                let type = req.body[keyname+'_type']; if(typeof(type)!='string') throw new Error("missing type for key "+keyname);
                let value = req.body[keyname+'_value']; 
                if(type!='object' && type!='array'){
                    if(typeof(value)!='string') throw new Error("missing value for key "+keyname);
                }
                switch(type){
                    case 'null': obj[keyname]=null; break;
                    case 'undefined': obj[keyname]=undefined; break;
                    case 'string': obj[keyname]=value; break;
                    case 'number': obj[keyname]=Number(value); break;
                    case 'boolean': obj[keyname]=(value=='true'); break;
                    case 'object': obj[keyname]={}; break;
                    case 'array': obj[keyname]=[]; break;
                    default: throw new Error("invalid type "+type+" for key "+keyname);
                }
                MemManager.Update(objPath, keyname, obj[keyname]);
            }
            else if(field!='value') {
                throw new Error("invalid form data field "+field);
            }

        }
        res.status(200).send("successfully updated");
    }
    catch(e){
        res.status(400).send(e.message);
    }
    //res.redirect('/get'+req.body.objpath);
})

app.use(express.static("public"));

// Start the server and listen on the specified port
app.listen(port, () => {
    console.log(`Example app listening on port ${port}`);
});

MemManager.Flush();
setInterval(()=>{
    MemManager.Flush(); 
},1000*60) //real world should be every 4+ hours

