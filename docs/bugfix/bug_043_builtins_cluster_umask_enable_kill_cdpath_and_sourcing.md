/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-043: Builtins Cluster Architecture, Stdout Fork Buffer Bleed, POSIX Prefix Persistence, & Exec Vector Isolation
> **หมวดหมู่:** สถาปัตยกรรม Builtin เชลล์, การจัดการบัฟเฟอร์ I/O ข้าม Fork, การสืบทอดสภาพแวดล้อม POSIX, และการค้นหาไฟล์สคริปต์ (Builtins Registry, Stdout Buffering across Fork, POSIX Environment Persistence & Sourcing Isolation)  
> **ระดับความรุนแรง:** สูงมาก (Critical - 100% Bit-for-bit exact parity in official GNU Bash `builtins.tests`, 250 lines, and all 7 companion scripts `builtins1.sub`–`builtins2.sub`, `source1.sub`–`source5.sub`)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c), [`src/core/exec.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/exec.c), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันชุดทดสอบอย่างเป็นทางการของ GNU Bash ในกลุ่ม **Cluster G1: Builtins & Sourcing Family** (`builtins.tests` ขนาด 250 บรรทัด พร้อมสคริปต์บริวาร 7 ชุด):
```bash
bash test/run_gnu_test.sh builtins.tests -v
```

พบข้อขัดแย้งและบั๊กทางระบบปฏิบัติการ 7 ประเด็นหลัก:

1. **Stdout Libc Full-Buffering Bleed ข้าม Fork ใน Command Substitution:**
   - เมื่อรันแบบ Redirect ลงไฟล์ (`run_gnu_test.sh` หรือ `> /tmp/out.txt`) Libc Stdio จะเปลี่ยนจาก Line-buffered เป็น Full-buffered (4096 ไบต์) ผลลัพธ์จากการเรียก `printf` ในคำสั่ง `declare -p` ก่อนหน้ายังค้างอยู่ในบัฟเฟอร์ เมื่อคำสั่งถัดมาเรียก `sigone=$(kill -l | sed ...)` ฟังก์ชัน `zbash_subshell_exec` สั่ง `fork()` โดยไม่ได้สั่ง `fflush(NULL)` ก่อน กระบวนการลูกจึงโคลนบัฟเฟอร์ที่ค้างอยู่ และเมื่อปิดไปป์หรือออกจากกระบวนการ ข้อความ `declare -x foo=""...` ก็รั่วไหลเข้าไปในไปป์ ส่งผลให้ตัวแปร `sigone` กลายเป็นสตริง declare แทนที่จะเป็นชื่อสัญญาณ `SIGHUP`
2. **การทำลาย Argument Vector ในคำสั่ง `exec -a <name>`:**
   - ใน `builtin_exec` โค้ดเดิมนำอาร์เรย์ `argv + idx` มาแก้ไข `new_argv[0] = custom_arg0` โดยตรง ซึ่งไปทับพอยน์เตอร์ในโครงสร้างคำสั่งเดิม ทำให้เมื่อโปรแกรมล้มเหลว โค้ด `zbash_free_cmd` สั่ง `free()` พอยน์เตอร์บนสแต็กจนเกิด `malloc: pointer being freed was not allocated` อีกทั้งยังส่งผลให้ `execvp` พยายามรันชื่อ Override (`specialname`) แทนที่จะรันโปรแกรมจริง (`${THIS_SH}`)
3. **การขาดหายไปของ Builtin `times`:**
   - ใน `enable -ps` และ `enable -aps` มาตรฐานระบุ Special Builtins ทั้งหมด 16 ตัว แต่ ZBASH ขาดคำสั่ง `times` ทำให้ `builtin_enable` ข้ามคำสั่งนี้และฟ้องว่า `enable -n times`
4. **พฤติกรรม Prefix Assignment Persistence ใน POSIX Special Builtins และ `declare -x`:**
   - ในโหมด POSIX (`set -o posix`) การกำหนดค่าตัวแปรก่อนหน้า Special Builtin (`AVAR=foo . ./source1.sub`, `AVAR=foo eval ...`, `AVAR=foo :`) ต้องคงสภาพ (Persist) อยู่ในเชลล์แม่ถาวร ไม่ถูกกู้คืน เช่นเดียวกับ `FOO='$$' declare -x FOO` ที่มีผลเท่ากับ `export`
5. **การแยกสโคป Positional Parameters ในการเรียก Source พร้อม Arguments:**
   - คำสั่ง `. ./source3.sub x y z` ต้องส่ง `$1, $2, $3` ให้กับสคริปต์ที่ถูก Source มองเห็นเสมือนพารามิเตอร์ของตนเอง แต่เมื่อสคริปต์ทำงานจบ ตำแหน่ง `$@` ของเชลล์แม่ต้องคงเดิม เว้นแต่สคริปต์จะเรียก `set --` ปรับเปลี่ยนโดยชัดแจ้ง (`source4.sub`)
6. **การจัดการ Umask และ Symbolic Permission Manipulation:**
   - คำสั่ง `umask` ต้องรองรับทั้ง Octal Notation (`022`, `0`), Symbolic Notation (`-S`, `u=rwx,g=rx,o=rx`), และ POSIX Reusable Command Output (`-p`, `umask 0022`)
7. **Signal Lookup Table Parity บน macOS Darwin:**
   - คำสั่ง `kill -l` และ `trap -l` บน macOS Darwin ต้องแสดงตารางสัญญาณ 31 สัญญาณในรูปแบบ 4 คอลัมน์ 8 บรรทัดตรงกันทุกอักขระ พร้อมรองรับการแปลงเลขสัญญาณ (`kill -l 1` -> `HUP`), แปลง Signal Exit Code (`kill -l 129` -> `HUP`), และแจ้งเตือนข้อผิดพลาดตรงตามมาตรฐาน

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **POSIX Libc Fork Buffering Invariance:**
   การโคลน Address Space ของ Unix ด้วย `fork()` เป็นแบบ Copy-on-Write ซึ่งจะสำเนา User-Space Memory ทั้งหมดรวมถึง Libc Stdio File Stream Buffers (`FILE *`) หากสตรีมใดมีข้อมูลที่ค้างรอส่ง (Dirty Buffer) และไม่มีการสั่ง `fflush(NULL)` ก่อน `fork()` กระบวนการลูกจะนำบัฟเฟอร์นั้นไป Flushed ซ้ำสอง และหากกระบวนการลูกทำการ `dup2(pfd[1], STDOUT_FILENO)` บัฟเฟอร์ที่ค้างอยู่จะถูกระบายลงใน Pipe ทันที
2. **POSIX Special Builtin Variable Assignment Scope:**
   ตามมาตรฐาน IEEE Std 1003.1 (POSIX) หมวด Special Built-In Utilities กำหนดว่าการกำหนดตัวแปรนำหน้าคำสั่งพิเศษ (เช่น `.`, `:`, `eval`, `exec`, `exit`, `export`, `readonly`, `return`, `set`, `shift`, `times`, `trap`, `unset`) จะต้องคงสภาพอยู่ใน Environment ของเชลล์แม่หลังจากคำสั่งทำงานจบ ต่างจากคำสั่งธรรมดาที่ตัวแปรจะคงอยู่เฉพาะในสภาพแวดล้อมชั่วคราวของคำสั่งนั้นเท่านั้น

---

## 3. การสืบสวนเชิงลึกและการแก้ไข (Investigation & Implementation)

### 3.1 ป้องกัน Libc Stdio Buffer Bleed ข้าม Fork
เพิ่มการเรียก `fflush(NULL)` ก่อนหน้าคำสั่ง `fork()` ในทุกจุดของระบบ:
- `src/core/exec.c`: ใน `zbash_subshell_exec`, Compound Subshell `( ... )`, External Command Dispatch, และ `execute_pipeline`
- `src/builtins/builtins.c`: ใน `builtin_command`
- `src/core/engine.c`: ในการแยกรันเบื้องหลัง (`&`) และ Subshell

```c
fflush(NULL);
pid_t pid = fork();
```

### 3.2 ปรับปรุง `builtin_exec` แยก Heap-Allocated Argument Vector
สร้างอาร์เรย์พอยน์เตอร์ใหม่ `new_argv` ขนาด `new_argc + 1` ไม่ดัดแปลง `argv` ดั้งเดิมของคำสั่ง เพื่อป้องกันปัญหา Double-free ใน Heap Memory และส่งผ่านพาธโปรแกรมไปยัง `execvp(prog, new_argv)` อย่างถูกต้อง:

```c
const char *prog = argv[idx];
int new_argc = argc - idx;
char **new_argv = malloc((new_argc + 1) * sizeof(char *));
for (int i = 0; i < new_argc; i++) new_argv[i] = argv[idx + i];
new_argv[new_argc] = NULL;
...
execvp(prog, new_argv);
fprintf(stderr, "zbash: exec: %s: not found\n", prog);
free(new_argv);
exit(127);
```

### 3.3 สร้าง Builtin `times` และขยาย Special Builtins Registry
- เพิ่ม `#include <sys/resource.h>` ใน `src/builtins/builtins.c`
- คำนวณเวลาการทำงานของโปรเซสเชลล์และกระบวนการลูกด้วย `getrusage(RUSAGE_SELF, ...)` และ `getrusage(RUSAGE_CHILDREN, ...)`
- เพิ่ม `"times"` เข้าสู่ `zbash_is_builtin` และตารางพิเศษของ `builtin_enable`

### 3.4 ระบบจัดการ Prefix Environment และ POSIX Persistence
- ใน `src/core/exec.c` เพิ่มโครงสร้าง `saved_env_t` บันทึกค่าเดิมและแอตทริบิวต์เดิม (`old_attr`)
- ตรวจสอบว่าคำสั่งเป็น Special Builtin ภายใต้ `state->posix` หรือเป็น `export` / `declare -x` หรือไม่ หากใช่ จะคงสภาพตัวแปรไว้ในเชลล์แม่และกำหนดแอตทริบิวต์ `ZBASH_ATTR_EXPORT`

### 3.5 ระบบแยก Positional Parameters ใน `zbash_source_file_args`
- ใน `src/core/engine.c` เก็บรักษาค่า `saved_argc` และ `saved_argv` เดิม
- หากมีการส่งอาร์กิวเมนต์มายังสคริปต์ที่ถูก Source สคริปต์จะใช้ชุดอาร์กิวเมนต์ใหม่ แต่เมื่อทำงานเสร็จสิ้นจะคืนค่าเดิม เว้นแต่สคริปต์จะสั่ง `set --` ดัดแปลงอย่างจงใจ

---

## 4. ผลลัพธ์การทดสอบยืนยัน (Verification & Parity Proof)

### 4.1 Differential Parity บน `builtins.tests` (GNU Official Suite)
```bash
bash test/run_gnu_test.sh builtins.tests -v
```
**ผลลัพธ์:**
```text
================================================================================
⚡ ZEPHYR DIFFERENTIAL TEST HARNESS: GNU BASH OFFICIAL TEST SUITE (~52 SUITES, 2,500+ ASSERTIONS)
   Target Engine  : /Volumes/Mac Backup/zero-tools/zbash/bin/zbash
   Golden Baseline: /bin/bash (GNU bash, version 3.2.57(1)-release (arm64-apple-darwin26))
   Total Tests    : 1
================================================================================
   🎯 Filter Selected: builtins.tests

[#01/01] PASS 🏆 builtins.tests            ( 250 lines | bash:   48.66ms | zbash:   49.76ms | ⚡ 0.98x)

================================================================================
📊 GNU BASH OFFICIAL TEST SUITE: SCOREBOARD SUMMARY
--------------------------------------------------------------------------------
  • ทั้งหมดที่รัน (Executed)     : 1 / 1 ชุด
  • ผ่านสมบูรณ์ (Passed)        : 1 ชุด
  • ไม่ผ่าน (Failed)            : 0 ชุด
  • อัตราความถูกต้อง (Parity Rate): 100.0%
================================================================================
🎉 ALL GNU TESTS PASSED 100% BIT-FOR-BIT EXACT MATCH! 💎
```

### 4.2 การทดสอบ Regression รวม 7 ชุดแรก
```bash
bash test/run_gnu_test.sh --limit 7
```
**ผลลัพธ์:**
- `alias.tests`: **PASS 🏆 (100.0%)**
- `appendop.tests`: **PASS 🏆 (100.0%)**
- `arith-for.tests`: **PASS 🏆 (100.0%)**
- `arith.tests`: **PASS 🏆 (100.0%)**
- `array.tests`: **PASS 🏆 (100.0%)**
- `braces.tests`: **PASS 🏆 (100.0%)**
- `builtins.tests`: **PASS 🏆 (100.0%)**
- **รวม 7 / 7 ชุด: PASS 100.0% Bit-for-Bit Exact Match!**

### 4.3 การทดสอบชุดภายใน Internal Test Suite
```bash
bash test/test_zbash.sh
```
**ผลลัพธ์:**
- `Testing zbash ... [ 100 / 100 PASS (100.0%) ]`
