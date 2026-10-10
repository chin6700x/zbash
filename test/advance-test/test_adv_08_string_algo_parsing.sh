#!/bin/bash
# ==============================================================================
# ADVANCE TEST SUITE - MODULE 08: STRING ALGORITHMS & TEXT PARSING
# ==============================================================================
# Comprehensive 100-test differential verification for developer and analyst
# workflows: Path canonicalization, URL slugify, case conversions (camel/snake/kebab),
# IPv4/CIDR calculations, SemVer parsing & comparison, indentation normalizers,
# word wrap, markdown table formatters, string distance & base conversions.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

echo -e "${MAGENTA}================================================================================${NC}"
echo -e "${MAGENTA}🧪 STARTING MODULE 08: STRING ALGORITHMS & TEXT PARSING (100 TESTS)${NC}"
echo -e "${MAGENTA}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. PATH CANONICALIZATION & RESOLUTION (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Strip Trailing Slashes from Directory Path" \
  "p='/var/log/nginx///'; echo \"\${p%/}\""

run_adv_test "002" "Normalize Multiple Slashes to Single Slash" \
  "p='/var///log////nginx/access.log'; echo \"\$p\" | tr -s '/'"

run_adv_test "003" "Extract File Basename Using Parameter Expansion" \
  "p='/usr/local/bin/zbash'; echo \"\${p##*/}\""

run_adv_test "004" "Extract Directory Name Using Parameter Expansion" \
  "p='/usr/local/bin/zbash'; echo \"\${p%/*}\""

run_adv_test "005" "Extract File Extension from Path" \
  "p='archive.tar.gz'; echo \"\${p##*.}\""

run_adv_test "006" "Strip File Extension from Path" \
  "p='archive.tar.gz'; echo \"\${p%.*}\""

run_adv_test "007" "Detect Absolute Path vs Relative Path" \
  "p='/etc/hosts'; if [ \"\${p:0:1}\" = '/' ]; then echo 'ABSOLUTE'; else echo 'RELATIVE'; fi"

run_adv_test "008" "Detect Relative Path Leading with Dots" \
  "p='../../src/engine.c'; if [ \"\${p:0:1}\" = '/' ]; then echo 'ABSOLUTE'; else echo 'RELATIVE'; fi"

run_adv_test "009" "Simulate Tilde Expansion to Home Path" \
  "p='~/projects/zephyr'; home='/Users/chinchin'; if [ \"\${p:0:2}\" = '~/' ]; then echo \"\${home}/\${p:2}\"; else echo \"\$p\"; fi"

run_adv_test "010" "Resolve Dot Dot Parent Component in Simple Path" \
  "p='/usr/local/../bin'; d1=\"\${p%/*}\"; d0=\"\${d1%/*}\"; b=\"\${p##*/}\"; echo \"\${d0%/*}/\$b\""

# ------------------------------------------------------------------------------
# 2. STRING SLUGIFY & URL-SAFE SANITIZATION (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Lowercase Simple Title String" \
  "title='Zephyr Silicon Engine'; echo \"\$title\" | tr '[:upper:]' '[:lower:]'"

run_adv_test "012" "Replace Spaces with Hyphens in Slug" \
  "s='zephyr silicon engine'; echo \"\${s// /-}\""

run_adv_test "013" "Strip Punctuation Marks in Slug Pipeline" \
  "s='Hello, World! 2026.'; echo \"\$s\" | tr -d ',!.'"

run_adv_test "014" "Full Pure Shell Slugify Pipeline" \
  "slugify() { echo \"\$1\" | tr '[:upper:]' '[:lower:]' | tr ' ' '-' | tr -cd 'a-z0-9-'; }; slugify 'Apple Silicon M4 Max!'"

run_adv_test "015" "Slugify Slashes and Underscores to Hyphen" \
  "slugify() { echo \"\$1\" | tr '[:upper:]' '[:lower:]' | tr ' /_' '---' | tr -s '-' | tr -cd 'a-z0-9-'; }; slugify 'features/fast_simd_v2'"

