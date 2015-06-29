#include <string.h>

// TODO TEST THIS..

int main(void) {

	return 1;
}

void string_static_trim_right(char *string) {
	int i;

	for (i = strlen(string) - 1; (i >= 0) && (string[i] == ' '); i--) {
		//string[i] = string[i+1];
		string[i] = '\0';
	}
}

void string_static_trim_left(char *string) {
	int i, j;

	for (i = 0; (string[i] != '\0') && (string[i] == ' '); i++)
		;

	if (i == 0) {
		return;
	}

	for (j = i; string[j - 1] != '\0'; j++) {
		string[j - i] = string[j];
	}
}

void string_static_trim(char *string) {
	string_static_trim_left(string);
	string_static_trim_right(string);
}
