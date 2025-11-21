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
        
        let obj = {};
        let oldObj = MemManager.Read(objPath);
        let oldIsArray = Array.isArray(oldObj) ? "true" : "false";
        if(oldIsArray!=objIsArray){
            throw new Error("object type mismatch at path "+objPath+", expected "+(oldIsArray=='true'?'array':'object')+" got "+(objIsArray=='true'?'array':'object'));
        }

        //todo: it would be better to parse keys on the client side and send a structured object
        //build new object from form data fields
        //if obj is array, keys will be 0_type and 0_value, 1_type and 1_value, etc
        //if obj is object, keys will be keyname_type and keyname_value
        let maxKey = 0;
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
                isNumber = !isNaN(Number(keyname));
                if(objIsArray=='true' && !isNumber){
                    throw new Error("invalid array index "+keyname);
                }
                if(objIsArray=='true'){
                    maxKey = Math.max(maxKey, Number(keyname)+1);
                }
                //MemManager.Update(objPath, isNumber ? Number(keyname) : keyname, obj[keyname]);
            }
            else if(field!='value') {
                throw new Error("invalid form data field "+field);
            }
        }

        if(objIsArray=='true'){
            //delete array items that were removed
            let i=0;
            if(maxKey-oldObj.length>MAXUPDATEARRAYINCREMENTSIZE){
                //too large of a gap
                throw new Error("array size gap too large, max key "+maxKey+", old length "+oldObj.length);
            }

            for(;i<oldObj.length;i++){
                if(typeof(obj[i])=='undefined'){
                    MemManager.Delete(objPath, i);
                }
                else{
                    MemManager.Update(objPath, i, obj[i]);
                }
            }
            //append new items:
            for(;i<maxKey;i++){
                MemManager.Append(objPath, obj[i]);
            }
        }
        else{
            //delete keys that were removed
            let oldKeys = Object.keys(oldObj);
            for(let i=0;i<oldKeys.length;i++){
                let k = oldKeys[i];
                if(typeof(obj[k])=='undefined'){
                    MemManager.Delete(objPath, k);
                }
            }
            let newKeys = Object.keys(obj);
            for(let i=0;i<newKeys.length;i++){
                let k = newKeys[i];
                MemManager.Update(objPath, k, obj[k]);
            }
        }
        res.status(200).send({error:false,freeMem:MemManager.CalculateFree()});
    }
    catch(e){
        res.status(400).send({error:e.message});
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

