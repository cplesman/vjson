#include "json.h"

i64 jsonobj::AppendText(const char *key, const char *val) {
    unsigned long vlen = 0;
    while (val[vlen]) vlen++;
    return AppendText(key, val, vlen);
}
i64 jsonobj::AppendText(const char *key, const char *val, const unsigned long len) {
    i64 str;
    long err = jsonstring_Create(&str);
    if(err<0) { return err; }
    jsonstring* sstr = (jsonstring*)g_jsonMem->Lock(str);
    err = sstr->SetString(val, len);
    g_jsonMem->Unlock(str);
    if(err<0){ jsonstring_Delete(str); return err; }
    return AppendObj(key, str); // will free str on failure
}
i64 jsonobj::AppendText(const char *key, i64 val) {
    i64 str;
    long err = jsonstring_Create(&str);
    if(err<0) { return err; }
    jsonstring* sstr = (jsonstring*)g_jsonMem->Lock(str);
    err = sstr->SetString(val);
    g_jsonMem->Unlock(str);
    if(err<0){ jsonstring_Delete(str); return err; }
    return AppendObj(key, str); // will free str on failure
}
long jsonobj::AppendObj(const char *key, i64 val) {
    i64 kp = g_jsonMem->Alloc(sizeof(jsonkeypair));
    if(!kp) { return JSON_ERROR_OUTOFMEMORY; }
    jsonkeypair* kpPtr = (jsonkeypair*)g_jsonMem->Lock(kp);
    long err;
    if((err=kpPtr->Init(key, val))<0){ kpPtr->Free()/*will free key as well*/; g_jsonMem->Unlock(kp); return err; }
    if( (err = addChild(kp))<0/*Resize failed*/ ){ kpPtr->val=0/*clear so free doesn't delete*/;kpPtr->Free()/*will free KEY as well*/; }
    g_jsonMem->Unlock(kp);
    return err;
}
//append empty obj
i64 jsonobj::AppendObj(const char *key) { 
    i64 val; long err = jsonobj_Create(&val);
    if(!val) { return JSON_ERROR_OUTOFMEMORY; }
    err = AppendObj(key,val);
    if(err<0) {jsonobj_Delete(val); return err;}
    return val;
}
//append empty array
i64 jsonobj::AppendArray(const char *key) { 
    i64 val; long err = jsonarray_Create(&val);
    if(!val) { return JSON_ERROR_OUTOFMEMORY; }
    err = AppendObj(key,val);
    if(err<0) {jsonarray_Delete(val); return err;}
    return val;
}
i64 jsonobj::AppendNumber(const char *key, double num) {
    i64 number;
    long err = jsonnumber_Create(&number);
    if(err<0) { return err; }
    jsonnumber* numPtr = (jsonnumber*)g_jsonMem->Lock(number);
    numPtr->num = num;
    g_jsonMem->Unlock(number);
    return AppendObj(key, number); // will free number on failure
}
i64 jsonobj::AppendBoolean(const char *key, bool p_b) {
    i64 boolean;
    long err = jsonboolean_Create(&boolean);
    if(err<0) { return err; }
    jsonboolean* numPtr = (jsonboolean*)g_jsonMem->Lock(boolean);
    numPtr->b = p_b;
    g_jsonMem->Unlock(boolean);
    return AppendObj(key, boolean); // will free boolean on failure
}
i64 jsonobj::AppendNull(const char *key) {
    i64 null;
    long err = jsonnull_Create(&null);
    if(err<0) { return err; }
    return AppendObj(key, null); // will free null on failure
}
i64 jsonobj::AppendUndefined(const char *key) {
    i64 undefined;
    long err = jsonundefined::Create(&undefined);
    if(err<0) { return err; }
    return AppendObj(key, undefined); // will free undefined on failure
}

i64 jsonarray::AppendText(const char *val) {
    unsigned long vlen = 0;
    while (val[vlen]) vlen++;
    return AppendText(val, vlen);
}
i64 jsonarray::AppendText(i64 txtLoc) {
    i64 str;
    long err = jsonstring_Create(&str);
    if(err<0) { return err; }
    jsonstring* sstr = (jsonstring*)g_jsonMem->Lock(str);
    err = sstr->SetString(txtLoc);
    g_jsonMem->Unlock(str);
    if(err<0){ jsonstring_Delete(str); return err; }
    return AppendObj(str); // will free str on failure
}
i64 jsonarray::AppendText(const char *val, const unsigned long len) {
    i64 str;
    long err = jsonstring_Create(&str);
    if(err<0) { return err; }
    jsonstring* sstr = (jsonstring*)g_jsonMem->Lock(str);
    err = sstr->SetString(val, len);
    g_jsonMem->Unlock(str);
    if(err<0){ jsonstring_Delete(str); return err; }
    return AppendObj(str); // will free str on failure
}
//append empty obj
i64 jsonarray::AppendObj() { 
    i64 val; long err = jsonobj_Create(&val);
    if(!val) { return JSON_ERROR_OUTOFMEMORY; }
    err = AppendObj(val);
    if(err<0) {jsonobj_Delete(val); return err;}
    return val;
}
i64 jsonarray::AppendArray() { 
    i64 val; long err = jsonarray_Create(&val);
    if(!val) { return JSON_ERROR_OUTOFMEMORY; }
    err = AppendObj(val);
    if(err<0) {jsonarray_Delete(val); return err;}
    return val;
}
long jsonarray::AppendObj(i64 val) {
    if (m_size >= m_totalsize) {
        int err = Resize(m_totalsize ? m_totalsize * 2 : 4);
        if (err < 0) return err;
    }
    i64* dataPtr = (i64*)g_jsonMem->Lock(m_dataLoc);
    dataPtr[m_size] = val;
    g_jsonMem->Unlock(m_dataLoc);
    m_size++;
    return (m_size-1);
}
i64 jsonarray::AppendNumber(double num) {
    i64 number;
    long err = jsonnumber_Create(&number);
    if(err<0) { return err; }
    jsonnumber* numPtr = (jsonnumber*)g_jsonMem->Lock(number);
    numPtr->num = num;
    g_jsonMem->Unlock(number);
    return AppendObj(number); // will free number on failure
}
i64 jsonarray::AppendBoolean(bool p_b) {
    i64 boolean;
    long err = jsonboolean_Create(&boolean);
    if(err<0) { return err; }
    jsonboolean* numPtr = (jsonboolean*)g_jsonMem->Lock(boolean);
    numPtr->b = p_b;
    g_jsonMem->Unlock(boolean);
    return AppendObj(boolean); // will free boolean on failure
}
i64 jsonarray::AppendNull() {
    i64 null;
    long err = jsonnull_Create(&null);
    if(err<0) { return err; }
    return AppendObj(null); // will free null on failure
}
i64 jsonarray::AppendUndefined() {
    i64 undefined;
    long err = jsonundefined::Create(&undefined);
    if(err<0) { return err; }
    return AppendObj(undefined); // will free undefined on failure
}
