/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-039: Arithmetic For-Loop Whitespace/Empty Conditions, Quoted Sub-Expressions, Braced Loops, & Type Builtin Function Unparser
> **หมวดหมู่:** โครงสร้างควบคุมและประเมินคณิตศาสตร์ (Control Structures, Arithmetic Evaluation & Builtins)  
> **ระดับความรุนแรง:** สูง (High - Bit-for-bit divergence in official GNU Bash test suites)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/arith.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/arith.c), [`src/core/control.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/control.c), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันการทดสอบอย่างเป็นทางการของ GNU Bash ชุดที่ 3 `arith-for.tests`:
```bash
bash test/run_gnu_test.sh arith-for.tests
```
พบว่าเกิดข้อผิดพลาดหลายประการ:
1. **ลูปเงื่อนไขว่างใน `for (( ; ; ))` หรือ `for ((i=0; ; i++))` หยุดทำงานทันที:**
   - เมื่อเงื่อนไขเป็นช่องว่างหรือว่างเปล่า เช่น `for (( ; i < 3; i++ ))` หรือ `for ((i = 0; ;i++ ))` ลูปถูกตัดจบก่อนเริ่มรอบ หรือทำงานไม่ครบ
2. **เงื่อนไขคณิตศาสตร์ที่ครอบด้วยเครื่องหมายคำพูดคู่/เดี่ยว (`for (( i=0; "i < 3" ; i++ ))`):**
   - ตัวประเมินคณิตศาสตร์คืนค่า 0 ทำให้ลูปไม่ทำงานเลย
3. **ไวยากรณ์ `for (( ... ))` ที่มี Semicolon ไม่เท่ากับ 2 ตัว:**
   - เช่น `for (( i=0; "i < 3" ))` (1 ตัว) และ `for (( i=0; i < 3; i++; 7 ))` (3 ตัว) ระบบเดิมไม่รายงาน Syntax Error และไม่คืน Exit Code 2
4. **ลูปแบบใช้ปีกกา `{ ... }` แทน `do ... done` เช่น `for ((i=0; i < 20; i++)) { : ; }`:**
   - ระบบค้างหรืออ่านคำสั่งไม่รู้จบ เนื่องจาก `is_statement_incomplete()` คอยมองหาเฉพาะคำว่า `done` เท่านั้น
5. **คำสั่งในตัว `type fx`:**
   - รายงาน `zbash: type: fx: not found` แทนที่จะ Unparse และจัดรูปแบบ Function AST ออกมาอย่างประณีต

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **POSIX / C For-Loop Invariance:**
   ในภาษา C และ Bash ไวยากรณ์ `for (init; cond; step)` หากไม่ระบุ `cond` หรือระบุเป็นช่องว่าง จะต้องถือว่าเงื่อนไขเป็น True (1) เสมอ แต่ใน ZBASH เดิม โค้ดเพียงตรวจสอบ `if (cond_str && *cond_str)` เมื่อมีช่องว่าง `" "` จึงถูกส่งเข้า `zbash_eval_arithmetic()` ซึ่งตีความเป็น 0 และสั่ง `break` ทันที
2. **Bash Quoted Arithmetic Sub-Expressions:**
   ในระบบ Bash ดั้งเดิม ไวยากรณ์ใน `$(( ... ))` และ `(( ... ))` อนุญาตให้นำสตริงที่ครอบด้วย Single Quote หรือ Double Quote มาคำนวณซ้อนได้ โดยระบบต้องถอด Quote แล้วประเมินนิพจน์ภายในแบบ Recursive
3. **Strict Loop Validation:**
   ใน Bash `for (( ... ))` บังคับต้องมี Semicolon นอกเครื่องหมายคำพูดเท่ากับ 2 ตัวเท่านั้น หากน้อยกว่า 2 ตัวเป็น `syntax error: arithmetic expression required` และหากมากกว่า 2 ตัวเป็น `syntax error: ';' unexpected` พร้อม Exit Code 2
4. **Braced Loop Grammar vs `do ... done`:**
   นับตั้งแต่ Bash 2.04 มีการรองรับ `for (( ... )) { ... }` นอกเหนือจาก `do ... done` ทำให้การตรวจจับคำสั่งไม่สมบูรณ์จะรอคอยเฉพาะโทเค็น `done` ไม่ได้ หากพบบล็อก `{` ต้องรอการปิดด้วย `}`
5. **Function AST Pretty Printing in `type`:**
   ใน Bash ดั้งเดิม คำสั่ง `type <func>` ไม่ได้พิมพ์ข้อความดิบ แต่เป็นการเดิน AST และจัดหน้า Indentation 4 spaces พร้อมเติม Semicolon `;` และแปลงช่องว่างของนิพจน์ `for ((` เป็น `1;` ตามมาตรฐาน AST

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

1. ใน `src/core/control.c` ฟังก์ชัน `exec_for_c()` ตรวจสอบเงื่อนไขแบบหยาบ:
   ```c
   if (cond_str && *cond_str) {
       long long cval = zbash_eval_arithmetic(state, cond_str);
       if (cval == 0) break;
   }
   ```
   ทำให้สตริงที่เป็นช่องว่าง `" "` ถูกประเมินเป็น `0` และหลุดลูปทันที
2. ใน `src/core/arith.c` ฟังก์ชัน `eval_primary()` ไม่ได้ตรวจจับอักขระ `"` หรือ `'` ทำให้สตริงกลายเป็นค่าที่ไม่ถูกรับรู้และคืนค่า 0
3. ใน `src/core/engine.c` ฟังก์ชัน `is_statement_incomplete()` นับเฉพาะ `loop_d` ที่เปิดด้วย `for` และปิดด้วย `done` เท่านั้น เมื่อเจอลูปปีกกา `for ((...)) { : ; }` ตัวนับ `loop_d` ไม่เคยลดลง ทำให้สะสมบรรทัดต่อไปเรื่อย ๆ
4. ใน `src/builtins/builtins.c` ฟังก์ชัน `builtin_type()` ตรวจสอบเฉพาะ Builtin และ `$PATH` แต่ไม่เคยเรียกดูฟังก์ชันในเชลล์ผ่าน `zbash_find_func()`

---

## 4. แผนงานและการแก้ไข (Remediation & Fix)

### 4.1 ตรวจสอบช่องว่างในเงื่อนไข For-Loop (`src/core/control.c`)
สร้างผู้ช่วย `is_str_empty_or_ws()` และตรวจสอบก่อนประเมินนิพจน์:
```c
if (cond_str && !is_str_empty_or_ws(cond_str)) {
    long long cval = zbash_eval_arithmetic(state, cond_str);
    if (cval == 0) break;
}
```

### 4.2 ตรวจสอบ Semicolon และความถูกต้องของไวยากรณ์ For-Loop
นับจำนวน Semicolon นอกเครื่องหมายคำพูด หาก `< 2` รายงาน `syntax error: arithmetic expression required` และหาก `> 2` รายงาน `syntax error: ';' unexpected` พร้อมคืนค่า Exit Code 2

### 4.3 รองรับทั้ง `do ... done` และ `{ ... }` ใน For-Loop
รองรับการเริ่มบล็อกด้วย `{` และจบด้วย `}` พร้อมปรับปรุง `is_statement_incomplete()` ใน `src/core/engine.c` ให้ติดตาม `loop_needs_body` และ `do_d` ควบคู่กับ `brace_d`

### 4.4 รองรับ Quoted Expressions ในตัวประเมินคณิตศาสตร์ (`src/core/arith.c`)
ใน `eval_primary()`:
```c
if (**p == '"' || **p == '\'') {
    char q = **p; (*p)++;
    const char *qstart = *p;
    while (**p && **p != q) (*p)++;
    size_t qlen = *p - qstart;
    if (**p == q) (*p)++;
    char qbuf[2048];
    if (qlen >= sizeof(qbuf)) qlen = sizeof(qbuf) - 1;
    strncpy(qbuf, qstart, qlen);
    qbuf[qlen] = '\0';
    const char *qp = qbuf;
    return eval_expr(state, &qp);
}
```

### 4.5 ยกระดับคำสั่ง `type` ใน `src/builtins/builtins.c`
- รองรับค้นหา Functions, Keywords, Aliases, Builtins, และ PATH Binaries
- รองรับแฟล็ก `-t`, `-p`, `-a`, `-f`
- สร้าง `print_func_body()` สำหรับ Unparse นิพจน์ฟังก์ชันออกมาตรงตามมาตรฐาน Bash 100%

---

## 5. ผลการทดสอบยืนยัน (Verification & Proof)

1. รันการทดสอบ GNU Bash `arith-for.tests`:
   ```bash
   bash test/run_gnu_test.sh arith-for.tests
   ```
   **ผลลัพธ์:**
   ```
   [#01/01] PASS 🏆 arith-for.tests  ( 94 lines | bash: 26.43ms | zbash: 10.62ms | ⚡ 2.49x)
   🎉 ALL GNU TESTS PASSED 100% BIT-FOR-BIT EXACT MATCH! 💎
   ```
2. รันการทดสอบถอยกลับ (Regression Check) บนชุดทดสอบก่อนหน้า:
   - `alias.tests`: 100% PASS 🏆 (⚡ 1.15x)
   - `appendop.tests`: 100% PASS 🏆 (⚡ 0.99x)
   - `arith-for.tests`: 100% PASS 🏆 (⚡ 2.49x)
3. รันชุดทดสอบความถูกต้องระดับระบบ (`test_zbash.sh`):
   - `[ 100 / 100 PASS (100.0%) ]` ผ่านสมบูรณ์ทุกข้อ

---

## 6. ข้อคิดและกฎเหล็กสำหรับสถาปัตยกรรม (Architectural Lessons)

> **"A shell control flow parser must not make assumptions about trailing delimiters: syntax trees can terminate via matching delimiters or closing keywords alike."**

1. ลูปคณิตศาสตร์แบบ C-style ในเชลล์ ต้องปฏิบัติตามมาตรฐาน ANSI C เสมอ เมื่อละเว้นเงื่อนไขจะถือว่าเป็นจริงเสมอ
2. การแยกคำสั่งหลายบรรทัด (Line Continuation & Incomplete Statements) ต้องไม่ผูกติดเฉพาะคำสำคัญ `done` แต่ต้องคำนึงถึงบล็อกปีกกา `{ ... }` เสมอ
3. คำสั่งสำรวจประเภทคำสั่ง (`type`, `command -v`) คือหนึ่งในหัวใจสำคัญที่สุดของการทำงานร่วมกับสคริปต์ POSIX/Bash ระบบต้องสามารถระบุและแสดงผลข้อมูลฟังก์ชันได้อย่างแม่นยำ