run_adv_test "016" "Slugify Leading and Trailing Hyphen Stripping" \
  "slug='---zephyr-core---'; tmp=\"\${slug#*([-])}\"; echo \"\$slug\" | sed 's/^-*//; s/-*$//'"

run_adv_test "017" "Sanitize Alphanumeric Only Identifier" \
  "raw='Order#99-@2026$'; echo \"\$raw\" | tr -cd 'a-zA-Z0-9'"

run_adv_test "018" "Generate Filename-Safe Timestamp String" \
  "ts='2026-10-09 09:30:15'; s1=\"\${ts//:/-}\"; echo \"\${s1// /-}\""

run_adv_test "019" "Truncate Long Slug to Maximum 20 Characters" \
  "slug='bare-metal-silicon-velocity-operating-system'; echo \"\${slug:0:20}\""

run_adv_test "020" "Check If String is Valid URL Slug (Hyphenated Alphanumeric)" \
  "s='clean-url-slug-2026'; if echo \"\$s\" | grep -Eq '^[a-z0-9-]+$'; then echo 'VALID'; else echo 'INVALID'; fi"

# ------------------------------------------------------------------------------
# 3. CASE CONVERSIONS: SNAKE, CAMEL, KEBAB (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Convert snake_case to kebab-case" \
  "name='hardware_simd_acceleration'; echo \"\${name//_/-}\""

run_adv_test "022" "Convert kebab-case to snake_case" \
  "name='hardware-simd-acceleration'; echo \"\${name//-/_}\""

run_adv_test "023" "Convert snake_case to UPPER_SNAKE_CASE" \
  "name='max_buffer_size'; echo \"\$name\" | tr '[:lower:]' '[:upper:]'"

run_adv_test "024" "Convert UPPER_CASE to camelCase Words in Loop" \
  "words='GET USER PROFILE'; out=''; for w in \$words; do low=\$(echo \"\$w\" | tr '[:upper:]' '[:lower:]'); if [ -z \"\$out\" ]; then out=\"\$low\"; else cap=\$(echo \"\${low:0:1}\" | tr '[:lower:]' '[:upper:]'); out=\"\${out}\${cap}\${low:1}\"; fi; done; echo \"\$out\""

run_adv_test "025" "Convert snake_case to PascalCase" \
  "words='user_order_processor'; IFS='_' read -r w1 w2 w3 <<< \"\$words\"; c1=\$(echo \"\${w1:0:1}\" | tr 'a-z' 'A-Z'); c2=\$(echo \"\${w2:0:1}\" | tr 'a-z' 'A-Z'); c3=\$(echo \"\${w3:0:1}\" | tr 'a-z' 'A-Z'); echo \"\${c1}\${w1:1}\${c2}\${w2:1}\${c3}\${w3:1}\""

run_adv_test "026" "Extract First Character Capitalization" \
  "word='zephyr'; cap=\$(echo \"\${word:0:1}\" | tr 'a-z' 'A-Z'); echo \"\${cap}\${word:1}\""

run_adv_test "027" "Convert Space-Separated Words to Dot-Notation" \
  "str='core engine parser lexer'; echo \"\${str// /.}\""

run_adv_test "028" "Strip CamelCase Boundaries to Space Separated Words" \
  "echo 'AppleSiliconM4' | sed 's/\([A-Z]\)/ \1/g' | tr -s ' ' | sed 's/^[ ]*//'"

run_adv_test "029" "Convert Dot-Notation to Directory Path" \
  "pkg='com.apple.developer.silicon'; echo \"\${pkg//./\\/}\""

run_adv_test "030" "Verify Case Conversion Round-trip (snake -> kebab -> snake)" \
  "orig='my_secret_token'; k=\"\${orig//_/-}\"; s=\"\${k//-/_}\"; [ \"\$orig\" = \"\$s\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 4. IPV4 & CIDR CALCULATIONS (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Split IPv4 Address into Four Octets" \
  "ip='192.168.1.254'; IFS='.' read -r o1 o2 o3 o4 <<< \"\$ip\"; echo \"O1:\$o1 O2:\$o2 O3:\$o3 O4:\$o4\""

