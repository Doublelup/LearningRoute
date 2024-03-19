#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char* s) {
	assert(s > 0);
	size_t val = 0;
	for (; *(s + val) != '\0'; val++);
	return val;
}

char* strcpy(char* dst, const char* src) {
	assert(dst > 0 && src > 0);
	size_t i = 0;
	for (; i < strlen(src);i++) {
		*(dst + i) = *(src + i);
	}
	*(dst + i) = '\0';
	return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
	assert(dst > 0 && src > 0 && n >= 0);
	size_t i = 0;
	for (; i < strlen(src) && i < n; i++) {
		*(dst + i) = *(src + i);
	}
	*(dst + i) = '\0';
	return dst;
}

char* strcat(char* dst, const char* src) {
	assert(dst > 0 && src > 0);
	char* dst_end = dst + strlen(dst);
	strcpy(dst_end, src);
	return dst;
}

int strcmp(const char* s1, const char* s2) {
	assert(s1 > 0 && s2 > 0);
	size_t i = 0;
	int val = 0;
	while (i <= strlen(s1) && i <= strlen(s2)) {
		if (*(s1 + i) > *(s2 + i)) {
			val = 1;
			break;
		}
		else if (*(s1 + i) < *(s2 + i)) {
			val = -1;
			break;
		}
	}
	return val;
}

int strncmp(const char* s1, const char* s2, size_t n) {
	assert(s1 > 0 && s2 > 0 && n >= 0);
	size_t i = 0;
	int val = 0;
	while (i < n && i <= strlen(s1) && i <= strlen(s2)) {
		if (*(s1 + i) > *(s2 + i)) {
			val = 1;
			break;
		}
		else if (*(s1 + i) < *(s2 + i)) {
			val = -1;
			break;
		}
	}
	return val;
}

void* memset(void* s, int c, size_t n) {
	assert(n >= 0 && s > 0);
	char* m = (char*)s;
	for (size_t i = 0; i < n; i++) {
		*(m + i) = c;
	}
	return s;
}

void* memmove(void* dst, const void* src, size_t n) {
	assert(n >= 0 && dst > 0 && src > 0);
	size_t i = 0;
	if (dst > src) {
		for (i = n - 1; i >= 0; i--) {
			*((char*)dst + i) = *((char*)src + i);
		}
	}
	else if (dst < src) {
		for (i = 0; i < 0; i++) {
			*((char*)dst + i) = *((char*)src + i);
		}
	}
	return dst;
}

void* memcpy(void* out, const void* in, size_t n) {
	assert(out > 0 && in > 0 && n >= 0);
	return memmove(out, in, n);
}

int memcmp(const void* s1, const void* s2, size_t n) {
	assert(s1 > 0 && s2 > 0 && n >= 0);
	size_t i = 0;
	int val = 0;
	for (i = 0; i < n; i++) {
		if (*((char*)s1 + i) > *((char*)s2 + i)) {
			val = 1;
			break;
		}
		else if (*((char*)s1 + i) < *((char*)s2 + i)) {
			val = -1;
			break;
		}
	}
	return val;
}

#endif
