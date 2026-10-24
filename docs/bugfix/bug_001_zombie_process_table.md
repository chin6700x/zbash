# 🪲 BUG-001: Zombie Process Table Accumulation / Watchdog Hang
> **หมวดหมู่:** การจัดการโปรเซสและเคอร์เนล (Process Lifecycle)  
> **ระดับความรุนแรง:** ร้ายแรงมาก (Critical - Infinite Hang / System Lock)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) (ฟังก์ชัน: `zbash_run_line`)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ในการรันสคริปต์ตรวจสอบการทำงานของ Background Process แบบ Watchdog:

```bash
( sleep 0.05 ) &
bg_pid=$!

while kill -0 "$bg_pid" 2>/dev/null; do
    sleep 0.01
done

echo "PROCESS_SUPERVISION_SUCCESS"
```

**ผลลัพธ์ที่ผิดพลาด:**
โปรแกรมค้างอยู่ในลูป `while` ตลอดกาล ไม่ยอมพิมพ์ `"PROCESS_SUPERVISION_SUCCESS"` ออกมา แม้ว่าโปรเซส `sleep` จะจบการทำงานไปนานแล้วก็ตาม

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในสถาปัตยกรรม Unix Process Model ตั้งแต่ยุค AT&T Bell Labs เมื่อโปรเซสลูก (Child Process) เรียก `exit()` เคอร์เนลจะไม่ลบโครงสร้างออกจาก **Process Table** ของระบบทันที แต่จะเปลี่ยนสถานะเป็น **ZOMBIE** (`<defunct>`) เพื่อเก็บข้อมูล Exit Code และสถิติการใช้ทรัพยากรไว้ รอให้โปรเซสแม่ (Parent) มารับผลลัพธ์ผ่าน `wait()` หรือ `waitpid()`

คำสั่ง `kill -0 $pid` ทำหน้าที่ส่งสัญญาณ Null Signal เพื่อตรวจสอบว่า *"โปรเซสนี้ยังมีตัวตนอยู่ในตารางเคอร์เนลหรือไม่"* หากโปรเซสลูกกลายเป็น Zombie มันจะยังคงมีตัวตนอยู่ในตารางของระบบ ส่งผลให้ `kill -0` คืนค่า Exit Code `0` (Success) ตลอดเวลา!

GNU Bash 3.2+ หลบเลี่ยงปัญหานี้ด้วยการแอบเคลียร์ตารางลูก (Job Control Reaping) เป็นระยะภายในลูปหลักของเชลล์ แต่ในเชลล์ที่สร้างขึ้นมาใหม่ส่วนใหญ่มักไม่เข้าใจกลไกนี้ และรอเก็บศพเฉพาะตอนที่ผู้ใช้สั่ง `wait` แบบระบุ PID เท่านั้น

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อใช้คำสั่ง `ps aux | grep defunct` ระหว่างที่สคริปต์ค้าง จะพบโปรเซสดังนี้:
```text
chinchin  84920   0.0  0.0        0      0   ??  Z    5:40PM   0:00.00 (zbash) <defunct>
```
โปรเซสแม่ (`zbash`) ไม่ยอมรับ Exit Status ของลูก ทำให้ลูกติดค้างอยู่ในสถานะ Zombie และขัดขวางการหลุดออกจากลูปของ Watchdog

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ในไฟล์ [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c) เราได้เพิ่มกลไก **Non-blocking Zombie Reaper** บรรทัดแรกสุดของฟังก์ชันประมวลผลคำสั่ง `zbash_run_line()`:

```c
/*
 * Non-blocking child reaping (Zombie Exorcism)
 * ดักเก็บศพซอมบี้ที่ตายแล้วทั้งหมดในระบบอย่างไม่ขัดจังหวะการทำงาน (WNOHANG)
 * ก่อนจะเริ่มรันคำสั่งบรรทัดใหม่ทุกครั้ง
 */
while (waitpid(-1, NULL, WNOHANG) > 0);
```

การเรียก `waitpid(-1, NULL, WNOHANG)` ส่งผลให้เคอร์เนลลบ Record ของ Zombie ออกจาก Process Table ทันทีในหน่วยไมโครวินาที โดยไม่หยุดรอโปรเซสที่ยังทำงานอยู่

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
./bin/zbash -c '( sleep 0.02 ) & p=$!; while kill -0 $p 2>/dev/null; do sleep 0.01; done; echo "REAPED_OK"'
# ผลลัพธ์: REAPED_OK ภายใน ~30 มิลลิวินาที
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **เชลล์คือสุัปเหร่อของระบบปฏิบัติการ:** อย่าพึ่งพาให้ระบบปฏิบัติการเคลียร์โปรเซสลูกเอง เชลล์มีหน้าที่รับผิดชอบโดยตรงในการเก็บศพลูกของตน
2. **ใช้ WNOHANG เป็นระยะ:** การเคลียร์โปรเซสแบบ Non-blocking ก่อนรันคำสั่งถัดไปคือการป้องกันซอมบี้ที่เบาและมีประสิทธิภาพที่สุดในภาษา C
