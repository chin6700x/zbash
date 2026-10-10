# 📖 แผนแม่บทการจัดทำหนังสือ: "ผ่าสถาปัตยกรรม zbash"
> **"สร้าง Unix Shell ระดับโลกด้วย C และ ARM64 Assembly จากศูนย์สู่ 1,000 การทดสอบสมบูรณ์แบบ"**  
> **(Crafting zbash: Building a Bare-Metal POSIX Shell from Scratch with C & ARM64 Assembly)**  
> **ผู้ประพันธ์และสถาปนิกหลัก:** วิโรจน์ ชูแก้ว (Wirot Chookeaw - `chin6700x`)  
> **ที่ตั้งโครงการ:** `zbash/docs/book/`  
> **โฟลเดอร์เนื้อหาต้นฉบับภาษาไทย:** `zbash/docs/book/th/`  
> **คลังภูมิปัญญาหลังบ้าน:** `zbash/docs/book/dev.md` (100 Timeless Wisdoms)  
> **สถานะโครงการ:** Approved & In-Progress 🚀

---

## 🧭 1. วิสัยทัศน์และวัตถุประสงค์ของหนังสือ (Vision & Purpose)

ตลอดระยะเวลา 40 ปีที่ผ่านมา นักพัฒนาซอฟต์แวร์ทั่วโลกคุ้นเคยกับการใช้ `bash` ทุกวันผ่าน Terminal, CI/CD, Docker, และคลาวด์ แต่มีน้อยคนนักที่จะเข้าใจว่า:  
**"ภายใต้คำสั่งเรียบง่ายอย่าง `echo "hello" | grep h` เบื้องหลังระบบปฏิบัติการ ระบบท่อไปป์ ตัวแยกคำ (Lexer) และการจัดการ Process ทำงานอย่างไรกันแน่?"**

ตำราหรือคอร์สส่วนใหญ่มักสอนเพียงวิธีเขียน Shell Script หรือไม่ก็โยนโค้ดของ GNU Bash ขนาดมหึมากว่า 150,000 บรรทัดมาให้ดู ซึ่งซับซ้อนเกินกว่าที่ใครจะเริ่มต้นทำความเข้าใจได้จากศูนย์

หนังสือเล่มนี้ถูกเขียนขึ้นจาก **ประสบการณ์จริงและโค้ดจริงของ `zbash`** โครงการที่พิสูจน์แล้วว่า:
> **"เราสามารถสร้าง Unix Shell ขนาดเพียง 86 KB ด้วยภาษา C และ ARM64 NEON Assembly ที่สามารถทดแทน GNU Bash ดั้งเดิม (1.9 MB) ได้อย่างสมบูรณ์แบบ ผ่านการทดสอบความเข้ากันได้บิตต่อบิต 1,000 จาก 1,000 ข้อ (100.0%) และเร็วกว่าเดิม 2x ถึง 4.3x!"**

**เป้าหมายสูงสุดของหนังสือเล่มนี้:**  
เพื่อ **"จุดประกายความกล้าหาญทางวิศวกรรม"** ให้นักพัฒนาซอฟต์แวร์ไทยและทั่วโลก กล้าก้าวข้ามการเป็นเพียงผู้ใช้งาน (Consumer) สู่การเป็นผู้สร้างสรรค์โครงสร้างพื้นฐานระดับระบบปฏิบัติการ (System Software Creator)

---

## ⚖️ 2. กฎเหล็กและมาตรฐานการประพันธ์ของหนังสือ (Editorial Standards & Guiding Rules)

เพื่อให้การเขียนหนังสือทุกบทดำเนินไปในทิศทางเดียวกันอย่างประณีต ถูกต้อง ปลอดภัย และไม่ต้องกลับมาแก้ไขซ้ำซ้อน **ผู้เขียนและผู้ช่วยเขียนทุกคนต้องยึดถือกฎเหล็ก 5 ข้อนี้อย่างเคร่งครัด**:

### 📌 กฎข้อที่ 1: "บทคารวะและวิวัฒนาการต่อยอด" (Reverence to Pioneers)
* **ต้องให้เกียรติและคารวะผู้บุกเบิกเสมอ:** Stephen Bourne (`sh`), Brian Fox & Chet Ramey (GNU `bash`), David Korn (`ksh`), Paul Falstad (`zsh`), Dennis Ritchie & Ken Thompson (`C & Unix`)
* **ห้ามใช้คำพูดรุนแรง ดูหมิ่น หรือด้อยค่าระบบเดิมเด็ดขาด:** ไม่พูดถึงข้อเสียในเชิงลบ แต่ให้อธิบายในมุมมองของ **"วิวัฒนาการของฮาร์ดแวร์และยุคสมัย"** (ในอดีตระบบถูกออกแบบมาอย่างดีที่สุดสำหรับ RAM 4–16 MB และ Single-core 16/32-bit ขณะที่ปัจจุบันคือยุค Apple Silicon 64-bit, 128-bit NEON, และ Unified Memory)