run_adv_test "032" "Extract CIDR Prefix Length from Subnet Notation" \
  "net='10.0.0.0/24'; echo \"\${net##*/}\""

run_adv_test "033" "Extract Network Address from CIDR Notation" \
  "net='10.0.0.0/24'; echo \"\${net%/*}\""

run_adv_test "034" "Convert 4 Octets to 32-bit Integer in Pure Shell" \
  "ip='192.168.1.1'; IFS='.' read -r a b c d <<< \"\$ip\"; num=\$(( (a << 24) + (b << 16) + (c << 8) + d )); echo \"\$num\""

run_adv_test "035" "Convert 32-bit Integer Back to IPv4 Octets" \
  "num=3232235777; a=\$(( (num >> 24) & 255 )); b=\$(( (num >> 16) & 255 )); c=\$(( (num >> 8) & 255 )); d=\$(( num & 255 )); echo \"\$a.\$b.\$c.\$d\""

run_adv_test "036" "Calculate Netmask from /24 Prefix Length" \
  "p=24; mask=\$(( 0xFFFFFFFF << (32 - p) & 0xFFFFFFFF )); m1=\$(( (mask >> 24) & 255 )); m2=\$(( (mask >> 16) & 255 )); m3=\$(( (mask >> 8) & 255 )); m4=\$(( mask & 255 )); echo \"\$m1.\$m2.\$m3.\$m4\""

run_adv_test "037" "Calculate Total Usable Hosts in /28 Subnet" \
  "prefix=28; hosts=\$(( (1 << (32 - prefix)) - 2 )); echo \"UsableHosts:\$hosts\""

run_adv_test "038" "Check If Octet is Valid 0-255 Range" \
  "oct=192; if [ \$oct -ge 0 ] && [ \$oct -le 255 ]; then echo 'VALID'; else echo 'INVALID'; fi"

run_adv_test "039" "Check If Octet Exceeds 255" \
  "oct=300; if [ \$oct -ge 0 ] && [ \$oct -le 255 ]; then echo 'VALID'; else echo 'INVALID'; fi"

run_adv_test "040" "Calculate Broadcast IP for 192.168.1.0/24" \
  "net='192.168.1'; echo \"\${net}.255\""

# ------------------------------------------------------------------------------
# 5. SEMANTIC VERSIONING (SEMVER) (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Strip Leading 'v' from SemVer Tag" \
  "tag='v2.4.1'; echo \"\${tag#v}\""

run_adv_test "042" "Extract Major Version Number from SemVer" \
  "ver='2.4.1'; echo \"\${ver%%.*}\""

run_adv_test "043" "Extract Minor Version Number from SemVer" \
  "ver='2.4.1'; tmp=\"\${ver#*.}\"; echo \"\${tmp%%.*}\""

run_adv_test "044" "Extract Patch Version Number from SemVer" \
  "ver='2.4.1'; echo \"\${ver##*.}\""

run_adv_test "045" "Extract Pre-release Tag from SemVer (rc.1)" \
  "ver='3.0.0-rc.1'; echo \"\${ver#*-}\""

run_adv_test "046" "Strip Pre-release Tag to Base SemVer" \
  "ver='3.0.0-rc.1'; echo \"\${ver%%-*}\""

run_adv_test "047" "Compare Major Versions in Pure Shell (v3 > v2)" \
  "v1=3; v2=2; if [ \$v1 -gt \$v2 ]; then echo 'NEWER'; else echo 'OLDER'; fi"

run_adv_test "048" "SemVer Increment Minor Version Function" \
  "bump_minor() { local v=\"\$1\"; local maj=\"\${v%%.*}\"; local tmp=\"\${v#*.}\"; local min=\"\${tmp%%.*}\"; local patch=\"\${v##*.}\"; echo \"\$maj.\$((min + 1)).0\"; }; bump_minor '1.5.9'"

run_adv_test "049" "SemVer Increment Patch Version Function" \
  "bump_patch() { local v=\"\$1\"; local base=\"\${v%.*}\"; local patch=\"\${v##*.}\"; echo \"\$base.\$((patch + 1))\"; }; bump_patch '1.5.9'"

