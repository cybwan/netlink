package main

/*
#include <stdbool.h>

#include <stdlib.h>
int test(int a,int b) {
	return b|~a;
}

int mask(uint a,uint b) {
	return ~(a<<(32-b));
}

#cgo CFLAGS: -g -Og -W -Wextra -Wno-unused-parameter
*/
import "C"
import (
	"fmt"
)

func main() {
	// ret := AddAddrNoHook("7.7.7.0/24 ens33:1", "ens33")
	// fmt.Println(ret)
	//DelRouteNoHook("7.7.7.0/24")

	a := byte(0b01010101)
	b := byte(0b11110101)
	fmt.Printf("A=%08b\n", a)
	fmt.Printf("B=%08b\n", b|^a)

	c := uint8(C.test(C.int(a), C.int(b)))
	fmt.Printf("C=%08b\n", ^a)
	fmt.Printf("C=%08b\n", c)
	fmt.Printf("Mask=%032b\n", uint32(C.mask(C.uint(0xFFFFFFFF), C.uint(24))))
}

// func AddAddrNoHook(address, ifName string) int {
// 	var ret int
// 	IfName, err := netlink.LinkByName(ifName)
// 	if err != nil {
// 		return -1
// 	}
// 	Address, err := netlink.ParseAddr(address)
// 	if err != nil {
// 		return -1
// 	}
// 	err = netlink.AddrAdd(IfName, Address)
// 	if err != nil {
// 		return -1
// 	}
// 	return ret
// }
