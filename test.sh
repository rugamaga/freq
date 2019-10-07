#!/bin/bash

TARGET=bin/freq

try() {
  expected="$1"
  input="$2"

  echo "$input" | $TARGET > tmp.ll
  actual=`lli tmp.ll`

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

try 0 0
try 42 42

try 30 "10 + 10 + 10"

try 10 "10 - 10 + 10"
try 10 "10 + 10 - 10"

try 10 "-10 +  10 +  10"
try 10 " 10 + -10 +  10"
try 10 " 10 +  10 + -10"

try 10 "-10 + +10 + +10"
try 10 "+10 + -10 + +10"
try 10 "+10 + +10 + -10"

try 10 "+++10"

# これが通らない
# try 10 "- - 10"

echo OK
