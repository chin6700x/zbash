/*
 * ZEPHYR WEB-OS MAC - WINDOW MANAGER ENGINE
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const WindowManager = (function () {
    let topZIndex = 100;
    let activeWindow = null;

    function init() {
        document.querySelectorAll(".window").forEach(win => {
            setupWindow(win);
        });
    }

    function focusWindow(win) {
        if (!win) return;
        topZIndex++;
        win.style.zIndex = topZIndex;
        document.querySelectorAll(".window").forEach(w => w.classList.remove("active"));
        win.classList.add("active");
        activeWindow = win;

        // Update active app title in menu bar
        const titleEl = win.querySelector(".window-title span");
        const menuAppEl = document.getElementById("activeAppTitle");
        if (titleEl && menuAppEl) {
            menuAppEl.innerText = titleEl.innerText;
        }
    }

    function setupWindow(win) {
        const titlebar = win.querySelector(".window-titlebar");
        const btnClose = win.querySelector(".traffic-close");
        const btnMin = win.querySelector(".traffic-min");
        const btnMax = win.querySelector(".traffic-max");

        // Focus on click
        win.addEventListener("mousedown", () => focusWindow(win));

        // Dragging
        let isDragging = false;
        let startX, startY, origLeft, origTop;

        titlebar.addEventListener("mousedown", (e) => {
            if (e.target.classList.contains("traffic-btn")) return;
            isDragging = true;
            focusWindow(win);

            startX = e.clientX;
            startY = e.clientY;
            origLeft = win.offsetLeft;
            origTop = win.offsetTop;

            function onMouseMove(moveEvent) {
                if (!isDragging) return;
                const dx = moveEvent.clientX - startX;
                const dy = moveEvent.clientY - startY;

                let newLeft = origLeft + dx;
                let newTop = origTop + dy;

                // Restrict top boundary under menubar
                if (newTop < 30) newTop = 30;

                win.style.left = `${newLeft}px`;
                win.style.top = `${newTop}px`;
            }

            function onMouseUp() {
                isDragging = false;
                window.removeEventListener("mousemove", onMouseMove);
                window.removeEventListener("mouseup", onMouseUp);
            }

            window.addEventListener("mousemove", onMouseMove);
            window.addEventListener("mouseup", onMouseUp);
        });

        // Traffic Light Handlers
        if (btnClose) {
            btnClose.addEventListener("click", (e) => {
                e.stopPropagation();
                closeWindow(win.id);
            });
        }

        if (btnMin) {
            btnMin.addEventListener("click", (e) => {
                e.stopPropagation();
                minimizeWindow(win.id);
            });
        }

        if (btnMax) {
            btnMax.addEventListener("click", (e) => {
                e.stopPropagation();
                win.classList.toggle("maximized");
            });
        }

        // Resizing
        setupResizing(win);
    }

    function setupResizing(win) {
        const handleBR = win.querySelector(".resize-br");
        if (!handleBR) return;

        handleBR.addEventListener("mousedown", (e) => {
            e.stopPropagation();
            let startW = win.offsetWidth;
            let startH = win.offsetHeight;
            let startX = e.clientX;
            let startY = e.clientY;

            function onMouseMove(moveEvent) {
                const newW = Math.max(380, startW + (moveEvent.clientX - startX));
                const newH = Math.max(240, startH + (moveEvent.clientY - startY));
                win.style.width = `${newW}px`;
                win.style.height = `${newH}px`;
            }

            function onMouseUp() {
                window.removeEventListener("mousemove", onMouseMove);
                window.removeEventListener("mouseup", onMouseUp);
            }

            window.addEventListener("mousemove", onMouseMove);
            window.addEventListener("mouseup", onMouseUp);
        });
    }

    function openWindow(winId) {
        const win = document.getElementById(winId);
        if (!win) return;
        win.style.display = "flex";
        win.classList.remove("minimized");
        focusWindow(win);

        // Update dock running status
        const appId = win.getAttribute("data-app");
        if (appId) {
            const dockItem = document.querySelector(`.dock-item[data-app="${appId}"]`);
            if (dockItem) dockItem.classList.add("running");
        }
    }

    function closeWindow(winId) {
        const win = document.getElementById(winId);
        if (!win) return;
        win.style.display = "none";
        win.classList.remove("active");

        const appId = win.getAttribute("data-app");
        if (appId) {
            const dockItem = document.querySelector(`.dock-item[data-app="${appId}"]`);
            if (dockItem) dockItem.classList.remove("running");
        }
    }

    function minimizeWindow(winId) {
        const win = document.getElementById(winId);
        if (!win) return;
        win.classList.add("minimized");
    }

    return {
        init: init,
        open: openWindow,
        close: closeWindow,
        minimize: minimizeWindow,
        focus: focusWindow
    };
})();
