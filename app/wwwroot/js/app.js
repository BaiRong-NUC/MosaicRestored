const elements = {
    fileInput: document.querySelector("#fileInput"),
    dropzone: document.querySelector("#dropzone"),
    fileMeta: document.querySelector("#fileMeta"),
    inputBody: document.querySelector("#inputBody"),
    resultBody: document.querySelector("#resultBody"),
    restoreButton: document.querySelector("#restoreButton"),
    clearButton: document.querySelector("#clearButton"),
    statusText: document.querySelector("#status"),
    downloadLink: document.querySelector("#downloadLink"),
    fidelity: document.querySelector("#fidelity"),
    fidelityValue: document.querySelector("#fidelityValue"),
    upscale: document.querySelector("#upscale"),
    faceUpsample: document.querySelector("#faceUpsample"),
    backgroundEnhance: document.querySelector("#backgroundEnhance"),
};

let selectedFile = null;
let inputUrl = null;
let resultUrl = null;

function setStatus(message, tone = "") {
    elements.statusText.textContent = message;
    elements.statusText.className = `status ${tone}`.trim();
}

function revokeUrl(url) {
    if (url) URL.revokeObjectURL(url);
}

function showImage(container, url, alt) {
    container.classList.remove("busy");
    container.replaceChildren();

    const image = document.createElement("img");
    image.src = url;
    image.alt = alt;
    container.appendChild(image);
}

function showPlaceholder(container, text, tone = "") {
    container.classList.remove("busy");
    container.replaceChildren();

    const placeholder = document.createElement("div");
    placeholder.className = `placeholder ${tone}`.trim();
    placeholder.textContent = text;
    container.appendChild(placeholder);
}

function formatSize(bytes) {
    if (bytes < 1024 * 1024) {
        return `${(bytes / 1024).toFixed(1)} KB`;
    }
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

function resetDownload() {
    elements.downloadLink.hidden = true;
    elements.downloadLink.removeAttribute("href");
}

function setSelectedFile(file) {
    if (!file || !file.type.startsWith("image/")) {
        setStatus("Choose an image file", "warning");
        return;
    }

    revokeUrl(inputUrl);
    revokeUrl(resultUrl);

    selectedFile = file;
    inputUrl = URL.createObjectURL(file);
    resultUrl = null;

    showImage(elements.inputBody, inputUrl, "Input image");
    showPlaceholder(elements.resultBody, "Waiting");
    elements.fileMeta.textContent = `${file.name} - ${formatSize(file.size)}`;
    elements.restoreButton.disabled = false;
    elements.clearButton.disabled = false;
    resetDownload();
    setStatus("Ready");
}

function clearAll() {
    revokeUrl(inputUrl);
    revokeUrl(resultUrl);

    selectedFile = null;
    inputUrl = null;
    resultUrl = null;
    elements.fileInput.value = "";
    elements.fileMeta.textContent = "";
    elements.restoreButton.disabled = true;
    elements.clearButton.disabled = true;
    resetDownload();
    showPlaceholder(elements.inputBody, "No image selected");
    showPlaceholder(elements.resultBody, "Waiting");
    setStatus("Ready");
}

function buildRestoreParams() {
    return new URLSearchParams({
        upscale: elements.upscale.value,
        face_upsample: String(elements.faceUpsample.checked),
        background_enhance: String(elements.backgroundEnhance.checked),
        codeformer_fidelity: elements.fidelity.value,
    });
}

async function restoreImage() {
    if (!selectedFile) return;

    elements.restoreButton.disabled = true;
    elements.resultBody.replaceChildren();
    elements.resultBody.classList.add("busy");
    resetDownload();
    setStatus("Restoring...");

    try {
        const response = await fetch(`/api/restore?${buildRestoreParams()}`, {
            method: "POST",
            headers: {
                "Content-Type": selectedFile.type || "application/octet-stream",
            },
            body: await selectedFile.arrayBuffer(),
        });

        const contentType = response.headers.get("Content-Type") || "";
        if (!response.ok) {
            let message = `Restore failed (${response.status})`;
            if (contentType.includes("application/json")) {
                const payload = await response.json();
                if (payload.detail) message = payload.detail;
            } else {
                const text = await response.text();
                if (text) message = text;
            }
            throw new Error(message);
        }

        const blob = await response.blob();
        revokeUrl(resultUrl);
        resultUrl = URL.createObjectURL(blob);

        showImage(elements.resultBody, resultUrl, "Restored image");
        elements.downloadLink.href = resultUrl;
        elements.downloadLink.hidden = false;
        setStatus("Complete", "success");
    } catch (error) {
        showPlaceholder(
            elements.resultBody,
            error.message || "Restore failed",
            "error",
        );
        setStatus("Restore failed", "error");
    } finally {
        elements.restoreButton.disabled = !selectedFile;
    }
}

function handleDroppedFile(event) {
    event.preventDefault();
    elements.dropzone.classList.remove("dragover");
    setSelectedFile(event.dataTransfer.files[0]);
}

elements.dropzone.addEventListener("click", () => elements.fileInput.click());
elements.dropzone.addEventListener("dragover", (event) => {
    event.preventDefault();
    elements.dropzone.classList.add("dragover");
});
elements.dropzone.addEventListener("dragleave", () => {
    elements.dropzone.classList.remove("dragover");
});
elements.dropzone.addEventListener("drop", handleDroppedFile);
elements.fileInput.addEventListener("change", () => {
    setSelectedFile(elements.fileInput.files[0]);
});
elements.restoreButton.addEventListener("click", restoreImage);
elements.clearButton.addEventListener("click", clearAll);
elements.fidelity.addEventListener("input", () => {
    elements.fidelityValue.textContent = Number(
        elements.fidelity.value,
    ).toFixed(2);
});
