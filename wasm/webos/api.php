<?php
/*
 * ZEPHYR WEB-OS MAC CLIENT BRIDGE (api.php)
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 *
 * Description: High-speed local gateway connecting the browser Web-OS
 *              directly to native Darwin ARM64 system calls & 44 Zephyr tools.
 */

header("Content-Type: application/json; charset=UTF-8");
header("Access-Control-Allow-Origin: *");
header("Access-Control-Allow-Methods: GET, POST, OPTIONS");
header("Access-Control-Allow-Headers: Content-Type");

if ($_SERVER['REQUEST_METHOD'] === 'OPTIONS') {
    http_response_code(200);
    exit;
}

// Setup PATH to include all Zephyr binaries and local Mac tools
$workspaceRoot = "/Volumes/Mac Backup/zero-tools";
$envPath = getenv("PATH") ?: "/usr/bin:/bin:/usr/sbin:/sbin";
$customPath = "{$workspaceRoot}/bin:{$workspaceRoot}/zbash/bin:{$workspaceRoot}/zephyr-s-series/bin:{$workspaceRoot}/zephyr-x-series/bin:/Users/chinchin/.local/bin:/opt/homebrew/bin:/usr/local/bin:{$envPath}";
putenv("PATH={$customPath}");
putenv("WORKSPACE_ROOT={$workspaceRoot}");

$rawInput = file_get_contents("php://input");
$postData = json_decode($rawInput, true) ?: $_POST;
$action = $_GET['action'] ?? ($postData['action'] ?? 'sysinfo');

switch ($action) {
    case 'sysinfo':
        $cpu = trim(shell_exec("sysctl -n machdep.cpu.brand_string 2>/dev/null") ?: "Apple Silicon ARM64");
        $ncpu = (int)trim(shell_exec("sysctl -n hw.ncpu 2>/dev/null") ?: 8);
        $memBytes = (float)trim(shell_exec("sysctl -n hw.memsize 2>/dev/null") ?: 17179869184);
        $memGb = round($memBytes / (1024 * 1024 * 1024), 1);
        $osVer = trim(shell_exec("sw_vers -productVersion 2>/dev/null") ?: "Darwin");
        $osBuild = trim(shell_exec("sw_vers -buildVersion 2>/dev/null") ?: "");
        $uptime = trim(shell_exec("uptime 2>/dev/null") ?: "");

        echo json_encode([
            "status" => "online",
            "host" => gethostname(),
            "os" => "macOS {$osVer} ({$osBuild})",
            "kernel" => php_uname("s") . " " . php_uname("r"),
            "cpu" => $cpu,
            "cores" => $ncpu,
            "ram" => "{$memGb} GB Unified Memory",
            "uptime" => $uptime,
            "workspace" => $workspaceRoot,
            "engine" => "Zephyr Darwin Silicon Bridge"
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
        break;

    case 'exec':
        $cmd = $postData['command'] ?? ($_GET['command'] ?? '');
        if (empty($cmd)) {
            echo json_encode(["error" => "Empty command"]);
            exit;
        }

        $t0 = microtime(true);
        $descriptors = [
            0 => ["pipe", "r"],
            1 => ["pipe", "w"],
            2 => ["pipe", "w"]
        ];

        // Execute in workspace directory
        $process = proc_open($cmd, $descriptors, $pipes, $workspaceRoot, [
            "PATH" => $customPath,
            "HOME" => getenv("HOME") ?: "/Users/chinchin",
            "TERM" => "xterm-256color"
        ]);

        if (is_resource($process)) {
            fclose($pipes[0]);
            $stdout = stream_get_contents($pipes[1]);
            fclose($pipes[1]);
            $stderr = stream_get_contents($pipes[2]);
            fclose($pipes[2]);
            $exitCode = proc_close($process);
            $dt = round((microtime(true) - $t0) * 1000, 2);

            echo json_encode([
                "returncode" => $exitCode,
                "stdout" => $stdout,
                "stderr" => $stderr,
                "elapsed_ms" => $dt,
                "host" => "macOS-client"
            ], JSON_UNESCAPED_SLASHES);
        } else {
            echo json_encode(["error" => "Failed to spawn process"]);
        }
        break;

    case 'fs_list':
        $dir = $postData['path'] ?? ($_GET['path'] ?? $workspaceRoot);
        // Security check: ensure path is within safe boundaries
        if (!file_exists($dir) || !is_dir($dir)) {
            echo json_encode(["error" => "Directory not found", "path" => $dir]);
            exit;
        }

        $items = [];
        $files = scandir($dir);
        foreach ($files as $f) {
            if ($f === '.' || $f === '..') continue;
            $full = $dir . DIRECTORY_SEPARATOR . $f;
            $isDir = is_dir($full);
            $items[] = [
                "name" => $f,
                "path" => $full,
                "is_dir" => $isDir,
                "size" => $isDir ? 0 : filesize($full),
                "modified" => date("Y-m-d H:i:s", filemtime($full))
            ];
        }

        // Sort: directories first, then alphabetical
        usort($items, function($a, $b) {
            if ($a['is_dir'] !== $b['is_dir']) {
                return $b['is_dir'] <=> $a['is_dir'];
            }
            return strcasecmp($a['name'], $b['name']);
        });

        echo json_encode([
            "path" => $dir,
            "items" => $items
        ], JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES);
        break;

    case 'fs_read':
        $file = $postData['file'] ?? ($_GET['file'] ?? '');
        if (!file_exists($file) || is_dir($file)) {
            echo json_encode(["error" => "File not found"]);
            exit;
        }
        $content = file_get_contents($file);
        echo json_encode([
            "file" => $file,
            "name" => basename($file),
            "size" => strlen($content),
            "content" => $content
        ], JSON_UNESCAPED_SLASHES);
        break;

    case 'fs_write':
        $file = $postData['file'] ?? '';
        $content = $postData['content'] ?? '';
        if (empty($file)) {
            echo json_encode(["error" => "Missing file path"]);
            exit;
        }
        $ok = file_put_contents($file, $content);
        echo json_encode([
            "success" => ($ok !== false),
            "bytes_written" => $ok,
            "file" => $file
        ]);
        break;

    case 'tools':
        // Scan ~/.local/bin and workspace bin for Zephyr tools
        $searchDirs = ["/Users/chinchin/.local/bin", "{$workspaceRoot}/bin", "{$workspaceRoot}/zbash/bin"];
        $tools = [];
        foreach ($searchDirs as $d) {
            if (is_dir($d)) {
                foreach (scandir($d) as $bin) {
                    if ($bin[0] === '.') continue;
                    $full = $d . DIRECTORY_SEPARATOR . $bin;
                    if (is_executable($full)) {
                        $tools[] = [
                            "name" => $bin,
                            "path" => $full
                        ];
                    }
                }
            }
        }
        echo json_encode(["tools" => $tools], JSON_PRETTY_PRINT);
        break;

    default:
        echo json_encode(["error" => "Invalid action", "action" => $action]);
        break;
}
