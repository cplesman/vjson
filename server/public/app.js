const output = document.getElementById('output');
const statusBar = document.getElementById('statusBar');

function showResult(data, isError = false) {
    output.textContent = JSON.stringify(data, null, 2);
    statusBar.className = `alert ${isError ? 'alert-danger' : 'alert-success'} mt-3`;
    statusBar.textContent = isError ? 'Request failed.' : 'Request completed.';
    statusBar.classList.remove('d-none');
}

function parseJson(text, fallback) {
    if (!text || text.trim() === '') {
        return fallback;
    }
    return JSON.parse(text);
}

async function callApi(url, options) {
    const response = await fetch(url, options);
    const data = await response.json();
    if (!response.ok || !data.ok) {
        throw new Error(data.error || 'Unknown error');
    }
    return data;
}

document.getElementById('readForm').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
        const path = document.getElementById('readPath').value || '/';
        const data = await callApi(`/api/read?path=${encodeURIComponent(path)}`);
        showResult(data);
    } catch (error) {
        showResult({ error: error.message }, true);
    }
});

document.getElementById('findForm').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
        const path = document.getElementById('findPath').value;
        const query = document.getElementById('findQuery').value;
        const data = await callApi(`/api/find?path=${encodeURIComponent(path)}&query=${encodeURIComponent(query)}`);
        showResult(data);
    } catch (error) {
        showResult({ error: error.message }, true);
    }
});

document.getElementById('appendForm').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
        const body = {
            path: document.getElementById('appendPath').value,
            key: document.getElementById('appendKey').value || undefined,
            value: parseJson(document.getElementById('appendValue').value, {})
        };

        const data = await callApi('/api/append', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        showResult(data);
    } catch (error) {
        showResult({ error: error.message }, true);
    }
});

document.getElementById('updateForm').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
        const body = {
            path: document.getElementById('updatePath').value,
            key: document.getElementById('updateKey').value,
            value: parseJson(document.getElementById('updateValue').value, null)
        };

        const data = await callApi('/api/update', {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        showResult(data);
    } catch (error) {
        showResult({ error: error.message }, true);
    }
});

document.getElementById('deleteForm').addEventListener('submit', async (event) => {
    event.preventDefault();
    try {
        const body = {
            path: document.getElementById('deletePath').value,
            key: document.getElementById('deleteKey').value
        };

        const data = await callApi('/api/delete', {
            method: 'DELETE',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(body)
        });
        showResult(data);
    } catch (error) {
        showResult({ error: error.message }, true);
    }
});
