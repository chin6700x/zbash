/*
 * ZEPHYR WEB-OS MAC - ACTIVITY MONITOR APP
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const MonitorApp = (function () {
    let refreshTimer = null;

    function init() {
        refreshStats();
        // Auto refresh every 4 seconds when window is active
        const win = document.getElementById("win-monitor");
        if (win) {
            win.addEventListener("mousedown", () => refreshStats());
        }

        const btnRefresh = document.getElementById("btnRefreshMonitor");
        if (btnRefresh) {
            btnRefresh.addEventListener("click", () => refreshStats());
        }
    }

    async function refreshStats() {
        const cpuVal = document.getElementById("monCpuVal");
        const ramVal = document.getElementById("monRamVal");
        const hostVal = document.getElementById("monHostVal");
        const tbody = document.getElementById("monProcTbody");

        // 1. Fetch system info
        const sys = await ZephyrBridge.getSysInfo();
        if (sys && !sys.error) {
            if (cpuVal) cpuVal.innerText = `${sys.cpu} (${sys.cores} Cores)`;
            if (ramVal) ramVal.innerText = sys.ram;
            if (hostVal) hostVal.innerText = `${sys.os} - ${sys.host}`;
        }

        // 2. Fetch top 15 processes from Mac
        if (tbody) {
            const procRes = await ZephyrBridge.exec("ps -axro pid,%cpu,%mem,time,comm | head -n 16");
            if (procRes && procRes.stdout) {
                tbody.innerHTML = "";
                const lines = procRes.stdout.trim().split("\n");
                // Skip header line
                for (let i = 1; i < lines.length; i++) {
                    const parts = lines[i].trim().split(/\s+/);
                    if (parts.length >= 5) {
                        const pid = parts[0];
                        const cpu = parts[1];
                        const mem = parts[2];
                        const time = parts[3];
                        const cmd = parts.slice(4).join(" ");

                        const tr = document.createElement("tr");
                        tr.innerHTML = `
                            <td>${escapeHtml(pid)}</td>
                            <td><strong style="color: #fff;">${escapeHtml(cmd.split("/").pop())}</strong></td>
                            <td style="color: ${parseFloat(cpu) > 10 ? '#00f2fe' : '#94a3b8'};">${escapeHtml(cpu)}%</td>
                            <td>${escapeHtml(mem)}%</td>
                            <td>${escapeHtml(time)}</td>
                        `;
                        tbody.appendChild(tr);
                    }
                }
            }
        }
    }

    function escapeHtml(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    return {
        init: init,
        refresh: refreshStats
    };
})();
