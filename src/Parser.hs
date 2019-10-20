{-# LANGUAGE ViewPatterns #-}

module Parser where

import Tokenizer

import Text.Parsec
import Data.Char
import Data.List

data Syntax =
  Root [Syntax] |
  Func String [Syntax] Syntax |
  Block [Syntax] |
  If Syntax Syntax Syntax |
  Loop Syntax |
  Let Syntax (Maybe Syntax) |
  Add Syntax Syntax |
  Sub Syntax Syntax |
  Mul Syntax Syntax |
  Div Syntax Syntax |
  Assign Syntax Syntax |
  Equal Syntax Syntax |
  NotEqual Syntax Syntax |
  LessThan Syntax Syntax |
  LessThanEqual Syntax Syntax |
  GreaterThan Syntax Syntax |
  GreaterThanEqual Syntax Syntax |
  Num Token |
  Var Token
  deriving (Show, Eq)