### 📌 กฎข้อที่ 2: มาตรฐานไฟล์ Markdown (`.md`) 100%
* ทุกบทต้องเขียนเป็นไฟล์ Markdown (`.md`) เท่านั้น (ยกเลิก `.txt`) เพื่อรองรับ Syntax Highlighting สำหรับโค้ดภาษา `c`, `arm64`, และ `bash`
* ตารางเปรียบเทียบและกล่องข้อความต้องจัดหน้าด้วยมาตรฐาน Markdown ที่สะอาด สบายตา และพร้อมแปลงเป็น PDF/eBook ได้ทันที
* สารบัญรวมต้องแยกเป็นไฟล์เดี่ยวอิสระ: [00_table_of_contents.md](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/book/th/00_table_of_contents.md)

### 📌 กฎข้อที่ 3: สถานะของ `dev.md` คือ "คลังเครื่องมือหลังบ้าน" เท่านั้น
* ไฟล์ [dev.md](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/book/dev.md) เป็นคลังภูมิปัญญา 100 ข้อสำหรับผู้เขียนใช้ค้นหาและหยิบยกมาประดับในหนังสือ
* **ห้ามใส่ชื่อไฟล์ `dev.md` หรือลิงก์ `[dev.md ข้อ 061]` ลงในเนื้อหาหนังสือที่ผู้อ่านเห็นเด็ดขาด!**
* ในเนื้อหาหนังสือจริง ให้แสดงเฉพาะ **"ข้อความคำพูดที่สง่างาม + ชื่อบุคคลเจ้าของคำพูดจริง"** โดยตรง

### 📌 กฎข้อที่ 4: ลิขสิทธิ์คำกล่าวและปรัชญาของผู้ประพันธ์ (Author Ownership)
* คำคม สัจธรรม หรือหลักคิดใดที่ไม่มีเจ้าของในอดีต หรือเป็นปรัชญาเฉพาะของโครงการ ZEPHYR / zbash **ถือเป็นคำกล่าว ความคิด และลิขสิทธิ์ของ วิโรจน์ ชูแก้ว (Wirot Chookeaw - `chin6700x`) 100%**
* ตัวอย่างประโยคทองของผู้ประพันธ์:
  - *"เร็วแต่ผลลัพธ์ผิด คือไร้ค่า — เร็วและถูกต้องตรงกันบิตต่อบิต คือการปฏิวัติ"*
  - *"Honor the pioneers, master the present, and craft the future."*
* เมื่อนำมาใส่ในหนังสือ ให้ระบุชื่อเครดิตเป็น **`— วิโรจน์ ชูแก้ว (Wirot Chookeaw)`** อย่างเป็นทางการ

### 📌 กฎข้อที่ 5: Pure Open Engineering (ไม่มีการเอ่ยถึงรุ่น Pro)
* หนังสือเล่มนี้เขียนขึ้นเพื่อมอบความรู้และวิทยาทานแก่วงการนักพัฒนาอย่างแท้จริง
* **ห้ามเอ่ยถึงรุ่น Pro, zbash-pro, หรือเรื่องการค้าใดๆ ทั้งสิ้น** เพื่อรักษาความบริสุทธิ์ของหนังสือให้เป็นตำราวิชาการระดับคลาสสิกเหนือกาลเวลา

---

## 🎯 3. กลุ่มผู้อ่านเป้าหมาย (Target Audience)

1. **นักพัฒนาซอฟต์แวร์ทั่วไป (Software Engineers & DevOps):** ที่ต้องการเข้าใจลึกซึ้งว่า Terminal, Process, Pipes, และ Subshell ทำงานอย่างไรในระดับลึก
2. **นักศึกษาวิศวกรรมคอมพิวเตอร์และวิทยาการคอมพิวเตอร์:** ที่ต้องการศึกษา Systems Programming, OS Concepts, Compilers & Interpreters จากตัวอย่างที่จับต้องได้จริง
3. **ผู้หลงใหลในภาษา C และ Assembly:** ที่อยากเห็นการประยุกต์ใช้ ARM64 NEON SIMD Registers (128-bit) ในการเร่งความเร็วการประมวลผลข้อความระดับ 35+ GB/s

---

## 📚 4. โครงสร้างสารบัญและ 19 บทเรียนหลัก (Table of Contents)

