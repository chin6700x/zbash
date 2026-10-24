#!/bin/bash
# ==============================================================================
# ADVANCE TEST SUITE - MODULE 07: REST API, JSON STREAMING & WEBHOOKS
# Author: Wirot Chookeaw (chin6700x)
# Copyright: (c) 2026 Wirot Chookeaw.
# License: Apache License 2.0
# ==============================================================================
# Comprehensive 100-test differential verification for developer and analyst
# workflows: HTTP header parsing, status codes, query strings, URL encoding/decoding,
# JWT base64url dissection, webhook signature verification, NDJSON streams,
# SSE event demuxing, and API retry-backoff simulation.
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/common_adv.sh"

generate_api_fixtures

HTTP_RAW="$FIXTURES_DIR/api/http_response.raw"
WEBHOOK_JSON="$FIXTURES_DIR/api/webhook_event.json"
NDJSON_FILE="$FIXTURES_DIR/api/stream.ndjson"
SSE_FILE="$FIXTURES_DIR/api/sse_events.txt"
JWT_FILE="$FIXTURES_DIR/api/jwt_token.txt"

echo -e "${MAGENTA}================================================================================${NC}"
echo -e "${MAGENTA}🧪 STARTING MODULE 07: REST API, JSON STREAMING & WEBHOOK INTEGRATION (100 TESTS)${NC}"
echo -e "${MAGENTA}================================================================================${NC}"

# ------------------------------------------------------------------------------
# 1. HTTP RESPONSE LINE & STATUS CODE PARSING (TESTS #001 - #010)
# ------------------------------------------------------------------------------

run_adv_test "001" "Extract Full HTTP Status Line" \
  "head -n 1 '$HTTP_RAW'"

run_adv_test "002" "Extract HTTP Status Code Number" \
  "head -n 1 '$HTTP_RAW' | awk '{print \$2}'"

run_adv_test "003" "Extract HTTP Status Reason Text" \
  "line=\$(head -n 1 '$HTTP_RAW'); echo \"\${line#* * }\""

run_adv_test "004" "Check If Status Code is 200 OK via Pure Shell" \
  "code=\$(head -n 1 '$HTTP_RAW' | awk '{print \$2}'); [ \"\$code\" -eq 200 ]; echo \$?"

run_adv_test "005" "Classify HTTP Status 2xx Success Class" \
  "code=204; if [ \$code -ge 200 ] && [ \$code -lt 300 ]; then echo '2xx_SUCCESS'; else echo 'OTHER'; fi"

run_adv_test "006" "Classify HTTP Status 4xx Client Error Class" \
  "code=404; if [ \$code -ge 400 ] && [ \$code -lt 500 ]; then echo '4xx_CLIENT_ERROR'; else echo 'OTHER'; fi"

run_adv_test "007" "Classify HTTP Status 5xx Server Error Class" \
  "code=502; if [ \$code -ge 500 ] && [ \$code -lt 600 ]; then echo '5xx_SERVER_ERROR'; else echo 'OTHER'; fi"

run_adv_test "008" "Detect HTTP Protocol Version (HTTP/1.1 vs HTTP/2)" \
  "proto=\$(head -n 1 '$HTTP_RAW' | awk '{print \$1}'); echo \"\$proto\""

run_adv_test "009" "Simulate Redirection Status Code Check" \
  "code=301; if [ \$code -eq 301 ] || [ \$code -eq 302 ] || [ \$code -eq 307 ] || [ \$code -eq 308 ]; then echo 'REDIRECT'; else echo 'NO_REDIRECT'; fi"

run_adv_test "010" "Extract HTTP Status into Shell Variable Tuple" \
  "read -r proto code msg < '$HTTP_RAW'; echo \"PROTO:\$proto CODE:\$code MSG:\$msg\""

# ------------------------------------------------------------------------------
# 2. HTTP HEADER EXTRACTION & NORMALIZATION (TESTS #011 - #020)
# ------------------------------------------------------------------------------

run_adv_test "011" "Extract Content-Type Header Value" \
  "grep -i '^Content-Type:' '$HTTP_RAW' | awk -F': ' '{print \$2}'"

run_adv_test "012" "Extract MIME Type Stripping Charset" \
  "ct='application/json; charset=utf-8'; echo \"\${ct%%;*}\""

run_adv_test "013" "Extract Charset from Content-Type" \
  "ct='application/json; charset=utf-8'; echo \"\${ct##*charset=}\""

