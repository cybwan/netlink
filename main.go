package main

/*
#include <stdio.h>

extern int goCallbackHandler(int, int);

static int doAdd(int a, int b) {
  printf("a+b=%d+%d\n",a,b);
  return goCallbackHandler(5, b);
}
*/
import "C"
import "fmt"

//export goCallbackHandler
func goCallbackHandler(a, b C.int) C.int {
	return a + b
}

// This is the public function, callable from outside this package.
// It forwards the parameters to C.doAdd(), which in turn forwards
// them back to goCallbackHandler(). This one performs the addition
// and yields the result.
func MyAdd(a, b int) int {
	return int(C.doAdd(C.int(a), C.int(b)))
}

func main() {
	fmt.Println(MyAdd(1, 2))
}
