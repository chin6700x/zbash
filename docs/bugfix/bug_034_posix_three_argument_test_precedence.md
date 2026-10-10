# 🪲 BUG-034: POSIX 3-Argument Test Binary Operator Precedence (`[ ! = esac ]`)
> **หมวดหมู่:** การประเมินนิพจน์ตรวจสอบ (Test Builtin & Conditional Operators)  
> **ระดับความรุนแรง:** ปานกลาง (Medium - Incompatible POSIX Test Evaluation)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ในชุดทดสอบ `DARWIN-SH/parser/case1.0` และ `builtins/case1.0` มีคำสั่งทดสอบเปรียบเทียบข้อความที่ดูประหลาดแต่ถูกต้องตามมาตรฐาน POSIX ดังนี้:

```bash
[ ! = esac ]
echo $?
```

* **ผลลัพธ์บน macOS `/bin/bash`:** ได้ผลลัพธ์เป็น `1` (False)
* **ผลลัพธ์บน `zbash` ก่อนแก้ไข:** ได้ผลลัพธ์เป็น `0` (True) หรือเกิด Syntax Error

สาเหตุเพราะ `zbash` มองเห็นเครื่องหมาย `!` ที่ตำแหน่งแรก จึงตีความว่าเป็น Unary Negation (การกลับค่าความจริง) แล้วพยายามประเมิน `= esac` ทำให้ได้ผลลัพธ์ที่ผิดพลาดโดยสิ้นเชิง

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในมาตรฐาน **POSIX.1-2017 (IEEE Std 1003.1-2017) หมวดคำสั่ง `test` / `[`** ได้ระบุกฎเกณฑ์ตายตัวสำหรับจำนวนอาร์กิวเมนต์ 3 ตัวไว้ชัดเจนว่า:

> *"3 arguments:*  
> *• If $2 is any binary primary, then the result is the result of the binary test of $1 and $3.*  
> *• If $1 is '!', then the result is the negation of the two-argument test of $2 and $3.*  
> *• If $1 is '(' and $3 is ')', then the result is the one-argument test of $2."*

ลำดับความสำคัญ (Precedence) บังคับว่า **หากอาร์กิวเมนต์ตัวที่ 2 เป็น Binary Operator (เช่น `=`, `!=`, `-eq`, `-lt` ฯลฯ) เครื่องหมาย `!` ตัวแรกจะต้องถูกปฏิบัติเป็น "สตริงตัวอักษรธรรมดา (String Operand)" ทันที** ห้ามถือว่าเป็นตัวดำเนินการกลับค่าความจริงเด็ดขาด! การที่ `!` ทำหน้าที่ได้ทั้งตัวดำเนินการและสตริงข้อความ คือหนึ่งในความคลุมเครือที่มีอายุยาวนาน 40 ปีของ Unix

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อสืบค้นฟังก์ชัน `eval_test_expr()` และ `eval_simple_test()` ใน [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c):
โค้ดเดิมตรวจสอบ `if (strcmp(argv[0], "!") == 0)` เป็นลำดับแรกสุดเสมอ ทำให้เมื่อส่ง `[ ! = esac ]` เข้ามา (`argc == 3`, `argv[0] == "!"`, `argv[1] == "="`, `argv[2] == "esac"`) โค้ดจะตัด `!` ออกไป แล้วส่ง `= esac` เข้าไปประเมิน ซึ่งทำลายความถูกต้องของ Binary Comparison ทันที

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

เพิ่มตรรกะตรวจสอบ POSIX 3-Argument Binary Primary Rule ก่อนการตรวจสอบ Unary `!` ใน [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c):

```c
static bool is_binary_test_op(const char *op) {
    if (!op) return false;
    return (strcmp(op, "=") == 0 || strcmp(op, "==") == 0 ||
            strcmp(op, "!=") == 0 || strcmp(op, "-eq") == 0 ||
            strcmp(op, "-ne") == 0 || strcmp(op, "-lt") == 0 ||
            strcmp(op, "-le") == 0 || strcmp(op, "-gt") == 0 ||
            strcmp(op, "-ge") == 0 || strcmp(op, "-nt") == 0 ||
            strcmp(op, "-ot") == 0 || strcmp(op, "-ef") == 0);
}

static int eval_test_expr(zbash_state_t *state, int argc, char **argv) {
    if (argc == 0) return 1;

    /* POSIX 3-Argument Rule: Binary primary takes precedence over unary '!' */
    if (argc == 3 && is_binary_test_op(argv[1])) {
        return eval_binary_test(state, argv[0], argv[1], argv[2]) ? 0 : 1;
    }

    /* Unary negation */
    if (strcmp(argv[0], "!") == 0) {
        return (eval_test_expr(state, argc - 1, argv + 1) == 0) ? 1 : 0;
    }
    ...
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

รันคำสั่งยืนยันความถูกต้องระหว่าง `/bin/bash` และ `./bin/zbash`:

```bash
./bin/zbash -c '[ ! = esac ]; echo $?'
# Output: 1 (ตรงกับ /bin/bash 100%)

./bin/zbash -c '[ ! = ! ]; echo $?'
# Output: 0 (ตรงกับ /bin/bash 100%)

bash test/run_apple_sh.sh parser/case1.0
# Output: PASS 🏆 parser/case1.0 (100% Exact Match)
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

ในภาษา C หรือระบบแปลภาษาทั่วไป Operator Precedence มักจะเป็นแบบคงที่ แต่ในมาตรฐาน POSIX Shell คำว่า `!` มีความกำกวมระหว่าง Token ประเภท Keyword/Operator กับ Word ธรรมดา ผู้สร้างเชลล์ยุคใหม่ต้องยึดตาราง POSIX N-Argument Rules (1, 2, 3, 4 arguments) เป็น First-Class Execution Path ก่อนที่จะใช้ Recursive Descent Parser ทั่วไป