run_adv_test "014" "Extract Content-Length Header as Integer" \
  "grep -i '^Content-Length:' '$HTTP_RAW' | awk '{print \$2}'"

run_adv_test "015" "Extract Rate Limit Remaining Quota" \
  "grep -i '^X-RateLimit-Remaining:' '$HTTP_RAW' | awk '{print \$2}'"

run_adv_test "016" "Extract Rate Limit Quota Ratio Calculation" \
  "rem=\$(grep -i '^X-RateLimit-Remaining:' '$HTTP_RAW' | awk '{print \$2}'); lim=\$(grep -i '^X-RateLimit-Limit:' '$HTTP_RAW' | awk '{print \$2}'); echo \"\$rem/\$lim\""

run_adv_test "017" "Normalize Header Case (Lowercase Keys)" \
  "echo 'Authorization: Bearer sec_tok_991' | tr '[:upper:]' '[:lower:]'"

run_adv_test "018" "Extract ETag Header and Strip Quotes" \
  "etag=\$(grep -i '^ETag:' '$HTTP_RAW' | awk '{print \$2}'); echo \"\${etag//\\\"/}\""

run_adv_test "019" "Check If Response is JSON via Header Match" \
  "grep -iq '^Content-Type:.*application/json' '$HTTP_RAW'; echo \$?"

run_adv_test "020" "Separate Raw HTTP Headers from Body" \
  "sed -e '/^$/q' '$HTTP_RAW' | wc -l | tr -d ' '"

# ------------------------------------------------------------------------------
# 3. URL QUERY STRING PARSING & COMPOSITION (TESTS #021 - #030)
# ------------------------------------------------------------------------------

run_adv_test "021" "Extract Query String from Full URL" \
  "url='https://api.example.com/v1/search?q=apple&page=2&limit=50'; echo \"\${url#*?}\""

run_adv_test "022" "Extract URL Base Path Stripping Query String" \
  "url='https://api.example.com/v1/search?q=apple&page=2&limit=50'; echo \"\${url%\\?*}\""

run_adv_test "023" "Parse Specific Query Parameter (q)" \
  "qs='q=apple&page=2&limit=50'; echo \"\$qs\" | tr '&' '\n' | grep '^q=' | cut -d= -f2"

run_adv_test "024" "Parse Specific Query Parameter (page)" \
  "qs='q=apple&page=2&limit=50'; echo \"\$qs\" | tr '&' '\n' | grep '^page=' | cut -d= -f2"

run_adv_test "025" "Parse Specific Query Parameter (limit)" \
  "qs='q=apple&page=2&limit=50'; echo \"\$qs\" | tr '&' '\n' | grep '^limit=' | cut -d= -f2"

run_adv_test "026" "Build Query String from Key-Value Pairs" \
  "q='golang'; page=3; limit=20; echo \"q=\${q}&page=\${page}&limit=\${limit}\""

run_adv_test "027" "Count Number of Query Parameters in Query String" \
  "qs='q=apple&category=electronics&sort=asc&page=1'; echo \"\$qs\" | tr '&' '\n' | wc -l | tr -d ' '"

run_adv_test "028" "Check If Query Parameter Exists (has sort)" \
  "qs='q=apple&category=electronics&sort=asc&page=1'; echo \"\$qs\" | grep -q 'sort=' && echo 'EXISTS' || echo 'MISSING'"

run_adv_test "029" "Extract Domain Name from Full HTTPS URL" \
  "url='https://api.service.internal:8080/v2/items'; tmp=\"\${url#https://}\"; echo \"\${tmp%%/*}\""

run_adv_test "030" "Extract URL Port Number If Present" \
  "host='api.service.internal:8080'; echo \"\${host##*:}\""

# ------------------------------------------------------------------------------
# 4. PURE SHELL URL PERCENT ENCODING & DECODING (TESTS #031 - #040)
# ------------------------------------------------------------------------------

run_adv_test "031" "Decode %20 Spaces to Literal Spaces" \
  "raw='hello%20world%20zephyr'; echo \"\${raw//%20/ }\""

run_adv_test "032" "Decode Plus Signs to Spaces in Form Query" \
  "raw='john+doe+junior'; echo \"\${raw//+/ }\""

run_adv_test "033" "Decode %3A Colon in URL Encoded String" \
  "raw='http%3A%2F%2Flocalhost'; s1=\"\${raw//%3A/:}\"; echo \"\${s1//%2F/\\/}\""

