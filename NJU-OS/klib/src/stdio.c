#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

enum NUM_TYPE { DEC, HEX };
static const char num[] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
	'a', 'b', 'c', 'd', 'e', 'f'
};

void putnum(enum NUM_TYPE type, unsigned int n) {
	int stack[20], tt = 0;
	int mod;
	
	switch (type) {
	case DEC:
		mod = 10;
		if ((int)n < 0) {
			putch('-');
			n = -(int)n;
		}
		break;
	case HEX:
		mod = 16;
		putstr("0x");
		break;
	default:
		panic("Number Type Error");
		break;
	}
	if(type==DEC&&n==0)putch('0');
	while (n) {
		stack[ ++ tt] = n % mod;
		n /= mod;
	}
	while (tt) {
		putch(num[stack[tt -- ]]);
	}
}

int printf(const char *fmt, ...) {
	va_list ap;
	char cur_fmt;
	int d;
	char c, *s;
	uintptr_t p;
	
	va_start(ap, fmt);
	while (*fmt) {
		switch (cur_fmt = *fmt++) {
		case '%':
			panic_on(!(*fmt), "Format error");
			switch (cur_fmt = *fmt++) {
			case 'c':
				c = (char)va_arg(ap, int);
				putch(c);
				break;
			case 'd':
				d = va_arg(ap, int);
				putnum(DEC, d);
				break;
			case 's':
				s = va_arg(ap, char *);
				putstr(s);
				break;
			case 'x':
				d = va_arg(ap, int);
				putnum(HEX, d);
				break;
			case 'p':
				p = va_arg(ap, uintptr_t);
				putnum(HEX, p);
				break;
			default:
				panic("No corresponding format\n");
				break;
			}
			break;
		default:
			putch(cur_fmt);
			break;
		}
	}
	va_end(ap);

	return 0;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
//   if (fmt == NULL)return -1;

//     va_list ap ;
//     char* str = (char*)fmt;
//     unsigned ret_num = 0;
//     int ArgIntVal = 0;
//     //unsigned long ArgHexVal = 0;
//     char* ArgStrVal = NULL;
//     //double ArgFloVal = 0.0;
//     //unsigned long val_seg = 0;
//     //unsigned long val_temp = 0;
//     //int cnt = 0;
//     unsigned long i = 0;

//     va_start(ap, fmt);
//     while ((*str) != '\0') {
//         switch (*str) {
//         // case '\t':
//         //     *(out+ret_num)=' ';
//         //     *(out+ret_num+1)=' ';
//         //     *(out+ret_num+2)=' ';
//         //     *(out+ret_num+3)=' ';
//         //     ret_num += 4;
//         //     str++;
//         //     break;
//         case '%':
//             str++;
//             switch (*str) {
//             case 'c':
//                 ArgIntVal = va_arg(ap, int);
//                 *(out+ret_num)=ArgIntVal;
//                 ret_num++;
//                 str++;
//                 continue;
//             case 'd':
//                 ArgIntVal = va_arg(ap, int);
//                 if (ArgIntVal < 0) {
//                     ArgIntVal = -ArgIntVal;
//                     *(out+ret_num)='-';
//                     ret_num++;
//                 }
//                 for (i = power(10,log(ArgIntVal,10)); i > 0; i /= 10) {
//                     if (ArgIntVal / i > 0) {
//                         *(out+ret_num)=((ArgIntVal / i) % 10 + '0');
//                         ret_num++;
//                     }
//                 }
//                 i = 0;
//                 str++;
//                 continue;
//             case 'o':
//                 ArgIntVal = va_arg(ap, int);
//                 if (ArgIntVal < 0) {
//                     ArgIntVal = -ArgIntVal;
//                     *(out+ret_num)='-';
//                     ret_num++;
//                 }
//                 for (i = power(010,log(ArgIntVal,010)); i > 0; i /= 010) {
//                     if (ArgIntVal / i > 0) {
//                         *(out+ret_num)=((ArgIntVal / i) % (010) + '0');
//                         ret_num++;
//                     }
//                 }
//                 i = 0;
//                 str++;
//                 continue;
//             case 'x':
//                 ArgIntVal = va_arg(ap, int);
//                 if (ArgIntVal < 0) {
//                     ArgIntVal = -ArgIntVal;
//                     *(out+ret_num)='-';
//                     ret_num++;
//                 }
//                 for (i = power(0x10,log(ArgIntVal,0x10)); i > 0; i /= 0x10) {
//                     if (ArgIntVal / i > 0) {
//                         *(out+ret_num)=((ArgIntVal / (0x10)) + '0');
//                         ret_num++;
//                     }
//                 }
//                 i = 0;
//                 str++;
//                 continue;
//             case 's':
//                 ArgStrVal = va_arg(ap, char*);
//                 i = 0;
//                 while (*(ArgStrVal + i) != '\0') {
//                     *(out+ret_num)=(*(ArgStrVal + i));
//                     ret_num++;
//                     i++;
//                 }
//                 i = 0;
//                 str++;
//                 continue;
//             // case 'f':
//             //     //6位精度
//             //     ArgFloVal = va_arg(ap, double);
//             //     if (ArgFloVal < 0) {
//             //         putch('-');
//             //         ret_num++;
//             //         ArgFloVal = -ArgFloVal;
//             //     }
//             //     val_temp = (unsigned long)ArgFloVal;
//             //     for (i = power(10, log(ArgIntVal, 10)); i > 0; i /= 10) {
//             //         if (val_temp / i > 0) {
//             //             putch((val_temp / i) % 10 + '0');
//             //             ret_num++;
//             //         }
//             //     }
//             //     val_temp = (unsigned long)((ArgFloVal - val_temp) * 1000000);
//             //     if(val_temp>0){
//             //		putch('.');
//             //	}
//             //     for (i = power(10, log(ArgIntVal, 10)); i > 0; i /= 10) {
//             //         if (val_temp / i > 0) {
//             //             putch((val_temp / i) % 10 + '0');
//             //             ret_num++;
//             //         }
//             //     }
//             //     i = 0;
//             //     str++;
//             //     continue;
//             case '%':
//                 *(out+ret_num)=*str;
//                 ret_num++;
//                 continue;
//             case '\0':
//                 *(out+ret_num)=*(str - 1);
//                 continue;
//             default://wrong
//                 *(out+ret_num)=*(str - 1);
//                 ret_num++;
//                 continue;
//             }
//             break;
//         default:
//             *(out+ret_num)=*str;
//             ret_num++;
//             str++;
//             break;
//         }
//     }
// va_end(ap);
// return ret_num;
panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
