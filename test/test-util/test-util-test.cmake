
if (NOT TEST_EXECUTABLE)
  message(FATAL_ERROR "Usage: ${CMAKE_COMMAND} -DTEST_EXECUTABLE=<EXECUTABLE> -P ${CMAKE_CURRENT_LIST_FILE}")
endif()

execute_process(COMMAND "${TEST_EXECUTABLE}"
  OUTPUT_VARIABLE TEST_OUTPUT
  ERROR_VARIABLE TEST_OUTPUT
  RESULT_VARIABLE TEST_RESULT)

if(NOT TEST_RESULT EQUAL 0)
  message(FATAL_ERROR "Test executable returned error: ${TEST_RESULT}")
endif()

message("OUTPUT:

${TEST_OUTPUT}")

set(D5 "\\.\\.\\.\\.\\.")
set(HEX "[0-9a-f]")
set(H2 "${HEX}${HEX}")
set(H4 "${H2}${H2}")
set(H48 "${H4} ${H4} ${H4} ${H4} ${H4} ${H4} ${H4} ${H4}")
set(TEST_REGEX "^PASS: descrip1
FAIL: descrip2
${D5} faildesc2
${D5} defined at test2\\( *1\\)
PASS: descrip3
${D5} next line
${D5} more lines
${D5} too many
FAIL: descrip3
${D5} next line
${D5} more lines
${D5} too many
${D5} faildesc3
${D5} aaz
${D5} bb
${D5}[ ]
${D5} ccc
${D5} defined at test4\\( *3\\)
PASS: descrip4
FAIL: descrip4
${D5} defined at test6\\( *5\\)
PASS: 1 *== *1
FAIL: 1 *== *2
${D5} defined at .*\\.c\\([0-9]+\\)
FAIL: 1 *== *2
${D5} test1
${D5} defined at .*\\.c\\([0-9]+\\)
FAIL: 1 *== *2
${D5} foo
${D5} bar
${D5} defined at .*\\.c\\([0-9]+\\)
PASS: 1 < 2
FAIL: 2 < 2
${D5} defined at .*\\.c\\([0-9]+\\)
FAIL: 2 < 2
${D5} test2
${D5} defined at .*\\.c\\([0-9]+\\)
PASS: \"abc\" *== *\\(\\(char *\\[\\]\\) *{'a', *'b', *'c', *'\\\\0'}\\)
PASS: \"abc\" *== *\\(\\(char *\\[\\]\\) *{'a', *'b', *'c'}\\)
FAIL: \"abc\" *== *\\(\\(char *\\[\\]\\) *{'a', *'b', *'e', *'\\\\0'}\\)
${D5} defined at .*\\.c\\([0-9]+\\)
PASS: \"abc\" *== *\"abc\"
FAIL: \"abcd\" *== *\"abc\"
${D5} defined at .*\\.c\\([0-9]+\\)
FAIL: \"abc\" *== *\"abcd\"
${D5} defined at .*\\.c\\([0-9]+\\)
FAIL: \"abe\" *== *\"abc\"
${D5} defined at .*\\.c\\([0-9]+\\)
${D5} 0x${HEX}+: 00 *\\.
${D5} 0x${HEX}+: 0002 0621 60 *\\.\\.\\.!`
${D5} 0x${HEX}+: ${H48} ABCDEFGHIJKLMNOP
${D5} 0x${HEX}+: ${H4} ${H4} ${H4} ${H4} ${H4} ${H4} ${H2} *QRSTUVWXYZ012
${D5} data is \\(nil\\)")

function(test_expect_base STAT EXPR EXP GOT EXTRA)
  set(TEST_REGEX "${TEST_REGEX}
${STAT}: ${EXPR}")
  if("${STAT}" STREQUAL FAIL)
    set(TEST_REGEX "${TEST_REGEX}
${D5} +Expected: +${EXP}
${D5} +Got: +${GOT}")
    if(NOT "${EXTRA}" STREQUAL "")
      set(TEST_REGEX "${TEST_REGEX}
${EXTRA}")
    endif()
    set(TEST_REGEX "${TEST_REGEX}
${D5} defined at .*\\.c\\([0-9]+\\)")
  endif()
  set(TEST_REGEX "${TEST_REGEX}" PARENT_SCOPE)
endfunction()

function(test_expect STAT EXPR EXP GOT)
  test_expect_base("${STAT}" "${EXPR}" "${EXP}" "${GOT}" "")
  set(TEST_REGEX "${TEST_REGEX}" PARENT_SCOPE)
endfunction()

# test_compare_eps tests
function(test_expect_eps STAT EXPR EXP GOT DELTA EPS)
  test_expect_base("${STAT}" "${EXPR}" "${EXP}" "${GOT}"
    "${D5} +Delta: +${DELTA}
${D5} +EPS: +${EPS}")
  set(TEST_REGEX "${TEST_REGEX}" PARENT_SCOPE)
endfunction()

function(test_expect_poff STAT EXPR EXP GOT A B S)
  test_expect_base("${STAT}" "${EXPR}" "${EXP}" "${GOT}"
  "${D5} +${A}
${D5} +${B}
${D5} +size: ${S}")
  set(TEST_REGEX "${TEST_REGEX}" PARENT_SCOPE)
endfunction()


# test_compare_ii tests
test_expect(PASS "2 *== *2" 2 2)
test_expect(FAIL "1 *== *2" 2 1)
test_expect(FAIL "3 *<= *2" 2 3)
test_expect(PASS "2 *<= *2" 2 2)
test_expect(PASS "1 *<= *2" 2 1)
test_expect(FAIL "3 *< *2"  2 3)
test_expect(FAIL "2 *< *2"  2 2)
test_expect(PASS "1 *< *2"  2 1)
test_expect(PASS "3 *>= *2" 2 3)
test_expect(PASS "2 *>= *2" 2 2)
test_expect(FAIL "1 *>= *2" 2 1)
test_expect(PASS "3 *> *2"  2 3)
test_expect(FAIL "2 *> *2"  2 2)
test_expect(FAIL "1 *> *2"  2 1)
test_expect(FAIL "2 *!= *2" 2 2)
test_expect(PASS "1 *!= *2" 2 1)

# test_compare_iu tests
test_expect(PASS "2 *== *2" 2u 2)
test_expect(FAIL "1 *== *2" 2u 1)
test_expect(FAIL "-1 *== *\\( *uintmax_t *\\) *-1" "[0-9]+u" -1)
test_expect(FAIL "3 *<= *2" 2u 3)
test_expect(PASS "2 *<= *2" 2u 2)
test_expect(PASS "1 *<= *2" 2u 1)
test_expect(PASS "-1 *<= *2" 2u -1)
test_expect(FAIL "3 *< *2" 2u 3)
test_expect(FAIL "2 *< *2" 2u 2)
test_expect(PASS "1 *< *2" 2u 1)
test_expect(PASS "-1 *< *2" 2u -1)
test_expect(PASS "3 *>= *2" 2u 3)
test_expect(PASS "2 *>= *2" 2u 2)
test_expect(FAIL "1 *>= *2" 2u 1)
test_expect(FAIL "-1 *>= *2" 2u -1)
test_expect(PASS "3 *> *2" 2u 3)
test_expect(FAIL "2 *> *2" 2u 2)
test_expect(FAIL "1 *> *2" 2u 1)
test_expect(FAIL "-1 *> *2" 2u -1)
test_expect(FAIL "2 *!= *2" 2u 2)
test_expect(PASS "1 *!= *2" 2u 1)
test_expect(FAIL "-1 *!= *\\( *uintmax_t *\\) *-1" "[0-9]+u" -1)

# test_compare_ui tests
test_expect(PASS "2 *== *2" 2 2u)
test_expect(FAIL "1 *== *2" 2 1u)
test_expect(FAIL "\\( *uintmax_t *\\) *-1 *== *-1" -1 "[0-9]+u")
test_expect(FAIL "3 *<= *2" 2 3u)
test_expect(PASS "2 *<= *2" 2 2u)
test_expect(PASS "1 *<= *2" 2 1u)
test_expect(FAIL "2 *<= *-1" -1 2u)
test_expect(FAIL "3 *< *2" 2 3u)
test_expect(FAIL "2 *< *2" 2 2u)
test_expect(PASS "1 *< *2" 2 1u)
test_expect(FAIL "2 *< *-1" -1 2u)
test_expect(PASS "3 *>= *2" 2 3u)
test_expect(PASS "2 *>= *2" 2 2u)
test_expect(FAIL "1 *>= *2" 2 1u)
test_expect(PASS "2 *>= *-1" -1 2u)
test_expect(PASS "3 *> *2" 2 3u)
test_expect(FAIL "2 *> *2" 2 2u)
test_expect(FAIL "1 *> *2" 2 1u)
test_expect(PASS "2 *> *-1" -1 2u)
test_expect(FAIL "2 *!= *2" 2 2u)
test_expect(PASS "1 *!= *2" 2 1u)
test_expect(FAIL "\\( *uintmax_t *\\) *-1 *!= *-1" -1 "[0-9]+u")

# test_compare_uu tests
test_expect(PASS "2 *== *2" 2u 2u)
test_expect(FAIL "1 *== *2" 2u 1u)
test_expect(FAIL "3 *<= *2" 2u 3u)
test_expect(PASS "2 *<= *2" 2u 2u)
test_expect(PASS "1 *<= *2" 2u 1u)
test_expect(FAIL "3 *< *2" 2u 3u)
test_expect(FAIL "2 *< *2" 2u 2u)
test_expect(PASS "1 *< *2" 2u 1u)
test_expect(PASS "3 *>= *2" 2u 3u)
test_expect(PASS "2 *>= *2" 2u 2u)
test_expect(FAIL "1 *>= *2" 2u 1u)
test_expect(PASS "3 *> *2" 2u 3u)
test_expect(FAIL "2 *> *2" 2u 2u)
test_expect(FAIL "1 *> *2" 2u 1u)
test_expect(FAIL "2 *!= *2" 2u 2u)
test_expect(PASS "1 *!= *2" 2u 1u)

# test_compare_dd tests
test_expect(PASS "2\\. *== *2\\." "2\\.0" "2\\.0")
test_expect(FAIL "1\\. *== *2\\." "2\\.0" "1\\.0")
test_expect(FAIL "3\\. <= 2\\."   "2\\.0" "3\\.0")
test_expect(FAIL "nexttoward *\\( *2\\. *, *HUGE_VAL *\\) *<= *2\\."
  "2\\.0" "2\\.0+[1-9]")
test_expect(PASS "2\\. *<= *2\\." "2\\.0" "2\\.0")
test_expect(PASS "1\\. *<= *2\\." "2\\.0" "1\\.0")
test_expect(FAIL "3\\. *< *2\\." "2\\.0" "3\\.0")
test_expect(FAIL "2\\. *< *2\\." "2\\.0" "2\\.0")
test_expect(PASS "nexttoward *\\( *2\\. *, *- *HUGE_VAL *\\) *< *2\\."
  "2\\.0" "1\\.9+[1-9]")
test_expect(PASS "1\\. *< *2\\." "2\\.0" "1\\.0")
test_expect(PASS "3\\. *>= *2\\." "2\\.0" "3\\.0")
test_expect(PASS "2\\. *>= *2\\." "2\\.0" "2\\.0")
test_expect(FAIL "nexttoward *\\( *2\\. *, *- *HUGE_VAL *\\) *>= *2\\."
  "2\\.0" "1\\.9+[1-9]")
test_expect(FAIL "1\\. *>= *2\\." "2\\.0" "1\\.0")
test_expect(PASS "3\\. *> *2\\." "2\\.0" "3\\.0")
test_expect(PASS "nexttoward *\\( *2\\. *, *HUGE_VAL *\\) *> *2\\."
  "2\\.0" "2\\.0+[1-9]")
test_expect(FAIL "2\\. *> *2\\." "2\\.0" "2\\.0")
test_expect(FAIL "1\\. *> *2\\." "2\\.0" "1\\.0")
test_expect(FAIL "2\\. *!= *2\\." "2\\.0" "2\\.0")
test_expect(PASS "1\\. *!= *2\\." "2\\.0" "1\\.0")

test_expect_eps(FAIL "fabs\\( *2\\. *- *3\\. *\\) *< *1\\."
  "3\\.0" "2\\.0" "-1\\.0" "1\\.0")
test_expect_eps(PASS "fabs\\( *nexttoward *\\( *2\\. *, *HUGE_VAL *\\) *- *3\\. *\\) *< 1\\."
  "3\\.0" "2\\.0+[1-9]" "-1\\.0" "1\\.0")
test_expect_eps(PASS "fabs\\( *2\\.2 *- *3\\. *\\) *< *1\\."
  "3\\.0" "2\\.(19*|20*)[0-9]?" "-0\\.8" "1\\.0")
test_expect_eps(PASS "fabs\\( *3\\.8 *- *3\\. *\\) *< *1\\."
  "3\\.0" "3\\.(79*|80*)[0-9]?" "0\\.8" "1\\.0")
test_expect_eps(PASS "fabs\\( *nexttoward *\\(4\\. *, *- *HUGE_VAL\\) *- *3\\.\\) *< 1."
  "3\\.0" "3\\.9+[0-9]" "1\\.0" "1\\.0")
test_expect_eps(FAIL "fabs\\( *4\\. *- *3\\. *\\) *< *1\\."
  "3\\.0" "4\\.0" "1\\.0" "1\\.0")

# test_compare_ss tests
test_expect(PASS "strcmp *\\( *\"abc\" *, *\"abc\" *\\) *== *0"
 "\"abc\"" "\"abc\"")
test_expect(FAIL "strcmp *\\( *\"abc\" *, *\"abcd\" *\\) *== *0"
 "\"abcd\"" "\"abc\"")
test_expect(FAIL "strcmp *\\( *\"abcd\" *, *\"abc\" *\\) *== *0"
  "\"abc\"" "\"abcd\"")
test_expect(FAIL "strcmp *\\( *\"abt\" *, *\"abc\" *\\) *== *0"
  "\"abc\"" "\"abt\"")
test_expect(FAIL "strcmp *\\( *\"abt\" *, *\"ab\\\\ta\\\\vv\\\\f\\\\a\\\\bc\\\\ntt vv\" *\\) *== *0"
  "\"ab\\\\ta\\\\vv\\\\f\\\\a\\\\bc\\\\ntt vv\"" "\"abt\"")
test_expect(FAIL "strcmp *\\( *\"abt\" *, *\"ab\\\\344\\\\x03 vv\" *\\) *== *0"
  "\"ab\\\\xe4\\\\x03 vv\"" "\"abt\"")

set(TEST_REGEX "${TEST_REGEX}
${D5} 0x${HEX}+: ${H4} e403 ${H4} ${H2}00 *ab\\.\\. vv\\.")

# test_compare_ssn test
test_expect(PASS "strncmp *\\( *\"abc\" *, *\"abc\" *, *3 *\\) *== *0"
  "\"abc\"" "\"abc\"")
test_expect(FAIL "strncmp *\\( *\"abc\" *, *\"abd\" *, *3 *\\) *== *0"
  "\"abd\"" "\"abc\"")
test_expect(PASS "strncmp *\\( *\"abc\" *, *\"abd\" *, *2 *\\) *== *0"
  "\"ab\"" "\"ab\"")
test_expect(PASS "strncmp *\\( *\"ab\" *, *\"abd\" *, *2 *\\) *== *0"
  "\"ab\"" "\"ab\"")
test_expect(PASS "strncmp *\\( *\"abt\" *, *\"ab\" *, *2 *\\) *== *0"
  "\"ab\"" "\"ab\"")
test_expect(PASS "strncmp *\\( *\"abt\" *, *\"ac\" *, *1 *\\) *== *0"
  "\"a\"" "\"a\"")
test_expect(FAIL "strncmp *\\( *\"abcd\" *, *\"abc\" *, *4 *\\) *== *0"
  "\"abc\"" "\"abcd\"")
test_expect(FAIL "strncmp *\\( *\"abc\" *, *\"abcdefghijkl\" *, *4 *\\) *== *0"
  "\"abcd\"" "\"abc\"")

# Usually, address values cannot be guessed because of ASLR (Address space
# layout randomization).
test_expect(PASS "NULL == NULL" "NULL" "NULL")
test_expect(PASS "&a == &a" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&b == &a" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[5\\] == c *\\+ *5" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "c *\\+ *5 == &c\\[5\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect(FAIL "&c\\[3\\] <= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[2\\] <= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[1\\] <= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[0\\] <= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect(FAIL "&c\\[3\\] < &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[2\\] < &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[1\\] < &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[0\\] < &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect(PASS "&c\\[3\\] >= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(PASS "&c\\[2\\] >= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[1\\] >= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[0\\] >= &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect(PASS "&c\\[3\\] > &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[2\\] > &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[1\\] > &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&c\\[0\\] > &c\\[2\\]" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect(PASS "&a != &b" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")
test_expect(FAIL "&a != &a" "0[xX][0-9a-fA-F]+" "0[xX][0-9a-fA-F]+")

test_expect_poff(PASS "&c\\[0\\] - &c\\[0\\] == 0" "\\+0" "\\+0" "" "" "")
test_expect_poff(PASS "&c\\[2\\] - &c\\[0\\] == 2" "\\+2" "\\+2" "" "" "")
test_expect_poff(PASS "&c\\[0\\] - &c\\[2\\] == -2" "-2" "-2" "" "" "")
test_expect_poff(FAIL "&c\\[3\\] - &c\\[0\\] == 2" "\\+2" "\\+3"
  "&c\\[3\\]: +0[xX][0-9a-fA-F]+"
  "&c\\[0\\]: +0[xX][0-9a-fA-F]+" "[1-9][0-9]*")

test_expect_poff(FAIL "&l+ - &m+ == 0" "\\+0" "[\\+-][0-9]+"
  "&l+: +0[xX][0-9a-fA-F]+"
  "&m+:
${D5} +0[xX][0-9a-fA-F]+" "[0-9][1-9]*")

set(TEST_REGEX "${TEST_REGEX}
$")

if (NOT TEST_OUTPUT MATCHES "${TEST_REGEX}")
  message(SEND_ERROR "Result output test failed: OUTPUT does not match

${TEST_REGEX}")
endif()
