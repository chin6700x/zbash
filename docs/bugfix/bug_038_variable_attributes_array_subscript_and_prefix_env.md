/*
 * Author: Wirot Chookeaw (chin6700x)
 * Copyright: (c) 2026 Wirot Chookeaw.
 * License: Apache License 2.0
 */

# 🪲 BUG-038: Variable Attributes System, Array Subscript Assignment, & Prefix Environment Restoration
> **หมวดหมู่:** ไวยากรณ์ สตริง และการขยายตัวแปร (Lexing, Variables & Scoping)  
> **ระดับความรุนแรง:** สูง (High - Bit-for-bit divergence in official GNU Bash test suites)  
> **ไฟล์ที่เกี่ยวข้อง:** [`include/zbash.h`](file:///Volumes/Mac%20Backup/zero-tools/zbash/include/zbash.h), [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c), [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันการทดสอบอย่างเป็นทางการของ GNU Bash `appendop.tests`:
```bash
bash test/run_gnu_test.sh appendop.tests
```
พบว่าเกิดข้อผิดพลาดหลายจุด:
1. **การกำหนดค่าก่อนหน้าคำสั่ง (Prefix Assignments):**
   ```bash
   a=14
   a+=5 printenv a
   echo $a
   ```
   - GNU Bash ให้ผลลัพธ์: `5` แล้วตามด้วย `14` (ตัวแปร `$a` ในเชลล์แม่ไม่ถูกแก้ไขอย่างถาวร)
   - ZBASH เดิม: ให้ผลลัพธ์ `145` และ `$a` ถูกเปลี่ยนเป็น `145` ถาวร
2. **ตัวแปรที่มี Attribute เป็นตัวเลข (`typeset -i` / `declare -i`):**
   ```bash
   b=4+1; typeset -i b; b+=37; echo $b
   ```
   - GNU Bash ให้ผลลัพธ์: `42` (คำนวณ `(4+1) + 37` ในระบบคณิตศาสตร์)
   - ZBASH เดิม: ให้ผลลัพธ์ `4+137` (เป็นการต่อสตริงธรรมดา)
3. **การกำหนดค่าระบุ Subscript ในอาร์เรย์ (`x=( 1 2 [2]+=7 4 5 )` และ `x+=( [3]+=9 [5]=9 )`):**
   - ZBASH เดิม: แยกไม่ถูก มอง `[2]+=7` เป็นสตริงธรรมดาของอาร์เรย์
4. **ตัวแปร Readonly (`readonly x+=7` และ `x+=5`):**
   - ZBASH เดิม: ไม่มีการติดตาม Attribute `readonly` ทำให้ยอมให้แก้ไขและคืนค่า Exit Code 0 แทนที่จะรายงาน Error และคืนค่า 1

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **POSIX Environment Scoping สำหรับ Simple Commands:**
   ตามมาตรฐาน POSIX และ Bash ตัวแปรที่กำหนดนำหน้าคำสั่งแบบ `VAR=val cmd` ต้องมีผลเฉพาะต่อ Process ที่ถูกรันเท่านั้น เมื่อคำสั่งนั้นทำงานเสร็จ ค่าของ Environment ในเชลล์หลักต้องถูก Restore กลับคืนสภาพเดิมเสมอ
2. **สถาปัตยกรรม Variable Attributes ของ ksh93 และ Bash:**
   ในระบบดั้งเดิม ตัวแปรในเชลล์ไม่ได้มีเพียง Key-Value เท่านั้น แต่ยังมี Bitmask Flag กำกับ เช่น `ATTR_INTEGER`, `ATTR_READONLY`, `ATTR_EXPORT` หากตัวแปรมี `ATTR_INTEGER` การ Assign ทุกครั้ง (ทั้ง `=` และ `+=`) จะต้องผ่าน Arithmetic Evaluator โดยอัตโนมัติ
3. **Array Subscript Compounds:**
   ในไวยากรณ์ `arr=( [idx]=val [idx]+=val )` Bash อนุญาตให้กระโดดตำแหน่ง Index และทำการบวกค่าสะสมเฉพาะช่องได้

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

1. ใน `execute_single_cmd()` เมื่อพบ `cmd_offset > 0` โค้ดเดิมเพียงแค่เรียก `setenv()` ใส่สภาพแวดล้อมโดยตรง ทำให้การเปลี่ยนแปลงกลายเป็นการเปลี่ยนแปลงถาวรของโพรเซส
2. ใน `check_array_def()` โค้ดเดิมวนลูปอ่านโทเค็นโดยสมมติว่าเป็นการเรียงลำดับจาก `0, 1, 2, ...` เสมอ ไม่รองรับการตรวจสอบว่าโทเค็นขึ้นต้นด้วย `[` หรือไม่
3. โครงสร้าง `zbash_state_t` เดิมไม่มีตารางเก็บ Attribute และไม่มีการนับเลขบรรทัด `state->lineno` สำหรับรายงาน Error ของ Readonly Variable

---

## 4. โค้ดภาษา C ที่แก้ไข (The Fix)

### 4.1 เพิ่มโครงสร้างติดตาม Variable Attributes ใน `include/zbash.h`:
```c
#define ZBASH_MAX_VAR_ATTRS 256

#define ZBASH_ATTR_INTEGER  (1 << 0)
#define ZBASH_ATTR_READONLY (1 << 1)
#define ZBASH_ATTR_EXPORT   (1 << 2)

typedef struct {
    char *name;
    uint32_t flags;
} zbash_var_attr_t;
```

### 4.2 ระบบ Safe Prefix Assignment Save/Restore ใน `execute_single_cmd()`:
```c
typedef struct {
    char *name;
    char *old_val;
    bool was_set;
} saved_env_t;
saved_env_t saved_envs[ZBASH_MAX_ARGS];
int saved_count = 0;
/* บันทึกค่าเดิม -> รันคำสั่ง -> คืนค่าเดิมหลังคำสั่งเสร็จ */
```

### 4.3 อัปเกรด `check_array_def()` ให้รองรับ Subscript Arithmetic:
```c
if (final_tok[0] == '[') {
    char *b_close = strchr(final_tok, ']');
    if (b_close && (b_close[1] == '=' || (b_close[1] == '+' && b_close[2] == '='))) {
        *b_close = '\0';
        int sub_idx = (int)zbash_eval_arithmetic(state, final_tok + 1);
        bool elem_append = (b_close[1] == '+');
        /* คำนวณค่าและใส่ลงใน subscript index ที่ถูกต้อง */
    }
}
```

### 4.4 การบล็อกการแก้ไขตัวแปร Readonly ใน `zbash_assign_var()`:
```c
if (zbash_is_readonly(state, name)) {
    const char *src = (state->source_depth > 0) ? state->source_stack[state->source_depth - 1] : "zbash";
    if (state->lineno > 0) {
        fprintf(stderr, "%s: line %d: %s: readonly variable\n", src, state->lineno, name);
    } else {
        fprintf(stderr, "%s: %s: readonly variable\n", src, name);
    }
    state->last_status = 1;
    return 1;
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
bash test/run_gnu_test.sh -v appendop.tests
```

**ผลลัพธ์:**
```text
[#01/01] PASS 🏆 appendop.tests  (  83 lines | bash:   16.61ms | zbash:   40.12ms | ⚡ 0.41x)
🎉 ALL GNU TESTS PASSED 100% BIT-FOR-BIT EXACT MATCH! 💎
```

---

## 6. ข้อพึงระวังในอนาคต (Engineering Guidelines)
- เมื่อมีการแก้ไข Layout ของโครงสร้างใน `include/zbash.h` ต้องรัน `make clean && make` เสมอ เพื่อป้องกันปัญหา Stale Object File Memory Offset Desynchronization
- การเพิ่ม Builtin ใหม่ที่จัดการกับตัวแปร (เช่น `local`, `export`, `readonly`, `typeset`) ต้องส่งผ่าน `zbash_state_t *state` เพื่อให้เข้าถึง Attribute ตารางตัวแปรเสมอ
