# 🪲 BUG-037: Darwin BSD mktemp Strict Suffix & Interactive FIFO Hangs in Test Runners
> **หมวดหมู่:** สภาพแวดล้อมการทดสอบและข้อกำหนดเฉพาะของ Darwin BSD (Harness & Platform Traps)  
> **ระดับความรุนแรง:** ปานกลาง (Medium - Test Harness Freeze & Flaky Script Execution)  
> **ไฟล์ที่เกี่ยวข้อง:** [`test/common_trio.sh`](file:///Volumes/Mac%20Backup/zero-tools/zbash/test/common_trio.sh), [`test/run_apple_sh.sh`](file:///Volumes/Mac%20Backup/zero-tools/zbash/test/run_apple_sh.sh)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

ขณะรันชุดทดสอบ Apple Darwin POSIX Suite ด้วยสคริปต์รันเนอร์ เกิดปัญหา 2 ประการบน macOS:

1. **ปัญหา BSD `mktemp` Error:**
   ```bash
   mktemp /tmp/test_out_XXXXXX.txt
   # Output บน macOS: mktemp: mkstemps: File exists
   ```
2. **ปัญหา Runner แขวนนิ่งไม่สิ้นสุด (Deadlock Hang):**
   เมื่อรันข้อ `builtins/fc1.0` สคริปต์รันเนอร์หยุดนิ่ง ไม่สามารถจบการทำงานได้ ต้องกด Ctrl+C เพื่อบังคับปิด

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

1. **ความแตกต่างระหว่าง GNU `mktemp` กับ BSD `mktemp`:**
   บน Linux (GNU coreutils) คำสั่ง `mktemp` ยอมรับ Template ที่มี Extension ต่อท้าย เช่น `XXXXXX.txt` แต่บน Darwin/FreeBSD คำสั่ง `mktemp` บังคับว่า **สตริง Template ต้องลงท้ายด้วยตัวอักษร 'X' อย่างน้อย 3 ตัวเท่านั้น** (ห้ามมีนามสกุลต่อท้าย) มิฉะนั้นการสร้างไฟล์ชั่วคราวจะล้มเหลว
2. **การทดสอบ Interactive Shell History ด้วย FIFO ใน `fc1.0`:**
   ชุดทดสอบดั้งเดิมของ Apple Darwin ในโฟลเดอร์ `sh/tests/builtins/fc1.0` มีการสร้าง Named Pipe (`mkfifo`) และส่งข้อมูลจำลองเข้าไปเพื่อทดสอบการเปิด Editor แก้ไข History ซึ่งเป็นการจำลองระบบ Interactive ที่หากไม่มีตัวจับเวลาคอยกำกับ (Watchdog Timeout) กระบวนการจะบล็อกรอ I/O บน FIFO ชั่วกาลนาน

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

1. ตรวจสอบโค้ดในรันเนอร์ พบการใช้ `TMP_FILE="$(mktemp /tmp/test_run_XXXXXX.txt)"` ซึ่งทำให้เกิด Error บน Darwin
2. ตรวจสอบโปรเซสด้วย `ps aux | grep fc1.0` พบว่าคำสั่งค้างอยู่ที่ `read` จาก FIFO ที่ไม่มีตัวเขียน

---

## 4. โค้ดภาษา C และสคริปต์แก้ไข (The Fix)

ใน [`test/common_trio.sh`](file:///Volumes/Mac%20Backup/zero-tools/zbash/test/common_trio.sh):

### 4.1 แก้ไข Template ของ `mktemp` ให้ถูกต้องตามมาตรฐาน BSD:
```bash
TMP_STDOUT="$(mktemp /tmp/zbash_test_out_XXXXXX)"
TMP_STDERR="$(mktemp /tmp/zbash_test_err_XXXXXX)"
```

### 4.2 เสริมระบบป้องกัน Hang ด้วย Perl Microsecond Alarm Watchdog:
```bash
run_with_timeout() {
    local timeout_sec="$1"
    shift
    perl -e '
        use strict;
        use warnings;
        my $timeout = shift;
        $SIG{ALRM} = sub { kill "KILL", $$; exit 124; };
        alarm($timeout);
        exec(@ARGV);
    ' "$timeout_sec" "$@"
}
```
และนำมาครอบการรันของทุกชุดทดสอบใน `run_apple_sh.sh` และ `run_gnu_test.sh`

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
bash test/run_apple_sh.sh builtins/fc1.0
# Output: ผ่านการทำงานตาม Timeout 5 วินาที ไม่ค้างเติ่งอีกต่อไป

bash test/run_apple_sh.sh set-e
# Output: สร้างไฟล์ชั่วคราวฉลุย 35/35 ข้อโดยไม่มีข้อผิดพลาด mktemp
```

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

ชุดทดสอบระดับปฏิบัติการจริงของระบบปฏิบัติการย่อมมีทั้งเคส Interactive, Signal trapping, และ FIFO IPC การสร้าง Test Harness ที่เชื่อถือได้ระดับสากลต้องมี **Fail-Safe Watchdog Timer** เสมอ เพื่อขจัดปัญหา False Negative และ Runner Hang
