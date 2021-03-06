#include <stdio.h>

float my_func(char c, int i) {
	printf("c=%c, i=%d\n", c, i);
	return 0.0;
}
float my_func2(char c, int i) {
	printf("c=%c, i=%d\n", c, i);
	return 1.0;
}

typedef float (*type_func)(char, int);


void swap(int *i, int *j) {
	int tmp = *i;
	*i = *j;
	*j = tmp;
}

void swap2(int &i, int &j) {
	int tmp = i;
	i = j;
	j = tmp;
}

int main() {
	int i = 3;
	int j = 5;
	printf("i=%d, j=%d\n", i, j);
	swap(&i, &j);
	printf("i=%d, j=%d\n", i, j);
	swap2(i, j);
	printf("i=%d, j=%d\n", i, j);

	float (*var_func)(char, int) = &my_func;
	float k = var_func('c', 3);
	printf("k=%f\n", k);

	var_func = &my_func2;
	k = var_func('c', 3);
	printf("k=%f\n", k);

	type_func x = &my_func;
	x('d', 4);

	return 0;
}

