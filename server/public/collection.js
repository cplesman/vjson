const query = new URLSearchParams(window.location.search);
const collectionPath = query.get('path') || '/';
const pageSize = 20;

const collectionPathEl = document.getElementById('collectionPath');
const body = document.getElementById('objectsBody');
const statusEl = document.getElementById('status');
const pageLabel = document.getElementById('pageLabel');
const prevBtn = document.getElementById('prevBtn');
const nextBtn = document.getElementById('nextBtn');
const refreshBtn = document.getElementById('refreshBtn');
const createKeyInput = document.getElementById('createKey');
const createValueInput = document.getElementById('createValue');
const createBtn = document.getElementById('createBtn');

let page = 1;
let hasNext = false;
let collectionIsArray = false;

function setStatus(message, isError = false) {
    statusEl.className = `alert ${isError ? 'alert-danger' : 'alert-info'}`;
    statusEl.textContent = message;
    statusEl.classList.remove('d-none');
}

function getType(value) {
    if (Array.isArray(value)) {
        return 'array';
    }
    if (value === null) {
        return 'null';
    }
    return typeof value;
}

function getFieldCount(value) {
    if (Array.isArray(value)) {
        return value.length;
    }
    if (value && typeof value === 'object') {
        return Object.keys(value).length;
    }
    return '-';
}

function preview(value) {
    try {
        const text = JSON.stringify(value);
        return text.length > 100 ? `${text.slice(0, 100)}...` : text;
    } catch (error) {
        return String(value);
    }
}

function buildObjectPath(key) {
    if (collectionPath === '/') {
        return `/${key}`;
    }
    const normalized = collectionPath.endsWith('/') ? collectionPath.slice(0, -1) : collectionPath;
    return `${normalized}/${key}`;
}

function rowTemplate(key, value) {
    const objectPath = buildObjectPath(key);
    const href = `/object.html?path=${encodeURIComponent(objectPath)}`;

    return `
      <tr>
        <td class="mono fw-semibold"><a href="${href}">${key}</a></td>
        <td>${getType(value)}</td>
        <td>${getFieldCount(value)}</td>
        <td class="mono small">${preview(value)}</td>
        <td><button class="btn btn-sm btn-outline-danger" data-delete-key="${key}">Delete</button></td>
      </tr>
    `;
}

async function apiCall(url, options) {
    const response = await fetch(url, options);
    const result = await response.json();
    if (!response.ok || !result.ok) {
        throw new Error(result.error || 'Request failed');
    }
    return result;
}

function toDeleteKey(rawKey) {
    if (collectionIsArray) {
        const parsed = Number(rawKey);
        if (Number.isInteger(parsed)) {
            return parsed;
        }
    }
    return rawKey;
}

async function onDeleteObject(key) {
    const yes = window.confirm(`Delete object "${key}"?`);
    if (!yes) {
        return;
    }

    try {
        await apiCall('/api/delete', {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ path: collectionPath, key: toDeleteKey(key) })
        });
        setStatus(`Deleted "${key}".`);
        await loadCollection();
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function onCreateObject() {
    try {
        const value = JSON.parse(createValueInput.value);
        const key = createKeyInput.value.trim();
        const payload = { path: collectionPath, value };
        if (!collectionIsArray && key.length > 0) {
            payload.key = key;
        }

        const result = await apiCall('/api/append', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        });

        createKeyInput.value = '';
        setStatus(`Created object with key "${result.id}".`);
        page = 1;
        await loadCollection();
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function loadCollection() {
    setStatus(`Loading ${collectionPath} (page ${page})...`);
    body.innerHTML = '';
    pageLabel.textContent = `Page ${page}`;
    collectionPathEl.textContent = `Path: ${collectionPath}`;

    try {
        const url = `/api/read?path=${encodeURIComponent(collectionPath)}&depth=1&page=${page}&pageSize=${pageSize}`;
        const response = await fetch(url);
        const result = await response.json();

        if (!response.ok || !result.ok) {
            throw new Error(result.error || 'Failed to load collection');
        }

        const data = result.data;
        let entries;

        if (Array.isArray(data)) {
            collectionIsArray = true;
            entries = data.map((value, index) => [String(index + (page - 1) * pageSize), value]);
        } else if (data && typeof data === 'object') {
            collectionIsArray = false;
            entries = Object.entries(data);
        } else {
            throw new Error('Selected path is not a collection object/array');
        }

        if (entries.length === 0) {
            body.innerHTML = '<tr><td colspan="5" class="text-center text-secondary py-4">No objects on this page.</td></tr>';
        } else {
            body.innerHTML = entries.map(([key, value]) => rowTemplate(key, value)).join('');
        }

        hasNext = entries.length === pageSize;
        prevBtn.disabled = page <= 1;
        nextBtn.disabled = !hasNext;

        setStatus(`Loaded ${entries.length} object(s).`);
    } catch (error) {
        prevBtn.disabled = page <= 1;
        nextBtn.disabled = true;
        setStatus(error.message, true);
        body.innerHTML = '<tr><td colspan="5" class="text-center text-danger py-4">Failed to load collection.</td></tr>';
    }
}

prevBtn.addEventListener('click', () => {
    if (page <= 1) {
        return;
    }
    page -= 1;
    loadCollection();
});

nextBtn.addEventListener('click', () => {
    if (!hasNext) {
        return;
    }
    page += 1;
    loadCollection();
});

refreshBtn.addEventListener('click', loadCollection);
createBtn.addEventListener('click', onCreateObject);
body.addEventListener('click', (event) => {
    const button = event.target.closest('[data-delete-key]');
    if (!button) {
        return;
    }
    const key = button.getAttribute('data-delete-key');
    onDeleteObject(key);
});
loadCollection();
