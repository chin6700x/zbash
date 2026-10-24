/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-042: Brace Expansion Engine, Backtick Pipe Shielding, Lone Backslash Stripping, & Readonly Assignment Timing
> **หมวดหมู่:** การขยายวงเล็บปีกกา การแยกคำคำสั่งไพป์ไลน์ และลำดับการกำหนดแอตทริบิวต์ตัวแปร (Brace Expansion, Backtick Shielding, Lexer & Builtin Variable Attributes)  
> **ระดับความรุนแรง:** สูงมาก (Critical - 100% Bit-for-bit exact parity in official GNU Bash `braces.tests`, 75 lines, plus regression parity across `appendop.tests` & `array.tests`)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/parser/braces.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/braces.c), [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c), [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันชุดทดสอบอย่างเป็นทางการของ GNU Bash ลำดับที่ 6 `braces.tests` (ขนาด 75 บรรทัด) และตรวจสอบผลกระทบต่อเนื่อง (Regression):
```bash
bash test/run_gnu_test.sh braces.tests -v
```

พบปัญหาทางสถาปัตยกรรม 5 ประเดื่นหลัก:

1. **การขาดหายไปของ Brace Expansion Engine ใน ZBASH:**
   - คำสั่งรูปแบบนิพจน์ช่วง `{start..end}` ทั้งจำนวนเต็มบวก/ลบ (`{1..10}`, `{10..1}`, `{-1..-10}`, `{-20..0}`) และตัวอักษร ASCII ทั้งเดินหน้าและถอยหลัง (`{a..f}`, `{f..a}`, `{a..A}`, `{A..a}`) รวมทั้งนิพจน์เครื่องหมายจุลภาคแบบผลคูณคาร์ทีเซียน (`a{1,2}b{3,4}`, `{{0..10},braces}`) ไม่ได้รับการขยายและคงรูปเป็นข้อความดิบ
2. **เครื่องหมาย Pipe (`|`) ภายใน Backticks ชนกับการตัด Pipeline ขั้นแรก:**
   - ในคำสั่ง `echo XXXX\{`echo a b c | tr ' ' ','`\}`, ตัวสแกน Pipeline ใน `parse_pipeline` และ `scan_word` ตัดแบ่งคำสั่งที่ `|` ภายใน Backticks ทันที ทำให้ตัวคำสั่ง `echo` หลักถูกฉีกขาดและกระบวนการพัง
3. **การถอดเครื่องหมาย Backslash เดี่ยว (Lone Backslash Quote Removal):**
   - ในผลลัพธ์ของ `{a..A}` และ `{A..a}` ซึ่งมีอักขระ ASCII ย้อนกลับผ่านช่วงเครื่องหมายขีดล่างและแบ็กสแลช `\` มาตรฐาน Bash กำหนดให้ลบ Unquoted Backslash ออกขณะขยายคำสั่ง แต่ ZBASH เดิมคงอักขระ `\` ไว้ทำให้ผลลัพธ์ต่างไป 1 ไบต์
4. **ลำดับการตั้งสถานะ Readonly ใน Builtin Declare/Readonly:**
   - ใน `readonly x+=7` หากตัวแปร `x` มีแอตทริบิวต์ Integer (`-i`) เดิม ZBASH บันทึกแอตทริบิวต์ Readonly ก่อนการคำนวณบวกค่า ทำให้ `zbash_assign_var` ปฏิเสธการคำนวณและแจ้งเตือน `x: readonly variable` ก่อนที่ค่า `x` จะถูกปรับเป็น 16
5. **การตรวจสอบ Unexpected Token สำหรับ Compound Array Assignment:**
   - ใน `printf "%s\n" -a a=(a 'b  c')` ไวยากรณ์ `=(` ถูก Preserve โดยไม่ได้ตรวจสอบว่าคำสั่งแรกคือ Declaration Builtin หรือไม่ ทำให้หลุดไปเป็นอาร์กิวเมนต์ของ `printf` แทนที่จะแจ้งเตือน `syntax error near unexpected token '('` (Exit code 2)

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **Brace Expansion Ordering in Shell Expansion Pipeline:**
   ตามข้อกำหนดของ Bash Brace Expansion จะต้องเกิดขึ้น **ก่อน** Parameter Expansion, Command Substitution, และ Arithmetic Expansion และจะต้องรักษาสถานะอักขระหลบหนี (`\{`, `\}`, `\,`) รวมทั้งต้องไม่ทำงานภายใน Single Quotes หรือ Double Quotes
2. **Backtick Subshell Scanning Collisions in Lexer:**
   ในระบบดั้งเดิมของ ZBASH ตัวแยกคำและตัวแบ่งไปป์ไลน์คำนวณเฉพาะ Quote (`'`, `"`) และวงเล็บ `$()` แต่ไม่ได้ข้ามส่วนของ Backticks (`` `...` ``) ส่งผลให้อักขระพิเศษเช่น `|`, `;`, `&&`, `||` ภายในแบ็กทิกไปกระตุ้นการแยกคำสั่งในระดับนอกสุด
3. **Attribute Mutation vs Variable Assignment Order:**
   ในฟังก์ชัน `builtin_declare` การกำหนด `zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY)` เกิดขึ้นทันทีก่อนบล็อก `if (eq)` เมื่อเข้าสู่ `zbash_assign_var` ฟังก์ชันตรวจสอบพบว่าตัวแปรมีสถานะ Readonly อยู่แล้ว จึงปฏิเสธการกำหนดค่าทันที ทั้งที่ความจริงคำสั่ง `readonly VAR=VAL` หรือ `readonly VAR+=VAL` ต้องทำหน้าที่กำหนดค่าให้เสร็จสิ้นก่อน แล้วจึงล็อกตัวแปรเป็น Readonly ในขั้นตอนสุดท้าย

---

## 3. การสืบสวนเชิงลึกและการแก้ไข (Investigation & Implementation)

### 3.1 สร้าง Brace Expansion Engine สมบูรณ์แบบ ([`src/parser/braces.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/braces.c))
- พัฒนาฟังก์ชัน `zbash_expand_braces` รองรับ:
  - Sequence Expansion `{start..end}`: คำนวณช่วงตัวเลข (`sscanf`) ทั้งบวก/ลบ เดินหน้าและถอยหลัง และช่วงตัวอักษร ASCII (`a-z`, `A-Z`) พร้อมทั้งตรวจสอบ Type mismatch (หากผสมเลขกับตัวอักษรให้คงค่าเดิมไว้)
  - Comma Expansion `{a,b,...}`: ติดตาม Nesting Depth, ข้าม Quotes และ Subshells, คำนวณผลคูณคาร์ทีเซียนแบบย้อนกลับ (Recursive Cartesian Expansion)
  - การจัดการ Escapes: รองรับ `\{`, `\}`, `\,` และคงรูปวงเล็บว่าง `{}` หรือ `{ }` ไว้อย่างถูกต้อง

### 3.2 ป้องกันการชนของ Backtick Command Substitution ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c))
- เพิ่มการติดตาม Backtick Depth ใน `scan_word`, `parse_pipeline`, `zbash_parse_line`, และตัวแยกคำสั่งแบบมีเงื่อนไข (`&&`, `||`, `;`) เพื่อให้เครื่องหมายไปป์และเซมิโคลอนภายในแบ็กทิกได้รับการปกป้องอย่างสมบูรณ์

### 3.3 ถอด Unquoted Lone Backslash ในการขยายคำ ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c))
- ในฟังก์ชัน `zbash_expand_vars` ปรับปรุงให้ถอด Lone Backslash ที่ไม่ได้รับการ Quote ออกจากสตริงผลลัพธ์ สอดคล้องกับพฤติกรรม Quote Removal ของ Bash

### 3.4 ปรับลำดับการล็อก Readonly ใน `builtin_declare` ([`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c))
- ย้ายการเรียก `zbash_set_var_attr(state, name, ZBASH_ATTR_READONLY)` ไปอยู่ *หลัง* การกำหนดค่าใน `zbash_assign_var` หรือ Array assignment เพื่อให้การกำหนดค่าเริ่มต้นและการบวกเพิ่ม (`+=`) บนตัวแปรที่เพิ่งถูกประกาศเป็น Readonly ดำเนินการจนเสร็จสิ้นก่อนการล็อก

### 3.5 ย้ายการตรวจสอบ Unexpected Token ก่อน Compound Array Preservation ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c))
- ตรวจสอบว่าคำสั่งเป็น Declaration Builtin หรือไม่ก่อน หากคำสั่งเป็น Builtin ทั่วไป เช่น `printf` และพบอาร์กิวเมนต์ `=(` หรือ `+=(` ที่ไม่ได้ใส่เครื่องหมายคำพูด ให้ส่งคืน `syntax error near unexpected token '('` (รหัส 2) ทันที

---

## 4. ผลการทดสอบและการพิสูจน์ (Verification & Proof)

1. **GNU Official `braces.tests`:**
   ```
   [#01/01] PASS 🏆 braces.tests ( 75 lines | bash: 19.03ms | zbash: 17.84ms | ⚡ 1.07x FASTER )
   100.0% Parity Rate | 0 diff bytes!
   ```
2. **GNU Official Regression Sweep (Limit 6):**
   ```
   [#01/06] PASS 🏆 alias.tests      (  37 lines | ⚡ 1.18x )
   [#02/06] PASS 🏆 appendop.tests   (  83 lines | ⚡ 1.39x )
   [#03/06] PASS 🏆 arith-for.tests  (  94 lines | ⚡ 1.13x )
   [#04/06] PASS 🏆 arith.tests      ( 284 lines | ⚡ 0.68x )
   [#05/06] PASS 🏆 array.tests      ( 405 lines | ⚡ 0.99x )
   [#06/06] PASS 🏆 braces.tests     (  75 lines | ⚡ 0.89x )
   GRAND TOTAL: 6 / 6 PASSED (100.0% Exact Match) | ⚡ 1.03x FASTER
   ```
3. **Internal Differential Parity Suite:**
   ```
   Testing zbash ... [ 100 / 100 PASS (100.0%) ] (627.8 ms)
   ```
4. **Clang Clean Compilation:**
   ```
   -Wall -Wextra: 0 errors, 0 warnings.
   ```