run_adv_test "034" "Encode Space Characters to %20" \
  "str='DeepMind Zephyr POSIX'; echo \"\${str// /%20}\""

run_adv_test "035" "Encode Ampersand to %26" \
  "str='R&D Lab'; echo \"\${str//&/%26}\""

run_adv_test "036" "URL Query Builder with Encoded Search Query" \
  "query='macOS ARM64'; base='https://api.github.com/search?q='; echo \"\${base}\${query// /%20}\""

run_adv_test "037" "Decode Multi-Character Percent Codes (%2C to comma)" \
  "raw='apple%2Cbanana%2Corange'; echo \"\${raw//%2C/,}\""

run_adv_test "038" "Decode %40 to At Symbol in Email Param" \
  "raw='user%40example.com'; echo \"\${raw//%40/@}\""

run_adv_test "039" "Decode %2F to Forward Slash" \
  "raw='api%2Fv1%2Fusers'; echo \"\${raw//%2F/\\/}\""

run_adv_test "040" "Round-trip Encode Space and Decode Back" \
  "orig='speed up'; enc=\"\${orig// /%20}\"; dec=\"\${enc//%20/ }\"; [ \"\$orig\" = \"\$dec\" ]; echo \$?"

# ------------------------------------------------------------------------------
# 5. BEARER TOKENS & JWT DECOMPOSITION (TESTS #041 - #050)
# ------------------------------------------------------------------------------

run_adv_test "041" "Extract Bearer Token from Authorization Header" \
  "hdr='Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9'; echo \"\${hdr#*Bearer }\""

run_adv_test "042" "Extract JWT Header Segment (Part 1)" \
  "jwt=\$(cat '$JWT_FILE'); echo \"\${jwt%%.*}\""

run_adv_test "043" "Extract JWT Payload Segment (Part 2)" \
  "jwt=\$(cat '$JWT_FILE'); tmp=\"\${jwt#*.}\"; echo \"\${tmp%%.*}\""

run_adv_test "044" "Extract JWT Signature Segment (Part 3)" \
  "jwt=\$(cat '$JWT_FILE'); echo \"\${jwt##*.}\""

run_adv_test "045" "Verify JWT Format Has Exactly Three Segments" \
  "jwt=\$(cat '$JWT_FILE'); num=\$(echo \"\$jwt\" | tr '.' '\n' | wc -l | tr -d ' '); [ \$num -eq 3 ]; echo \$?"

run_adv_test "046" "Validate Minimum Length of JWT Token" \
  "jwt=\$(cat '$JWT_FILE'); [ \${#jwt} -gt 50 ]; echo \$?"

run_adv_test "047" "Extract Token Signing Algorithm (alg: HS256)" \
  "jwt=\$(cat '$JWT_FILE'); hdr_b64=\"\${jwt%%.*}\"; echo \"\$hdr_b64\" | base64 -D 2>/dev/null || echo \"\$hdr_b64\" | base64 -d 2>/dev/null"

run_adv_test "048" "Decode JWT Payload JSON String" \
  "jwt=\$(cat '$JWT_FILE'); tmp=\"\${jwt#*.}\"; p_b64=\"\${tmp%%.*}\"; (echo \"\$p_b64\" | base64 -D 2>/dev/null || echo \"\$p_b64\" | base64 -d 2>/dev/null) | grep -o '\"name\":\"[^\"]*\"'"

run_adv_test "049" "Extract Subject ID from Decoded JWT" \
  "jwt=\$(cat '$JWT_FILE'); tmp=\"\${jwt#*.}\"; p_b64=\"\${tmp%%.*}\"; (echo \"\$p_b64\" | base64 -D 2>/dev/null || echo \"\$p_b64\" | base64 -d 2>/dev/null) | grep -o '\"sub\":\"[^\"]*\"' | cut -d: -f2 | tr -d '\"'"

run_adv_test "050" "Extract Token Expiry/Issued-At Timestamp (iat)" \
  "jwt=\$(cat '$JWT_FILE'); tmp=\"\${jwt#*.}\"; p_b64=\"\${tmp%%.*}\"; (echo \"\$p_b64\" | base64 -D 2>/dev/null || echo \"\$p_b64\" | base64 -d 2>/dev/null) | grep -o '\"iat\":[0-9]*' | cut -d: -f2"

# ------------------------------------------------------------------------------
# 6. BASE64 & BASE64URL TRANSFORMATIONS (TESTS #051 - #060)
# ------------------------------------------------------------------------------

