/*
 * ZEPHYR WEB-OS MAC - CLIENT BRIDGE API
 * Author: Wirot Chookeaw (chin6700x)
 * License: Apache License 2.0 (c) 2026
 */

const ZephyrBridge = (function () {
    const API_URL = "api.php";

    async function request(action, data = {}, method = "POST") {
        try {
            let url = `${API_URL}?action=${encodeURIComponent(action)}`;
            let opts = { method: method };

            if (method === "POST") {
                opts.headers = { "Content-Type": "application/json" };
                opts.body = JSON.stringify(data);
            }

            const res = await fetch(url, opts);
            if (!res.ok) throw new Error(`HTTP ${res.status}: ${res.statusText}`);
            return await res.json();
        } catch (err) {
            console.error(`Bridge request error [${action}]:`, err);
            return { error: err.message };
        }
    }

    return {
        getSysInfo: async function () {
            return await request("sysinfo", {}, "GET");
        },

        exec: async function (command) {
            return await request("exec", { command: command }, "POST");
        },

        listFiles: async function (path = "") {
            return await request("fs_list", { path: path }, "POST");
        },

        readFile: async function (filePath) {
            return await request("fs_read", { file: filePath }, "POST");
        },

        writeFile: async function (filePath, content) {
            return await request("fs_write", { file: filePath, content: content }, "POST");
        },

        getTools: async function () {
            return await request("tools", {}, "GET");
        }
    };
})();
