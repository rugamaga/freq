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
try 0 "fun main(){ 0 }"
try 42 "fun main(){ 42 }"

# --------- tests for add
try 30 "fun main(){ 10 + 10 + 10 }"

# --------- tests for sub
try 10 "fun main(){ 10 - 10 + 10 }"
try 10 "fun main(){ 10 + 10 - 10 }"

try 10 "fun main(){ -10 +  10 +  10 }"
try 10 "fun main(){  10 + -10 +  10 }"
try 10 "fun main(){  10 +  10 + -10 }"

try 10 "fun main(){ -10 + +10 + +10 }"
try 10 "fun main(){ +10 + -10 + +10 }"
try 10 "fun main(){ +10 + +10 + -10 }"

# --------- tests for uni-plus
try 10 "fun main(){ +++10 }"

# --------- tests for uni-minus
try 10 "fun main(){ - - 10 }"
try 10 "fun main(){ --10 }"
try 10 "fun main(){ -+-10 }"

try "-10" "fun main(){ -+10 }"
try "-10" "fun main(){ +-10 }"

# --------- tests for file read
try_file 20 "test/add.fq"

# --------- tests for mul
try 100 "fun main(){ 10 * 10 }"
try 1000 "fun main(){ 10 * 10 * 10 }"
try 200 "fun main(){ 10 * (10 + 10) }"
try 110 "fun main(){ (10 * 10) + 10 }"
try 1000 "fun main(){ 10 * (10 * 10) }"
try 1000 "fun main(){ (10 * 10) * 10 }"
try 0 "fun main(){ 10 * 0 }"
try 0 "fun main(){ 0 * 10 }"
try 0 "fun main(){ 10 * -0 }"
try 0 "fun main(){ -0 * 10 }"

# --------- tests for div
try 1 "fun main(){ 10 / 10 }"
try 1 "fun main(){ 100 / 10 / 10 }" # it means, div is left-assoc...
try 100 "fun main(){ 100 / (10 / 10) }"
try 1 "fun main(){ (100 / 10) / 10 }"
# In LLVM, how to catch this???
# try_except "10 / 0" # zero division.
try 0 "fun main(){ 0 / 10 }"
# In LLVM, how to catch this???
# try_except 0 "10 / -0" # zero division
try 0 "fun main(){ -0 / 10 }"

# --------- tests for equality

try 1 "fun main(){ 0 == 0 }"
try 0 "fun main(){ 0 == 1"
try 0 "fun main(){ 1 == 0"

try 1 "fun main(){ 0 == (0 + 0) }"
try 0 "fun main(){ 0 == (1 + 0) }"
try 0 "fun main(){ 0 == (0 + 1) }"

try 1 "fun main(){ 0 == (1 * 0) }"
try 1 "fun main(){ 0 == (0 * 1) }"

# --------- tests for inequality

try 0 "fun main(){ 0 != 0 }"
try 1 "fun main(){ 0 != 1 }"
try 1 "fun main(){ 1 != 0 }"

try 0 "fun main(){ 0 != (0 + 0) }"
try 1 "fun main(){ 0 != (1 + 0) }"
try 1 "fun main(){ 0 != (0 + 1) }"

try 0 "fun main(){ 0 != (1 * 0) }"
try 0 "fun main(){ 0 != (0 * 1) }"

# --------- tests for lt
try 1 "fun main(){ 10 < 100 }"
try 0 "fun main(){ 100 < 10 }"
try 0 "fun main(){ 10 < 10 }"

# --------- tests for lteq
try 1 "fun main(){ 10 <= 100 }"
try 0 "fun main(){ 100 <= 10 }"
try 1 "fun main(){ 10 <= 10 }"

# --------- tests for gt
try 0 "fun main() { 10 > 100 }"
try 1 "fun main() { 100 > 10 }"
try 0 "fun main() { 10 > 10 }"

# --------- tests for gteq
try 0 "fun main() { 10 >= 100 }"
try 1 "fun main() { 100 >= 10 }"
try 1 "fun main() { 10 >= 10 }"

# --------- tests for stmts
try 22 "fun main(){ 10 + 1; 20 + 2 }"

# --------- tests for variables
try 10 "fun main() { let a = 10; a }"
try 11 "fun main() { let a = 10; a + 1 }"
try 7 "fun main() { let a = 10; let b = 3; a - b }"
try 7 "fun main() { let alpha = 10; let beta = 3; alpha - beta }"
try 7 "fun main() { let a0 = 10; let a1 = 3; a0 - a1 }"

# --------- tests for ret
try 10 "fun main() { ret 10 }"
try 10 "fun main() { ret 10; 20 }"
try 10 "fun main() { ret 10; ret 20 }"

# --------- tests for block
try 3 "fun main() { { let a = 1; let b = 2; a + b } }"

# --------- tests for func
try 1 "fun main() { let a = 1; a } fun sub() { let a = 2; a }"
try 2 "fun sub() { let a = 1; a } fun main() { let a = 2; a }"

echo OK
