const express = require('express');
const path = require('path');
const db = require('./server_db');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(express.json({ limit: '2mb' }));
app.use(express.static(path.join(__dirname, 'public')));

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

    if (root.users == null) {
        db.Append('/', 'users', {});
    }

    if (root.products == null) {
        db.Append('/', 'products', {
            demo: {
                name: 'Sample Product',
                price: 19.99,
                inStock: true
            }
        });
    }

    if (root.logs == null) {
        db.Append('/', 'logs', []);
    }

    db.Flush();
}

function getValue(body) {
    // Support either "value" or legacy "obj" payload naming.
    if (Object.prototype.hasOwnProperty.call(body, 'value')) {
        return body.value;
    }
    return body.obj;
}

app.get('/api/read', (req, res) => {
    try {
        const objectPath = req.query.path || '/';
        const depth = req.query.depth ? Number(req.query.depth) : undefined;
        const result = depth == null ? db.Read(objectPath) : db.Read(objectPath, depth);
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

        const depth = req.query.depth ? Number(req.query.depth) : undefined;
        const result = depth == null ? db.Find(collectionPath, query) : db.Find(collectionPath, query, depth);
        res.json({ ok: true, data: result });
    } catch (err) {
        res.status(400).json({ ok: false, error: err.message });
    }
});

app.post('/api/append', (req, res) => {
    try {
        const objectPath = req.body.path;
        const key = req.body.key;
        const value = getValue(req.body);

        if (!objectPath) {
            return res.status(400).json({ ok: false, error: 'Missing required field: path' });
        }

        let id;
        if (typeof key === 'string' && key.length > 0) {
            id = db.Append(objectPath, key, value);
        } else {
            id = db.Append(objectPath, value);
        }

        db.Flush();
        return res.json({ ok: true, id });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.put('/api/update', (req, res) => {
    try {
        const objectPath = req.body.path;
        const key = req.body.key;
        const value = getValue(req.body);

        if (!objectPath || key == null) {
            return res.status(400).json({ ok: false, error: 'Missing required fields: path and key' });
        }

        const updated = db.Update(objectPath, key, value);
        db.Flush();

        return res.json({ ok: true, updated });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
    }
});

app.delete('/api/delete', (req, res) => {
    try {
        const objectPath = req.body.path;
        const key = req.body.key;

        if (!objectPath || key == null) {
            return res.status(400).json({ ok: false, error: 'Missing required fields: path and key' });
        }

        const deleted = db.Delete(objectPath, key);
        db.Flush();

        return res.json({ ok: true, deleted });
    } catch (err) {
        return res.status(400).json({ ok: false, error: err.message });
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
