/*
 * ZEPHYR WEB-OS MAC - ZPAD CODE EDITOR APP
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const EditorApp = (function () {
    let currentFilePath = "/Volumes/Mac Backup/zero-tools/README.md";

    function init() {
        const btnSave = document.getElementById("btnEditorSave");
        const btnRun = document.getElementById("btnEditorRun");

        if (btnSave) {
            btnSave.addEventListener("click", () => saveCurrentFile());
        }

        if (btnRun) {
            btnRun.addEventListener("click", () => runScript());
        }

        // Load initial file
        openFile(currentFilePath);
    }

    async function openFile(filePath) {
        currentFilePath = filePath;
        const filenameEl = document.getElementById("editorFilename");
        const textarea = document.getElementById("editorTextarea");
        const statusEl = document.getElementById("editorStatus");

        if (filenameEl) filenameEl.innerText = filePath.split("/").pop();
        if (statusEl) statusEl.innerText = `Loading ${filePath}...`;

        const res = await ZephyrBridge.readFile(filePath);
        if (res && res.content !== undefined) {
            if (textarea) textarea.value = res.content;
            if (statusEl) statusEl.innerText = `Ready | Size: ${res.size} bytes | Path: ${filePath}`;
        } else {
            if (statusEl) statusEl.innerText = `Failed to load: ${res.error || "File error"}`;
        }
    }

    async function saveCurrentFile() {
        const textarea = document.getElementById("editorTextarea");
        const statusEl = document.getElementById("editorStatus");

        if (!textarea) return;
        if (statusEl) statusEl.innerText = `Saving ${currentFilePath}...`;

        const res = await ZephyrBridge.writeFile(currentFilePath, textarea.value);
        if (res && res.success) {
            if (statusEl) statusEl.innerText = `✔ Saved successfully (${res.bytes_written} bytes written)`;
        } else {
            if (statusEl) statusEl.innerText = `❌ Error saving: ${res.error || "Save failed"}`;
        }
    }

    async function runScript() {
        WindowManager.open("win-terminal");
        if (currentFilePath.endsWith(".sh")) {
            TerminalApp.run(`zbash "${currentFilePath}"`);
        } else {
            TerminalApp.run(`swc "${currentFilePath}"`);
        }
    }

    return {
        init: init,
        openFile: openFile,
        save: saveCurrentFile
    };
})();
