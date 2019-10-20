{-# LANGUAGE ViewPatterns #-}

module Tokenizer where

import Data.Char
import Data.List

data Token =
  RETURN |
  LOOP |
  ELSE |
  LET |
  FUN |
  IF |
  EQUAL |
  NOT_EQUAL |
  LESS_THAN_EQUAL |
  GREATER_THAN_EQUAL |
  LESS_THAN |
  GREATER_THAN |
  ASSIGN |
  PLUS |
  MINUS |
  MUL |
  DIV |
  LEFT_PAREN |
  RIGHT_PAREN |
  LEFT_BRACKET |
  RIGHT_BRACKET |
  LEFT_BRACE |
  RIGHT_BRACE |
  SEMICOLON |
  COMMA |
  NUM String |
  IDENT String
  deriving (Show, Eq)

keyword :: String -> String -> (Maybe String)
keyword pattern (stripPrefix pattern -> Just rest)
  | isAlphaNum $ head rest = Nothing
  | otherwise = Just rest
keyword _ _ = Nothing

operator :: String -> String -> (Maybe String)
operator = stripPrefix

space :: String -> (Maybe String)
space s
  | isSpace $ head s = Just $ dropWhile isSpace s
  | otherwise = Nothing

ident :: String -> (Maybe (String, String))
ident target
  | isAlpha $ head target = Just (
      takeWhile isAlphaNum target,
      dropWhile isAlphaNum target
    )
  | otherwise = Nothing

number :: String -> (Maybe (String, String))
number target
  | isDigit $ head target = Just (
      takeWhile isDigit target,
      dropWhile isDigit target
    )
  | otherwise = Nothing

tokenize :: String -> [Token]
tokenize [] = []
tokenize (space -> Just rest) = tokenize rest
tokenize (keyword "return" -> Just rest) = RETURN : tokenize rest
tokenize (keyword "loop" -> Just rest) = LOOP : tokenize rest
tokenize (keyword "else" -> Just rest) = ELSE : tokenize rest
tokenize (keyword "let" -> Just rest) = LET : tokenize rest
tokenize (keyword "fun" -> Just rest) = FUN : tokenize rest
tokenize (keyword "if" -> Just rest) = IF : tokenize rest
tokenize (operator "==" -> Just rest) = EQUAL : tokenize rest
tokenize (operator "!=" -> Just rest) = NOT_EQUAL : tokenize rest
tokenize (operator "<=" -> Just rest) = LESS_THAN_EQUAL : tokenize rest
tokenize (operator ">=" -> Just rest) = GREATER_THAN_EQUAL : tokenize rest
tokenize (operator "<" -> Just rest) = LESS_THAN : tokenize rest
tokenize (operator ">" -> Just rest) = GREATER_THAN : tokenize rest
tokenize (operator "=" -> Just rest) = ASSIGN : tokenize rest
tokenize (operator "+" -> Just rest) = PLUS : tokenize rest
tokenize (operator "-" -> Just rest) = MINUS : tokenize rest
tokenize (operator "*" -> Just rest) = MUL : tokenize rest
tokenize (operator "/" -> Just rest) = DIV : tokenize rest
tokenize (operator "(" -> Just rest) = LEFT_PAREN : tokenize rest
tokenize (operator ")" -> Just rest) = RIGHT_PAREN : tokenize rest
tokenize (operator "[" -> Just rest) = LEFT_BRACKET : tokenize rest
tokenize (operator "]" -> Just rest) = RIGHT_BRACKET : tokenize rest
tokenize (operator "{" -> Just rest) = LEFT_BRACE : tokenize rest
tokenize (operator "}" -> Just rest) = RIGHT_BRACE : tokenize rest
tokenize (operator ";" -> Just rest) = SEMICOLON : tokenize rest
tokenize (operator "," -> Just rest) = COMMA : tokenize rest
tokenize (ident -> Just (i, rest)) = (IDENT i) : (tokenize rest)
tokenize (number -> Just (n, rest)) = (NUM n) : (tokenize rest)
tokenize _ = undefined
