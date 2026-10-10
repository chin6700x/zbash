# 🪲 BUG-021: The Infamous "set -e" (errexit) Subshell & Conditional Madness
> **หมวดหมู่:** ตรรกะการควบคุมข้อผิดพลาด (Error Handling & errexit Traps)  
> **ระดับความรุนแรง:** ร้ายแรง (Critical - Unexpected Pipeline Abort / Inconsistent Semantics)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

นี่คือหนึ่งในปริศนาที่โด่งดังที่สุดในประวัติศาสตร์ 40 ปีของ Bash:

```bash
set -e

# เคสที่ 1: คำสั่งล้มเหลวในเงื่อนไข if -> เชลล์ไม่หลุด (ถูกต้อง)
if false; then
    echo "NO"
fi

# เคสที่ 2: ฟังก์ชันที่มีคำสั่งล้มเหลว ถูกเรียกใน if -> พฤติกรรมต่างกันในแต่ละเวอร์ชัน!
my_func() {
    false
    echo "FUNC_DONE"
}
if my_func; then
    echo "IF_SUCCESS"
fi
```

ใน GNU Bash เวอร์ชันดั้งเดิม (Bash 3.2 บน macOS) กับ Bash 4.x/5.x มีพฤติกรรมเรื่อง `set -e` ภายใน Subshells, Functions, และ Command Substitutions ที่เปลี่ยนแปลงไปมาไม่ต่ำกว่า 5 ครั้งในรอบ 20 ปี จนโปรแกรมเมอร์ทั่วโลกขนานนามว่า **"The errexit Nightmare"**

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

POSIX ระบุไว้คลุมเครือว่า:  
*"เชลล์จะต้องออกจากโปรแกรมทันทีหากคำสั่งใดคืนค่าสถานะไม่เป็นศูนย์ ยกเว้นคำสั่งนั้นจะเป็นส่วนหนึ่งของเงื่อนไข `while`, `until`, `if`, หรือส่วนหนึ่งของตัวดำเนินการ `&&` หรือ `||`"*

แต่ปัญหาคือ:
1. หากคำสั่งนั้นอยู่ใน **Function** ที่ถูกเรียกภายในเงื่อนไข `if` ตัว `set -e` ภายใน Function นั้นควรจะถูกปิดกั้น (Disabled) หรือไม่?
2. หากคำสั่งนั้นอยู่ใน **Pipeline** เช่น `false | true` ในโหมดปกติเชลล์จะไม่หยุด แต่ถ้าเปิด `set -o pipefail` เชลล์จะหยุดทันที
3. ภายใน **Subshell `( ... )`** หากคำสั่งใน Subshell คืนค่าผิดพลาด Subshell ควรจะ Exit ทันที และส่งผลให้ Parent Exit ตามหรือไม่?

ความคลุมเครือนี้ทำให้สคริปต์อัตโนมัติหลายล้านสคริปต์ในโลกต้องเจอกับอาการ "สคริปต์หยุดทำงานกลางคันอย่างไร้สาเหตุ"

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อเปรียบเทียบพฤติกรรมระหว่าง macOS `/bin/bash` (v3.2.57) กับ ZBASH:
- ใน GNU Bash 3.2 เมื่ออยู่ในเงื่อนไข `if` หรือด้านซ้ายของ `||`: แฟล็ก `errexit` จะถูก **ระงับการทำงานชั่วคราว (Temporarily Suppressed)** ไม่เพียงแต่สำหรับคำสั่งนั้น แต่รวมถึงทุกคำสั่งย่อยที่อยู่ลึกลงไปใน Subshell หรือ Function Call ด้วย!

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ใน [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) เราได้สร้างกลไก **Errexit Suppression Context**:

```c
/* โครงสร้างควบคุมสถานะการดักจับข้อผิดพลาด */
bool old_suppress = state->suppress_errexit;

if (is_conditional_context) {
    /* ระงับการทำงานของ set -e ชั่วคราวเมื่ออยู่ใน if, while, ||, && */
    state->suppress_errexit = true;
}

int status = zbash_execute_command(cmd);

/* คืนสถานะการระงับกลับมา */
state->suppress_errexit = old_suppress;

/* ตรวจสอบว่าจะต้อง Abort โปรเซสหรือไม่ */
if (state->set_errexit && !state->suppress_errexit && status != 0) {
    /* หยุดการทำงานของสคริปต์ตามกฎ set -e */
    state->should_exit = true;
    return status;
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'set -e; if false; then echo 1; else echo 2; fi; echo "STILL_ALIVE"'
# ผลลัพธ์:
# 2
# STILL_ALIVE (สคริปต์ไม่ดับวูบกลางคัน)
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **Explicit Error Scoping:** อย่ามอง `set -e` เป็นตัวแปร Global บูลีนเดี่ยวๆ แต่ต้องมองเป็น Scope Stack ที่ถูกผลักและดึง (Push/Pop) ตามระดับของ Control Flow
2. **Defensive POSIX Compatibility:** พฤติกรรมของ `set -e` เป็นเรื่องที่มีการโต้เถียงกันมากที่สุดในโลก Unix หากทำตาม POSIX อย่างแข็งทื่อ สคริปต์ของโลกจริงจะพังพินาศ ต้องมี Suppression Context เสมอ