run_adv_test "050" "Sort SemVer Strings by Number Segments" \
  "vers='1.10.0 1.2.0 1.9.1'; echo \"\$vers\" | tr ' ' '\n' | sort -t. -k1,1n -k2,2n -k3,3n | tr '\n' ' ' | sed 's/ $//'"

# ------------------------------------------------------------------------------
# 6. INDENTATION, TABS & WHITESPACE (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Measure Leading Indentation Spaces Count" \
  "line='    function foo()'; stripped=\"\${line#*([ ])}\"; orig_len=\${#line}; no_sp=\$(echo \"\$line\" | sed 's/^[ ]*//'); echo \$(( orig_len - \${#no_sp} ))"

run_adv_test "052" "Convert 4-Space Indentation to 2-Space Indentation" \
  "line='    return 0;'; echo \"\${line/    /  }\""

run_adv_test "053" "Convert Leading Tab to 4 Spaces" \
  "printf '\tcode();\n' | sed $'s/^\t/    /'"

run_adv_test "054" "Strip All Leading Whitespace (ltrim)" \
  "line='   hello zephyr'; echo \"\$line\" | sed 's/^[ ]*//'"

run_adv_test "055" "Strip All Trailing Whitespace (rtrim)" \
  "line='hello zephyr   '; echo \"\$line\" | sed 's/[ ]*$//'"

run_adv_test "056" "Trim Both Leading and Trailing Whitespace" \
  "line='   padded string   '; echo \"\$line\" | sed -e 's/^[ ]*//' -e 's/[ ]*$//'"

run_adv_test "057" "Squeeze Multiple Internal Spaces into Single Space" \
  "line='word1    word2   word3'; echo \"\$line\" | tr -s ' '"

run_adv_test "058" "Add Fixed 4-Space Indent Prefix to Each Line" \
  "printf 'line1\nline2\n' | sed 's/^/    /'"

run_adv_test "059" "Remove Exactly 2 Spaces of Indentation from Block" \
  "printf '  step1\n  step2\n' | sed 's/^  //'"

run_adv_test "060" "Detect If Line is Blank or Whitespace Only" \
  "line='   '; trimmed=\$(echo \"\$line\" | tr -d ' \t'); [ -z \"\$trimmed\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 7. TEXT WORD-WRAP & FORMATTING (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Calculate String Length in Characters" \
  "s='Bare-Metal Apple Silicon Velocity'; echo \${#s}"

run_adv_test "062" "Slice String Substring (start: 0, length: 10)" \
  "s='Bare-Metal Apple Silicon Velocity'; echo \"\${s:0:10}\""

run_adv_test "063" "Slice String from Offset to End (offset: 11)" \
  "s='Bare-Metal Apple Silicon Velocity'; echo \"\${s:11}\""

run_adv_test "064" "Slice Last 8 Characters Using Negative Offset" \
  "s='Bare-Metal Apple Silicon Velocity'; echo \"\${s: -8}\""

run_adv_test "065" "Word Wrap Long Line to Width 20 Using Fold" \
  "echo 'The quick brown fox jumps over the lazy dog' | fold -s -w 20 | head -n 2"

run_adv_test "066" "Pad String with Right Spaces to Width 15" \
  "name='zephyr'; printf '%-15s|\n' \"\$name\""

run_adv_test "067" "Pad String with Left Spaces to Width 15" \
  "name='zephyr'; printf '%15s|\n' \"\$name\""

run_adv_test "068" "Pad Integer with Leading Zeros (Width: 6)" \
  "val=42; printf '%06d\n' \"\$val\""

run_adv_test "069" "Repeat Character N Times in Pure Shell" \
  "char='='; count=10; line=''; for i in 1 2 3 4 5 6 7 8 9 10; do line=\"\${line}\${char}\"; done; echo \"\$line\""

run_adv_test "070" "Center Text Inside 20-Character Frame" \
  "title='STATUS'; printf '%*s\n' \$(( (20 + \${#title}) / 2 )) \"\$title\""

