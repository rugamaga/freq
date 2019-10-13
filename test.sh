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

echo OK
