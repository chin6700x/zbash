# 🪲 BUG-036: The Complete POSIX "set -e" (errexit) Suppression Hierarchy in Conditionals & AND-OR Lists
> **หมวดหมู่:** ตรรกะการควบคุมข้อผิดพลาด (Error Handling & errexit Semantics)  
> **ระดับความรุนแรง:** ร้ายแรงสูงสุด (Critical - False Process Termination in Scripts)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ในชุดทดสอบ `DARWIN-SH/set-e/` สคริปต์พื้นฐานหลายตัวพังและหยุดทำงานก่อนกำหนด เช่น:

```bash
set -e

# เคสที่ 1: คำสั่งเงื่อนไข while/until/if ที่เป็น false ทำให้เชลล์หลุดทันที!
while false; do
    :
done
echo "STILL_ALIVE"

# เคสที่ 2: ฝั่งซ้ายของ && หรือ || ที่ล้มเหลว ทำให้เชลล์หลุดทันที!
false && true
exit 0

# เคสที่ 3: คำสั่งที่ถูกกลับค่าด้วย !
! false
echo "OK"
```

* **ผลลัพธ์บน macOS `/bin/bash`:** ทำงานจนจบ บรรทัด `STILL_ALIVE` พิมพ์ออกมาปกติ, Exit code เป็น `0`
* **ผลลัพธ์บน `zbash` ก่อนแก้ไข:** เชลล์หยุดทำงานทันทีที่พบ `false` ตัวแรก ส่งคืน Exit code `1` ทำลายการทำงานของสคริปต์

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในมาตรฐาน **POSIX.1-2017 (Shell & Utilities - set -e)** ระบุข้อยกเว้นของการหลุดการทำงานของ `set -e` ไว้อย่างชัดเจน 4 กรณีหลัก:

1. **Compound List Conditions:** คำสั่งที่เป็นเงื่อนไขทดสอบของ `if`, `elif`, `while`, หรือ `until` **จะต้องไม่ส่งผลให้เชลล์ออกจากโปรแกรมเด็ดขาด** เมื่อคืนค่าไม่เป็นศูนย์
2. **Inverted Pipelines (`!`):** คำสั่งใดๆ ที่นำหน้าด้วยเครื่องหมายกลับค่า `!` จะต้องไม่ทำให้เชลล์ออกจากโปรแกรม
3. **Non-Final Operands of AND-OR Lists (`&&` และ `||`):** คำสั่งใดๆ ที่อยู่ฝั่งซ้ายของตัวดำเนินการ `&&` หรือ `||` (ทุกคำสั่งที่ไม่ใช่ Pipeline สุดท้ายของชุด AND-OR) จะต้องถูกยกเว้นจาก `set -e`
4. **Execution Inheritance:** หากฟังก์ชันหรือ Subshell ถูกเรียกภายในบริบทที่ `set -e` ถูกระงับ คำสั่งทั้งหมดภายในฟังก์ชันหรือ Subshell นั้นจะต้องถูกระงับ `set -e` ด้วยเช่นกัน

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อสืบค้นการทำงานใน [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c):
1. ใน `exec_if` และ `exec_while`: ตัวประเมินเงื่อนไขเรียก `zbash_run_line(state, cond)` โดยปล่อยให้ `state->errexit` ยังคงเป็น `true` ทำให้เมื่อ `false` รัน คำสั่งตั้งค่า `state->should_exit = true` ทันที
2. ใน `has_cond` (ลูปประเมิน `&&` และ `||`): ลูปแยกตัดคำสั่งทีละท่อนแล้วเรียก `zbash_run_line(state, start)` โดยไม่ได้ปิด `state->errexit` ในคำสั่งที่ `op != 3` (คำสั่งที่ไม่ใช่คำสั่งสุดท้าย) ทำให้ฝั่งซ้ายที่คืนค่าผิดพลาดตัดจบการทำงานของเชลล์ทั้งไฟล์
3. ใน Subshell `( ... )`, Brace Group `{ ... }`, และ Arithmetic `(( ... ))`: โค้ดส่งคืนค่า Exit status ตรงๆ โดยไม่ได้ตรวจสอบ `if (state->errexit && rc != 0)` เพื่อเซ็ต `state->should_exit` สำหรับเคสที่ควรจะหลุดจริงๆ

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

นำสถาปัตยกรรม **POSIX Errexit Suppression Hierarchy** มาใช้ครอบคลุมทุกจุดประเมินใน [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c):

### 4.1 ระงับใน `exec_if` และ `exec_while`:
```c
bool saved_errexit = state->errexit;
state->errexit = false;
int cond_rc = zbash_run_line(state, cond_buf);
state->errexit = saved_errexit;
state->should_exit = false; /* ป้องกันสถานะตกค้าง */
```

### 4.2 ระงับใน Inverted Command (`!`):
```c
bool saved_invert_e = state->errexit;
if (invert) state->errexit = false;
...
if (invert) {
    state->errexit = saved_invert_e;
    state->should_exit = false;
}
```

### 4.3 ระงับในนิพจน์ AND-OR Lists (`has_cond`):
```c
if (!skip_next) {
    bool suppress_e = (op != 3); /* op 3 คือ Pipeline สุดท้าย */
    bool saved_e = state->errexit;
    if (suppress_e) state->errexit = false;

    zbash_run_line(state, start);

    if (suppress_e) {
        state->errexit = saved_e;
        state->should_exit = false;
    }
    if (state->should_exit) break;
}
```

### 4.4 บังคับส่งสัญญาณออกใน Subshell & Brace เมื่อล้มเหลวจริง:
```c
if (state->errexit && state->last_status != 0) {
    state->should_exit = true;
    state->exit_code = state->last_status;
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

รันชุดทดสอบทางการ Apple Darwin หมวด `set-e` ครบทั้ง 35 ข้อ:

```bash
bash test/run_apple_sh.sh set-e
```

**ผลลัพธ์หลังแก้ไข:**
```
================================================================================
📊 APPLE DARWIN POSIX SH: SCOREBOARD SUMMARY
--------------------------------------------------------------------------------
  • ทั้งหมดที่รัน (Executed)     : 35 / 35 ข้อ
  • ผ่านสมบูรณ์ (Passed)        : 35 ข้อ (100.0% PASS 🏆)
  • ไม่ผ่าน (Failed)            : 0 ข้อ
  • เวลารวม /bin/bash          : 360.13 ms
  • เวลารวม ./bin/zbash         : 379.10 ms
================================================================================
🎉 ALL TESTS PASSED 100% BIT-FOR-BIT DARWIN POSIX SH PARITY! 💎
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

`set -e` คือฟีเจอร์ที่ซับซ้อนและมีข้อยกเว้นมากที่สุดในระบบเชลล์ การตรวจสอบ `errexit` ต้องไม่ได้ทำแบบทื่อๆ ที่ปลายทางของคำสั่งเดี่ยว แต่ต้องประเมินผ่าน **Execution Context Frame Stack** ที่รับรู้ว่าคำสั่งปัจจุบันกำลังถูกรันในฐานะ "เงื่อนไขทดสอบ" หรือ "ส่วนหนึ่งของ AND-OR List" หรือไม่
