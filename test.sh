#!/bin/bash

OPT=$*
TARGET=bin/freq

try() {
  expected="$1"
  input="$2"

  echo "$input" | $TARGET $OPT > tmp.ll
  actual=`lli tmp.ll`

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try_except() {
  input="$1"

  echo "$input" | $TARGET $OPT > tmp.ll
  lli tmp.ll &>/dev/null
  actual=$?
  if [ $actual != 0 ]; then
    echo "$input return error code, correctly"
  else
    echo "$input should return error code, but got $actual"
    exit 1
  fi
}

try_file() {
  expected="$1"
  input="$2"

  $TARGET $OPT -i "$2" -o "tmp.ll"
  actual=`lli tmp.ll`

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

# --------- tests for num
try 0 0
try 42 42

# --------- tests for add
try 30 "10 + 10 + 10"

# --------- tests for sub
try 10 "10 - 10 + 10"
try 10 "10 + 10 - 10"

try 10 "-10 +  10 +  10"
try 10 " 10 + -10 +  10"
try 10 " 10 +  10 + -10"

try 10 "-10 + +10 + +10"
try 10 "+10 + -10 + +10"
try 10 "+10 + +10 + -10"

# --------- tests for uni-plus
try 10 "+++10"

# --------- tests for uni-minus
try 10 "- - 10"
try 10 "--10"
try 10 "-+-10"

try "-10" "-+10"
try "-10" "+-10"

# --------- tests for file read
try_file 20 "test/add.fq"

# --------- tests for mul
try 100 "10 * 10"
try 1000 "10 * 10 * 10"
try 200 "10 * (10 + 10)"
try 110 "(10 * 10) + 10"
try 1000 "10 * (10 * 10)"
try 1000 "(10 * 10) * 10"
try 0 "10 * 0"
try 0 "0 * 10"
try 0 "10 * -0"
try 0 "-0 * 10"

# --------- tests for div
try 1 "10 / 10"
try 1 "100 / 10 / 10" # it means, div is left-assoc...
try 100 "100 / (10 / 10)"
try 1 "(100 / 10) / 10"
# In LLVM, how to catch this???
# try_except "10 / 0" # zero division.
try 0 "0 / 10"
# In LLVM, how to catch this???
# try_except 0 "10 / -0" # zero division
try 0 "-0 / 10"

# --------- tests for equality

# not yet implemented

# try 1 "0 == 0"
# try 0 "0 == 1"
# try 0 "1 == 0"

# try 1 "0 == (0 + 0)"
# try 0 "0 == (1 + 0)"
# try 0 "0 == (0 + 1)"

# try 1 "0 == (1 * 0)"
# try 1 "0 == (0 * 1)"

# --------- tests for inequality

# not yet implemented

# try 0 "0 == 0"
# try 1 "0 == 1"
# try 1 "1 == 0"

# try 0 "0 == (0 + 0)"
# try 1 "0 == (1 + 0)"
# try 1 "0 == (0 + 1)"

# try 0 "0 == (1 * 0)"
# try 0 "0 == (0 * 1)"

echo OK