run_adv_test "051" "Encode Basic Auth Credentials (admin:secret123)" \
  "echo -n 'admin:secret123' | base64"

run_adv_test "052" "Decode Basic Auth Credentials Back" \
  "echo 'YWRtaW46c2VjcmV0MTIz' | base64 -D 2>/dev/null || echo 'YWRtaW46c2VjcmV0MTIz' | base64 -d"

run_adv_test "053" "Convert Standard Base64 to Base64URL (- instead of +)" \
  "b64='a+b/c=='; s1=\"\${b64//+/-}\"; s2=\"\${s1//\\//_}\"; echo \"\${s2//=/}\""

run_adv_test "054" "Convert Base64URL to Standard Base64 (+ and /)" \
  "b64url='a-b_c'; s1=\"\${b64url//-/+}\"; echo \"\${s1//_//}\""

run_adv_test "055" "Calculate Base64 Padding Requirement in Pure Shell" \
  "str='abcde'; rem=\$(( \${#str} % 4 )); pad=\$(( (4 - rem) % 4 )); echo \"PAD:\$pad\""

run_adv_test "056" "Pad Base64 String to Length Multiple of 4" \
  "pad_b64() { local s=\"\$1\"; local rem=\$(( \${#s} % 4 )); if [ \$rem -eq 2 ]; then echo \"\${s}==\"; elif [ \$rem -eq 3 ]; then echo \"\${s}=\"; else echo \"\$s\"; fi; }; pad_b64 'eyJuYW1lIjoiQWxpY2UifQ'"

run_adv_test "057" "Encode API Secret Key for Storage" \
  "echo -n 'zephyr-bare-metal-silicon' | base64"

run_adv_test "058" "Verify Decoded Secret Matches Original" \
  "s=\$(echo -n 'zephyr-bare-metal-silicon' | base64); dec=\$( (echo \"\$s\" | base64 -D 2>/dev/null || echo \"\$s\" | base64 -d) ); [ \"\$dec\" = 'zephyr-bare-metal-silicon' ]; echo \$?"

run_adv_test "059" "Encode Empty String to Base64" \
  "echo -n '' | base64 | tr -d '\n'"

run_adv_test "060" "Base64 Length Ratio Check (roughly 4/3)" \
  "s='hello world this is an api test string'; b=\$(echo -n \"\$s\" | base64); [ \${#b} -gt \${#s} ]; echo \$?"

# ------------------------------------------------------------------------------
# 7. WEBHOOK SIGNATURE & PAYLOAD VERIFICATION (TESTS #061 - #070)
# ------------------------------------------------------------------------------

run_adv_test "061" "Read Raw Webhook JSON Payload" \
  "cat '$WEBHOOK_JSON'"

run_adv_test "062" "Extract Webhook Event Type" \
  "grep -o '\"event\":\"[^\"]*\"' '$WEBHOOK_JSON' | cut -d: -f2 | tr -d '\"'"

run_adv_test "063" "Extract Webhook Order ID" \
  "grep -o '\"order_id\":\"[^\"]*\"' '$WEBHOOK_JSON' | cut -d: -f2 | tr -d '\"'"

run_adv_test "064" "Extract Webhook Amount" \
  "grep -o '\"amount\":[0-9.]*' '$WEBHOOK_JSON' | cut -d: -f2"

run_adv_test "065" "Calculate SHA256 Checksum of Webhook Payload" \
  "shasum -a 256 '$WEBHOOK_JSON' 2>/dev/null | awk '{print \$1}' || sha256sum '$WEBHOOK_JSON' | awk '{print \$1}'"

run_adv_test "066" "Simulate Webhook Signature Comparison (Match)" \
  "sig1='sha256=abcdef123456'; sig2='sha256=abcdef123456'; [ \"\$sig1\" = \"\$sig2\" ]; echo \$?"

run_adv_test "067" "Simulate Webhook Signature Comparison (Mismatch/Tamper)" \
  "sig1='sha256=abcdef123456'; sig2='sha256=tampered99999'; [ \"\$sig1\" = \"\$sig2\" ]; echo \$?"

run_adv_test "068" "Extract Signature Algorithm from Header" \
  "sig_hdr='sha256=abcdef123456'; echo \"\${sig_hdr%%=*}\""

run_adv_test "069" "Extract Raw Hash from Signature Header" \
  "sig_hdr='sha256=abcdef123456'; echo \"\${sig_hdr#*=}\""

