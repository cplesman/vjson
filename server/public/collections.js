const body = document.getElementById('collectionsBody');
const statusEl = document.getElementById('status');
const refreshBtn = document.getElementById('refreshBtn');

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

function getSize(value) {
    if (Array.isArray(value)) {
        return value.length;
    }
    if (value && typeof value === 'object') {
        return Object.keys(value).length;
    }
    return '-';
}

function getPreview(value) {
    try {
        const text = JSON.stringify(value);
        if (!text) {
            return '';
        }
        return text.length > 110 ? `${text.slice(0, 110)}...` : text;
    } catch (error) {
        return String(value);
    }
}

function rowTemplate(name, value) {
    const type = getType(value);
    const size = getSize(value);
    const preview = getPreview(value);
        const encodedPath = encodeURIComponent(`/${name}`);

    return `
      <tr>
                <td class="fw-semibold mono"><a href="/collection.html?path=${encodedPath}">${name}</a></td>
        <td>${type}</td>
        <td>${size}</td>
        <td class="mono small">${preview}</td>
      </tr>
    `;
}

async function loadCollections() {
    setStatus('Loading collections from read(path=/, depth=1)...');
    body.innerHTML = '';

    try {
        const response = await fetch('/api/read?path=%2F&depth=1');
        const result = await response.json();

        if (!response.ok || !result.ok) {
            throw new Error(result.error || 'Failed to load collections');
        }

        const root = result.data || {};
        const keys = Object.keys(root);

        const collectionKeys = keys.filter((key) => {
            const value = root[key];
            return Array.isArray(value) || (value && typeof value === 'object');
        });

        if (collectionKeys.length === 0) {
            body.innerHTML = '<tr><td colspan="4" class="text-center text-secondary py-4">No collections found at root.</td></tr>';
            setStatus('Loaded successfully. No collections found.');
            return;
        }

        body.innerHTML = collectionKeys.map((key) => rowTemplate(key, root[key])).join('');
        setStatus(`Loaded ${collectionKeys.length} collection(s).`);
    } catch (error) {
        setStatus(error.message, true);
        body.innerHTML = '<tr><td colspan="4" class="text-center text-danger py-4">Failed to load collections.</td></tr>';
    }
}

refreshBtn.addEventListener('click', loadCollections);
loadCollections();
