/*!
  \file   monalibc.cpp
  \brief  mona c library

  Copyright (c) 2002,2003,2004 shadow
  All rights reserved.<BR>
  \b License NYSL<BR>
  \b Create 2004/02/17
  \author  shadow

  $Revision$
  $Date$
*/
#include <monalibc.h>
#include <string.h>

#define P_FLAG_WIDTH 0x01
#define P_FLAG_PRECISION 0x02


int uitos(char* s, unsigned int n, int real_width, int base, char flag);
size_t __power(size_t x, size_t y);

/*!
  \brief equivalent to the function sprintf

  \param s      buf printed characters
  \param format specifies how subsequent arguments
  \param arg    a variable number of arguments
  \return  the number of characters printed (including '\\0' end of string)
*/
int vsprintf(char *s, const char *format, va_list arg){
  int result = 0;
  int loop;
  int i;
  int width;
  char flag;
  int precision;
  char p_flag;

  for(i = 0; format[i] != '\0'; i++){
    if(format[i] == '%'){
      loop = 1;
      flag = 0;
      p_flag = 0;
      width = 0;
      precision = 0;
      while(loop){
        i++;
        switch(format[i]){
          case 's':
            result += strcpy2(&s[result], va_arg(arg, char *));
            result--;
            loop = 0;
            break;
          case 'd':
          case 'i':
            result += itos(&s[result], va_arg(arg, int), width, 10, flag);
            loop = 0;
            break;
          case 'o':
            flag |= P_FORMAT_UNSIGNED;
            result += itos(&s[result], va_arg(arg, int), width, 8, flag);
            loop = 0;
            break;
          case 'u':
            flag |= P_FORMAT_UNSIGNED;
            result += itos(&s[result], va_arg(arg, int), width, 10, flag);
            loop = 0;
            break;
          case 'X':
            flag |= P_FORMAT_CAPITAL;
          case 'x':
            flag |= P_FORMAT_UNSIGNED;
            result += itos(&s[result], va_arg(arg, int), width, 16, flag);
            loop = 0;
            break;
          case 'f':
            result += ftos(&s[result], va_arg(arg, double), width, precision, flag);
            loop = 0;
            break;
          case 'c':
            s[result++] = va_arg(arg, char);
            loop = 0;
            break;
          case '%':
            s[result++] = '%';
            loop = 0;
            break;
          case '-':
            flag |= P_FORMAT_MINUS;
            break;
          case '+':
            flag |= P_FORMAT_PLUS;
            break;
          case ' ':
            flag |= P_FORMAT_SPACE;
            break;
          case '0':
            if(!(p_flag & P_FLAG_WIDTH) && !(p_flag & P_FLAG_PRECISION)){
              flag |= P_FORMAT_ZERO;
              break;
            }
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if(p_flag & P_FLAG_PRECISION){
              precision = precision*10 + (int)(format[i] - '0');
            } else {
              width = width*10 + (int)(format[i] - '0');
              p_flag |= P_FLAG_WIDTH;
            }
            break;
          case '.':
            p_flag |= P_FLAG_PRECISION;
            break;
          case '\0':
            i--;
            loop = 0;
            break;
        }
      }
    } else {
      s[result++] = format[i];
    }
  }
  s[result] = 0;
  return result;
}

int sprintf(char *s, const char *format, ...){
  int result;
  va_list args;

  va_start(args, format);
  result = vsprintf(s, format, args);
  va_end(args);

  return result;
}