หนังสือแบ่งออกเป็น **6 ภาค (Parts) รวม 19 บทย่อย** โดยบันทึกเป็น Markdown Files (`.md`) ในไดเรกทอรี `zbash/docs/book/th/`:

```
zbash/docs/book/th/
├── 00_table_of_contents.md              # 📚 สารบัญและแผนที่นำทางของหนังสือ
├── ch01_the_genesis.md                  # การเดินทางของ Unix Shell: จากมรดก 40 ปี สู่ zbash
├── ch02_design_philosophy.md            # ปรัชญาการออกแบบ: 86 KB, Zero-Dependency, Bare-Metal
├── ch03_architecture_overview.md        # สถาปัตยกรรมภาพรวม: จากคีย์บอร์ดสู่ OS Execution
├── ch04_simd_vector_lexer.md            # ความลับของ Lexer: เร่งความเร็วด้วย ARM64 NEON SIMD 128-bit
├── ch05_grammar_quoting_hell.md         # ปราบพยศไวยากรณ์: แกะรอย Quoting Hell (", ', \) และ Tokenizer
├── ch06_ast_construction.md             # การสร้าง Abstract Syntax Tree (AST) ขนาดกะทัดรัด
├── ch07_parameter_expansion.md          # ศิลปะการขยายตัวแปร: ${VAR:-def}, ${#arr[@]}, ${VAR:offset:len}
├── ch08_arithmetic_engine.md            # เครื่องคิดเลขในตัว: สร้างตัวประเมินคณิตศาสตร์ $(( a + b * c ))
├── ch09_arrays_in_c.md                  # โครงสร้าง Indexed Arrays ใน C แบบไร้ Overhead
├── ch10_fork_exec_internals.md          # ดำดิ่งระบบปฏิบัติการ: เข้าใจ fork(), execve(), และ Process Lifecycle
├── ch11_pipes_and_redirections.md       # ท่อไปป์และการเปลี่ยนทิศทาง: pipe(), dup2(), และ done < file
├── ch12_control_flow_loops.md           # ลูปและเงื่อนไข: while, for, until, if-then-else, case-esac
├── ch13_subshells_and_scope.md          # ความต่างระดับชีวิต: Subshells (...) vs Grouping {...}
├── ch14_builtin_commands.md             # ทำไมคำสั่ง cd, export, read จึงไม่สามารถเป็นโปรแกรมภายนอกได้?
├── ch15_differential_testing.md        # "เร็วแต่ผลลัพธ์ผิด คือไร้ค่า": ปรัชญาการทดสอบระดับ Differential
├── ch16_anatomy_of_1000_tests.md        # ผ่าโครงสร้างชุดทดสอบ 10 โมดูล 1,000 ข้อ vs macOS Bash
├── ch17_hunting_edge_cases.md           # บันทึกการล่าบั๊กสุดแสบ: Memory Leaks, Infinite Redirections
├── ch18_wasm_webos_porting.md           # จาก Terminal สู่เบราว์เซอร์: พอร์ต zbash สู่ WebAssembly & WebOS
└── ch19_the_creators_mindset.md         # บทส่งท้าย: ปลดปล่อยพลังแห่งการสร้างซอฟต์แวร์ระดับโลก
```

---

## 📝 5. รายละเอียดเนื้อหาแต่ละบท (Chapter-by-Chapter Breakdown)

### ภาคที่ 1: กำเนิดและปรัชญาการออกแบบ (The Genesis & Philosophy)
* **บทที่ 1: การเดินทางของ Unix Shell: จากมรดก 40 ปี สู่ zbash**
  - คารวะผู้บุกเบิก: Stephen Bourne, Brian Fox, Chet Ramey, David Korn
  - วิวัฒนาการของฮาร์ดแวร์: ยุค RAM 4–16 MB สู่ชิป Apple Silicon ARM64 NEON และ Unified Memory
  - zbash แตกต่างจาก Bash และ Zsh อย่างไรในแง่ขนาด (86 KB), ประสิทธิภาพ และความเข้ากันได้
* **บทที่ 2: ปรัชญาการออกแบบ: 86 KB, Zero-Dependency, Bare-Metal**
  - กฎเหล็ก 3 ข้อของ zbash: True Drop-in Replacement, Bit-Exact Parity, และ Extreme Velocity
  - การพึ่งพาเพียง `libSystem.B` (หรือ `libc`) ตัดทิ้ง external libraries ทั้งหมด
* **บทที่ 3: สถาปัตยกรรมภาพรวม: จากคีย์บอร์ดสู่ OS Execution**
  - Flow การทำงานของ Shell Loop: Readline ➔ Lexer ➔ Parser ➔ AST ➔ Expander ➔ Evaluator ➔ Process Dispatch

