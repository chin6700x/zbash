/*
 * ZEPHYR WEB-OS MAC - TERMINAL APP
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const TerminalApp = (function () {
    let history = [];
    let historyIdx = -1;

    function init() {
        const screen = document.getElementById("termScreen");
        const input = document.getElementById("termInput");

        if (!screen || !input) return;

        // Print welcome banner
        printBanner();

        // Keyboard handler
        input.addEventListener("keydown", async (e) => {
            if (e.key === "Enter") {
                const cmd = input.value.trim();
                input.value = "";
                if (!cmd) return;

                history.push(cmd);
                historyIdx = history.length;

                await runCommand(cmd);
            } else if (e.key === "ArrowUp") {
                e.preventDefault();
                if (history.length > 0 && historyIdx > 0) {
                    historyIdx--;
                    input.value = history[historyIdx];
                }
            } else if (e.key === "ArrowDown") {
                e.preventDefault();
                if (historyIdx < history.length - 1) {
                    historyIdx++;
                    input.value = history[historyIdx];
                } else {
                    historyIdx = history.length;
                    input.value = "";
                }
            }
        });

        // Chips handler
        document.querySelectorAll(".term-chip").forEach(chip => {
            chip.addEventListener("click", () => {
                const cmd = chip.getAttribute("data-cmd");
                input.value = cmd;
                input.focus();
            });
        });

        // Focus input on screen click
        screen.addEventListener("click", () => input.focus());
    }

    function printBanner() {
        const screen = document.getElementById("termScreen");
        screen.innerHTML = `\x1b[36m⚡ ZEPHYR WEB-OS MAC TERMINAL (v2.2.0)\x1b[0m
Host: Apple Silicon Darwin Native Client
Workspace: /Volumes/Mac Backup/zero-tools
44 Silicon Tools Ready: \x1b[32msgrep, swc, scat, shead, xfind, xcp, zmake, zbash...\x1b[0m
Type any Unix command or click quick chips above.\n\n`;
        formatAnsi(screen);
    }

    async function runCommand(cmd) {
        const screen = document.getElementById("termScreen");
        if (cmd === "clear") {
            screen.innerHTML = "";
            return;
        }

        // Print command prompt line
        const promptLine = document.createElement("div");
        promptLine.innerHTML = `<span style="color: #10b981; font-weight: bold;">zephyr@mac</span>:<span style="color: #00f2fe;">~</span>$ <span style="color: #fff;">${escapeHtml(cmd)}</span>`;
        screen.appendChild(promptLine);

        // Execute via Mac Client Bridge
        const t0 = performance.now();
        const res = await ZephyrBridge.exec(cmd);
        const dt = (performance.now() - t0).toFixed(2);

        if (res.stdout) {
            const outSpan = document.createElement("div");
            outSpan.innerHTML = formatAnsiText(res.stdout);
            screen.appendChild(outSpan);
        }

        if (res.stderr) {
            const errSpan = document.createElement("div");
            errSpan.style.color = "#ef4444";
            errSpan.innerHTML = formatAnsiText(res.stderr);
            screen.appendChild(errSpan);
        }

        if (res.error) {
            const errSpan = document.createElement("div");
            errSpan.style.color = "#ef4444";
            errSpan.innerText = `Bridge Error: ${res.error}`;
            screen.appendChild(errSpan);
        }

        // Print execution timing badge
        const timeBadge = document.createElement("div");
        timeBadge.style.color = "#64748b";
        timeBadge.style.fontSize = "0.74rem";
        timeBadge.style.marginBottom = "0.5rem";
        timeBadge.innerText = `[${dt} ms on Mac Host | Exit: ${res.returncode ?? 0}]`;
        screen.appendChild(timeBadge);

        screen.scrollTop = screen.scrollHeight;
    }

    function escapeHtml(text) {
        return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
    }

    function formatAnsiText(text) {
        let esc = escapeHtml(text);
        return esc
            .replace(/\x1b\[31m/g, '<span style="color: #ef4444;">')
            .replace(/\x1b\[32m/g, '<span style="color: #10b981;">')
            .replace(/\x1b\[33m/g, '<span style="color: #f59e0b;">')
            .replace(/\x1b\[34m/g, '<span style="color: #3b82f6;">')
            .replace(/\x1b\[35m/g, '<span style="color: #8b5cf6;">')
            .replace(/\x1b\[36m/g, '<span style="color: #00f2fe;">')
            .replace(/\x1b\[1m/g, '<strong>')
            .replace(/\x1b\[0m/g, '</span></strong>');
    }

    function formatAnsi(el) {
        el.innerHTML = formatAnsiText(el.innerText);
    }

    return {
        init: init,
        run: runCommand
    };
})();
