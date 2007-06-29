static TRESWORD ResWordsTable[] =
{
  { "shr", SHR },
  { "or", '|' },
  { "and", '&' },
  { "unsigned", UNSIGNED },
  { "float", FLOAT },
  { "not", '~' },
  { "xor", '^' },
  { "mod", '%' },
  { "int", INT },
  { "div", '/' },
  { "shl", SHL },
  { "double", DOUBLE },
};

#define RES_C_WORD_MIN_LEN  2

#define RES_C_WORD_MAX_LEN  8

static signed char HashCharValue['z' - '0' + 1]=
{
  SCHAR_MIN, // '0'
  SCHAR_MIN, // '1'
  SCHAR_MIN, // '2'
  SCHAR_MIN, // '3'
  SCHAR_MIN, // '4'
  SCHAR_MIN, // '5'
  SCHAR_MIN, // '6'
  SCHAR_MIN, // '7'
  SCHAR_MIN, // '8'
  SCHAR_MIN, // '9'
  SCHAR_MIN, // ':'
  SCHAR_MIN, // ';'
  SCHAR_MIN, // '<'
  SCHAR_MIN, // '='
  SCHAR_MIN, // '>'
  SCHAR_MIN, // '?'
  SCHAR_MIN, // '@'
  SCHAR_MIN, // 'A'
  SCHAR_MIN, // 'B'
  SCHAR_MIN, // 'C'
  SCHAR_MIN, // 'D'
  SCHAR_MIN, // 'E'
  SCHAR_MIN, // 'F'
  SCHAR_MIN, // 'G'
  SCHAR_MIN, // 'H'
  SCHAR_MIN, // 'I'
  SCHAR_MIN, // 'J'
  SCHAR_MIN, // 'K'
  SCHAR_MIN, // 'L'
  SCHAR_MIN, // 'M'
  SCHAR_MIN, // 'N'
  SCHAR_MIN, // 'O'
  SCHAR_MIN, // 'P'
  SCHAR_MIN, // 'Q'
  SCHAR_MIN, // 'R'
  SCHAR_MIN, // 'S'
  SCHAR_MIN, // 'T'
  SCHAR_MIN, // 'U'
  SCHAR_MIN, // 'V'
  SCHAR_MIN, // 'W'
  SCHAR_MIN, // 'X'
  SCHAR_MIN, // 'Y'
  SCHAR_MIN, // 'Z'
  SCHAR_MIN, // '['
  SCHAR_MIN, // '\'
  SCHAR_MIN, // ']'
  SCHAR_MIN, // '^'
  SCHAR_MIN, // '_'
  SCHAR_MIN, // '`'
     -6, // 'a'
  SCHAR_MIN, // 'b'
  SCHAR_MIN, // 'c'
      8, // 'd'
      2, // 'e'
     -6, // 'f'
  SCHAR_MIN, // 'g'
  SCHAR_MIN, // 'h'
      4, // 'i'
  SCHAR_MIN, // 'j'
  SCHAR_MIN, // 'k'
     14, // 'l'
     -2, // 'm'
      0, // 'n'
     -7, // 'o'
  SCHAR_MIN, // 'p'
  SCHAR_MIN, // 'q'
      4, // 'r'
     -6, // 's'
      4, // 't'
     -5, // 'u'
     -2, // 'v'
  SCHAR_MIN, // 'w'
      1, // 'x'
  SCHAR_MIN, // 'y'
  SCHAR_MIN, // 'z'
};

