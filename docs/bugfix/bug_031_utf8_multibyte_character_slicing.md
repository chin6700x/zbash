# 🪲 BUG-031: UTF-8 Multibyte Character Boundary Slicing & Length
> **หมวดหมู่:** ไวยากรณ์ สตริง และการขยายตัวแปร (Lexing & Multibyte Unicode Expansions)  
> **ระดับความรุนแรง:** สูง (High - Silent Corruption of Non-ASCII Text & Thai Diacritics)  
> **ไฟล์ที่เกี่ยวข้อง:** [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c)  
> **ค้นพบและแก้ไขโดย:** วิโรจน์ ชูแก้ว (`chin6700x`)  

---

## 1. อาการของปัญหา (Symptom & PoC Script)

เมื่อจัดการข้อความที่มีอักขระหลายไบต์ (Multi-byte UTF-8) เช่น ภาษาไทย หรือ Emoji:

```bash
x="ภาษาไทย"
echo "${#x}"          # นับจำนวนตัวอักษร
echo "${x:0:4}"       # ตัดคำ 4 ตัวอักษรแรก
```

**ผลลัพธ์ที่ผิดพลาด (ก่อนแก้ไข):**
```text
21
ภ
```
* `${#x}` ได้ 21 แทนที่จะเป็น `7` (คืนค่าเป็นจำนวนไบต์ดิบ `strlen(x) = 21` เพราะอักษรไทย 1 ตัวใช้ 3 ไบต์ใน UTF-8)
* `${x:0:4}` ได้ `ภ` ตามด้วยเศษไบต์ที่ฉีกขาด แทนที่จะได้คำว่า `ภาษา` (เพราะระบบตัดที่ตำแหน่ง 4 ไบต์ แทนที่จะเป็น 4 ตัวอักษร Unicode Code Points)!

---

## 2. รากเหง้าประวัติศาสตร์ 40 ปี (Historical Root Cause)

ในยุคแรกเริ่มของ Unix และ Bourne Shell (1979) โลกของคอมพิวเตอร์รู้จักเพียงรหัส ASCII 7-บิต (0-127) ซึ่ง 1 ตัวอักษรมีขนาดเท่ากับ 1 ไบต์เสมอ ฟังก์ชันมาตรฐานอย่าง `strlen()` จึงถูกนำมาใช้คำนวณความยาวสตริงและออฟเซตการตัดคำโดยไม่มีปัญหา

จนกระทั่งการกำเนิดของ UTF-8 ในปี 1992 และการนำมาใช้ในระบบปฏิบัติการยุคใหม่ GNU Bash ได้เพิ่มการตรวจสอบ Locale (`LC_ALL`, `LC_CTYPE`, `LANG`) เพื่อสลับโหมดระหว่าง Byte Counting (เมื่อ `LC_ALL=C`) กับ Character Counting (เมื่ออยู่ในโหมด UTF-8)  
แต่หากเชลล์ยุคใหม่ที่เขียนด้วยภาษา C ใช้เพียง `strlen()` หรือการเลื่อนพอยน์เตอร์แบบไบต์ดิบในการประมวลผลสตริง ข้อความที่ไม่ใช่ภาษาอังกฤษทั้งหมดจะถูกฉีกแบ่งกลางไบต์ ทำให้เกิดอักขระชำรุด (Unicode Replacement Character ``) และสร้างความเสียหายแก่ฐานข้อมูล

---

## 3. การสืบสวนเชิงลึก (Investigation & Forensics)

โครงสร้างของ UTF-8 มีกฎเกณฑ์ที่ชัดเจนและสามารถตรวจสอบได้อย่างรวดเร็วในระดับหน่วยความจำ:
- ไบต์ต่อเนื่อง (Continuation Bytes) จะมีบิตนำหน้าเป็น `10xxxxxx` เสมอ (`(byte & 0xC0) == 0x80`)
- ไบต์เริ่มต้นของตัวอักษรใหม่ (Lead Byte หรือ ASCII) จะมีบิตนำหน้าไม่ใช่ `10xxxxxx` (`(byte & 0xC0) != 0x80`)

