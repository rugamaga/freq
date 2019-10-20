module Main where

import Tokenizer
import Parser

main :: IO ()
main = print $ tokenize "fun main() { return 1 }"
