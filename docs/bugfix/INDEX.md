# 🪲 ZBASH BUGFIX ARCHIVES & 40-YEAR BASH QUIRKS CATALOG
> **"Master Registry of Unix/POSIX/Bash Quirks, Edge Cases & C Engineering Solutions"**  
> **สถาปนิกและผู้ดูแล:** วิโรจน์ ชูแก้ว (`chin6700x`) | **โครงการ:** ZBASH Engine  
> **ที่ตั้งคลังเอกสาร:** `zbash/docs/bugfix/` | **ใบอนุญาต:** Apache License 2.0 (c) 2026

---

## 🧭 วัตถุประสงค์ของคลังประวัติศาสตร์บั๊ก (The Mission)

ตลอดระยะเวลากว่า 40 ปีของ GNU Bash และ Unix Shells โลกของซอฟต์แวร์ติดอยู่ในวังวนของพฤติกรรมกำกวม (Ambiguous Behaviors) และกับดักทางสถาปัตยกรรมที่ไม่มีใครกล้าแก้ไข  
คลังเอกสารนี้จัดเก็บ **"แฟ้มประวัติเดี่ยว (Modular Bug Dossiers)"** แยกเป็นรายข้อ เพื่อให้:
1. **ค้นหาและอ้างอิงได้ง่าย:** แต่ละไฟล์มีรหัสเฉพาะ (ID), ตัวอย่างคำสั่งที่พัง (PoC), รากเหง้าประวัติศาสตร์ 40 ปี, และโค้ดภาษา C ที่แก้ไขใน ZBASH
2. **ขยายได้ไม่จำกัด (Zero Merge Conflicts):** เมื่อพบเคสใหม่ ไม่ว่าจะเป็นวิศวกรหรือ AI Agent สามารถเพิ่มไฟล์ `bug_XXX_<name>.md` ได้ทันที
3. **เป็นพิมพ์เขียวสำหรับผู้สร้าง Shell รุ่นถัดไป:** พิสูจน์ว่าเชลล์สมัยใหม่สามารถแทนที่ GNU Bash ได้แบบ 100% Drop-in โดยไม่ต้องแบกรับหนี้ทางเทคนิค 40 ปี

---

## 📋 สารบัญแฟ้มประวัติศาสตร์บั๊ก (Master Catalog)

### 🔴 หมวดที่ 1: การจัดการโปรเซสและสัญญาณเคอร์เนล (Process, Signals & Lifecycle)
* **[BUG-001: Zombie Process Table Accumulation](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_001_zombie_process_table.md)** — ปรากฏการณ์ซอมบี้ค้างฟ้าในตารางเคอร์เนล และ Watchdog วนลูปไม่รู้จบ
* **[BUG-002: The Exit Trap Short-Circuit Deadlock](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_002_exit_trap_deadlock.md)** — กับดัก Deadlock เมื่อคำสั่ง `exit` กีดกันไม่ให้ EXIT Trap ทำงาน
* **[BUG-009: Background Job Ampersand Boundary](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_009_background_ampersand_boundary.md)** — เครื่องหมาย `&` โดนกลืนเป็นอาร์กิวเมนต์ของโปรแกรมแทนที่จะรันเบื้องหลัง
* **[BUG-020: PID Non-Deterministic Test Invariance](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_020_pid_test_invariance.md)** — กับดัก Process ID สุ่มในชุดทดสอบอัตโนมัติ และการแก้ด้วย Axiom Validation
* **[BUG-027: Subshell Trap Reset & Non-Propagation](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_027_subshell_trap_inheritance.md)** — กฎแปลกของ POSIX ที่บังคับล้างสัญญาณ Trap ทุกตัวเมื่อเข้าสู่ Subshell
* **[BUG-028: Signal Interruption Races & Slow Syscalls](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_028_signal_eintr_restart_races.md)** — การแข่งขันของสัญญาณขัดจังหวะ (EINTR) ระหว่างคำสั่ง `wait`

---

