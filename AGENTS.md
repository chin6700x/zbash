# ⚡ AGENTS.MD: ZBASH SUB-ECOSYSTEM MANIFESTO & ENGINEERING STANDARD
> **"Bare-Metal POSIX Parity · 86 KB Zero-Dependency Footprint · Modern Apple Silicon Velocity"**  
> **Author & Architect:** Wirot Chookeaw (`chin6700x`) | **License:** Apache License 2.0 &copy; 2026  
> **Workspace Root:** `/Volumes/Mac Backup/zero-tools/zbash`

---

## 🧭 1. วิสัยทัศน์และเป้าหมายสูงสุด (The Grand Vision & Objective)

**ZBASH** ถูกสร้างขึ้นเพื่อพิสูจน์ว่า เราไม่จำเป็นต้องทนใช้ **GNU Bash ที่มีอายุ 40 ปี** (ซึ่งเทอะทะ ขนาด 1.4+ MB, สตาร์ทช้า, และเต็มไปด้วยหนี้ทางเทคนิค) อีกต่อไป โดยการสร้าง Modern POSIX Shell ที่:
1. **เร็วขึ้นอย่างก้าวกระโดดบน Apple Silicon:** สตาร์ทในหน่วยไมโครวินาที (Sub-millisecond startup), เร่งความเร็วการตัดแบ่งคำ (Lexing) ด้วย **ARM64 NEON 128-bit SIMD**, และจัดการหน่วยความจำแบบ Zero-Overhead Arena
2. **ขนาดจิ๋วระดับ 86 KB:** คอมไพล์เป็น Pure Bare-Metal Binary ปราศจาก External Dependency (ไม่ต้องพึ่ง GNU Readline หรือ ncurses) และสามารถพอร์ตสู่ WebAssembly (WASM) สำหรับ WebOS ได้อย่างไร้รอยต่อ
3. **100% Drop-in Replacement:** สามารถนำไปแทนที่ `/bin/bash` หรือ `/bin/sh` ในระบบ CI/CD, Build Systems, DevOps Scripts, และ Terminal โดยสคริปต์เดิมของมนุษยชาติทำงานได้ถูกต้องตรงกันแบบ **Bit-for-Bit Exact Match 100%**

---

## 🏛️ 2. ปรัชญาและกฎเหล็กการพัฒนา (Core Engineering Principles)

### 📌 กฎเหล็กข้อที่ 1: True Drop-in Parity (แทนที่ได้ทันที 100%)
* **ห้ามประดิษฐ์ Syntax ใหม่:** เราไม่เดินตามรอยความผิดพลาดของเชลล์ยุคใหม่ที่พยายามเปลี่ยนไวยากรณ์จนสคริปต์เดิมพัง
* **ยึดตามมาตรฐาน POSIX.1-2017 และพฤติกรรมจริงของ GNU Bash 3.2+:** Exit Code, STDOUT, STDERR, Whitespace, และ Quoting ต้องตรงกันบิตต่อบิต
* **Zero Learning Curve:** วิศวกรและระบบอัตโนมัติสามารถเปลี่ยนมาใช้ `zbash` ได้ทันทีโดยไม่ต้องแก้ไขสคริปต์แม้แต่บรรทัดเดียว

### 📌 กฎเหล็กข้อที่ 2: ความปลอดภัยและการคืนทรัพยากรระดับเคอร์เนล (Kernel Resource Hygiene)
* **Transactional I/O:** ทุกคำสั่งที่เปลี่ยนเส้นทาง Standard Descriptors (0, 1, 2) ในระดับ Builtin หรือ Loops (`done < file`) ต้องใช้ `dup()` สำรองไว้ก่อน และ `dup2()` กู้คืนกลับมาทันทีเมื่อจบลูป เพื่อรักษา Terminal Keyboard FD เสมอ
* **Zero Zombie Toleration:** ต้องเคลียร์ศพ Zombie Processes ออกจากตารางเคอร์เนลอย่างสม่ำเสมอด้วย `while (waitpid(-1, NULL, WNOHANG) > 0);` เพื่อป้องกัน Watchdog และ `kill -0` ค้าง
* **Zero Temp Inode Leaks:** ไฟล์ชั่วคราวใดๆ ที่สร้างขึ้น (เช่น Here-Strings `<<<`) ต้องถูก `unlink()` ทันทีหลังเปิด Descriptor ป้องกัน Inode เต็มดิสก์

