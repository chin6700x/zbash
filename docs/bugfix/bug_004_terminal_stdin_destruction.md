# 🪲 BUG-004: Terminal STDIN Destruction in Loop Redirection
> **หมวดหมู่:** การจัดการสตรีมนำเข้าและเทอร์มินัล (STDIN & Redirection)  
> **ระดับความรุนแรง:** สูง (High - Loss of Interactive Terminal Input)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) (ฟังก์ชัน: `zbash_run_while_loop`)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ในสคริปต์ที่มีการอ่านไฟล์ด้วยลูป และตามด้วยการถามคำถามผู้ใช้ผ่านคีย์บอร์ด:

```bash
while read item; do
    echo "Item: $item"
done < file.txt

# บรรทัดนี้ควรจะรอให้ผู้ใช้พิมพ์ข้อความจากคีย์บอร์ด:
read -p "Press enter to continue: " confirm
```

**ผลลัพธ์ที่ผิดพลาด:**
คำสั่ง `read` บรรทัดล่างไม่หยุดรอการพิมพ์จากผู้ใช้ แต่ข้ามผ่านไปทันทีเสมือนหน้าจอเทอร์มินัลหายไป หรือเจอสภาวะ End-of-File ถาวร!

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

เมื่อลูป `while ... done < file.txt` ทำงานในกระบวนการหลัก (Main Process) ของเชลล์ การเปลี่ยนทิศทางสตรีมนำเข้าจะเรียกคำสั่งระบบ:
`dup2(file_fd, STDIN_FILENO);`

คำสั่งนี้จะ **ทำลาย File Descriptor หมายเลข 0 (ซึ่งเชื่อมต่อกับคีย์บอร์ดของเทอร์มินัล `/dev/tty`) ทิ้งไปอย่างถาวร** และสวมรอยด้วยตัวอธิบายไฟล์ของ `file.txt` แทน

เมื่อลูป `while` อ่านจนกระทั่งจบไฟล์ ตัวอธิบายไฟล์หมายเลข 0 จะติดอยู่ในสถานะ **EOF** ตลอดไป ทำให้คำสั่ง `read` ใดๆ หลังจากนั้นไม่สามารถติดต่อกับแป้นพิมพ์ของผู้ใช้ได้อีกเลย

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อตรวจสอบปลายทางของ FD 0 หลังจบลูปด้วย `readlink /dev/fd/0` (หรือตรวจสอบผ่าน fstat):
พบว่า FD 0 ยังคงชี้ไปยัง `file.txt` ที่อ่านจนสุดไบต์แล้ว ไม่ได้ชี้กลับมาที่ Pseudo-terminal Slave (`/dev/ttys00X`) เหมือนตอนเริ่มต้นรันเชลล์

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ในไฟล์ [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) เราได้ใช้รูปแบบ **Transactional I/O Pattern**:

```c
/* 1. สำรอง Terminal STDIN ดั้งเดิมไว้ใน FD ตัวใหม่ก่อนเข้าลูป */
int saved_in = dup(STDIN_FILENO);

/* 2. เปลี่ยนเส้นทาง STDIN ชั่วคราวสำหรับตัวลูป */
apply_redirections(&loop_cmd);

/* 3. ประมวลผลลูปจนกระทั่งเสร็จสิ้น... */
while (...) {
    ...
}

/* 4. กู้คืน Terminal STDIN กลับมาที่ FD 0 และปิดตัวสำรองทิ้ง */
dup2(saved_in, STDIN_FILENO);
close(saved_in);
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c 'while read l; do :; done <<< "hello"; echo $? && [ -t 0 ] || echo "STDIN_RESTORED"'
# ผลลัพธ์: คืนสถานะ STDIN กลับมายังโหมดปกติอย่างถูกต้อง
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **Transactional Descriptors:** ทุกการเปลี่ยนแปลง Standard FDs (0, 1, 2) ในระดับ Builtin หรือ Compound Loops ต้องมีโครงสร้างแบบ Transaction เสมอ: Save -> Mutate -> Restore
2. **ห้ามละเมิดสิทธิ์ Terminal:** คีย์บอร์ดและหน้าจอแสดงผลของผู้ใช้คือทรัพยากรศักดิ์สิทธิ์ของ Shell ห้ามปล่อยให้สคริปต์ทำลายการเชื่อมต่อนี้โดยไม่กู้คืน
