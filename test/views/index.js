

module.exports = {
    htmlheader:`<!DOCTYPE html>
        <html lang="en">
            <head>
                <title>VJSON Object</title>
            </head>
            <body>`,
    htmlfooter: `</body></html>`,
    init:function(db){
        this.db = db;
    },
    expressfunc:function(req,res,next){
        try{
            let objpath = req.url.slice(4);
            let obj = this.db.Read(objpath);
            let html = this.htmlheader + '<table>';
            let keys = Object.keys(obj);
            for(let i=0;i<keys.length;i++){
                let type = typeof(obj[keys[i]]);
                switch(type){
                    case 'object': type = `<a href="${(req.url+'/'+keys[i])}">object</a>`; break;
                    default: break; //type = type
                }
                html += '<tr><td>' + keys[i] + '</td><td>' + type + '</td></tr>';
            }
            html += '</table>';
            html += this.htmlfooter;
            res.status(200).send(html);
            //res.render('object',{keys:Object.keys(obj),obj:JSON.stringify(obj)});
        }
        catch(e){
            next();
        }
    }      
}