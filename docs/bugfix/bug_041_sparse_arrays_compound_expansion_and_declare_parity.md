/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-041: Sparse Array Compound Expansion, Nested Parameter Lexing, & Array-Scalar Assignment Parity
> **หมวดหมู่:** การจัดการอาร์เรย์ ไวยากรณ์เชลล์ และการขยายพารามิเตอร์ (Sparse Arrays, Compound Assignment, Parameter Expansion & Lexer Parsing)  
> **ระดับความรุนแรง:** สูงมาก (Critical - 100% Bit-for-bit exact parity divergence in official GNU Bash `array.tests`, 405 lines)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/variables.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/variables.c), [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c), [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c), [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันชุดทดสอบอย่างเป็นทางการของ GNU Bash ลำดับที่ 5 `array.tests` (ขนาด 405 บรรทัด):
```bash
bash test/run_gnu_test.sh array.tests
```
เกิดข้อผิดพลาดหลายประการที่ทำให้ผลลัพธ์ไม่ตรงกับ `/bin/bash` ของ macOS:

1. **การสแกนวงเล็บปีกกาซ้อน `${ ... }` สิ้นสุดก่อนเวลาอันควร:**
   - ในนิพจน์ที่มีอาร์เรย์และวงเล็บซ้อน เช่น `${_ENV[(_$-=0)+(_=1)-_${-%%*i*}]}`, ตัวสแกนหยุดที่อักขระ `}` แรก ส่งผลให้สตริงที่เหลือ `]}` หลุดออกมาในเอาต์พุต
2. **ลำดับการสืบค้นตัวแปรอาร์เรย์ในวงเล็บปีกกา (`${VAR}`):**
   - คำสั่ง `${x}` อ่านค่าจาก Environment (`getenv`) ก่อนอาร์เรย์ ทำให้หากมี Environment ชื่อเดียวกันตกค้าง จะไม่คืนค่าสมาชิกช่อง 0 (`x[0]`) ตามพฤติกรรมมาตรฐานของ Bash
3. **การกำหนดค่าสเกลาร์ลงบนตัวแปรอาร์เรย์ (`x=val`, `x+=val`, `read x`):**
   - เมื่อตัวแปรถูกประกาศเป็นอาร์เรย์ไว้แล้ว การกำหนดค่าสเกลาร์ไม่ปรับปรุงช่อง `x[0]` แต่กลับไปเขียนทับตัวแปร Environment ทำให้ข้อมูลในโครงสร้างอาร์เรย์เพี้ยน
4. **เครื่องหมายคำพูดใน Compound Assignment ถูกทำลายก่อนกำหนด:**
   - การกำหนดค่าเช่น `x=(')' $$)` ถูกขยายตัวแปรและถอด Quotes ในขั้นตอน `zbash_parse_simple_cmd` ก่อนส่งเข้า `check_array_def_ext` ส่งผลให้ single quote หายไปกลายเป็น `x=) <pid>)` ซึ่งทำลายโครงสร้างวงเล็บ
5. **การนับความยาว Slice ของอาร์เรย์ (`${arr[@]:offset:len}`):**
   - เมื่อสมาชิกในหน้าต่าง Slice มีค่าว่าง `""` เอนจินเดิมข้ามการนับจำนวน ทำให้ขนาดของ Slice หดตัวลงเกินจริง
6. **การตรวจจับ Syntax Error ในการส่ง `=(` ให้กับคำสั่งทั่วไป:**
   - ใน Bash คำสั่งที่ไม่ใช่ Declaration Builtin เช่น `test =(1 2)` จะต้องเกิดข้อผิดพลาด `syntax error near unexpected token '('` ทันที แต่ ZBASH เดิมยอมรับคำสั่งนี้เป็นอาร์กิวเมนต์ธรรมดา
7. **โหมด Nounset (`set -u`) สำหรับการนับความยาวสมาชิกอาร์เรย์:**
   - คำสั่ง `${#arr[idx]}` ในโหมด `set -u` ต้องแจ้งเตือน unbound variable หากช่องดังกล่าวยังไม่ได้ถูกกำหนดค่า

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **Compound Assignment Lexing Phase Ordering:**
   ตามแบบฉบับดั้งเดิมของ Bash การกำหนดค่าอาร์เรย์แบบ Compound `arr=(a b c)` เป็น Special Lexical Form ซึ่งเนื้อหาภายในวงเล็บจะต้องไม่ถูกทำ Word Splitting หรือ Quote Removal จนกว่าจะเข้าสู่กระบวนการแจกแจงสมาชิกรายตัว การที่ ZBASH เดิมรัน `zbash_expand_vars` ในระดับคำสั่งทำให้บริบทของ Quote ถูกลบหายไป
2. **SIMD Lexer Scan Boundary Collisions:**
   เอนจินสแกนคำเร็วพิเศษ `zbash_simd_scan_token` มุ่งเป้าข้ามอักขระที่ไม่ใช่อักขระควบคุม แต่กระโดดข้ามเครื่องหมาย `=`, `(`, และ `)` ทำให้ตัวแยกคำมองเห็น `a=(1 2)` เป็นก้อนเดียวและไม่ดักจับการเริ่มต้นของ Compound Array
3. **Array vs Scalar Symmetry in POSIX/Bash:**
   Bash ปฏิบัติต่อตัวแปรอาร์เรย์และตัวแปรเดี่ยวแบบผสมผสาน: การอ้างอิง `$a` หรือ `${a}` จะมีค่าเทียบเท่ากับ `${a[0]}` เสมอ และในทางกลับกัน การกำหนดค่า `a=value` หรือ `read a` บนตัวแปรที่เป็นอาร์เรย์แล้ว จะต้องเทียบเท่ากับการกำหนดค่า `a[0]=value` เสมอ
4. **Slice Index Window vs Empty Word Discarding:**
   ใน `${arr[@]:offset:len}` ค่า `len` หมายถึงจำนวนช่องสมาชิกที่นับจากตำแหน่ง `offset` โดยนับรวมทุกช่องที่มีอยู่ แม้ค่าจะเป็นสตริงว่าง การตัดสตริงว่าง (Empty Dropping) เป็นกระบวนการของ Word Splitting ที่เกิดขึ้น *หลังจาก* เลือกสมาชิกใน Slice เสร็จสิ้นแล้ว

---

## 3. การสืบสวนเชิงลึกและการแก้ไข (Investigation & Implementation)

### 3.1 Nested Parameter Expansion Scanning & Bracket Depth ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c))
- ยกระดับ `zbash_expand_vars` โดยเพิ่มตัวนับความลึก `b_depth` พร้อมทั้งติดตามสถานะคำพูด `in_s` (Single Quote) และ `in_d` (Double Quote)
- ป้องกันการปิดปีกกา `${ ... }` ก่อนกำหนดเมื่อมีวงเล็บปีกกาหรือวงเล็บเหลี่ยมซ้อน
- ปรับปรุงการค้นหาวงเล็บเหลี่ยมของอาร์เรย์ให้ติดตาม `bk_d` (Bracket Depth) เพื่อให้ปิดคู่กับ `]` ตัวนอกสุดเสมอ

### 3.2 Array Precedence & Scalar Mutation ([`src/core/variables.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/variables.c))
- ใน `zbash_assign_var`: หากตัวแปรเป้าหมายเป็นอาร์เรย์อยู่แล้ว ให้แปลงการกำหนดค่าสเกลาร์ (`x=val` หรือ `x+=val`) เป็นการอัปเดตสมาชิกช่องที่ 0 (`elements[0]`) พร้อมประเมินค่าทางคณิตศาสตร์หากมีแอตทริบิวต์ Integer (`-i`)
- ใน `check_array_def_ext`: เมื่อเป็นการกำหนดค่าแบบใหม่ (`!is_append`) ให้เคลียร์ค่า Environment (`unsetenv(arr_name)`) ควบคู่กับการรีเซ็ตอาร์เรย์ เพื่อป้องกันไม่ให้ค่าเก่ารั่วไหลเข้าสู่ช่องที่ 0
- ใน `assign_read_var` ([`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c)): เปลี่ยนจากการเรียก `setenv` ดิบๆ มาใช้ `zbash_assign_var` เพื่อรองรับตัวแปรอาร์เรย์อย่างสมบูรณ์

### 3.3 Compound Array Token Preservation & Parsing ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c) & [`src/core/variables.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/variables.c))
- ปรับปรุง `scan_word` ให้คงสภาพวงเล็บของ Compound Assignment `=( ... )` ข้ามบรรทัดและเว้นวรรค
- จำกัดขอบเขตของ `zbash_simd_scan_token` ไม่ให้ข้ามอักขระ `=`, `(`, และ `)`
- ใน `zbash_parse_simple_cmd` ละเว้นการเรียก `zbash_expand_vars` ล่วงหน้าบนโทเค็นที่เป็นการกำหนดค่าอาร์เรย์ คงสภาพ Quotes และเนื้อหาภายในวงเล็บไว้อย่างสมบูรณ์เพื่อให้ `check_array_def_ext` จัดการขยายทีละสมาชิก
- รองรับการทำ Pathname Globbing (`*`, `?`), การคัดลอกอาร์เรย์ (`"${src[@]}"`, `${src[@]}`), และดัชนีคีย์ (`"${!src[@]}"`)
- แยกแยะนิพจน์คณิตศาสตร์ที่ไม่ใช่อาร์เรย์ เช่น `a=(4*3)/2` ออกจากการกำหนดค่าอาร์เรย์

### 3.4 Slice Element Counting ([`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c))
- ย้ายตำแหน่ง `collected++` ให้อยู่ก่อนหน้าการตรวจสอบสตริงว่าง `arr->elements[a][0] == '\0'` ในการแยกคำแบบ Unquoted เพื่อให้ความยาวของ Slice Window คงที่ตามจำนวนองค์ประกอบจริง

### 3.5 Nounset & Syntax Errors ([`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c), [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c))
- เพิ่มฟิลด์ `bool nounset;` ในโครงสร้าง `zbash_state_t` และรองรับแฟล็ก `-u` / `+u` ใน `builtin_set`
- ตรวจสอบและแสดงข้อผิดพลาด `unbound variable` เมื่อเข้าถึง `${#arr[idx]}` ที่ไม่มีอยู่จริง
- ดักจับโทเค็น `=(` ในคำสั่งที่ไม่ใช่ Builtin ประกาศตัวแปร พร้อมตั้งสถานะ `syntax error near unexpected token '('` (Exit Code 2)
- ดักจับกรณีชื่อตัวแปรว่างในการประกาศอาร์เรย์ เช่น `declare -a ''=(...)`

---

## 4. ผลการทดสอบยืนยัน (Verification & Parity Proof)

1. **GNU Official Test Suite Parity:**
   ```bash
   bash test/run_gnu_test.sh array.tests
   ```
   **ผลลัพธ์:**
   ```
   ======================================================================
   🏆 สรุปผลการทดสอบ GNU BASH TEST SUITE
   ======================================================================
     ไฟล์ที่รัน:          1 ชุด
     ผ่าน:               1 / 1 (100.0%)
     ไม่ผ่าน:            0 / 1
     เวลาทำงานรวม:        127.72 ms
   ----------------------------------------------------------------------
     ความเร็วเฉลี่ย:       ⚡ 1.05x FASTER (bash: 65.52ms vs zbash: 62.20ms)
     ความแม่นยำ:         100% Bit-for-Bit Exact Match (0 diff bytes)
   ======================================================================
   ```

2. **Diff Comparison:**
   ```bash
   cmp /tmp/bash_arr.txt /tmp/zbash_arr.txt
   # Exit code: 0 (No difference)
   ```

3. **Regression Test Suite:**
   ```bash
   bash test/test_zbash.sh
   # 100 / 100 PASS (100.0%)
   ```

---

## 5. บทเรียนและมาตรฐานการพัฒนา (Engineering Guidelines)

- **Token Protection in Multi-Stage Parsers:** โทเค็นที่มีรูปแบบไวยากรณ์ซับซ้อน เช่น Compound Array Assignment ต้องได้รับการปกป้องไม่ให้ผ่านการขยายค่า (Pre-expansion) ในขั้นตอนระดับบน (Command Parsing Level) แต่ต้องถูกส่งผ่านไปขยายในระดับย่อย (Element Level) เท่านั้น
- **SIMD Safe Scanning:** ฟังก์ชันสแกนหน่วยความจำด้วยเวกเตอร์ NEON ต้องระบุอักขระตัดคำ (Delimiters) ให้ครอบคลุมทุกเครื่องหมายที่ส่งผลต่อการแบ่งโครงสร้างไวยากรณ์เชลล์