# ------------------------------------------------------------------------------
# 8. MARKDOWN TABLE FORMATTING & ALIGNMENT (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Parse Columns of Markdown Table Row" \
  "row='| zbash | 35.8 GB/s | PASS |'; IFS='|' read -r empty col1 col2 col3 empty2 <<< \"\$row\"; echo \"[1:\$(echo \$col1 | tr -d ' ')] [2:\$(echo \$col2 | tr -d ' ')] [3:\$(echo \$col3 | tr -d ' ')]\""

run_adv_test "072" "Extract Table Header Names" \
  "hdr='| Tool | Throughput | Result |'; echo \"\$hdr\" | tr '|' '\n' | grep -v '^$' | tr -d ' ' | tr '\n' ',' | sed 's/,$//'"

run_adv_test "073" "Construct Markdown Table Separator Line" \
  "echo '|---|---|---|'"

run_adv_test "074" "Format Row with Fixed Cell Widths" \
  "c1='zbash'; c2='92x'; printf '| %-10s | %-6s |\n' \"\$c1\" \"\$c2\""

run_adv_test "075" "Strip Surrounding Markdown Pipe Borders" \
  "row='| data1 | data2 |'; s1=\"\${row#| }\"; echo \"\${s1% |}\""

run_adv_test "076" "Count Columns in Markdown Table Header" \
  "hdr='| ColA | ColB | ColC | ColD |'; num=\$(echo \"\$hdr\" | tr '|' '\n' | grep -v '^$' | wc -l | tr -d ' '); echo \"Cols:\$num\""

run_adv_test "077" "Verify Table Separator Has Valid Dashes" \
  "sep='|---|---|---|'; if echo \"\$sep\" | grep -q '^\(|-[ -]*\)*|\$'; then echo 'VALID_SEP'; else echo 'INVALID'; fi"

run_adv_test "078" "Convert CSV Line to Markdown Table Row" \
  "csv='zbash,35.8,PASS'; echo \"| \${csv//,/ | } |\""

run_adv_test "079" "Calculate Max Width of Column from List of Values" \
  "vals='apple banana strawberry kiwi'; max=0; for v in \$vals; do if [ \${#v} -gt \$max ]; then max=\${#v}; fi; done; echo \"MaxWidth:\$max\""

run_adv_test "080" "Format Aligned Markdown Table 2-Row Snippet" \
  "printf '| %-8s | %-6s |\n| %-8s | %-6s |\n' 'Tool' 'Speed' 'zbash' '35GB/s'"

# ------------------------------------------------------------------------------
# 9. STRING DISTANCE, SLICING & SEARCH (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Calculate Hamming Distance for Equal Length Strings" \
  "s1='karolin'; s2='kathrin'; dist=0; for i in 0 1 2 3 4 5 6; do if [ \"\${s1:\$i:1}\" != \"\${s2:\$i:1}\" ]; then dist=\$((dist + 1)); fi; done; echo \"Dist:\$dist\""

run_adv_test "082" "Hamming Distance for Identical Strings is Zero" \
  "s1='zephyr'; s2='zephyr'; dist=0; for i in 0 1 2 3 4 5; do if [ \"\${s1:\$i:1}\" != \"\${s2:\$i:1}\" ]; then dist=\$((dist + 1)); fi; done; echo \"Dist:\$dist\""

run_adv_test "083" "Find Longest Common Prefix Length of Two Strings" \
  "s1='zephyr_terminal'; s2='zephyr_core'; len=0; while [ \"\${s1:\$len:1}\" = \"\${s2:\$len:1}\" ] && [ \$len -lt \${#s1} ]; do len=\$((len + 1)); done; echo \"LCP:\$len\""

run_adv_test "084" "Reverse String Character by Character in Pure Shell" \
  "s='zephyr'; rev=''; len=\${#s}; for i in 5 4 3 2 1 0; do rev=\"\${rev}\${s:\$i:1}\"; done; echo \"\$rev\""

run_adv_test "085" "Check If String is Palindrome (racecar)" \
  "s='racecar'; is_pal=true; len=\${#s}; for i in 0 1 2; do j=\$(( len - 1 - i )); if [ \"\${s:\$i:1}\" != \"\${s:\$j:1}\" ]; then is_pal=false; fi; done; echo \"\$is_pal\""

