/*
 * ZEPHYR WEB-OS MAC - FINDER APP
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const FinderApp = (function () {
    let currentPath = "/Volumes/Mac Backup/zero-tools";

    function init() {
        const grid = document.getElementById("finderGrid");
        const pathEl = document.getElementById("finderPath");

        if (!grid) return;

        // Load root
        loadDirectory(currentPath);

        // Sidebar clicks
        document.querySelectorAll(".sidebar-item").forEach(item => {
            item.addEventListener("click", () => {
                document.querySelectorAll(".sidebar-item").forEach(i => i.classList.remove("active"));
                item.classList.add("active");
                const path = item.getAttribute("data-path");
                if (path) loadDirectory(path);
            });
        });
    }

    async function loadDirectory(path) {
        currentPath = path;
        const grid = document.getElementById("finderGrid");
        const pathEl = document.getElementById("finderPath");

        if (pathEl) pathEl.innerText = path;
        grid.innerHTML = '<div style="color: #64748b; font-size: 0.85rem; padding: 1rem;">Loading contents...</div>';

        const res = await ZephyrBridge.listFiles(path);
        grid.innerHTML = "";

        if (res.error || !res.items) {
            grid.innerHTML = `<div style="color: #ef4444; padding: 1rem;">Error: ${res.error || "Cannot read directory"}</div>`;
            return;
        }

        // Add parent directory navigation if not root
        if (path !== "/Volumes/Mac Backup/zero-tools") {
            const parentItem = document.createElement("div");
            parentItem.className = "file-item";
            parentItem.innerHTML = `<div class="f-icon">⤴️</div><div class="f-name">.. (Up)</div>`;
            parentItem.addEventListener("click", () => {
                const parentPath = path.substring(0, path.lastIndexOf("/"));
                loadDirectory(parentPath || "/Volumes/Mac Backup/zero-tools");
            });
            grid.appendChild(parentItem);
        }

        res.items.forEach(item => {
            const el = document.createElement("div");
            el.className = "file-item";

            let icon = "📄";
            if (item.is_dir) icon = "📁";
            else if (item.name.endsWith(".c") || item.name.endsWith(".h")) icon = "🔷";
            else if (item.name.endsWith(".s") || item.name.endsWith(".asm")) icon = "⚙️";
            else if (item.name.endsWith(".sh")) icon = "📜";
            else if (item.name.endsWith(".md") || item.name.endsWith(".txt")) icon = "📘";
            else if (item.name.endsWith(".json") || item.name.endsWith(".xml")) icon = "📦";

            el.innerHTML = `<div class="f-icon">${icon}</div><div class="f-name">${escapeHtml(item.name)}</div>`;

            el.addEventListener("click", () => {
                if (item.is_dir) {
                    loadDirectory(item.path);
                } else {
                    // Open in Code Editor!
                    EditorApp.openFile(item.path);
                    WindowManager.open("win-editor");
                }
            });

            grid.appendChild(el);
        });
    }

    function escapeHtml(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    return {
        init: init,
        load: loadDirectory
    };
})();