### 🟡 หมวดที่ 2: ท่อส่งข้อมูลและการเปลี่ยนทิศทาง I/O (Pipelines & Redirections)
* **[BUG-003: Forgotten Pipe Descriptor Leak](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_003_forgotten_pipe_fd_leak.md)** — ท่อไปป์ค้างนิรันดร์เพราะ Shell แม่ลืมปิดปลายท่อเขียนในตัวเอง
* **[BUG-004: Terminal STDIN Destruction in Loop Redirection](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_004_terminal_stdin_destruction.md)** — ลูป `while ... done < file` ทำลายช่องทางคีย์บอร์ดของเทอร์มินัลถาวร
* **[BUG-006: Here-String Inode Exhaustion](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_006_here_string_inode_exhaustion.md)** — คำสั่ง `<<< "str"` สร้างไฟล์ชั่วคราวตกค้างจนผลาญ Inode หมดดิสก์
* **[BUG-014: Subshell Pipeline & Redirection Disconnection](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_014_subshell_pipeline_disconnection.md)** — วงเล็บ Subshell หลุดออกจากท่อไปป์เพราะ Fast-Path ลัดวงจร
* **[BUG-022: Process Substitution FIFO Lifetime Leak](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_022_process_substitution_fifo_leak.md)** — การรั่วไหลของ Named Pipe FIFO ในไวยากรณ์ `<(cmd)` และ `>(cmd)`
* **[BUG-025: The Lastpipe Conundrum & Environment Loss](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_025_lastpipe_subshell_isolation.md)** — ตัวแปรที่เปลี่ยนในท่อไปป์หายสาบสูญเพราะคำสั่งรันในกระบวนการลูก
* **[BUG-026: Redirection Evaluation Order Trap](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_026_redirection_order_duplication.md)** — ความแตกต่างระหว่าง `2>&1 >file` กับ `>file 2>&1` ที่สร้างความสับสนมา 40 ปี

---

### 🟢 หมวดที่ 3: ไวยากรณ์ สตริง และการขยายตัวแปร (Lexing & Expansions)
* **[BUG-008: Subshell Exit Code Masking in Assignments](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_008_subshell_exit_code_masking.md)** — คำสั่งกำหนดค่า `res=$(false)` กลืนกิน Exit Code 1 กลายเป็น 0
* **[BUG-012: Pattern Delimiter Slash Collision](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_012_pattern_delimiter_slash_collision.md)** — สแลชที่หนีหน้ากาก `\ /` ระเบิดไวยากรณ์การแทนที่ข้อความ `${path//\//_}`
* **[BUG-013: Trailing Dollar Quote Boundary Corruption](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_013_trailing_dollar_quote_corruption.md)** — เครื่องหมายดอลลาร์เปล่าหลุดกลืนกินเครื่องหมายคำพูดใน `echo "^$$$"`
* **[BUG-015: Prefix/Suffix Wildcard Globbing](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_015_prefix_suffix_wildcard_glob.md)** — การตัดทอน `${line#*:*:}` ล้มเหลวเพราะเอนจินใช้ `strstr` แทน POSIX Glob
* **[BUG-016: ANSI-C Escape Sequence Expansion](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_016_ansi_c_escape_expansion.md)** — รูปแบบ `$'Hello\nWorld'` ถูกพิมพ์ออกมาแบบดิบๆ เพราะละเลย Bashism สำคัญ
* **[BUG-018: IFS Whitespace vs Delimiter Ambiguity](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_018_ifs_whitespace_ambiguity.md)** — ความลึกลับของ IFS Whitespace (Coalescing) เทียบกับ Non-Whitespace Delimiter
* **[BUG-019: Array Multi-Argument Token Expansion](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_019_array_token_expansion.md)** — การแตกกระจายของ `"${arr[@]}"` ใน Argument Vector
* **[BUG-024: Word Splitting Traps in "$*" vs "$@"](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_024_star_vs_at_word_splitting.md)** — พฤติกรรมต่างกันสุดขั้วระหว่าง `"$*"` (Joined String) กับ `"$@"` (Distinct Words)
* **[BUG-031: UTF-8 Multibyte Character Boundary Slicing & Length](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_031_utf8_multibyte_character_slicing.md)** — การคำนวณความยาว `${#var}` และการตัดคำ `${var:offset:len}` บนอักขระหลายไบต์ ภาษาไทย และ Emoji
* **[BUG-032: Here-Document Unquoted Parameter Expansion & Semicolon Sequencing](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_032_heredoc_unquoted_parameter_expansion.md)** — การขยายค่าตัวแปรใน Here-Doc เมื่อ Delimiter ไม่ถูก Quote และการรันคำสั่งกำหนดตัวแปรก่อนหน้า Semicolon
* **[BUG-033: Array Append Syntax (arr+=(...)) & IFS Delimited Joining ("${arr[*]}")](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_033_array_append_and_ifs_star_expansion.md)** — การต่อสมาชิกอาร์เรย์ด้วยไวยากรณ์ `+=` และการเชื่อมต่อสตริงอาร์เรย์ด้วยอักขระตัวแรกของ `$IFS`
* **[BUG-035: Case Statement Keyword Collision & Semicolon Splitting](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_035_case_esac_keyword_and_semicolon_shattering.md)** — การกำกับคำสำคัญ `case ... in ... esac` และการป้องกันตัวคั่น `;;` แตกเป็นคำสั่งย่อย