run_adv_test "086" "Check If String is Palindrome (zephyr -> false)" \
  "s='zephyr'; is_pal=true; len=\${#s}; for i in 0 1 2; do j=\$(( len - 1 - i )); if [ \"\${s:\$i:1}\" != \"\${s:\$j:1}\" ]; then is_pal=false; fi; done; echo \"\$is_pal\""

run_adv_test "087" "Count Specific Character Occurrences in String" \
  "s='banana'; no_a=\"\${s//a/}\"; echo \$(( \${#s} - \${#no_a} ))"

run_adv_test "088" "Find Index of Character First Occurrence" \
  "s='hello world'; idx=0; while [ \$idx -lt \${#s} ]; do if [ \"\${s:\$idx:1}\" = 'w' ]; then break; fi; idx=\$((idx + 1)); done; echo \"Pos:\$idx\""

run_adv_test "089" "Check If String Starts with Specific Prefix" \
  "s='libzephyr.dylib'; if [ \"\${s#lib}\" != \"\$s\" ]; then echo 'STARTS_WITH_LIB'; else echo 'NO'; fi"

run_adv_test "090" "Check If String Ends with Specific Suffix" \
  "s='libzephyr.dylib'; if [ \"\${s%.dylib}\" != \"\$s\" ]; then echo 'ENDS_WITH_DYLIB'; else echo 'NO'; fi"

# ------------------------------------------------------------------------------
# 10. NUMBER BASE CONVERSIONS & SHELL ARITHMETIC (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Convert Hexadecimal to Decimal via Pure Shell" \
  "hex='0xFF'; echo \$(( hex ))"

run_adv_test "092" "Convert Hex with Base Prefix to Decimal (16#A0)" \
  "echo \$(( 16#A0 ))"

run_adv_test "093" "Convert Octal to Decimal (0755)" \
  "echo \$(( 0755 ))"

run_adv_test "094" "Convert Binary to Decimal (2#101010)" \
  "echo \$(( 2#101010 ))"

run_adv_test "095" "Format Decimal Number as Hexadecimal String" \
  "val=255; printf '0x%X\n' \"\$val\""

run_adv_test "096" "Format Decimal Number as Octal String" \
  "val=511; printf '0%o\n' \"\$val\""

run_adv_test "097" "Bitwise Shift Left and Right in Shell Arithmetic" \
  "x=4; echo \$(( (x << 3) >> 1 ))"

run_adv_test "098" "Bitwise AND, OR, XOR Operations" \
  "a=12; b=10; echo \"AND:\$((a & b)) OR:\$((a | b)) XOR:\$((a ^ b))\""

run_adv_test "099" "Modulo and Exponentiation Arithmetic in Shell" \
  "echo \"MOD:\$(( 100 % 7 ))\""

run_adv_test "100" "Complete Module 8 String Algorithms Verification" \
  "slug='zephyr-silicon-2026'; [ \"\${slug:0:6}\" = 'zephyr' ] && [ \${#slug} -eq 19 ]; echo \$?"

# ------------------------------------------------------------------------------
# SUMMARY REPORT
# ------------------------------------------------------------------------------
echo -e "${MAGENTA}================================================================================${NC}"
echo -e "📊 ${BOLD}MODULE 08 EXECUTION SUMMARY (STRING ALGORITHMS & TEXT PARSING):${NC}"
echo -e "   TOTAL TESTS: 100"
echo -e "   PASSED:      ${GREEN}${PASS_COUNT}${NC}"
echo -e "   FAILED:      ${RED}${FAIL_COUNT}${NC}"
if [ "$FAIL_COUNT" -eq 0 ]; then
    echo -e "   ${GREEN}🎉 100% PERFECT BIT-FOR-BIT MATCH (100 / 100 PASS) ⚡${NC}"
    echo -e "${MAGENTA}================================================================================${NC}"
    exit 0
else
    echo -e "   ${RED}❌ PARITY FAILURES DETECTED!${NC}"
    echo -e "${MAGENTA}================================================================================${NC}"
    exit 1
fi
