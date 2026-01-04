let nbindings = require('bindings');
let addon = nbindings('vjson.node');

class vjson {
    constructor(db_path){
        this.Open(db_path);
    }
    Open(db_path){
        return (this.db = addon.init(db_path));
    }
    Close(){
        delete this.db;
        this.db = null;
    }

    Flush(){
        addon.flush(this.db);
    }

    Read(obj_path, depth=0x7FFFFFFF){ //default depth is max int
        return addon.read(this.db, obj_path, depth);
    }
    Update(obj_path, obj_id, obj){
        return addon.update(this.db, obj_path, obj_id, obj);
    }

    Append(obj_path, obj_id/*new unique id*/, obj){ //append to obj
        if(obj==undefined){
            obj = obj_id;
            return addon.append(this.db, obj_path, obj);
        }
        return addon.append(this.db, obj_path, obj_id, obj);
    }

    Delete(obj_path,obj_id){
        return addon.delete(this.db, obj_path, obj_id);
    }

    CalculateFree(){
        return addon.calculateFree(this.db);
    }
}

module.exports = vjson;
