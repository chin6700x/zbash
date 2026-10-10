/*
 * ZEPHYR WEB-OS MAC - DOCK CONTROLLER
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const DockManager = (function () {
    function init() {
        const dock = document.getElementById("dock");
        if (!dock) return;

        const items = dock.querySelectorAll(".dock-item");

        // Magnification on hover
        dock.addEventListener("mousemove", (e) => {
            const dockRect = dock.getBoundingClientRect();
            const mouseX = e.clientX;

            items.forEach(item => {
                const rect = item.getBoundingClientRect();
                const itemCenterX = rect.left + rect.width / 2;
                const distance = Math.abs(mouseX - itemCenterX);

                const maxDistance = 120;
                if (distance < maxDistance) {
                    const factor = 1 - distance / maxDistance;
                    const scale = 1 + factor * 0.35;
                    const translateY = -factor * 12;
                    item.style.transform = `translateY(${translateY}px) scale(${scale})`;
                } else {
                    item.style.transform = "translateY(0px) scale(1)";
                }
            });
        });

        dock.addEventListener("mouseleave", () => {
            items.forEach(item => {
                item.style.transform = "translateY(0px) scale(1)";
            });
        });

        // Click app in dock
        items.forEach(item => {
            item.addEventListener("click", () => {
                const appId = item.getAttribute("data-app");
                const winId = `win-${appId}`;
                const win = document.getElementById(winId);

                if (!win) return;

                if (win.style.display === "none" || win.classList.contains("minimized")) {
                    WindowManager.open(winId);
                } else if (win.classList.contains("active")) {
                    WindowManager.minimize(winId);
                } else {
                    WindowManager.focus(win);
                }
            });
        });
    }

    return {
        init: init
    };
})();
