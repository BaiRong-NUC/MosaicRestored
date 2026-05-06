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

function setImageAspect(url) {
    const image = new Image();
    image.onload = () => {
        if (!image.naturalWidth || !image.naturalHeight) return;

        const aspect = `${image.naturalWidth} / ${image.naturalHeight}`;
        elements.inputBody.style.setProperty("--image-aspect", aspect);
        elements.resultBody.style.setProperty("--image-aspect", aspect);
    };
    image.src = url;
}

function createImageFrame(url, alt) {
    const frame = document.createElement("div");
    frame.className = "image-frame";

    const image = document.createElement("img");
    image.src = url;
    image.alt = alt;
    frame.appendChild(image);

    return frame;
}

function showInputImage(url) {
    elements.inputBody.classList.remove("busy");
    elements.inputBody.replaceChildren(createImageFrame(url, "Input image"));
}

function showPlaceholder(container, text, tone = "") {
    container.classList.remove("busy");
    container.replaceChildren();

    const placeholder = document.createElement("div");
    placeholder.className = `placeholder ${tone}`.trim();
    placeholder.textContent = text;
    container.appendChild(placeholder);
}

function updateComparePosition(frame, position) {
    const safePosition = Math.min(98, Math.max(2, position));
    const beforeImage = frame.querySelector(".compare-before .compare-image");

    frame.style.setProperty("--compare-position", `${safePosition}%`);
    frame.style.setProperty("--compare-position-number", String(safePosition));
    frame.setAttribute("aria-valuenow", String(Math.round(safePosition)));

    if (beforeImage) {
        beforeImage.style.width = `${10000 / safePosition}%`;
    }
}

function updateCompareFromPointer(frame, event) {
    const bounds = frame.getBoundingClientRect();
    const position = ((event.clientX - bounds.left) / bounds.width) * 100;
    updateComparePosition(frame, position);
}

function createCompareLabel(text, className) {
    const label = document.createElement("span");
    label.className = `compare-label ${className}`;
    label.textContent = text;
    return label;
}

function createCompareFrame(beforeUrl, afterUrl) {
    const frame = document.createElement("div");
    frame.className = "compare-frame";
    frame.setAttribute("role", "slider");
    frame.setAttribute("tabindex", "0");
    frame.setAttribute("aria-label", "Before and after comparison");
    frame.setAttribute("aria-valuemin", "0");
    frame.setAttribute("aria-valuemax", "100");

    const afterLayer = document.createElement("div");
    afterLayer.className = "compare-layer compare-after";
    const afterImage = document.createElement("img");
    afterImage.className = "compare-image";
    afterImage.src = afterUrl;
    afterImage.alt = "Restored image";
    afterLayer.appendChild(afterImage);

    const beforeLayer = document.createElement("div");
    beforeLayer.className = "compare-before";
    const beforeImage = document.createElement("img");
    beforeImage.className = "compare-image";
    beforeImage.src = beforeUrl;
    beforeImage.alt = "Original image";
    beforeLayer.appendChild(beforeImage);

    const divider = document.createElement("span");
    divider.className = "compare-divider";
    divider.setAttribute("aria-hidden", "true");

    const handle = document.createElement("span");
    handle.className = "compare-handle";
    handle.setAttribute("aria-hidden", "true");

    frame.append(
        afterLayer,
        beforeLayer,
        createCompareLabel("Before", "before"),
        createCompareLabel("After", "after"),
        divider,
        handle,
    );

    frame.addEventListener("pointerdown", (event) => {
        frame.setPointerCapture(event.pointerId);
        updateCompareFromPointer(frame, event);
    });
    frame.addEventListener("pointermove", (event) => {
        if (event.buttons !== 1) return;
        updateCompareFromPointer(frame, event);
    });
    frame.addEventListener("keydown", (event) => {
        const currentPosition =
            Number(frame.getAttribute("aria-valuenow")) || 50;
        if (event.key === "ArrowLeft") {
            event.preventDefault();
            updateComparePosition(frame, currentPosition - 4);
        }
        if (event.key === "ArrowRight") {
            event.preventDefault();
            updateComparePosition(frame, currentPosition + 4);
        }
    });

    updateComparePosition(frame, 50);
    return frame;
}

function showComparison(beforeUrl, afterUrl) {
    elements.resultBody.classList.remove("busy");
    elements.resultBody.replaceChildren(
        createCompareFrame(beforeUrl, afterUrl),
    );
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

    setImageAspect(inputUrl);
    showInputImage(inputUrl);
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

        showComparison(inputUrl, resultUrl);
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
