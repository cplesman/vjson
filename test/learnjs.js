

let obj = {
    "key1": "value1",
    "key2": 12345,
    "key3": {
        "subkey1": "subvalue1",
        "subkey2": [1,2,3,4,5]
    }
}

function lookAtObj(o){
    console.log(obj["key2"]);
    console.log(obj.key2);
}

lookAtObj(obj);