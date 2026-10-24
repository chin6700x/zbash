# 🪲 BUG-035: Keyword Collision & Semicolon Splitting in Case Statements (`case ... in ... esac`)
> **หมวดหมู่:** การแจงส่วนคำสั่งแบบหลายบรรทัด (Lexer, Parser & Statement Boundary)  
> **ระดับความรุนแรง:** ร้ายแรง (Critical - Parser Premature Truncation & Shattered Patterns)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อเขียนคำสั่ง `case` ที่มีคำว่า `esac` หรือ `in` ปรากฏเป็นคำทดสอบ หรือคำสั่ง `case` ที่มีตัวคั่น `;;` ในไฟล์สคริปต์:

```bash
# PoC 1: ใช้คำว่า esac เป็นคำทดสอบ
case esac in
    esac) echo "MATCHED_ESAC" ;;
    *) echo "FAIL" ;;
esac

# PoC 2: case หลายบรรทัดที่มี ;;
case "$val" in
    apple) echo "A" ;;
    banana) echo "B" ;;
esac
```

* **อาการที่พบ:**
  1. ตัวตรวจจับความสมบูรณ์ของคำสั่ง (`is_statement_incomplete`) มองเห็นคำว่า `esac` ในบรรทัดแรก (`case esac in`) และเข้าใจผิดว่าจบลูป `case` แล้ว ทำให้ Parser ตัดทิ้งคำสั่งที่เหลือ
  2. ตัวแยกคำสั่งด้วยเซมิโคลอนใน `zbash_run_line` ตัดแบ่งคำสั่งที่เครื่องหมาย `;;` ทำให้บล็อกของ `case` ถูกแยกชิ้นส่วนจนเกิด Syntax Error

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในไวยากรณ์ของ Bourne Shell และ Bash คำสำคัญ `case` มีโครงสร้างไวยากรณ์พิเศษกว่า `if` หรือ `while`:
1. คำว่า `in` หลังนิพจน์ทดสอบเป็น Reserved Word บังคับ
2. คำว่า `esac` คือตัวปิดบล็อก แต่คำว่า `esac` ก็สามารถเป็นสตริงอาร์กิวเมนต์ตัวทดสอบได้ เช่น `case esac in` หรือเป็น Pattern ตัวจับคู่ได้ เช่น `esac)`
3. ตัวคั่นรูปแบบของ `case` ใช้เครื่องหมายคู่ `;;` ซึ่งขัดกับตัวคั่นคำสั่ง `;` ทั่วไปของ Shell

หากตัวตัดคำ (Lexer) ใช้วิธีจับคู่สตริงแบบไร้บริบท (Context-insensitive token scan) จะทำให้เกิดการหลงทิศทันที

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

1. ใน `is_statement_incomplete()` ของ [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c):
   ตัวนับความลึก `case_d` ถูกลดค่าทันทีเมื่อพบคำว่า `esac` โดยไม่ได้ตรวจสอบว่า `case` นั้นพบคำว่า `in` หรือยัง (`case_needs_in`)
2. ใน `is_newline_separator()` และส่วนแบ่งเซมิโคลอน:
   ไม่ได้ละเว้นเครื่องหมาย `;;` ขณะที่ตัวแปรสถานะ `case_d > 0`

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

เพิ่มสถานะ `case_needs_in` และระงับการตัดเซมิโคลอนภายในบล็อก `case`:

```c
/* ใน is_statement_incomplete() */
if (is_word_match(p, "case", 4)) {
    case_d++;
    case_needs_in = true;
} else if (case_needs_in && is_word_match(p, "in", 2)) {
    case_needs_in = false;
} else if (is_word_match(p, "esac", 4)) {
    if (!case_needs_in && case_d > 0) {
        case_d--;
    }
}
```

พร้อมทั้งสร้างเอนจิน `exec_case()` ที่รองรับ Pattern Matching ด้วย `fnmatch()`, รองรับตัวเลือกสลับ `|` (Alternation), และคืนค่า Exit Code 0 เมื่อไม่มี Pattern ใดตรงตามมาตรฐาน POSIX

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'case esac in esac) echo OK ;; *) echo FAIL ;; esac'
# Output: OK (ตรงกับ /bin/bash 100%)

bash test/run_apple_sh.sh builtins/case1.0
# Output: PASS 🏆 builtins/case1.0 (100% Exact Match)
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

คำสำคัญของ Shell ไม่ใช่ Lexical Tokens สถิต แต่ขึ้นอยู่กับบริบททางไวยากรณ์ (Grammar Context) การทำ Tokenizer ต้องมี State Machine ขนาดเล็กสำหรับคำสำคัญคู่ เช่น `case ... in ... esac` เพื่อป้องกันการเข้าใจผิดเมื่อคำสำคัญนั้นถูกส่งเป็น Data Payload
