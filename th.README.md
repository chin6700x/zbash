# zbash

ตัวแปลภาษาคำสั่ง (Command Language Interpreter) และสภาพแวดล้อมเชลล์เชิงโต้ตอบสมรรถนะสูง พัฒนาขึ้นใหม่ตั้งแต่ระดับรากฐาน (First Principles) ด้วยภาษา ISO C11 ร่วมกับ ARM64 NEON Assembly

---

## 1. ปรัชญาการออกแบบและจุดมุ่งหมาย

zbash ถูกสร้างขึ้นมาเพื่อคิดค้นและออกแบบเลเยอร์การประมวลผลคำสั่งใหม่ทั้งหมดตั้งแต่ระดับฮาร์ดแวร์ซิลิคอนขึ้นมา โดยมุ่งเน้นความเร็วระดับสูงสุด การใช้หน่วยความจำที่กะทัดรัด และการยกระดับประสบการณ์การทำงานของนักพัฒนาให้คล่องตัวสูงสุด โดยปราศจากภาระโค้ดโบราณตกทอด

### วัตถุประสงค์หลัก

- **ความเร็ว Cold-Spawn ต่ำกว่าระดับมิลลิวินาที:** เริ่มต้นทำงานและรันคำสั่งได้ภายในเวลาไม่ถึง 0.5 ms เหมาะสำหรับงานสคริปต์ความถี่สูง ระบบอัตโนมัติ และการเปิดหน้าต่างเทอร์มินัลได้ทันทีในพริบตา
- **การแยกคำ (Lexing) แบบฮาร์ดแวร์เวกเตอร์:** ใช้เอนจิน Assembly สถาปัตยกรรม ARM64 NEON (`simd_lexer.s`) สแกนโทเค็นและข้ามตัวแบ่งวรรคตอนทีละ 16 ไบต์ใน 1 รอบสัญญาณนาฬิกา ผ่านรีจิสเตอร์เวกเตอร์ 128 บิต (`q0-q5`, `v16-v23`) รองรับ Throughput บัสหน่วยความจำสูงกว่า 35+ GB/s
- **ขนาดไฟล์กะทัดรัดระดับไมโคร:** คอมไพล์เป็นไบนารีเนทีฟแบบเบ็ดเสร็จในตัวเองขนาดเพียง ~70 KB ออกแบบมาให้ฟิตพอดีกับ L2 Cache ของซีพียู ช่วยตัดปัญหา Cache Thrashing โดยสิ้นเชิง และใช้หน่วยความจำเริ่มต้น (Resident Memory) ต่ำกว่า 0.5 MB
- **ไร้การพึ่งพาไลบรารีภายนอก (Zero External Dependencies):** พัฒนาขึ้นด้วย ISO C11 และ System Primitives มาตรฐานโดยตรง ไม่ต้องพึ่งพาไลบรารีเทอร์มินัลภายนอก หรือรันไทม์ขนาดใหญ่
- **การสตรีมท่อไปป์แบบ Zero-Copy:** กลไกการส่งต่อ Descriptor โดยตรง ช่วยลดภาระ Kernel Context Switching ขณะประมวลผลข้อมูลผ่านไปป์หลายทอด
- **สรีรศาสตร์เชิงโต้ตอบชั้นยอด:** รวมระบบเน้นสีไวยากรณ์แบบเรียลไทม์ (Syntax Highlighting), แถบริบบอนอธิบายแฟล็กคำสั่ง, การตรวจสอบสถานะ Git Repository ในตัว และโปรแกรมแก้ไขข้อความเชิงภาพแบบโมดัล (`zedit`) ฝังอยู่ในไบนารีหลักโดยตรง
- **สถาปัตยกรรมซิลิคอนแบบ Dual-Target:** สามารถคอมไพล์เป็นเนทีฟไบนารี Apple Silicon Darwin Mach-O และ WebAssembly (`zbash.wasm`) สำหรับการทำงานบนเว็บเบราว์เซอร์และ Cloud Sandbox

---

## 2. การสร้างและติดตั้ง (Building and Installation)

### สิ่งที่ต้องเตรียมล่วงหน้า

- macOS บนสถาปัตยกรรม Apple Silicon (ARM64) หรือสภาพแวดล้อม Linux POSIX
- คอมไพเลอร์ภาษา C ที่รองรับมาตรฐาน ISO C11 (Clang หรือ GCC)
- GNU Make

