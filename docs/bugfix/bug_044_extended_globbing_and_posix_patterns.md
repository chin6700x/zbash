/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-044: Extended Globbing, POSIX Collating Symbols, Bracket Matching, and Case Clause Nesting Parity
> **หมวดหมู่:** เอนจินการจับคู่รูปแบบ (Pattern Matching Engine), POSIX Extended Globbing (`extglob`), สัญลักษณ์เทียบเคียง POSIX และคลาสสมมูล (Collating Symbols & Equivalence Classes), การขยายพาธไฟล์แบบเรียกซ้ำ (Pathname Expansion), และการแจงส่วนตัวคั่นทางเลือกในคำสั่ง `case`  
> **ระดับความรุนแรง:** สูงมาก (Critical - 100% Bit-for-Bit exact parity across official GNU Bash `extglob.tests` 365L, `extglob2.tests` 90L, `extglob3.tests` 56L, `posixpat.tests` 233L — รวม 744 บรรทัด)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/parser/pattern.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/pattern.c), [`src/parser/cmd_parser.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/cmd_parser.c), [`src/core/control.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/control.c), [`src/builtins/builtin_test.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtin_test.c), [`src/parser/parser_internal.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/parser_internal.h), [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันชุดทดสอบอย่างเป็นทางการของ GNU Bash 3.2.57 ในกลุ่ม **Cluster G5: Extended Globbing & POSIX Pattern Matching**:
```bash
bash test/run_gnu_test.sh extglob.tests -v
bash test/run_gnu_test.sh extglob2.tests -v
bash test/run_gnu_test.sh extglob3.tests -v
bash test/run_gnu_test.sh posixpat.tests -v
```

พบข้อขัดแย้งเชิงโครงสร้างและพฤติกรรมที่ไม่ตรงกับ GNU Bash 5 ประการหลัก:

1. **การแตกทางเลือกในคำสั่ง `case` ด้วยเครื่องหมาย `|` ภายใน Extended Globbing:**
   - โค้ดแจงส่วน `case` ดั้งเดิมตัดสตริงทางเลือกด้วย `|` โดยตรวจจับเพียงว่าไม่ได้อยู่ใน Quote แต่ไม่ได้ตรวจสอบความลึกของวงเล็บ `p_depth` ส่งผลให้นิพจน์รูปแบบ เช่น:
     ```bash
     case $var in
       @(a|b)|c) echo match ;;
     esac
     ```
     ถูกตัดแยกเป็น `@(a`, `b)`, และ `c` แทนที่จะเป็น `@(a|b)` และ `c` ส่งผลให้การจับคู่ล้มเหลวและเกิด Syntax Error ทันที
2. **เครื่องหมายปิดวงเล็บ `)` ของแขนง `case` ถูกระงับโดยวงเล็บก้ามปูที่ยังไม่ปิด `[`:**
   - โค้ดเดิมนับ `b_depth` จาก `[` และทำให้เครื่องหมาย `)` ไม่ถูกมองว่าเป็นจุดสิ้นสุดของ Clause Pattern จนกว่าจะพบ `]` ส่งผลให้รูปแบบที่เจตนาจับคู่อักขระ `[` หรือนิพจน์ที่มี `]` หลบด้วย Escape เช่น `[[:alpha:]\])` หรือการใช้งานที่มี `[` ลอยๆ ไม่สามารถปิด Clause ได้
3. **การสูญหายของ Backslash Escapes ใน Case Patterns จาก Variable Expansion:**
   - โค้ดเดิมเรียก `zbash_expand_vars` โดยไม่ตรวจสอบ ทำให้ Backslash ที่ใช้ Escape อักขระพิเศษในรูปแบบ (เช่น `\[`, `\]`, `\*`) ถูกถอดออกก่อนส่งให้ฟังก์ชัน `zbash_fnmatch` ส่งผลให้อักขระพิเศษกลายเป็น Wildcard หรือเปลี่ยนความหมาย
4. **การขาดหายไปของ POSIX Collating Symbols, Equivalence Classes, และ Character Classes:**
   - ใน `posixpat.tests` มีการทดสอบสัญลักษณ์เทียบเคียง POSIX เช่น `[[.hyphen.]]`, `[[.space.]]`, `[[.tab.]]`, `[[.newline.]]`, `[[.grave-accent.]]`, และ Equivalence Classes `[[=c=]]` รวมถึงคลาสอักขระ `[:ascii:]` และ `[:word:]` หากไม่รองรับ จะทำให้การจับคู่นิพจน์ก้ามปูรายงานความผิดพลาด
5. **พฤติกรรม Asterisk ซ้ำหน้า Extended Globbing (`**(`):**
   - ในกรณีรูปแบบเช่น `ab**(e|f)` การยุบรวม `*` ที่ต่อเนื่องกันทำให้ดอกจันตัวที่สองที่สังกัดอยู่กับ `*(` หายไป กลายเป็น `ab*(e|f)` ซึ่งทำลายโครงสร้างการแมตช์ของ Extglob

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **Bash / ksh Extended Globbing Grammatical Precedence:**
   ในเชลล์ตระกูล Bourne ดั้งเดิม เครื่องหมาย `|` ถูกสงวนไว้สำหรับคั่นทางเลือกของคำสั่ง `case` แต่เมื่อ David Korn นำ Extended Globbing (`@(pat|pat)`, `*(pat|pat)`, `+(pat|pat)`, `?(pat|pat)`, `!(pat|pat)`) เข้ามาใน ksh และ GNU Bash นำมาใช้ต่อ เครื่องหมาย `|` ภายในวงเล็บของ Extglob จะต้องไม่ทำหน้าที่เป็นตัวคั่น Clause ของ `case` จึงต้องมีกลไกติดตาม Parenthesis Nesting Depth
2. **POSIX Shell Grammar Rule 4 (Pattern Lists in Case Conditional):**
   ตามมาตรฐาน POSIX.1-2017 หมวด Shell Grammar Rule 4 กำหนดว่า `)` จะเป็นตัวสิ้นสุดรายการรูปแบบของ `case` เสมอ โดยเครื่องหมาย `[` ที่เปิดไว้จะไม่ระงับการทำงานของ `)` เนื่องจากใน POSIX โครงสร้าง Bracket Pattern ภายในชื่อแฟ้มและนิพจน์ไม่ได้ครอบคลุมระดับไวยากรณ์คำสั่งเชลล์
3. **Double Wildcard in Extglob Prefix:**
   ตามกฎของ GNU Bash การยุบเครื่องหมาย `*` ซ้ำ (Consecutive Asterisk Compression) จะทำได้เฉพาะเมื่อ `*` ตัวถัดไปเป็น `*` อิสระ ไม่ใช่ส่วนนำหน้าของ Extglob Pattern Operator `*(`

---

## 3. การสืบสวนเชิงลึกและการแก้ไข (Investigation & Implementation)

### 3.1 การแจงส่วนทางเลือกของ `case` พร้อมตรวจสอบความลึกวงเล็บ (`p_depth`)
แก้ไข [`src/core/control.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/control.c) ให้นับ `p_depth` และไม่ให้ `b_depth` ยับยั้งเครื่องหมายปิด `)`:

```c
/* src/core/control.c: parse_case_pattern_clause */
int p_depth = 0;
while (*p) {
    if (*p == '\\' && *(p + 1)) {
        p += 2;
        continue;
    }
    if (*p == '(') { p_depth++; p++; continue; }
    if (*p == ')') {
        if (p_depth > 0) { p_depth--; p++; continue; }
        break; /* End of pattern list */
    }
    if (*p == '|' && p_depth == 0) {
        /* Valid clause alternative split */
    }
    p++;
}
```

### 3.2 การรักษาสภาพ Backslash ใน Case Patterns
ตรวจสอบก่อนขยายตัวแปร หากไม่มีเครื่องหมาย `$`, `` ` ``, หรือ `~` ให้ส่งรูปแบบตรงไปยัง `zbash_fnmatch` โดยไม่ผ่านการถอด Escape:

```c
/* src/core/control.c */
char *expanded_pattern = pattern;
if (strpbrk(pattern, "$`~")) {
    expanded_pattern = zbash_expand_vars(state, pattern);
}
```

### 3.3 เอนจิน POSIX Collating Elements & Equivalence Classes
สร้างฟังก์ชัน `get_collating_elem` ใน [`src/parser/pattern.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/pattern.c) เพื่อถอดรหัส Collating Symbol Aliases และ Equivalence Classes:

