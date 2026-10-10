# 🪲 BUG-032: Here-Document Unquoted Parameter Expansion & Semicolon Sequencing
> **หมวดหมู่:** ท่อส่งข้อมูลและการเปลี่ยนทิศทาง I/O (I/O Redirections & Heredoc Parsing)  
> **ระดับความรุนแรง:** ปานกลาง (Medium - Parameter Interpolation Failure in Shell Templates)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อรันสคริปต์ที่มีการสร้างเอกสาร Here-Document โดยกำหนดค่าตัวแปรในบรรทัดเดียวกันก่อนเรียกคำสั่ง:

```bash
name="Wirot"; cat << EOF
Hello $name
Cost: \$500
EOF
```

**ผลลัพธ์ที่ผิดพลาด (ก่อนแก้ไข):**
```text
Hello $name
Cost: \$500
```
เนื้อหาภายใน Here-Document ไม่ถูกขยายค่าตัวแปร (ยังคงเป็น `$name`) แม้ว่า Delimiter `EOF` จะไม่ได้ถูกครอบด้วยเครื่องหมายคำพูดก็ตาม!

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในมาตรฐาน POSIX.1-2017 และประวัติศาสตร์ 40 ปีของ Bourne Shell/GNU Bash พฤติกรรมของ Here-Document ขึ้นอยู่กับการ Quoting ของ Delimiter:
1. หาก Delimiter มีการครอบด้วย Quote เช่น `<< 'EOF'` หรือ `<< "EOF"`: เนื้อหาภายในจะถูกอ่านแบบ **Literal (ห้ามขยายตัวแปร ห้ามตีความ Backslash)**
2. หาก Delimiter ไม่มีการครอบด้วย Quote เช่น `<< EOF`: เนื้อหาภายในจะต้องผ่าน **Parameter Expansion, Command Substitution และ Arithmetic Expansion** เช่นเดียวกับข้อความใน Double Quotes

ใน `zbash` ยุคแรก กลไก Here-Document บันทึกเนื้อหาบรรทัดดิบๆ ลงในไฟล์ชั่วคราว `/tmp/zbash_hd_XXXXXX` เสมอโดยไม่ได้ตรวจสอบว่า Delimiter มี Quote หรือไม่ นอกจากนี้ ในกรณีที่มีคำสั่งกำหนดค่าตัวแปรนำหน้าด้วย Semicolon เช่น `cust='...'; cat << EOF` เอนจินเริ่มอ่านเนื้อหา Heredoc จาก Stream ทันทีก่อนที่คำสั่งกำหนดค่าตัวแปรด้านหน้าจะได้รันจริง

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

เมื่อสืบสวนการทำงานใน `zbash_run_stream()`:
1. ตัวแปร `char q` ตรวจจับว่า Delimiter เป็น `'` หรือ `"` แต่ค่าตัวแปรนี้ไม่ได้ถูกนำไปตัดสินใจว่าจะเรียก `zbash_expand_vars()` หรือไม่
2. เมื่อบรรทัด `name="Wirot"; cat << EOF` ถูกอ่านเข้ามา ตัวชี้วัด `hd` ชี้ไปที่ `<<` และลูป `getline()` เริ่มดูดบรรทัดถัดไปทันที ทำให้คำสั่ง `name="Wirot"` ยังไม่ได้ถูกดำเนินการ ค่าตัวแปร `$name` ในสภาพแวดล้อมจึงยังว่างเปล่า

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ใน [`src/core/engine.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/core/engine.c):

1. **ประมวลผลคำสั่งก่อนหน้า Semicolon ทันที:**
```c
if (hd) {
    char *last_semi = NULL;
    bool s_in_s = false, s_in_d = false;
    for (char *c = p; c < hd; c++) {
        if (*c == '\'' && !s_in_d) s_in_s = !s_in_s;
        else if (*c == '"' && !s_in_s) s_in_d = !s_in_d;
        else if (!s_in_s && !s_in_d && *c == ';') last_semi = c;
    }
    if (last_semi) {
        *last_semi = '\0';
        zbash_run_line(state, p);
        p = last_semi + 1;
        while (*p && isspace((unsigned char)*p)) p++;
    }
```

2. **ขยายค่าตัวแปรเมื่อ Delimiter ไม่ได้ถูก Quote (`q == 0`):**
```c
if (strcmp(htrim, delim) == 0) break;
if (q == 0) {
    char *exp_line = zbash_expand_vars(state, hline);
    fprintf(hd_fp, "%s\n", exp_line ? exp_line : hline);
    if (exp_line) free(exp_line);
} else {
    fprintf(hd_fp, "%s\n", hline);
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
$ /bin/bash -c 'cust="สมศักดิ์ รักไทย"; cat << EOF
- ลูกค้า: $cust
EOF'
- ลูกค้า: สมศักดิ์ รักไทย

$ bin/zbash -c 'cust="สมศักดิ์ รักไทย"; cat << EOF
- ลูกค้า: $cust
EOF'
- ลูกค้า: สมศักดิ์ รักไทย
```
ตรงกัน 100% บิตต่อบิต ทั้งการขยายค่าตัวแปรภาษาไทย และการรักษาสถานะ Literal เมื่อใช้ `<< 'EOF'`

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

- **Heredoc เป็นทั้งไวยากรณ์ I/O และเครื่องจักรเทมเพลต:** สคริปต์คอนฟิกูเรชัน (Cloud-init, Terraform, Docker templates) พึ่งพาความสามารถในการตัดสินใจระหว่างการขยายค่าตัวแปร (`<< EOF`) กับการคงรูปเดิม (`<< 'EOF'`) การรักษากฎข้อนี้ให้ตรงกับ POSIX จึงมีความสำคัญระดับภารกิจวิกฤต
