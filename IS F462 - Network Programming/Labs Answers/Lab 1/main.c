#include <stdio.h>
#include "function.h"

void main() {
	int a, b;
	printf("Enter two numbers:");
	scanf("%d %d", &a, &b);
	printf("Sum of the two numbers %d and %d is %d\n", a, b, add(a,b));
	return;
}
