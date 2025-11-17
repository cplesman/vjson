
let read = require('fs').readFileSync;
let join = require('path').join;
let db = require('../db');
let ejs = require('ejs');
let str = read(join(__dirname,'./object.ejs'),'utf8');


let g_htmlheader = 
`<!DOCTYPE html>
<html lang="en">
    <head>
        <title>VJSON Object</title>
    </head>
    <body>`;
let g_htmlfooter = 
`   </body>
</html>`;
let ejstemplate = ejs.compile(str);

module.exports = async function(req,res,next){
    try{
        let objpath = req.url.slice(4);
        let obj = db.Read(objpath);
        // let html = g_htmlheader + '<table>';
        // let keys = Object.keys(obj);
        // for(let i=0;i<keys.length;i++){
        //     let type = typeof(obj[keys[i]]);
        //     switch(type){
        //         case 'object': type = `<a href="${(req.url+'/'+keys[i])}">object</a>`; break;
        //         default: break; //type = type
        //     }
        //     html += '<tr><td>' + keys[i] + '</td><td>' + type + '</td></tr>';
        // }
        // html += '</table>';
        res.status(200).send(ejstemplate({obj,keyname:objpath}));
        //res.render('object',{keys:Object.keys(obj),obj:JSON.stringify(obj)});
    }
    catch(e){
        res.status(200).send(e.message);
    }
       
 }