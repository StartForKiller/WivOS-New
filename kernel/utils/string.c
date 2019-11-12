#include <utils/string.h>
#include <mm/mm.h>

char* itoa(int num, char* str, int base)
{
    int i = 0;
	char isNegative = 0;

	if (num == 0)
	{
		str[i++] = '0';
		str[i] = '\0';
		return str;
	}

	if (num < 0 && base == 10)
	{
		isNegative = 1;
		num = -num;
	}

	while (num != 0)
	{
		int rem = num % base;
		str[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0';
		num = num/base;
	}

	if (isNegative)
		str[i++] = '-';

	str[i] = '\0';

	reverse(str, i);

	return str;
}

void reverse(char str[], int length)
{
	int start = 0;
	int end = length -1;
	while (start < end)
	{
		char tmp = *(str+start);
		*(str+start) = *(str+end);
		*(str+end) = tmp;
		start++;
		end--;
	}
}

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

int strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	while (str1[i] && str1[i] == str2[i]) i++;

	return str1[i] - str2[i];
}

int strncmp(const char *str1, const char *str2, size_t len) {
	size_t i = 0;
	while (i < len - 1 && str1[i] == str2[i]) i++;
	return str1[i] - str2[i];
}

char *strcpy(char *dest, const char *src) {
	size_t i;
	for (i=0; src[i]; i++) {
		dest[i] = src[i];
	}
	dest[i] = '\0';
	
	return dest;
}

char *strchr(char *str, int ch) {
	for (; *str; str++)
		if(*str==(char)ch)
			return str;
			
	return (char*)0;
}

char *strchrnul(const char *s, int c) {
    while (*s)
        if ((*s++) == c)
            break;
    return (char *)s;
}

size_t
lfind(
		const char * str,
		const char accept
	 ) {
	size_t i = 0;
	while ( str[i] != accept) {
		i++;
	}
	return (size_t)(str) + i;
}

size_t
strspn(
		const char * str,
		const char * accept
	  ) {
	const char * ptr;
	const char * acc;
	size_t size = 0;
	for (ptr = str; *ptr != '\0'; ++ptr) {
		for (acc = accept; *acc != '\0'; ++acc) {
			if (*ptr == *acc) {
				break;
			}
		}
		if (*acc == '\0') {
			return size;
		} else {
			++size;
		}
	}
	return size;
}

char *
strpbrk(
		const char * str,
		const char * accept
	   ) {
	while (*str != '\0') {
		const char *acc = accept;
		while (*acc != '\0') {
			if (*acc++ == *str) {
				return (char *) str;
			}
		}
		++str;
	}
	return NULL;
}

char *
strtok_r(
		char * str,
		const char * delim,
		char ** saveptr
		) {
	char * token;
	if (str == NULL) {
		str = *saveptr;
	}
	str += strspn(str, delim);
	if (*str == '\0') {
		*saveptr = str;
		return NULL;
	}
	token = str;
	str = strpbrk(token, delim);
	if (str == NULL) {
		*saveptr = (char *)lfind(token, '\0');
	} else {
		*str = '\0';
		*saveptr = str + 1;
	}
	return token;
}

char * strdup(const char * s) {
	size_t l = strlen(s);
	return memcpy(malloc(l+1), s, l+1);
}

void *memset(void *arr, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char*)arr)[i] = (unsigned char)val;
		
	return arr;
}

int memcmp(const void *m1, const void *m2, size_t len) {
	size_t i;
	for (i = 0; i< len; i++) {
		if (((unsigned char*)m1)[i] > ((unsigned char*)m2)[i])
			return 1;
			
		if (((unsigned char*)m1)[i] < ((unsigned char*)m2)[i])
			return -1;
	}
	return 0;
}

void *memcpy(void *dest, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++) 
		((unsigned char*)dest)[i] = ((unsigned char*)src)[i];
		
	return dest;
}

void *memmove(void *dest, const void *src, size_t len) {
	unsigned char cpy[len];
	memcpy(cpy, src, len);
	return memcpy(dest, cpy, len);
}

void *kmemmove(void *dest, const void *src, size_t count) {
    size_t i = 0;
    unsigned char *dest2 = dest;
    const unsigned char *src2 = src;
    if (src > dest) {
        for (i = 0; i < count; i++) {
            dest2[i] = src2[i];
        }
    } else if (src < dest) {
        for (i = count; i > 0; i--) {
            dest2[i - 1] = src2[i - 1];
        }
    }
    return dest;
}

void *memchr(void *haystack, int needle, size_t size) {
	char *h = (char *)haystack;
	for (; size--; h++) {
		if (*h == (char)needle)
			return (void*)h;		
	}
	
	return (void*)0;
}