int itos(char *s, int n, int width, int base, char flag){
  int num;
  int real_width;
  int i;
  int j = 0;
  char charP = '+';
  
  if(s == NULL){
    return 0;
  }

  if(!(flag & P_FORMAT_UNSIGNED) && (n < 0)){/* negative number */
    flag |= P_FORMAT_PLUS;
    charP = '-';
    n = n * -1;
  }
  num = n;

  if((flag & P_FORMAT_SPACE) && !(flag & P_FORMAT_PLUS)){
    flag |= P_FORMAT_PLUS;
    charP = ' ';
  }

  for(real_width = 0; num != 0; real_width++){
    num /= base;
  }
  
  if((flag & P_FORMAT_ZERO) && (real_width >= width)){
    flag &= ~P_FORMAT_ZERO;
  }

  if(flag & P_FORMAT_PLUS){
    if(flag & P_FORMAT_MINUS){
      s[j] = charP;
      j++;
      j += uitos(&s[j], n, real_width, base, flag);
      for(i = 0; j < width; i++){
        s[j] = ' ';
         j++;
      }
    } else if(flag & P_FORMAT_ZERO){
      s[j] = charP;
      j++;
      for(i = 0; j < width - real_width; i++){
        s[j] = '0';
         j++;
      }
      j += uitos(&s[j], n, real_width, base, flag);
    } else {
      for(i = 0; j < width - real_width - 1; i++){
        s[j] = ' ';
         j++;
      }
      s[j] = charP;
      j++;
      j += uitos(&s[j], n, real_width, base, flag);
    }
  } else {
    if(flag & P_FORMAT_MINUS){
      j += uitos(&s[j], n, real_width, base, flag);
      for(i = 0; j < width; i++){
        s[j] = ' ';
         j++;
      }
    } else if(flag & P_FORMAT_ZERO){
      for(i = 0; j < width - real_width; i++){
        s[j] = '0';
         j++;
      }
      j += uitos(&s[j], n, real_width, base, flag);
    } else {
      for(i = 0; j < width - real_width; i++){
        s[j] = ' ';
         j++;
      }
      j += uitos(&s[j], n, real_width, base, flag);
    }
  }
  if(flag & P_FORMAT_TERMINATE){
    s[j] = 0;
    j++;
  }
  return j;
}

int uitos(char* s, unsigned int n, int real_width, int base, char flag){
  int j = 0;
  int i;
  size_t ch;
  char basechar = 'a';

  if(flag & P_FORMAT_CAPITAL){
    basechar = 'A';
  }

  for(i = real_width -1; i >= 0; i--){
    ch = n / __power(base, i);
    n %= __power(base, i);

    if(i == 0 && n > 9){
      s[j] = (basechar + n -10);
      j++;
    } else if (i == 0){
      s[j] = ('0' + n);
      j++;
    } else if (ch > 9){
      s[j] = (basechar + ch -10);
      j++;
    } else {
      s[j] = ('0' + ch);
      j++;
    }
  }

  return j;
}

int ftos(char *s, double n, int width, int precision, char flag){
  int num, fraction;
  int j = 0;
  char tmpflag = 0;

  if(s == NULL){
    return 0;
  }

  num = (int)n;
  fraction = (int)((n - (double)num)*__power(10, precision));
  if(precision > 0){
    if(flag & P_FORMAT_TERMINATE){
      flag &= ~P_FORMAT_TERMINATE;
      tmpflag = P_FORMAT_TERMINATE;
    }
    if(num != 0){
     width -= (precision + 1);
     j += itos(s, num, width, 10, flag);
     s[j] = '.';
     j++;
     if(fraction < 0) fraction *= -1;
     j += itos(&s[j], fraction, precision, 10, tmpflag);
    } else { /* num == 0 */
      if(fraction >= 0){
       s[j++] = '0';
       s[j++] = '.';
       j += itos(&s[j], fraction, precision, 10, tmpflag);
      } else { /* num == 0 && fraction < 0 */
       s[j++] = '-';
       s[j++] = '0';
       s[j++] = '.';
       fraction *= -1;
       j += itos(&s[j], fraction, precision, 10, tmpflag);
      }
    }
  } else { /* precision <= 0 */
    j += itos(s, num, width, 10, flag);
  }

  return j;
}

size_t __power(size_t x, size_t y){

  size_t result = x;
  size_t i;
  for(i = 1; i < y; i++){
    result *= x;
  }
  return result;
}

int strcpy2(char *s1, const char *s2){
  char *tmp = s1;

  while((*tmp++ = *s2++));

  return (int)(tmp - s1);
}
