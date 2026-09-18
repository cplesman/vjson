const express = require('express');
const path = require('path');
const db = require('./server_db');

const app = express();
const PORT = 3100;

app.use(express.json({ limit: '100mb', strict: false }));
app.use(express.static(path.join(__dirname, 'public')));

app.use((err, req, res, next) => {
    if (err && err.type === 'entity.parse.failed') {
        return res.status(400).json({ ok: false, error: 'Invalid JSON request body' });
    }
    return next(err);
});

function initDatabase() {
    let root;
    try {
        root = db.Read('/');
    } catch (err) {
        root = {};
    }

    if (!root || typeof root !== 'object') {
        root = {};
    }

    if (root.users == undefined) {
        db.Append('/', 'users', {});
    }

    if (root.products == undefined) {
        db.Append('/', 'products', {
            demo: {
                name: 'Sample Product',
                price: 19.99,
                inStock: true
            }
        });
    }

    if (root.logs == undefined) {
        db.Append('/', 'logs', []);
    }

}

function getValue(body) {
    // Support either "value" or legacy "obj" payload naming.
    if (Object.prototype.hasOwnProperty.call(body, 'value')) {
        return body.value;
    }
    return body.obj;
}

function requireObjectBody(req) {
    const body = req.body;
    if (!body || typeof body !== 'object' || Array.isArray(body)) {
        throw new Error('Request body must be a JSON object');
    }
    return body;
}

function toOptionalInt(value) {
    if (value == undefined || value === '') {
        return undefined;
    }
    const n = Number(value);
    if (!Number.isInteger(n) || n < 1) {
        throw new Error('Expected a positive integer');
    }
    return n;
}

function splitParentAndKey(objectPath) {
    if (!objectPath || objectPath === '/') {
        throw new Error('Path must not be root');
    }

    let normalized = objectPath;
    if (normalized.length > 1 && normalized.endsWith('/')) {
        normalized = normalized.slice(0, -1);
    }

    const segments = normalized.split('/').filter(Boolean);
    if (segments.length === 0) {
        throw new Error('Invalid path');
    }

    const key = segments.pop();
    const parentPath = segments.length ? `/${segments.join('/')}` : '/';
    return { parentPath, key };
}

app.get('/api/read', (req, res) => {
    try {
        const objectPath = req.query.path || '/';
        const depth = toOptionalInt(req.query.depth);
        const page = toOptionalInt(req.query.page);
        const pageSize = toOptionalInt(req.query.pageSize);

        let result;
        if (depth == undefined && page == undefined && pageSize == undefined) {
            result = db.Read(objectPath);
        } else {
            result = db.Read(
                objectPath,
                depth == undefined ? 0x7FFFFFFF : depth,
                page == undefined ? 1 : page,
                pageSize == undefined ? 0x7FFFFFFF : pageSize
            );
        }

        res.json({ ok: true, data: result });
    } catch (err) {
        res.status(400).json({ ok: false, error: err.message });
    }
});

app.get('/api/find', (req, res) => {
    try {
        const collectionPath = req.query.path || '/';
        const query = req.query.query;

        if (!query) {
            return res.status(400).json({ ok: false, error: 'Missing required query parameter: query' });
        }

        const depth = toOptionalInt(req.query.depth);
        const result = depth == undefined ? db.Find(collectionPath, query) : db.Find(collectionPath, query, depth);
        res.json({ ok: true, data: result });
    } catch (err) {
        res.status(400).json({ ok: false, error: err.message });
    }
});

app.post('/api/flush', (req, res) => {
    try {
        db.Flush();
        res.json({ ok: true });
    } catch (err) {
        res.status(400).json({ ok: false, error: err.message });
    }
});

app.post('/api/batch', (req, res) => {
    try {
        const body = requireObjectBody(req);
        const batch = body.batch;
        if (!Array.isArray(batch)) {
            return res.status(400).json({ ok: false, error: 'Missing required field: batch (array)' });
        }
        // Process the batch
        const results = [];
        for (const op of batch) {
            try {
                const { operation, path, key, obj } = op;
                const value = getValue(op);
                let result;
                switch (operation) {
                    case 'read':
                        result = db.Read(path);
                        break;
                    case 'append':
                        result = typeof key === 'string' && key.length > 0 ? db.Append(path, key, value) : db.Append(path, value);
                        break;
                    case 'update':
                        result = db.Update(path, key, value);
                        break;
                    case 'delete':
                        result = db.Delete(path, key);
                        break;
                    default:
                        throw new Error(`Unsupported operation type: ${operation}`);
                }
                results.push({ ok: true, result });
            } catch (err) {
                results.push({ ok: false, error: err.message });
            }
        }
        return res.json({ ok: true, results });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.post('/api/append', (req, res) => {
    try {
        const body = requireObjectBody(req);
        const objectPath = body.path;
        const key = body.key;
        const value = getValue(body);

        if (!objectPath) {
            return res.status(400).json({ ok: false, error: 'Missing required field: path' });
        }

        let id;
        if (typeof key === 'string' && key.length > 0) {
            id = db.Append(objectPath, key, value);
        } else {
            id = db.Append(objectPath, value);
        }

        return res.json({ ok: true, id });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.put('/api/update', (req, res) => {
    try {
        const body = requireObjectBody(req);
        let objectPath = body.path;
        let key = body.key;
        const value = getValue(body);

        if(key===undefined || key===null){
            let split = splitParentAndKey(objectPath);
            objectPath = split.parentPath;
            key = split.key;
        }

        if (!objectPath || key == undefined) {
            return res.status(400).json({ ok: false, error: 'Missing required fields: path and key' });
        }

        const updated = db.Update(objectPath, key, value);

        return res.json({ ok: true, updated });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.put('/api/replace', (req, res) => {
    try {
        const body = requireObjectBody(req);
        const objectPath = body.path;
        const value = getValue(body);

        if (!objectPath) {
            return res.status(400).json({ ok: false, error: 'Missing required field: path' });
        }

        const { parentPath, key } = splitParentAndKey(objectPath);
        const updated = db.Update(parentPath, key, value);
        return res.json({ ok: true, updated });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.delete('/api/delete', (req, res) => {
    try {
        const body = requireObjectBody(req);
        const objectPath = body.path;
        const key = body.key;

        if (!objectPath || !key) {
            return res.status(400).json({ ok: false, error: 'Missing required fields: path and key' });
        }

        const deleted = db.Delete(objectPath, key);

        return res.json({ ok: true, deleted });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.get('/api/info', (req, res) => {
    try {
        const objectPath = req.query.path || '/';

        let result;
        result = db.Info(objectPath);

        res.json({ ok: true, data: result });
    } catch (err) {
        res.status(400).json({ ok: false, error: err.message });
    }
});

app.get('/api/health', (req, res) => {
    res.json({ ok: true, freeBytes: db.CalculateFree() });
});

app.use((req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

initDatabase();

const server = app.listen(PORT, () => {
    console.log(`Data server running at http://localhost:${PORT}`);
});

function shutdown() {
    server.close(() => {
        try {
            db.Close();
        } catch (err) {
            // ignore shutdown errors
        }
        process.exit(0);
    });
}

process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);