### ภาคที่ 2: สถาปัตยกรรม Lexer & Parser (The Language Core)
* **บทที่ 4: ความลับของ Lexer: เร่งความเร็วด้วย ARM64 NEON SIMD 128-bit**
  - เจาะลึกโค้ด `simd_lexer.s`: การโหลดข้อมูล 16 ไบต์ลงเวกเตอร์รีจิสเตอร์ `v0`
  - การใช้คำสั่ง `cmeq`, `umaxv` ตรวจจับตัวแบ่งคำ (` `, `\t`, `\n`, quotes) ได้พร้อมกันใน 1 สัญญาณนาฬิกา
* **บทที่ 5: ปราบพยศไวยากรณ์: แกะรอย Quoting Hell (", ', \) และ Tokenizer**
  - ความซับซ้อนของการหลบอักขระ (Escaping) และความแตกต่างระหว่าง Single Quotes (Literal) กับ Double Quotes (Interpolated)
  - การจัดการ Token แบบ Dynamic Buffer โดยไม่เกิด Heap Fragmentation
* **บทที่ 6: การสร้าง Abstract Syntax Tree (AST) ขนาดกะทัดรัด**
  - โครงสร้าง Node ของ Shell: Simple Command, Pipeline, Logical AND/OR, Compound List
  - การออกแบบ AST struct ในภาษา C ให้กินแรมไม่เกิน 64 ไบต์ต่อโหนด

### ภาคที่ 3: เครื่องยนต์ขยายตัวแปรและคณิตศาสตร์ (Expansions & Evaluation)
* **บทที่ 7: ศิลปะการขยายตัวแปร: `${VAR:-def}`, `${#arr[@]}`, `${VAR:offset:len}`**
  - ลำดับขั้นการ Expand: Brace Expansion ➔ Parameter Expansion ➔ Command Substitution ➔ Arithmetic ➔ Word Splitting ➔ Pathname Expansion
  - การเขียนตัวแปลงสตริงแบบ Zero-Copy Slicing
* **บทที่ 8: เครื่องคิดเลขในตัว: สร้างตัวประเมินคณิตศาสตร์ `$(( a + b * c ))`**
  - การสร้าง Recursive Descent Parser สำหรับคำนวณนิพจน์คณิตศาสตร์ (Operator Precedence)
  - การรองรับตัวแปรในวงเล็บคณิตศาสตร์ เช่น `$(( x++ + y * 2 ))`
* **บทที่ 9: โครงสร้าง Indexed Arrays ใน C แบบไร้ Overhead**
  - จัดการอาร์เรย์ `arr[0]=val`, `arr[10]=val` (Sparse Array) อย่างไรโดยไม่ต้องจองหน่วยความจำมหาศาล

### ภาคที่ 4: เครื่องยนต์ประมวลผลคำสั่งและท่อไปป์ (Execution Engine & Plumbing)
* **บทที่ 10: ดำดิ่งระบบปฏิบัติการ: เข้าใจ `fork()`, `execve()`, และ Process Lifecycle**
  - การสร้าง Child Process ด้วย `fork()` และการส่งสัญญาณ Exit Code ผ่าน `waitpid()`
  - การค้นหา Path ใน `$PATH` อย่างรวดเร็ว
* **บทที่ 11: ท่อไปป์และการเปลี่ยนทิศทาง: `pipe()`, `dup2()`, และ `done < file`**
  - กายวิภาคของ Unix Pipe: การสลับ File Descriptors (0, 1, 2) ด้วย `dup2()`
  - ปัญหาคลาสสิกที่ทำให้ Shell ค้าง: ลืมปิด Write-End ของ Pipe ใน Parent Process!
  - การแก้บั๊ก Trailing Loop Redirection (`done < file`) ที่เราเจอในข้อทดสอบที่ #77
* **บทที่ 12: ลูปและเงื่อนไข: `while`, `for`, `until`, `if-then-else`, `case-esac`**
  - การรันคำสั่งเงื่อนไขแบบ Recursive Execution และการจัดการ `break` / `continue`
* **บทที่ 13: ความต่างระดับชีวิต: Subshells `(...)` vs Grouping `{...}`**
  - ทำไมตัวแปรที่เปลี่ยนใน Subshell ถึงหายไปเมื่อจบลูป? กลไกการ Fork Process ย่อย
* **บทที่ 14: ทำไมคำสั่ง `cd`, `export`, `read` จึงไม่สามารถเป็นโปรแกรมภายนอกได้?**
  - เบื้องหลัง Builtin Commands: กระบวนการที่ต้องเปลี่ยนแปลง State ของตัว Process แม่เอง

### ภาคที่ 5: มาตรฐานการทดสอบระดับโลก (1,000 Parity Tests)
* **บทที่ 15: "เร็วแต่ผลลัพธ์ผิด คือไร้ค่า": ปรัชญาการทดสอบระดับ Differential**
  - ทำไม Unit Test ธรรมดาจึงไม่เพียงพอสำหรับ Shell
  - การใช้ macOS `/bin/bash` เป็น Golden Truth แล้วเปรียบเทียบผลลัพธ์บิตต่อบิต (`cmp -s`)
* **บทที่ 16: ผ่าโครงสร้างชุดทดสอบ 10 โมดูล 1,000 ข้อ vs macOS Bash**
  - เจาะลึกการทดสอบ 10 หมวด: Builtins, Arithmetic, Variables, Loops, Conditionals, Functions, Pipelines, Subshells, Arrays, Stress Cases
* **บทที่ 17: บันทึกการล่าบั๊กสุดแสบ: Memory Leaks, Infinite Redirections**
  - กรณีศึกษาบั๊กจริง 5 ข้อที่เราค้นพบระหว่างการทดสอบ 1,000 ข้อและวิธีแก้ใน C

### ภาคที่ 6: ก้าวสู่อนาคตและบทเรียนชีวิต (Future & Mindset)
* **บทที่ 18: จาก Terminal สู่เบราว์เซอร์: พอร์ต zbash สู่ WebAssembly & WebOS**
  - การคอมไพล์ภาษา C สู่ WASM ด้วย Clang/Emscripten
  - การเชื่อมต่อ Virtual File System (VFS) และการรัน Shell บน Browser Canvas
* **บทที่ 19: บทส่งท้าย: ปลดปล่อยพลังแห่งการสร้างซอฟต์แวร์ระดับโลก**
  - สรุปแนวคิด: เมื่อความรู้พื้นฐานแน่นพอ นักพัฒนาคนเดียวก็สามารถสร้างระบบที่เขย่ามาตรฐานเดิมได้!

---

## 🛠️ 6. แผนการดำเนินงานและส่งมอบ (Execution Roadmap)

| เฟส (Phase) | รายละเอียดภารกิจ | ไฟล์ส่งมอบ | สถานะ (Status) |
|---|---|---|---|
| **Phase 1: โครงสร้างและรากฐาน** | ร่างโครงสร้าง 19 บท และเขียนบทนำ + ภาคที่ 1 (บทที่ 1-3) | `ch01` ถึง `ch03` | **เสร็จสมบูรณ์ 100% ✅** |
| **Phase 2: เจาะลึกภาษาและ Lexer** | เขียนภาคที่ 2 และภาคที่ 3 (Lexer, Parser, Expansions, Arrays) | `ch04` ถึง `ch09` | **เสร็จสมบูรณ์ 100% ✅** |
| **Phase 3: ระบบท่อไปป์และ Execution** | เขียนภาคที่ 4 (Plumbing, Redirection, Subshells) | `ch10` ถึง `ch14` | **เสร็จสมบูรณ์ 100% ✅** |
| **Phase 4: ชุดทดสอบและบทสรุป** | เขียนภาคที่ 5 และภาคที่ 6 (1,000 Tests, WASM, บทส่งท้าย) | `ch15` ถึง `ch19` | **เสร็จสมบูรณ์ 100% ✅** |
| **Phase 5: มาสเตอร์คลาสระบบขั้นสูง (Special)** | ภาคที่ 7: 6 บทพิเศษขยายสู่ 25 บท ([แผนงานใน p-20-25.md](p-20-25.md)) | `ch20` ถึง `ch25` | **เสร็จสมบูรณ์ 100% ✅** |

---

## 💎 7. คำประกาศเกียรติคุณของผู้ประพันธ์ (Author Dedication)
> *"หนังสือเล่มนี้ถูกสร้างขึ้นด้วยความคารวะต่อครูบาอาจารย์ผู้สร้างโลก Unix และอุทิศแด่นักพัฒนาซอฟต์แวร์ทุกคนที่มีความฝันอยากสร้างระบบปฏิบัติการของตนเอง เราจะไม่พูดถึงการค้าหรือธุรกิจใดๆ ในหน้านี้ แต่จะมอบทุกหยาดเหงื่อแห่งปัญญาทางวิศวกรรมให้เป็นสมบัติของโลกอย่างแท้จริง"*  
> — **วิโรจน์ ชูแก้ว (Wirot Chookeaw)**