```c
/* src/parser/pattern.c */
static int get_collating_elem(const char *s, const char **endptr) {
    /* Support POSIX collating symbol aliases:
     * "hyphen" -> '-', "space" -> ' ', "tab" -> '\t',
     * "newline" -> '\n', "grave-accent" -> '`'
     * Single character symbols [.c.] -> c
     * Equivalence classes [=c=] -> c
     */
}
```

พร้อมรองรับ Character Classes เพิ่มเติม `[:ascii:]` และ `[:word:]` ในฟังก์ชัน `brackmatch` และบังคับปิด Bracket Expression อย่างเคร่งครัด

### 3.4 การป้องกันการยุบ `*` หน้า Extglob `*(`
ในฟังก์ชัน `gmatch` ตรวจสอบไม่ให้ยุบ `*` หากตัวถัดไปคือ `*(`:

```c
/* src/parser/pattern.c */
while (*p == '*') {
    if (*(p + 1) == '(') break;
    p++;
}
```

---

## 4. ผลการทดสอบยืนยันผล (Regression Test Results)

รันชุดทดสอบความเข้ากันได้ GNU Bash ทั้ง 4 ชุด:

```bash
bash test/run_gnu_test.sh extglob.tests
# [#01/01] PASS 🏆 extglob.tests  (365 lines | ⚡ 1.05x FASTER)

bash test/run_gnu_test.sh extglob2.tests
# [#01/01] PASS 🏆 extglob2.tests ( 90 lines | ⚡ 0.95x FASTER)

bash test/run_gnu_test.sh extglob3.tests
# [#01/01] PASS 🏆 extglob3.tests ( 56 lines | ⚡ 1.03x FASTER)

bash test/run_gnu_test.sh posixpat.tests
# [#01/01] PASS 🏆 posixpat.tests (233 lines | ⚡ 1.08x FASTER)
```

รันการทดสอบภายในระบบ:
```bash
bash test/test_zbash.sh
# Testing zbash ... [ 100 / 100 PASS (100.0%) ]
```

---

## 5. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **Context-Sensitive Delimiters:**
   ตัวคั่นในภาษาเชลล์ (เช่น `|`, `)`, `;`) ไม่สามารถตัดแบ่งได้ด้วยการสแกนแบบ Flat Lexer บริบทเชิงลึกของ Parentheses ซ้อนจาก Extended Globbing มีผลต่อลำดับความสำคัญของไวยากรณ์โครงสร้างควบคุมเสมอ
2. **Backslash Preservation Lifecycle:**
   การถอด Backslash ต้องกระทำในขั้นตอนสุดท้าย (Quote Removal) ห้ามถอดล่วงหน้าระหว่างขั้นตอนประเมินรูปแบบ เพราะ Backslash เป็นตัวกำหนดความหมายระหว่าง Literal Character กับ Wildcard ในการจับคู่ Pattern