run_adv_test "070" "Check If Webhook Timestamp is Valid Recent Int" \
  "ts=\$(grep -o '\"timestamp\":[0-9]*' '$WEBHOOK_JSON' | cut -d: -f2); [ \$ts -gt 1700000000 ]; echo \$?"

# ------------------------------------------------------------------------------
# 8. NEWLINE-DELIMITED JSON (NDJSON) STREAM PROCESSING (TESTS #071 - #080)
# ------------------------------------------------------------------------------

run_adv_test "071" "Count Total Records in NDJSON Stream" \
  "wc -l < '$NDJSON_FILE' | tr -d ' '"

run_adv_test "072" "Filter Only Alert Events from Stream" \
  "grep '\"type\":\"alert\"' '$NDJSON_FILE' | grep -o '\"msg\":\"[^\"]*\"'"

run_adv_test "073" "Filter Metric Events from Stream" \
  "grep -c '\"type\":\"metric\"' '$NDJSON_FILE' | tr -d ' '"

run_adv_test "074" "Extract First Event ID in NDJSON Stream" \
  "head -n 1 '$NDJSON_FILE' | grep -o '\"id\":[0-9]*' | cut -d: -f2"

run_adv_test "075" "Extract Last Event ID in NDJSON Stream" \
  "tail -n 1 '$NDJSON_FILE' | grep -o '\"id\":[0-9]*' | cut -d: -f2"

run_adv_test "076" "Extract CPU Measurements from All Metric Records" \
  "grep '\"type\":\"metric\"' '$NDJSON_FILE' | grep -o '\"cpu\":[0-9.]*' | cut -d: -f2 | tr '\n' ' ' | sed 's/ $//'"

run_adv_test "077" "Find Peak CPU in Stream via Pure Shell Loop" \
  "max=0; while read -r line; do cpu=\$(echo \"\$line\" | grep -o '\"cpu\":[0-9.]*' | cut -d: -f2); if [ -n \"\$cpu\" ]; then int_cpu=\${cpu%%.*}; if [ \$int_cpu -gt \$max ]; then max=\$int_cpu; fi; fi; done < '$NDJSON_FILE'; echo \"MaxCPU:\$max\""

run_adv_test "078" "Extract Memory Measurements from All Metric Records" \
  "grep '\"type\":\"metric\"' '$NDJSON_FILE' | grep -o '\"mem\":[0-9.]*' | cut -d: -f2 | head -n 3 | tr '\n' ',' | sed 's/,$//'"

run_adv_test "079" "Validate Each NDJSON Line is Valid JSON Envelope" \
  "all_valid=true; while read -r line; do if [ \"\${line:0:1}\" != '{' ] || [ \"\${line: -1}\" != '}' ]; then all_valid=false; fi; done < '$NDJSON_FILE'; echo \"\$all_valid\""

run_adv_test "080" "Stream Transformation to Tab-Delimited TSV" \
  "cnt=0; while read -r line; do if [ \$cnt -ge 3 ]; then break; fi; id=\$(echo \"\$line\" | grep -o '\"id\":[0-9]*' | cut -d: -f2); typ=\$(echo \"\$line\" | grep -o '\"type\":\"[^\"]*\"' | cut -d: -f2 | tr -d '\"'); echo \"\$id\t\$typ\"; cnt=\$((cnt + 1)); done < '$NDJSON_FILE'"

# ------------------------------------------------------------------------------
# 9. SERVER-SENT EVENTS (SSE) STREAM DEMUX (TESTS #081 - #090)
# ------------------------------------------------------------------------------

run_adv_test "081" "Extract Event Names from SSE Stream" \
  "grep '^event:' '$SSE_FILE' | awk '{print \$2}' | tr '\n' ' ' | sed 's/ $//'"

run_adv_test "082" "Extract Data Payloads from SSE Stream" \
  "grep '^data:' '$SSE_FILE' | head -n 2"

run_adv_test "083" "Count Number of SSE Frames in Stream" \
  "grep -c '^event:' '$SSE_FILE' | tr -d ' '"

run_adv_test "084" "Demux SSE Stream to Key-Value Pairs" \
  "cnt=0; cur_ev=''; while read -r line; do if [ \$cnt -ge 2 ]; then break; fi; if [ \"\${line#event: }\" != \"\$line\" ]; then cur_ev=\"\${line#event: }\"; elif [ \"\${line#data: }\" != \"\$line\" ]; then echo \"[\$cur_ev] -> \${line#data: }\"; cnt=\$((cnt + 1)); fi; done < '$SSE_FILE'"

