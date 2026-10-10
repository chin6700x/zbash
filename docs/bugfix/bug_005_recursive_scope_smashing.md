# 🪲 BUG-005: Recursive Function Local Scope Smashing
> **หมวดหมู่:** ขอบเขตของตัวแปรและโครงสร้างหน่วยความจำ (Variable Scoping & Stack)  
> **ระดับความรุนแรง:** สูง (High - Silent Data Corruption in Recursive Algorithms)  
> **ไฟล์ที่เกี่ยวข้อง:** [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h) และ [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อเขียนฟังก์ชันที่มีการเรียกตัวเองซ้ำ (Recursion) เช่น การคำนวณ Factorial:

```bash
fact() {
    local n=$1
    if [ "$n" -le 1 ]; then
        echo 1
        return
    fi
    local prev=$(fact $(( n - 1 )))
    echo $(( n * prev ))
}

fact 4
```

**ผลลัพธ์ที่ผิดพลาด:**
ผลลัพธ์ที่ได้เพี้ยนไปอย่างสิ้นเชิง (เช่น ได้ 1 หรือได้ 2) แทนที่จะเป็น `24`!

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในมาตรฐานดั้งเดิมของ Bourne Shell (sh 1979) **ไม่มีแนวคิดเรื่องตัวแปร Local** ตัวแปรทุกตัวในระบบมีสถานะเป็น Global Variables ทั้งสิ้น!

เมื่อ GNU Bash นำคีย์เวิร์ด `local` เข้ามาใช้ หากเอนจินเขียนแบบมักง่ายโดยการนำตัวแปรไปยัดใส่ฟังก์ชันมาตรฐานของ C อย่าง `setenv(name, val, 1)` หรือเก็บใน Flat Hash Table ธรรมดา:
เมื่อฟังก์ชันชั้นนอกสุดทำงาน `local n=4` ตัวแปร `n` มีค่าเป็น 4  
แต่เมื่อฟังก์ชันชั้นในถูกเรียกจนถึงจุดฐาน `local n=1` ค่า `n=1` ในชั้นในสุดจะ **เขียนทับ (Clobber)** ค่า `n=4` ของชั้นนอกอย่างถาวร! เมื่อฟังก์ชันคลายตัวกลับมา ค่า `n` ในชั้นนอกจึงกลายเป็น 1 ไปด้วย

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อพิมพ์ค่า `n` ก่อนและหลังคำสั่ง `prev=$(fact $(( n - 1 )))`:
```text
Entering fact with n=4
Entering fact with n=3
Entering fact with n=2
Entering fact with n=1
Returning from fact: n was clobbered to 1 (Expected 4)!
```
ค่าตัวแปรใน Stack Frame ของ Caller ถูกเขียนทับโดย Callee เนื่องจากไม่มีกลไก Call Stack ที่แท้จริง

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ใน [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h) เราสร้าง **Call Scope Stack**:

```c
typedef struct {
    char *name;
    char *old_val;
    bool was_set;
} zbash_local_var_t;

struct zbash_state {
    ...
    zbash_local_var_t local_vars[512];
    int local_var_count;
};
```

และใน [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) เมื่อเข้าและออกจากฟังก์ชัน:

```c
/* บันทึกจุดมาร์กเกอร์ของ Call Frame ก่อนเริ่มฟังก์ชัน */
int saved_mark = state->local_var_count;

/* รันคำสั่งในตัวฟังก์ชัน... ทุกครั้งที่เจอ local ให้ push ค่าเดิมลงสแตก */

/* เมื่อออกจากฟังก์ชัน ทำการแกะสแตกถอยหลัง (Unwind LIFO) */
while (state->local_var_count > saved_mark) {
    state->local_var_count--;
    zbash_local_var_t *entry = &state->local_vars[state->local_var_count];
    if (entry->was_set) {
        setenv(entry->name, entry->old_val, 1);
        free(entry->old_val);
    } else {
        unsetenv(entry->name);
    }
    free(entry->name);
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'fact() { local n=$1; [ "$n" -le 1 ] && echo 1 && return; local p=$(fact $((n-1))); echo $((n*p)); }; fact 5'
# ผลลัพธ์: 120 ถูกต้องสมบูรณ์แบบ 100%
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **Activation Records:** Shell จะต้องปฏิบัติต่อฟังก์ชันเสมือนภาษาโปรแกรมระดับสูง (C/Rust) ที่มี Activation Records แยกกันในแต่ละระดับชั้นของการเรียก
2. **LIFO Stack Unwinding:** การเก็บ Snapshot ของค่าเดิมและกู้คืนแบบ Last-In First-Out คือหัวใจของการรักษา Scope Integrity