### 📌 กฎเหล็กข้อที่ 3: สถาปัตยกรรมหน่วยความจำแบบ Stack Frame Scope
* **Activation Records สำหรับฟังก์ชัน:** ตัวแปร `local` ในฟังก์ชัน ต้องได้รับการบันทึกสถานะเดิมลงใน Call Scope Stack (`local_vars`) และทำการคลายสแตก (Unwind LIFO) เมื่อออกจากฟังก์ชัน เพื่อป้องกัน Scope Smashing ใน Recursive Functions

---

## 🧪 3. มาตรฐานการทดสอบอันเข้มงวด (Mandatory Testing Standard)

ในการพัฒนาฟีเจอร์หรือแก้ไขบั๊กใดๆ ใน `zbash` **ต้องปฏิบัติตามมาตรฐานนี้อย่างเคร่งครัด**:

1. **Exhaustive Differential Parity Testing:**
   * สคริปต์ทดสอบต้องรันคำสั่งเดียวกันเทียบระหว่าง `/bin/bash` กับ `bin/zbash`
   * ตรวจสอบความถูกต้องด้วย `cmp -s` (สำหรับไบนารีและข้อความ) และตรวจสอบ Exit Code (`$?`)
2. **การทดสอบ 2 ชั้น (Two-Tier Test Matrix):**
   * **Tier 1 (1,000 Modular Basic Tests):** ครอบคลุม 10 หมวดหมู่พื้นฐานใน `test/1000-test/` (หมวดละ 100 ข้อ = 1,000 ข้อ)
   * **Tier 2 (Advance Enterprise Tests):** ครอบคลุม 10 หมวดหมู่การใช้งานจริงระดับองค์กรใน `test/advance-test/` (หมวดละ 100 ข้อ = 1,000 ข้อ) เช่น ETL, Docker, Git, CI/CD, Process Supervision, SemVer, REST Streams
3. **เกณฑ์การผ่าน (Acceptance Criteria):**
   * **100 / 100 PASS (100.0%)** ในทุกชุดทดสอบ ห้ามมี Regression แม้แต่ข้อเดียว

---

## 🪲 4. กฎบัตรการบันทึกบั๊กแบบโมดูลาร์ (Modular Bugfix Protocol)

> **"หนึ่งปัญหา คือ หนึ่งบทเรียน — ห้ามสุมรวมเป็นไฟล์เดียว แต่จงบันทึกเป็นแฟ้มประวัติเดี่ยวที่พร้อมสืบค้น"**

เพื่อรักษาความสะอาด ความเป็นระบบ และเปิดโอกาสให้ AI หรือวิศวกรรุ่นต่อไปสามารถศึกษาและต่อยอดได้อย่างรวดเร็ว กำหนดให้ใช้ระเบียบการบันทึกบั๊กดังนี้:

1. **ห้ามนำบั๊กทั้งหมดไปอัดรวมในไฟล์เดียว:**
   * แยกเป็น **1 ไฟล์ต่อ 1 บั๊ก/พฤติกรรมแปลก** เสมอในไดเรกทอรี `zbash/docs/bugfix/`
   * ตั้งชื่อไฟล์ตามแบบแผน: `bug_XXX_<short_slug>.md` เช่น:
     - `zbash/docs/bugfix/bug_001_zombie_process_table.md`
     - `zbash/docs/bugfix/bug_002_exit_trap_deadlock.md`
     - `zbash/docs/bugfix/bug_003_forgotten_pipe_fd_leak.md`