---

### 🔵 หมวดที่ 4: การคำนวณ ชนิดข้อมูล และสโคป (Arithmetic, Types & Scoping)
* **[BUG-005: Recursive Function Local Scope Smashing](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_005_recursive_scope_smashing.md)** — ตัวแปร `local` ทะลุข้ามมิติในฟังก์ชัน Recursion เพราะไม่มี Activation Records
* **[BUG-007: SIGFPE Hardware Trap Crash on Zero Division](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_007_sigfpe_hardware_trap.md)** — การคำนวณ `$(( 10 / 0 ))` กระชากฮาร์ดแวร์ซีพียูทำลายทั้งโปรเซส
* **[BUG-010: Arithmetic Parenthesis Nesting Depth Failure](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_010_arithmetic_paren_nesting.md)** — นิพจน์ซ้อน `$(( (1 << (32 - p)) - 2 ))` พังกลางคันจากวงเล็บชั้นใน
* **[BUG-011: Radix Literal Truncation in Base#Num](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_011_radix_literal_truncation.md)** — ตัวเลขต่างฐาน เช่น `$(( 16#A0 ))` ถูกตัดทอนเหลือแค่เลขฐาน 16
* **[BUG-017: Integer Operand Parsing Crash in Test Builtin](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_017_integer_operand_parsing_crash.md)** — คำสั่ง `[ "apple" -eq 0 ]` กลายเป็นจริงเพราะ `atoi()` เงียบ
* **[BUG-021: The Infamous "set -e" Subshell Abort Madness](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_021_errexit_pipefail_subshell_madness.md)** — ความบ้าคลั่งของ `set -e` (`errexit`) ในเงื่อนไข `if`, `||` และ Subshells
* **[BUG-029: Arithmetic Short-Circuiting in Ternary Operator](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_029_arithmetic_short_circuit_ternary.md)** — ตัวดำเนินการเงื่อนไข `$(( c ? a : b ))` ประเมินฝั่งที่ไม่ควรประเมินจนแครช
* **[BUG-030: PATH Hashing and Stale Binary Execution Traps](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_030_path_hashing_invalidation.md)** — กลไก Hash Table แคชตำแหน่งโปรแกรมของ Bash ทำให้สคริปต์รันไบนารีเก่าแม้จะลงตัวใหม่แล้ว
* **[BUG-034: POSIX 3-Argument Test Operator Precedence](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_034_posix_three_argument_test_precedence.md)** — กฎเกณฑ์ลำดับความสำคัญของตัวดำเนินการทวิภาคใน `[ ! = esac ]` เหนือเครื่องหมายปฏิเสธเดี่ยว
* **[BUG-036: The Complete POSIX "set -e" Suppression Hierarchy](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_036_complete_errexit_suppression_hierarchy.md)** — โครงสร้างลำดับชั้นการระงับ `set -e` ในเงื่อนไข `if/while/until`, เครื่องหมายปฏิเสธ `!`, และฝั่งซ้ายของ AND-OR Lists (`&&`, `||`)
* **[BUG-037: Darwin BSD mktemp Strict Template & FIFO Watchdog](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_037_darwin_bsd_mktemp_and_fifo_watchdog.md)** — ข้อกำหนดเฉพาะของคำสั่ง `mktemp` บน macOS Darwin และการสร้างระบบป้องกัน Runner Hang ด้วย Watchdog Timer
* **[BUG-038: Variable Attributes System, Array Subscript Assignment, & Prefix Environment Restoration](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_038_variable_attributes_array_subscript_and_prefix_env.md)** — ระบบ Attribute ตัวแปร (`typeset -i`, `readonly`), การคำนวณและกำหนดค่าช่องอาร์เรย์ (`[idx]=val`, `[idx]+=val`), และการคืนค่าสภาพแวดล้อมเดิมของ Prefix Assignment
* **[BUG-039: Arithmetic For-Loop Whitespace/Empty Conditions, Quoted Sub-Expressions, Braced Loops, & Type Builtin Function Unparser](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_039_arithmetic_for_empty_cond_quoted_expr_and_type_builtin.md)** — การประเมินเงื่อนไขว่างในลูปคณิตศาสตร์แบบ C, การถอด Quote นิพจน์ซ้อน, ลูปปีกกา `{ ... }`, และการแสดงผล Function AST ในคำสั่ง `type`
* **[BUG-040: Arithmetic Short-Circuit Side-Effects, Radix 2-64 Encoding, Consecutive Operators, & Nested Subshell Disambiguation](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_040_arithmetic_short_circuit_radix64_and_nested_subshells.md)** — กลไก Short-Circuit ข้ามผลข้างเคียง, ระบบถอดรหัสเลขต่างฐาน 2..64, การแยกโทเค็นตัวดำเนินการติดกัน (`4+++a`), และการแยกแยะ Subshell ซ้อนกับคำสั่งคณิตศาสตร์
* **[BUG-041: Sparse Array Compound Expansion, Nested Parameter Lexing, & Array-Scalar Assignment Parity](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_041_sparse_arrays_compound_expansion_and_declare_parity.md)** — การสแกนพารามิเตอร์ซ้อนพร้อม Bracket Depth, การจัดลำดับตัวแปรอาร์เรย์ในวงเล็บปีกกา, การผสานการกำหนดค่าสเกลาร์กับอาร์เรย์, และการคงสภาพ Compound Assignment
* **[BUG-042: Brace Expansion Engine, Backtick Pipe Shielding, Lone Backslash Stripping, & Readonly Assignment Timing](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_042_brace_expansion_pipeline_backticks_and_lone_backslash.md)** — การสร้างเอนจินขยายวงเล็บปีกกา `{start..end}` และ `{a,b,...}`, การปกป้องไพป์ไลน์ภายใน Backticks, การถอด Unquoted Backslash เดี่ยว, และการปรับลำดับการล็อกตัวแปร Readonly ใน Declare Builtin
* **[BUG-043: Builtins Cluster Architecture, Stdout Fork Buffer Bleed, POSIX Prefix Persistence, & Exec Vector Isolation](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_043_builtins_cluster_umask_enable_kill_cdpath_and_sourcing.md)** — การสร้างชุดคำสั่ง Builtin ตระกูล POSIX (`umask`, `enable`, `kill`, `trap`, `times`, `exec`, `cd` / `$CDPATH`), การป้องกัน Libc Stdio Buffer Bleed ข้าม Fork ใน Subshell Command Substitution, การคงสภาพตัวแปร Prefix Environment ใน Special Builtins, และการแยกสโคป Positional Parameters ในการเรียก Source พร้อม Arguments
* **[BUG-044: Extended Globbing, POSIX Collating Symbols, Bracket Matching, and Case Clause Nesting Parity](file:///Volumes/Mac%20Backup/zero-tools/zbash/docs/bugfix/bug_044_extended_globbing_and_posix_patterns.md)** — การแยกทางเลือกในคำสั่ง `case` พร้อมตรวจสอบความลึกวงเล็บ Extglob, สัญลักษณ์เทียบเคียง POSIX และคลาสสมมูล `[[.symbol.]]`, `[[=c=]]`, การรักษาสภาพ Backslash ใน Case Patterns, และการป้องกันการยุบเครื่องหมาย `*` หน้า `*(`

---

## 🛠️ วิธีการเพิ่มแฟ้มบันทึกบั๊กใหม่ (How to Add a New Bug Dossier)

เมื่อวิศวกรหรือ AI Agent ค้นพบบั๊กใหม่ หรือแก้ Edge Case ได้:
1. สร้างไฟล์ใหม่ในไดเรกทอรีนี้: `zbash/docs/bugfix/bug_XXX_<name>.md`
2. กรอกข้อมูลให้ครบทั้ง 6 ส่วนตาม Schema ใน [`zbash/AGENTS.md`](file:///Volumes/Mac%20Backup/zero-tools/zbash/AGENTS.md)
3. เพิ่มลิงก์และคำอธิบายสั้นๆ เข้ามาในสารบัญของไฟล์ `INDEX.md` นี้
4. ตรวจสอบว่าคำสั่งทดสอบ Regression Test รันผ่าน 100%

---
*ปรับปรุงล่าสุด: 10 ตุลาคม 2026 | ZBASH Core Engineering Team*