### การคอมไพล์จากซอร์สโค้ด

```sh
git clone https://github.com/chin6700x/zbash.git
cd zbash
make
```

ไฟล์ไบนารีที่คอมไพล์เสร็จสมบูรณ์จะอยู่ที่ `bin/zbash`

### การรันชุดทดสอบความถูกต้อง

รันชุดทดสอบครอบคลุมทุกระบบ (การตรวจไวยากรณ์, การทำงานเชิงโต้ตอบ, ตัวแก้ไขข้อความ และตัวแยกวิเคราะห์คำสั่ง):

```sh
make test
```

### การติดตั้งลงในระบบ

ติดตั้งไบนารี `zbash` ไปยัง `/usr/local/bin` สำหรับใช้งานทั่วทั้งระบบ:

```sh
sudo cp bin/zbash /usr/local/bin/zbash
sudo chmod 755 /usr/local/bin/zbash
```

---

## 3. ตัวเลือกและแฟล็กคำสั่ง (Invocation and Command-Line Options)

`zbash` รองรับการเรียกใช้งานทั้งแบบเซสชันโต้ตอบและสคริปต์อัตโนมัติ:

```text
zbash [-exnvi] [-c string | script_file [arguments...]]
```

| แฟล็ก / ออปชัน | คำอธิบาย |
|---|---|
| `-c string` | อ่านและประมวลผลคำสั่งจากข้อความในสตริงโดยตรง อาร์กิวเมนต์ที่เหลือจะถูกส่งเป็นพารามิเตอร์ตำแหน่ง (`$1`, `$2`, ...) |
| `-i` | บังคับให้ทำงานในโหมดโต้ตอบ (Interactive Shell) เปิดใช้งานการแก้ไขบรรทัด, การแสดงผลพร้อมท์ และประวัติคำสั่ง |
| `-s` | อ่านคำสั่งตามลำดับจาก Standard Input |
| `-e` | ยุติการทำงานทันทีหากมีคำสั่งหรือไปป์ไลน์ที่คืนค่าสถานะผิดพลาด (`errexit`) |
| `-x` | พิมพ์คำสั่งและอาร์กิวเมนต์ที่ถูกขยายค่าแล้วออกทาง stderr ก่อนเริ่มรันคำสั่งจริง (`xtrace`) |
| `-n` | อ่านคำสั่งเพื่อตรวจสอบความถูกต้องของไวยากรณ์โดยไม่รันคำสั่งจริง (`noexec`) |
| `-v` | พิมพ์บรรทัดคำสั่งที่ป้อนเข้ามาทันทีที่อ่านพบ (`verbose`) |
| `-r` | ทำงานในโหมดจำกัดสิทธิ์ (Restricted Mode ล็อก `$PATH`, `$SHELL` และการเปลี่ยนไดเรกทอรี) |
| `--norc` | ไม่โหลดไฟล์คอนฟิกูเรชันเริ่มต้นของผู้ใช้ (`~/.zbashrc`) |
| `--noprofile` | ไม่โหลดสคริปต์โปรไฟล์ล็อกอิน (`/etc/zprofile`, `~/.zprofile`) |
| `--help` | แสดงสรุปตัวเลือกการใช้งานและออกจากโปรแกรม |
| `--version` | แสดงข้อมูลเวอร์ชัน สถาปัตยกรรมเป้าหมาย และสัญญาอนุญาต |

---

## 4. คู่มือการใช้งาน (Usage Guide)

### 4.1 การรันสคริปต์

#### การรันไฟล์สคริปต์โดยตรง
ระบุพาธของไฟล์สคริปต์เป็นอาร์กิวเมนต์แรก:

```sh
zbash deploy.sh production
```

#### การใช้ส่วนหัว Shebang
กำหนด `zbash` ในบรรทัดแรกของไฟล์สคริปต์:

```sh
#!/usr/local/bin/zbash
set -e

echo "Starting automated task..."
for item in alpha beta gamma; do
    echo "Processing: ${item}"
done
```

กำหนดสิทธิ์ให้ไฟล์สามารถทำงานได้และสั่งรัน:

```sh
chmod +x deploy.sh
./deploy.sh
```

#### การรันคำสั่งแบบ Inline
ประมวลผลชุดคำสั่งได้ทันทีโดยไม่ต้องสร้างไฟล์สคริปต์:

