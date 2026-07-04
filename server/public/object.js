const query = new URLSearchParams(window.location.search);
const objectPath = query.get('path') || '/';

const objectPathEl = document.getElementById('objectPath');
const editor = document.getElementById('editor');
const statusEl = document.getElementById('status');
const reloadBtn = document.getElementById('reloadBtn');
const saveBtn = document.getElementById('saveBtn');

function setStatus(message, isError = false) {
    statusEl.className = `alert ${isError ? 'alert-danger' : 'alert-info'}`;
    statusEl.textContent = message;
    statusEl.classList.remove('d-none');
}

async function loadObject() {
    objectPathEl.textContent = `Path: ${objectPath}`;
    setStatus('Loading object...');

    try {
        const response = await fetch(`/api/read?path=${encodeURIComponent(objectPath)}&depth=10`);
        const result = await response.json();

        if (!response.ok || !result.ok) {
            throw new Error(result.error || 'Failed to load object');
        }

        editor.value = JSON.stringify(result.data, null, 2);
        setStatus('Object loaded.');
    } catch (error) {
        setStatus(error.message, true);
    }
}

async function saveObject() {
    setStatus('Saving object...');

    try {
        const parsed = JSON.parse(editor.value);
        const response = await fetch('/api/replace', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ path: objectPath, value: parsed })
        });

        const result = await response.json();
        if (!response.ok || !result.ok) {
            throw new Error(result.error || 'Failed to save object');
        }

        setStatus('Object saved successfully.');
    } catch (error) {
        setStatus(error.message, true);
    }
}

reloadBtn.addEventListener('click', loadObject);
saveBtn.addEventListener('click', saveObject);
loadObject();
