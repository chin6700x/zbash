/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-040: Arithmetic Short-Circuit Side-Effects, Radix 2-64 Encoding, Consecutive Operators, & Nested Subshell Disambiguation
> **หมวดหมู่:** การประเมินคณิตศาสตร์และไวยากรณ์เชลล์ (Arithmetic Evaluation, Number Parsing & Shell Grammar)  
> **ระดับความรุนแรง:** สูง (High - 100% Bit-for-bit exact parity divergence in official GNU Bash `arith.tests`)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/arith.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/arith.c), [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`src/core/exec.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/exec.c), [`src/core/control.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/control.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันการทดสอบอย่างเป็นทางการของ GNU Bash ไฟล์ที่ 4 `arith.tests` (ขนาด 284 บรรทัด):
```bash
bash test/run_gnu_test.sh arith.tests
```
พบว่าเกิดความคลาดเคลื่อนทางไวยากรณ์และการประเมินผลหลายจุด:
1. **Side-effects ในสาขาที่ถูก Short-circuit ทำงานผิดพลาด:**
   - ตัวอย่าง: `echo $(( 1 ? 20 : (x+=2) ))` ค่าตัวแปร `x` ถูกบวกเพิ่ม ทั้งที่เงื่อนไขเป็นจริง (สาขาเท็จไม่ควรประเมิน)
   - นิพจน์ตรรกะ `0 && (y=5)` หรือ `1 || (z=9)` ตัวแปรถูกเปลี่ยนแปลงค่าเช่นเดียวกัน
2. **ระบบตัวเลขต่างฐาน `base#number` ไม่รองรับฐาน 37 ถึง 64:**
   - ตัวเลขฐาน 64 เช่น `64#a`, `64#A`, `64#@`, `64#_` คืนค่าผิดพลาดหรือล้มเหลว
   - ตัวเลขฐานที่ไม่มีค่าตามหลัง เช่น `echo $(( 2# ))` ทำให้เกิด Syntax Error แทนที่จะประเมินเป็น `0` ตามข้อกำหนดของ Bash
3. **เครื่องหมายคณิตศาสตร์ติดกัน 3 ตัว (`4+++a`, `4---a`):**
   - ตัวแยกโทเค็นมองเป็น Postfix บนค่าคงที่ตัวเลข `4++` แล้วรายงานข้อผิดพลาด แทนที่จะแยกเป็น Binary `+` ตามด้วย Prefix `++` (คือ `4 + (++a)`)
4. **เครื่องหมายดอลลาร์ที่ถูก Escape (`\$iv`):**
   - ใน `$(( \$iv ))` สตริงที่ส่งเข้าคำนวณมีเครื่องหมาย `$` นำหน้าตัวแปร ระบบเดิมพยายามอ่านตัวแปร `$iv` สำเร็จ แทนที่จะเกิด Error `syntax error: operand expected (error token is "$iv")`
5. **การขยายตัวแปรพิเศษในวงเล็บปีกกา (`${$}`):**
   - นิพจน์ `${$} - $$` คืนค่า Process ID แทนที่จะเป็น `0` เนื่องจากตัวแปลงข้าม `${$}` ไป
6. **ตัวเลขต่างฐาน `2#44` ถูกตัดทอนเป็น Comment ในตัวนับบรรทัดต่อเนื่อง:**
   - ฟังก์ชัน `is_statement_incomplete()` ตรวจพบอักขระ `#` ใน `2#44` แล้วตัดสตริงทิ้งเป็น Comment ทำให้วงเล็บปิด `))` ไม่ถูกนับ เชลล์จึงค้างอ่านบรรทัดถัดไป
7. **ความสับสนระหว่าง Subshell ซ้อนกับคำสั่งคณิตศาสตร์:**
   - คำสั่ง `(( (echo a); (echo b) ))` เริ่มต้นด้วย `((` ทำให้ระบบเข้าใจผิดว่าเป็น Arithmetic Command `(( expr ))` แทนที่จะเป็น Nested Subshell

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **Short-Circuiting Evaluation Invariance in C vs Bash:**
   ในภาษา C ตัวดำเนินการ `? :`, `&&`, และ `||` มีกฎเหล็กว่านิพจน์ในฝั่งที่ไม่ถูกเลือกจะต้องไม่ถูกประเมิน (Unevaluated Expression) แต่ในเอนจิน `arith.c` เดิม ไม่มีการส่งผ่านสถานะ `active` (Active Branch Context) ลงไปใน Recursive-Descent ทุกระดับ ทำให้ `eval_assign()` และ `get_var_val()` ยังคงทำงานและเขียนค่าลง Memory
2. **Bash Radix 64 Encoding Specification:**
   Bash กำหนดให้ฐานตัวเลขตั้งแต่ 2 ถึง 36 ไม่แยกพิมพ์เล็ก-พิมพ์ใหญ่ (Case-Insensitive) แต่ฐาน 37 ถึง 64 บังคับ Case-Sensitive โดยเรียงลำดับ: `0-9` (0-9), `a-z` (10-35), `A-Z` (36-61), `@` (62), และ `_` (63) หากไม่มีการแยกความแตกต่างนี้ การคำนวณฐานสูงจะเพี้ยนทันที
3. **Lexical Disambiguation of Operator Sequences:**
   ลำดับอักขระ `+++` เกิดขึ้นได้เมื่อตัวดำเนินการบวกทวิภาคตามด้วยตัวดำเนินการบวกเอกภาค (`+ ++`) การสแกนตัวเลขตั้งแต่แรกจำเป็นต้องหยุดเมื่อพบ `+` หรือ `-` ตัวแรกหากตามด้วยเครื่องหมายซ้ำ
4. **Comment Word Boundary Detection:**
   ตามมาตรฐาน POSIX อักขระ `#` จะถือเป็นจุดเริ่มต้นของ Comment ก็ต่อเมื่ออยู่ต้นบรรทัด หรือนำหน้าด้วยช่องว่าง (Whitespace) หรือเครื่องหมาย `;` เท่านั้น การตรวจสอบ `#` โดดๆ โดยไม่ดูบริบทคำข้างหน้าทำให้โทเค็นตัวเลขต่างฐานพังทลาย

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

1. ใน `src/core/arith.c`:
   - ฟังก์ชัน Recursive-Descent ทุกตัว (`eval_expr`, `eval_ternary`, `eval_log_or`, `eval_log_and`, `eval_assign`, `eval_primary`) รับเพียง `(state, p)` ทำให้เมื่อเรียกสาขาที่ไม่แอ็กทีฟ โค้ดยังคงสั่ง `zbash_set_var_value()` และกลายพันธุ์ตัวแปร
2. ใน `parse_number()`:
   - ตรวจสอบ `base <= 36` และใช้ `tolower()` กับทุกตัวอักษร ทำให้ค่าฐาน 37..64 ผิดเพี้ยน และเมื่อพบคู่สตริง `2#` แล้วจบสตริง ฟังก์ชันคืนค่าล้มเหลว
3. ใน `src/core/engine.c`:
   - ฟังก์ชัน `is_statement_incomplete()` มีลูปตรวจ `#`:
     ```c
     if (buf[i] == '#' && !in_sq && !in_dq) {
         break; // ตัดข้อความที่เหลือทั้งหมดเป็น comment!
     }
     ```
     ส่งผลให้ `echo $(( 2#44 ))` ถูกตัดที่ `#44 ))` เหลือเพียง `echo $(( 2` และค้างสถานะรอวงเล็บปิด
4. ใน `src/parser/lexer.c`:
   - `expand_parameters_in_word()` ไม่รู้จัก `${$}` ในเคสปีกกา ทำให้ไม่ถูกแทนที่ด้วย Process ID

---

## 4. แผนงานและการแก้ไข (Remediation & Fix)

### 4.1 ขยายและส่งผ่าน `bool active` ทั่วนิพจน์คณิตศาสตร์ (`src/core/arith.c`)
ปรับโครงสร้างฟังก์ชันใน Recursive-Descent ให้รับ `bool active`:
- `eval_ternary()`: ประเมินเงื่อนไข ถ้าเป็นจริง ประเมิน `expr1` ด้วย `active` และ `expr2` ด้วย `false`
- `eval_log_or()` / `eval_log_and()`: ทำ Short-Circuit ตามค่าความจริงฝั่งซ้าย
- `eval_assign()`: หาก `!active` ให้อ่านโครงสร้างไวยากรณ์ข้ามไปโดยไม่บันทึกค่าลงตัวแปร
- `get_var_val()`: หาก `!active` ให้คืนค่า 0 ทันที ไม่แจ้ง Warning หรือ Error ใดๆ

### 4.2 ระบบถอดรหัสเลขต่างฐาน 2..64 ตามมาตรฐาน Bash
```c
if (base >= 2 && base <= 36) {
    char c = tolower((unsigned char)**p);
    if (c >= '0' && c <= '9') digit = c - '0';
    else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
} else if (base >= 37 && base <= 64) {
    char c = **p;
    if (c >= '0' && c <= '9') digit = c - '0';
    else if (c >= 'a' && c <= 'z') digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z') digit = c - 'A' + 36;
    else if (c == '@') digit = 62;
    else if (c == '_') digit = 63;
}
```
หากหลังเครื่องหมาย `#` ไม่มีตัวเลข ให้ถือว่ามีค่าเป็น 0

### 4.3 จัดการเครื่องหมายติดกัน `4+++a` และ `4---a`
ใน `eval_add_sub()`: เมื่อพบ `+` หรือ `-` ให้ตรวจสอบอักขระถัดไป หากเป็น `++` หรือ `--` ให้กินเฉพาะตัวแรกเป็นตัวดำเนินการ Binary แล้วปล่อยให้อีกสองตัวเป็น Prefix Operator ของพจน์ถัดไป

### 4.4 ป้องกัน Comment ทับซ้อนกับ Radix Hash ใน `engine.c`
```c
if (buf[i] == '#' && !in_sq && !in_dq) {
    if (i == 0 || isspace((unsigned char)buf[i - 1]) || buf[i - 1] == ';') {
        break;
    }
}
```

### 4.5 เพิ่มฟังก์ชัน `is_arithmetic_command()`
จำแนกระหว่าง `(( expr ))` และ `((cmd;); cmd)` โดยตรวจสอบว่าภายในวงเล็บชั้นนอกสุดมีคำสั่ง Subshell หรือ Semicolon ซับซ้อนที่บ่งบอกถึง Command List หรือไม่

---

## 5. ผลการทดสอบยืนยัน (Verification & Proof)

```bash
bash test/run_gnu_test.sh arith.tests
```

**ผลลัพธ์:**
```
====================================================================================================
🎯 GNU-TEST PARITY RUNNER: arith.tests
====================================================================================================
[1/1] arith.tests (284 lines)... 🏆 PASS (0.015s vs 0.019s | ⚡ 1.27x FASTER | 0 diff bytes)
====================================================================================================
📊 GNU-TEST EXECUTION SUMMARY
====================================================================================================
Total Suites Tested : 1
Passed Suites       : 1 (100.0%)
Failed Suites       : 0 (0.0%)
Total Run Time      : 0.035s
Bit-Exact Status    : 100% BIT-FOR-BIT EXACT MATCH WITH MACOS BASH! 💎
====================================================================================================
```

---

## 6. ข้อคิดและกฎเหล็กสำหรับอนาคต (Lessons Learned)
- การคำนวณคณิตศาสตร์ใน Shell ไม่ใช่เพียงการเรียกใช้ `strtoll()` แต่เป็น Full-featured C-like Expression Evaluator ที่มีทั้ง Side-effects, Dynamic Assignment, Array Indirection, และ Short-circuit Logic
- Token Lexing ต้องตระหนักถึง Word Boundaries เสมอ เพื่อไม่ให้สับสนระหว่าง Comment `#` กับ Radix Specifier `base#`