```sh
zbash -c 'for i in 1 2 3; do echo "Count: $i"; done'
```

---

### 4.2 สภาพแวดล้อมแบบโต้ตอบ (Interactive Environment)

เมื่อเปิดใช้งาน `zbash` โดยไม่ระบุอาร์กิวเมนต์ในหน้าต่างเทอร์มินัล โปรแกรมจะเข้าสู่โหมดโต้ตอบโดยอัตโนมัติ:

```sh
zbash
```

#### โครงสร้างพร้อมท์แบบ 2 บรรทัด (Two-Line Structured Prompt)
พร้อมท์ได้รับการจัดวางองค์ประกอบข้อมูลและการป้อนคำสั่งอย่างเป็นสัดส่วน:

```text
╭─ zbash in ~/projects/engine on git:(main)*
╰─❯ 
```

- **บรรทัดบน (แถบบริบท):** แสดงชื่อเชลล์, ไดเรกทอรีปัจจุบัน (ย่อด้วย `~`), และสถานะสาขาของ Git Repository
- **บรรทัดล่าง (แถบป้อนข้อมูล):** แสดงไฮไลต์สีไวยากรณ์ตามเวลาจริง พร้อมตัวระบุรหัสสถานะความผิดพลาดเมื่อคำสั่งก่อนหน้าไม่สำเร็จ

#### คุณสมบัติเด่นในโหมดโต้ตอบ
- **Real-Time Syntax Highlighting:** คำสั่งที่ถูกต้องจะแสดงไฮไลต์สีชัดเจน ในขณะที่คำสั่งที่ไม่ถูกต้องหรือพิมพ์ผิดจะมีการแจ้งเตือนล่วงหน้าก่อนกด Enter
- **Context-Aware Completion:** กดปุ่ม `Tab` เพื่อเติมเต็มชื่อไฟล์, พาธ, คำสั่ง Built-in และตัวแปรสภาพแวดล้อมโดยอัตโนมัติ
- **Flag Documentation Ribbons:** เมื่อพิมพ์แฟล็กคำสั่งทั่วไป (เช่น `-l`, `-v`) จะมีแถบริบบอนอธิบายความหมายของแฟล็กปรากฏขึ้นเหนือเคอร์เซอร์
- **Persistent History:** บันทึกประวัติคำสั่งลงในไฟล์ `~/.zbash_history` และซิงค์ข้ามหน้าต่างเทอร์มินัล
- **Embedded Editor (`zedit`):** เรียกใช้งานโปรแกรมแก้ไขข้อความเชิงภาพแบบโมดัลที่ฝังอยู่ในตัวได้ทันทีโดยไม่ต้องออกจากเชลล์:

```sh
zedit config.txt
```

---

### 4.3 การอ้างอิงไวยากรณ์ภาษาและสคริปต์ (Scripting Language Reference)

#### ตัวแปรและการขยายพารามิเตอร์ (Variables & Parameter Expansion)
```sh
# การกำหนดค่าตัวแปร
NAME="Orion"
PORT=8080

# การขยายค่าพื้นฐาน
echo "Service ${NAME} listening on port ${PORT}"

# พารามิเตอร์ตำแหน่งและตัวแปรพิเศษ
echo "Script name: $0"
echo "First argument: $1"
echo "All arguments: $@"
echo "Argument count: $#"
echo "Process PID: $$"
echo "Last exit status: $?"

# การกำหนดค่าเริ่มต้นเมื่อตัวแปรว่าง
echo "${TIMEOUT:-30}"
```

#### อาร์เรย์และการสไลซ์ (Arrays & Slicing)
```sh
# การประกาศอาร์เรย์
modules=(core parser engine codegen)

# การเข้าถึงสมาชิก
echo "First module: ${modules[0]}"
echo "All modules: ${modules[@]}"
echo "Element count: ${#modules[@]}"

# การสไลซ์อาร์เรย์ (ตำแหน่งเริ่มต้น:จำนวนสมาชิก)
echo "Slice: ${modules[@]:1:2}"
```

#### ท่อไปป์และการเปลี่ยนเส้นทาง I/O (Pipelines & Redirection)
```sh
# ไปป์ไลน์หลายทอด
cat access.log | grep "ERROR" | cut -d' ' -f5 | sort | uniq -c

# การเปลี่ยนเส้นทางเอาต์พุต (เขียนทับ และต่อท้าย)
echo "Init" > run.log
echo "Step 1" >> run.log

# การเปลี่ยนเส้นทางข้อผิดพลาด (stderr)
command_that_might_fail 2> error.log
command_with_combined_output > all.log 2>&1

# การเปลี่ยนเส้นทางอินพุต
while read line; do
    echo "Line: ${line}"
done < input.txt
```