2. **โครงสร้างมาตรฐานในแต่ละไฟล์บั๊ก (Bug Dossier Standard Schema):**
   ทุกไฟล์ต้องมีหัวข้อครบทั้ง 6 ส่วน:
   - **Header:** ชื่อบั๊ก, หมายเลข, หมวดหมู่, ระดับความรุนแรง, ผู้ค้นพบ
   - **1. อาการของปัญหา (Symptom & PoC Script):** สคริปต์ตัวอย่างที่เกิดข้อผิดพลาด
   - **2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause):** วิเคราะห์ว่าทำไม POSIX/Bash ถึงทำแบบนั้น
   - **3. การสืบสวนเชิงลึก (Investigation & Forensics):** การตรวจตราด้วย lsof, strace, หรือ gdb
   - **4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix):** ระบุไฟล์, ฟังก์ชัน, และ Code Diff
   - **5. การทดสอบยืนยันผล (Regression Test Command):** คำสั่งรันพิสูจน์ว่าแก้หายขาด
   - **6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom):** ข้อคิดเพื่อไม่ให้ติดกับดักเดิม
3. **Master Index Catalog:**
   * ต้องมีไฟล์ดัชนีรวม `zbash/docs/bugfix/INDEX.md` ที่ทำสารบัญลิงก์ไปยังไฟล์บั๊กทุกตัว

---

## 📁 5. โครงสร้างคลังโค้ด ZBASH (Sub-Workspace Architecture)

```
zbash/
├── Makefile                     # Build system (make, make clean, CFLAGS -O3 -flto)
├── README.md                    # เอกสารภาพรวม ZBASH
├── AGENTS.md                    # กฎบัตร ปรัชญา และมาตรฐานสำหรับ AI Agent (เอกสารฉบับนี้)
├── bin/                         # ไบนารีผลลัพธ์ (bin/zbash)
├── include/                     # Headers (zbash.h, zephyr_version.h)
├── src/
│   ├── main.c                   # Entry point, REPL, argument parsing
│   ├── core/
│   │   └── engine.c             # AST execution, pipelines, loops, builtins dispatch, I/O
│   ├── parser/
│   │   └── lexer.c              # Tokenizer, parameter expansion, quoting state machine
│   ├── builtins/
│   │   └── builtins.c           # Builtin commands (cd, echo, read, test, trap, wait, etc.)
│   └── asm/
│       └── simd_lexer.s         # ARM64 NEON Assembly for whitespace/quote acceleration
├── test/
│   ├── 1000-test/               # Basic Test Suite (10 modules x 100 tests)
│   └── advance-test/            # Enterprise Test Suite (10 modules x 100 tests)
└── docs/
    ├── book/th/                 # หนังสือ "สร้าง Shell ระดับเทพด้วย C" (25 บท)
    └── bugfix/                  # 🪲 Modular Bugfix Dossiers
        ├── INDEX.md             # สารบัญ Master Index
        ├── bug2fix.txt          # บันทึกภาพรวมเดิม
        └── bug_*.md             # แฟ้มบันทึกบั๊กแยกรายหัวข้อ
```

---

## 🤖 6. ข้อกำหนดสำหรับ AI Agent ในการพัฒนาต่อยอด

เมื่อ Agent ได้รับมอบหมายให้ทำงานในคลังโค้ด `zbash`:
1. **ยึดมั่นใน Apache License 2.0:** ทุกไฟล์โค้ดใหม่ต้องมี Header:
   ```c
   /*
    * Author: Wirot Chookeaw (chin6700x)
    * Copyright: (c) 2026 Wirot Chookeaw.
    * License: Apache License 2.0
    */
   ```
2. **ห้ามยอมรับ Regression:** เมื่อแก้ไขโค้ดใดๆ ต้องรันชุดทดสอบที่เกี่ยวข้องและต้องผ่าน **100% PASS** เสมอ
3. **เมื่อพบบั๊กใหม่ หรือแก้บั๊กแปลกๆ:** ต้องสร้างไฟล์ `bug_XXX_<name>.md` ใน `zbash/docs/bugfix/` ตามมาตรฐาน Schema ทันที และอัปเดต `INDEX.md`
4. **ความสะอาดและประสิทธิภาพระดับสูงสุด:** หลีกเลี่ยง Heap Allocations ที่ไม่จำเป็นใน Hot Loop ของ Lexer/Parser และคืนหน่วยความจำทุกครั้งเมื่อสิ้นสุดคำสั่ง

---
*บันทึกไว้ ณ วันที่ 9 ตุลาคม 2026 โดย วิโรจน์ ชูแก้ว (Wirot Chookeaw) และ AI Antigravity*
