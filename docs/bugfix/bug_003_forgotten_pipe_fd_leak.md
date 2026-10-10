# 🪲 BUG-003: Forgotten Pipe Descriptor Leak in Parent Process
> **หมวดหมู่:** ท่อส่งข้อมูลและตัวอธิบายไฟล์ (Pipelines & File Descriptors)  
> **ระดับความรุนแรง:** ร้ายแรงมาก (Critical - Permanent Hang / 0% CPU Deadlock)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) (ฟังก์ชัน: `zbash_execute_pipeline`)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันคำสั่งที่เชื่อมต่อท่อไปป์มากกว่า 2 ทอด (Multi-stage Pipeline):

```bash
cat large_data.csv | grep "ERROR" | wc -l
```

**ผลลัพธ์ที่ผิดพลาด:**
คำสั่งค้างสนิท ไม่ยอมคืนผลลัพธ์ ไม่ยอมคืน Prompt หน้าจอค้างนิ่งสนิท และซีพียูไม่มีการใช้งานใดๆ (0% CPU)

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

นี่คือหนึ่งในความลับที่ลึกซึ้งที่สุดของ Unix Kernel I/O:
เมื่อสร้างท่อด้วย `pipe(pipe_fds[j])` โปรเซสแม่จะถือคู่ของ Descriptors (ท่ออ่าน `[0]` และ ท่อเขียน `[1]`) จากนั้นจึงสั่ง `fork()` โปรเซสลูกขึ้นมาสำหรับแต่ละคำสั่ง

ในโปรเซสลูก ลูกแต่ละตัวจะใช้ `dup2()` สลับท่อเข้า STDIN หรือ STDOUT ของตนเอง แล้วปิดท่อที่ตนเองไม่ใช้ออกไป
แต่ **สัญญาณ End-of-File (EOF) จะถูกส่งไปยังโปรเซสตัวรับปลายทาง (`wc -l`) ก็ต่อเมื่อ "ไม่มีโปรเซสใดในระบบถือท่อเขียน (Write End) เปิดค้างอยู่อีกต่อไปแล้ว"**

หากตัวเชลล์แม่ (Parent Shell) **ลืมสั่ง `close(pipe_fds[j][1])` ในตัวของมันเอง** เคอร์เนลจะมองว่า *"ยังมีโปรแกรมตัวหนึ่ง (คือเชลล์แม่นั่นเอง) ที่ยังมีสิทธิ์เขียนข้อมูลลงท่อนี้อยู่ ดังนั้นท่อยังไม่ปิด"* ส่งผลให้ `wc -l` นั่งรอข้อมูลต่อจนหมดอายุขัย!

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อใช้คำสั่ง `lsof -p <pid_of_wc>` พบว่า:
`wc` ยังคงเปิด Read End ของ Pipe ค้างอยู่และติดอยู่ในบล็อกกิ้งซิสเต็มคอล `read()`  
และเมื่อตรวจ `lsof -p <pid_of_parent>` พบว่า:
`zbash` (โปรเซสแม่) ยังคงเปิด Write End ของ Pipe หมายเลขเดียวกันค้างไว้อยู่!

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ในไฟล์ [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) ภายในฟังก์ชัน `zbash_execute_pipeline()`:

```c
/*
 * กฎเหล็กของ POSIX Pipeline:
 * ทันทีที่ Fork โปรเซสลูกครบทุกตัว Parent ต้องปิด Descriptor ของท่อทุกอันทันที!
 */
for (int j = 0; j < n - 1; j++) {
    close(pipe_fds[j][0]);
    close(pipe_fds[j][1]);
}

/* จากนั้น Parent ค่อยวนลูปทำการรอเก็บผลลัพธ์จากลูกตามปกติ */
for (int i = 0; i < n; i++) {
    int status = 0;
    waitpid(pids[i], &status, 0);
    if (i == n - 1) {
        last_status = WIFEXITED(status) ? WEXITSTATUS(status) : (128 + WTERMSIG(status));
    }
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'echo -e "1\n2\n3" | grep "2" | wc -l | tr -d " "'
# ผลลัพธ์: 1 คืนผลลัพธ์ทันทีภายใน 1-2 มิลลิวินาที
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **Relinquish Ownership:** ทรัพยากรใดก็ตามที่ Parent สร้างขึ้นเพื่อเป็นสะพานเชื่อมต่อระหว่างลูกๆ Parent ต้องปล่อยมือ (Close/Release) ในฝั่งของตนเองทันทีหลังส่งมอบเสร็จ
2. **EOF คือผลรวมของระบบ:** EOF ไม่ได้เกิดจากตัวส่งข้อมูลตัวเดียวตายไป แต่เกิดจากการที่ *ทุกคนที่สามารถส่งข้อมูลได้* ปิดท่อหมดแล้วอย่างแท้จริง