ดังนั้น จำนวนตัวอักษร Unicode ใน UTF-8 จึงเท่ากับจำนวนไบต์ทั้งหมดที่ **ไม่ใช่ Continuation Byte** พอดี!

---

## 4. โค้ดภาษา C และวิธีแก้ใน ZBASH (The C Fix)

ใน [`src/parser/lexer.c`](file:///Volumes/Mac%20Backup/zero-tools/zbash/src/parser/lexer.c) เราได้สร้าง UTF-8 Helper Functions แบบ Zero-Overhead:

```c
static bool is_utf8_mode(void) {
    const char *lc = getenv("LC_ALL");
    if (!lc || !*lc) lc = getenv("LC_CTYPE");
    if (!lc || !*lc) lc = getenv("LANG");
    if (lc) {
        if (strstr(lc, "UTF-8") || strstr(lc, "utf-8") || strstr(lc, "UTF8") || strstr(lc, "utf8")) return true;
        if (strcmp(lc, "C") == 0 || strcmp(lc, "POSIX") == 0) return false;
    }
    return true; /* มาตรฐานของ Darwin / Apple Silicon คือ UTF-8 */
}

static size_t utf8_char_count(const char *s) {
    if (!s) return 0;
    if (!is_utf8_mode()) return strlen(s);
    size_t count = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) count++;
        s++;
    }
    return count;
}

static const char *utf8_char_at(const char *s, int char_idx) {
    if (!s || char_idx <= 0) return s;
    if (!is_utf8_mode()) {
        size_t slen = strlen(s);
        return ((size_t)char_idx >= slen) ? s + slen : s + char_idx;
    }
    int cur = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) {
            if (cur == char_idx) return s;
            cur++;
        }
        s++;
    }
    return s;
}
```

และปรับปรุงการคำนวณ `${#VAR}` และ `${VAR:offset:length}` ให้แปลงพิกัด Character Offset/Length ไปเป็น Byte Boundaries อย่างแม่นยำก่อนคัดลอกลงในบัฟเฟอร์:

```c
const char *p_start = utf8_char_at(env_v, offset);
const char *p_end = utf8_char_at(p_start, slen);
size_t byte_len = (size_t)(p_end - p_start);
dynamic_val = malloc(byte_len + 1);
if (dynamic_val) {
    memcpy(dynamic_val, p_start, byte_len);
    dynamic_val[byte_len] = '\0';
    val = dynamic_val;
}
```

---

## 5. การทดสอบยืนยันผล (Regression Test Command)

```bash
$ /bin/bash  -c 'x="ภาษาไทย"; echo "${#x} | ${x:0:4}"'
7 | ภาษา
$ bin/zbash -c 'x="ภาษาไทย"; echo "${#x} | ${x:0:4}"'
7 | ภาษา
```
ผลลัพธ์ตรงกันแบบ **100% Bit-for-Bit Exact Match** ทั้งตัวเลขนับความยาวและการตัดคำภาษาไทย

---

## 6. บทเรียนและพิมพ์เขียวสำหรับผู้สร้าง Shell ยุคใหม่ (Architectural Wisdom)

1. **อย่าสับสนระหว่าง Byte Index กับ Character Index:** ในโลกยุคปัจจุบัน ข้อมูลไม่ใช่ ASCII อีกต่อไป ระบบตัดคำของเชลล์ต้องเคารพขอบเขตของอักขระ Unicode เสมอ
2. **ประสิทธิภาพระดับ Bare-Metal โดยไม่ต้องพึ่งไลบรารีภายนอก:** การตรวจสอบบิต UTF-8 `(byte & 0xC0) != 0x80` ทำงานได้รวดเร็วมากในระดับ Single Clock Cycle บน Apple Silicon ARM64 โดยไม่จำเป็นต้องพึ่ง `libicu` หรือไลบรารีขนาดใหญ่ใดๆ