run_adv_test "085" "Filter Message Event from SSE Stream" \
  "grep -A 1 'event: message' '$SSE_FILE' | grep '^data:' | sed 's/^data: //'"

run_adv_test "086" "Extract User from SSE Message Event" \
  "grep -A 1 'event: message' '$SSE_FILE' | grep -o '\"user\":\"[^\"]*\"' | cut -d: -f2 | tr -d '\"'"

run_adv_test "087" "Detect Error Frame in SSE Stream" \
  "grep -q 'event: error' '$SSE_FILE' && echo 'ERROR_FOUND' || echo 'CLEAN'"

run_adv_test "088" "Extract Error Code from Error Frame" \
  "grep -A 1 'event: error' '$SSE_FILE' | grep -o '\"code\":[0-9]*' | cut -d: -f2"

run_adv_test "089" "Count Empty Line Delimiters in SSE Stream" \
  "grep -c '^$' '$SSE_FILE' | tr -d ' '"

run_adv_test "090" "Convert SSE Stream to Compact One-Line JSON Array" \
  "evs=\$(grep '^event:' '$SSE_FILE' | awk '{print \"\\\"\" \$2 \"\\\"\"}' | tr '\n' ',' | sed 's/,$//'); echo \"[\$evs]\""

# ------------------------------------------------------------------------------
# 10. API PAGINATION, BACKOFF & RETRY SIMULATION (TESTS #091 - #100)
# ------------------------------------------------------------------------------

run_adv_test "091" "Simulate API Pagination Next-Token Extraction" \
  "res='{\"data\":[1,2,3],\"next_token\":\"cursor_tok_456\",\"has_more\":true}'; echo \"\$res\" | grep -o '\"next_token\":\"[^\"]*\"' | cut -d: -f2 | tr -d '\"'"

run_adv_test "092" "Simulate Has-More Boolean Flag Check" \
  "res='{\"has_more\":true}'; echo \"\$res\" | grep -q '\"has_more\":true' && echo 'CONTINUE' || echo 'STOP'"

run_adv_test "093" "Simulate Pagination Exhaustion (has_more: false)" \
  "res='{\"has_more\":false}'; echo \"\$res\" | grep -q '\"has_more\":true' && echo 'CONTINUE' || echo 'STOP'"

run_adv_test "094" "Simulate Exponential Backoff Delay Generator (1, 2, 4, 8)" \
  "delays=''; cur=1; for i in 1 2 3 4; do delays=\"\$delays \$cur\"; cur=\$((cur * 2)); done; echo \"\${delays# }\""

run_adv_test "095" "Simulate Max Retry Counter (3 attempts max)" \
  "attempts=0; max_retries=3; while [ \$attempts -lt \$max_retries ]; do attempts=\$((attempts + 1)); done; echo \"FinalAttempts:\$attempts\""

run_adv_test "096" "Simulate Retry Loop with Transient 503 Service Unavailable" \
  "attempt=1; while [ \$attempt -le 3 ]; do if [ \$attempt -eq 3 ]; then status=200; else status=503; fi; if [ \$status -eq 200 ]; then echo \"SUCCESS_ON_ATTEMPT_\$attempt\"; break; fi; attempt=\$((attempt + 1)); done"

run_adv_test "097" "Simulate Rate Limit Reset Wait Time Calculation" \
  "reset_ts=1760000060; cur_ts=1760000000; wait_sec=\$((reset_ts - cur_ts)); echo \"WAIT:\${wait_sec}s\""

run_adv_test "098" "Construct Curl Authorization Header String in Variable" \
  "tok='sec_live_9941'; hdr=\"-H \\\"Authorization: Bearer \$tok\\\"\"; echo \"\$hdr\""

run_adv_test "099" "Construct Curl User-Agent and JSON Headers" \
  "app='ZephyrClient/1.0'; echo \"-H 'User-Agent: \$app' -H 'Content-Type: application/json'\""

run_adv_test "100" "Complete Module 7 Parity & Environment Proof" \
  "[ -f '$HTTP_RAW' ] && [ -f '$NDJSON_FILE' ] && [ -f '$SSE_FILE' ] && [ -f '$JWT_FILE' ]; echo \$?"

# ------------------------------------------------------------------------------
# SUMMARY REPORT
# ------------------------------------------------------------------------------
echo -e "${MAGENTA}================================================================================${NC}"
echo -e "📊 ${BOLD}MODULE 07 EXECUTION SUMMARY (API & JSON STREAMING):${NC}"
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
