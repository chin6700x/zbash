/*
 * ZEPHYR WEB-OS MAC - MASTER CONTROLLER (main.js)
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

document.addEventListener("DOMContentLoaded", async function () {
    // 1. Initialize Clock in Menu Bar
    function updateClock() {
        const clockEl = document.getElementById("menuClock");
        if (clockEl) {
            const now = new Date();
            clockEl.innerText = now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
        }
    }
    updateClock();
    setInterval(updateClock, 1000);

    // 2. Fetch System Hardware Info for Menu Bar Badge
    try {
        const sys = await ZephyrBridge.getSysInfo();
        const badge = document.getElementById("menuSysBadge");
        if (badge && sys && !sys.error) {
            badge.innerText = ` ${sys.cpu} (${sys.ram.split(' ')[0]} GB)`;
        }
    } catch (e) {
        console.warn("Failed to update sys badge:", e);
    }

    // 3. Initialize Window Manager & Dock
    WindowManager.init();
    DockManager.init();

    // 4. Initialize Core Apps
    TerminalApp.init();
    FinderApp.init();
    MonitorApp.init();
    EditorApp.init();

    // 5. Desktop Icons click handler
    document.querySelectorAll(".desk-icon").forEach(icon => {
        icon.addEventListener("click", () => {
            const appId = icon.getAttribute("data-app");
            if (appId) {
                WindowManager.open(`win-${appId}`);
            }
        });
    });

    // 6. Open Terminal window by default in centered position
    WindowManager.open("win-terminal");
});
