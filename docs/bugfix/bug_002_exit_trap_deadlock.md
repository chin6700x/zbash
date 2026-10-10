# 🪲 BUG-002: The Exit Trap Short-Circuit Deadlock
> **หมวดหมู่:** การจัดการสัญญาณและดักจับเหตุการณ์ (Signals & Traps)  
> **ระดับความรุนแรง:** สูง (High - Silent Failure / Resource Leak on Exit)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c) (ฟังก์ชัน: `builtin_exit`)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ในสคริปต์ที่มีการลงทะเบียนทำความสะอาดทรัพยากรเมื่อจบโปรแกรม (Exit Trap):

```bash
trap 'echo "CLEANUP_TEMP_FILES"' EXIT
exit 3
```

**ผลลัพธ์ที่ผิดพลาด:**
เชลล์ปิดตัวลงทันทีด้วยรหัสสถานะ `3` แต่ข้อความ `"CLEANUP_TEMP_FILES"` ไม่เคยถูกพิมพ์ออกมา ทำให้ไฟล์ชั่วคราวหรือ Lock Files ที่สร้างไว้ตกค้างในระบบ

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ตามมาตรฐาน POSIX.1-2017 สัญญาณจำลอง `EXIT` (หรือ signal 0) จะต้องถูกประมวลผลเมื่อเชลล์กำลังจะยุติการทำงาน ไม่ว่าจะเกิดจากการรันจนจบสคริปต์ หรือเกิดจากการเรียกคำสั่ง Builtin `exit [n]`

ในสถาปัตยกรรมของเชลล์ส่วนใหญ่ ฟังก์ชัน `builtin_exit()` ทำหน้าที่ตั้งค่าแฟล็กสถานะทางออก:
`state->should_exit = true;`

เมื่อ `builtin_exit()` พยายามสั่งรันคำสั่งที่บันทึกไว้ใน `state->exit_trap` โดยส่งให้ฟังก์ชันประมวลผลหลัก `zbash_run_line(state, trap_cmd)` ตัวฟังก์ชันประมวลผลกลับมีเงื่อนไข Short-Circuit ด่านแรกว่า:
```c
if (state->should_exit) return state->last_status;
```
ส่งผลให้ตัวเอนจินปฏิเสธการรันคำสั่ง Trap ทันที เพราะเข้าใจว่ากำลังอยู่ในสถานะที่ต้องรีบออกจากโปรแกรม เกิดเป็น **Deadlock ทางตรรกะการควบคุมการไหล**!

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อวางจุด Breakpoint ในตัวประมวลผล พบว่า:
1. `builtin_exit` ได้รับคำสั่งให้จบโปรแกรม
2. `state->should_exit` ถูกเซ็ตเป็น `1`
3. มีการดึงสตริง `"echo CLEANUP_TEMP_FILES"` ส่งเข้า `zbash_run_line`
4. แต่ `zbash_run_line` ตรวจพบ `state->should_exit == 1` จึง `return` ทันทีโดยไม่ได้ตีความสตริงเลยแม้แต่อักขระเดียว

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ในไฟล์ [`src/builtins/builtins.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/builtins/builtins.c) ภายในฟังก์ชัน `builtin_exit()` เราได้ทำกลไก **Temporary Exit State Suspension**:

```c
int builtin_exit(zbash_state_t *state, int argc, char **argv) {
    int code = (argc > 1) ? atoi(argv[1]) : state->last_status;

    if (state->exit_trap && strlen(state->exit_trap) > 0) {
        char *trap_cmd = strdup(state->exit_trap);
        
        /* ปลดล็อคสถานะทางออกชั่วคราว เพื่อเปิดทางให้เอนจินรันคำสั่ง Trap */
        state->should_exit = false;
        zbash_run_line(state, trap_cmd);
        free(trap_cmd);
        
        /* ล็อคสถานะทางออกกลับคืนมาอย่างสมบูรณ์ */
        state->should_exit = true;
    }

    state->exit_code = code;
    state->last_status = code;
    state->should_exit = true;
    return code;
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'trap "echo CLEANED" EXIT; exit 42'
# ผลลัพธ์:
# CLEANED
# [Exit Code = 42]
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **State Isolation ระหว่าง Main Flow กับ Handlers:** การใช้ Control Flags ร่วมกันระหว่าง Normal Script Execution กับ Asynchronous Trap Handlers เป็นกับดักอันตราย
2. **Context Suspension:** เมื่อต้องประมวลผลคำสั่งที่เป็น Cleanup Handlers คุณต้องระงับ (Suspend) สภาวะ Termination ชั่วคราว หรือรัน Handler ภายใต้ Sub-Execution Frame ที่แยกจากกัน