#### โครงสร้างควบคุมทิศทางการทำงาน (Control Flow Structures)

**เงื่อนไข (`if` / `elif` / `else`):**
```sh
if [ -f "/etc/config.json" ]; then
    echo "Configuration exists"
elif [ -d "/etc/config.d" ]; then
    echo "Configuration directory exists"
else
    echo "No configuration found"
fi
```

**การวนลูป (`for`, `while`, `until`):**
```sh
# ลูปตามรายการคำ
for target in arm64 x86_64 wasm; do
    echo "Compiling for ${target}"
done

# ลูปแบบมีเงื่อนไข (while)
COUNT=5
while [ $COUNT -gt 0 ]; do
    echo "Countdown: $COUNT"
    COUNT=$((COUNT - 1))
done

# ลูปไม่สิ้นสุดพร้อมคำสั่ง break
while true; do
    read -p "Enter command (q to quit): " user_input
    if [ "$user_input" = "q" ]; then
        break
    fi
done
```

**ตรรกะแบบแยกกิ่ง (Branch Logic):**
```sh
make build && echo "Build succeeded" || echo "Build failed"
```

#### นิพจน์ทางคณิตศาสตร์ (Arithmetic Expressions)
```sh
# การคำนวณและกำหนดค่าตัวแปรในตัว
(( count = 10 ))
(( count += 5 ))

# การคำนวณและแทนค่าในข้อความ
TOTAL=$(( 100 * 4 + 25 ))
echo "Result: $TOTAL"
```

#### การสร้างฟังก์ชันและขอบเขตตัวแปร (Functions & Scoping)
```sh
calculate_checksum() {
    local filepath="$1"
    if [ ! -f "$filepath" ]; then
        echo "File not found" >&2
        return 1
    fi
    shasum -a 256 "$filepath"
}

calculate_checksum "bin/zbash"
```

#### การดักจับสัญญาณและคืนทรัพยากร (Signal Traps & Cleanup)
```sh
cleanup() {
    echo "Cleaning temporary files..."
    rm -f /tmp/work_*.tmp
}

trap cleanup EXIT INT TERM
```

---

## 5. ตารางอ้างอิงคำสั่ง Built-in (Built-in Commands Reference)

`zbash` บรรจุคำสั่ง Built-in สำคัญที่ประมวลผลภายในโพรเซสด้วยความเร็วสูงสุด:

| คำสั่ง | รูปแบบการเรียกใช้งาน | คำอธิบายการทำงาน |
|---|---|---|
| `cd` | `cd [dir]` | เปลี่ยนไดเรกทอรีทำงานปัจจุบัน (รองรับ `cd -` เพื่อกลับไปยังไดเรกทอรีเดิม) |
| `pwd` | `pwd [-L \| -P]` | พิมพ์พาธสัมบูรณ์ของไดเรกทอรีทำงานปัจจุบัน |
| `echo` | `echo [-n] [args...]` | พิมพ์ข้อความโดยคั่นด้วยช่องว่างและตามด้วยการขึ้นบรรทัดใหม่ |
| `printf` | `printf format [args...]` | จัดรูปแบบและพิมพ์ข้อมูลตามสตริง Format |
| `export` | `export [name[=val]...]` | ส่งออกตัวแปรไปยังโพรเซสลูก |
| `unset` | `unset [-v] [name...]` | ยกเลิกการประกาศตัวแปรหรือฟังก์ชัน |
| `declare` | `declare [-r \| -i \| -x] [name=val]` | ประกาศตัวแปรพร้อมกำหนดคุณลักษณะ (อ่านอย่างเดียว, จำนวนเต็ม, ส่งออก) |
| `test` / `[` | `[ expr ]` | ประเมินค่านิพจน์เงื่อนไข (การตรวจสอบไฟล์, การเปรียบเทียบสตริง, จำนวนเต็ม) |
| `read` | `read [-r] [-p prompt] [var...]` | อ่านข้อมูลหนึ่งบรรทัดจาก Standard Input เข้าสู่ตัวแปร |
| `exit` | `exit [code]` | จบการทำงานของเชลล์พร้อมส่งรหัสสถานะออกไป |
| `exec` | `exec command [args...]` | แทนที่อิมเมจของโพรเซสเชลล์ด้วยคำสั่งเป้าหมายทันที |
| `eval` | `eval [args...]` | รวมอาร์กิวเมนต์เข้าด้วยกันและประมวลผลในฐานะคำสั่งเชลล์ |
| `source` / `.` | `source file [args...]` | โหลดและรันคำสั่งจากไฟล์เข้ามาในสภาพแวดล้อมเชลล์ปัจจุบัน |
| `pushd` | `pushd [dir]` | ดันไดเรกทอรีลงในสแตกไดเรกทอรีและสลับโฟลเดอร์ |
| `popd` | `popd` | ดึงไดเรกทอรีออกจากสแตกและเปลี่ยนกลับไปยังโฟลเดอร์นั้น |
| `dirs` | `dirs [-v]` | แสดงรายการในสแตกไดเรกทอรี |
| `history` | `history [-c]` | แสดงประวัติการป้อนคำสั่ง หรือล้างข้อมูลประวัติ |
| `jobs` | `jobs` | แสดงรายการงานที่กำลังทำงานอยู่เบื้องหลังพร้อม PID |
| `fg` | `fg [job_id]` | ดึงงานเบื้องหลังขึ้นมาทำงานด้านหน้า |
| `bg` | `bg [job_id]` | สั่งให้งานที่ถูกหยุดไว้ทำงานต่อในเบื้องหลัง |
| `trap` | `trap [action] [signal...]` | ลงทะเบียนฟังก์ชันจัดการสัญญาณระบบและเหตุการณ์วงจรชีวิต |
| `caller` | `caller [depth]` | ตรวจสอบ Call Frame และ Stack Trace ของฟังก์ชัน |
| `zedit` | `zedit [file]` | เปิดโปรแกรมแก้ไขข้อความเชิงภาพแบบโมดัลในตัว |

---

## 6. ลำดับการอ่านไฟล์คอนฟิกูเรชัน (Startup Files & Configuration)

ในขั้นตอนการเริ่มต้นทำงาน `zbash` จะค้นหาและโหลดไฟล์คอนฟิกตามลำดับดังนี้:

1. `/etc/zprofile` - การตั้งค่าระดับระบบสำหรับเซสชันการล็อกอิน
2. `~/.zprofile` - การตั้งค่าระดับผู้ใช้สำหรับเซสชันการล็อกอิน
3. `~/.zbashrc` - การตั้งค่าระดับผู้ใช้สำหรับโหมดโต้ตอบทั่วไป (แนะนำสำหรับการตั้งค่าตัวแปร PATH, นามแฝง และฟังก์ชัน)
4. `~/.zbash_history` - ไฟล์บันทึกประวัติคำสั่งเชิงโต้ตอบ

ตัวอย่างไฟล์คอนฟิก `~/.zbashrc`:

```sh
# กำหนดพาธสภาพแวดล้อม
export PATH="$HOME/.local/bin:/usr/local/bin:$PATH"
export EDITOR="zedit"

# นามแฝงคำสั่ง (Aliases)
alias ll="ls -la"
alias g="git"

# ฟังก์ชันอำนวยความสะดวก
mkcd() {
    mkdir -p "$1" && cd "$1"
}
```

---

## 7. เป้าหมาย WebAssembly (WebAssembly Target)

`zbash` สามารถคอมไพล์เป็น WebAssembly เพื่อรันบนเว็บเบราว์เซอร์, Node.js หรือสภาพแวดล้อม WASM รันไทม์ได้โดยตรง:

```sh
# คอมไพล์เป็น WebAssembly Core
emcc -O3 -DZBASH_WASM -Iinclude src/core/*.c src/parser/*.c src/builtins/*.c -o bin/zbash.wasm
```

โมดูล WASM ที่ได้จะทำหน้าที่เป็นเอนจินแยกคำสั่งและประมวลผลในหน่วยความจำแบบปิด (Isolated Sandbox) ปราศจากช่องโหว่ต่อระบบโฮสต์

---

## 8. สัญญาอนุญาต (License)

zbash เผยแพร่ภายใต้สัญญาอนุญาต **Apache License 2.0** ตรวจสอบรายละเอียดฉบับเต็มได้ที่ไฟล์ [LICENSE](LICENSE)
