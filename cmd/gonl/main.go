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
	"net"
	"syscall"

	"github.com/cybwan/netlink/pkg/netlink"
	"golang.org/x/sys/unix"
)

func main() {
	AddFDBNoHook("11:11:11:11:11:11", "ens33")
}

func AddFDBNoHook(macAddress, ifName string) int {
	var ret int
	MacAddress, err := net.ParseMAC(macAddress)
	if err != nil {
		return -1
	}
	IfName, err := netlink.LinkByName(ifName)
	if err != nil {
		return -1
	}

	// Make Neigh
	neigh := netlink.Neigh{
		Family:       syscall.AF_BRIDGE,
		HardwareAddr: MacAddress,
		LinkIndex:    IfName.Attrs().Index,
		State:        unix.NUD_PERMANENT,
		Flags:        unix.NTF_SELF,
	}
	err = netlink.NeighAppend(&neigh)
	if err != nil {
		fmt.Printf("err.Error(): %v\n", err.Error())
		return -1
	}
	return ret
}

func DelFDBNoHook(macAddress, ifName string) int {
	var ret int
	MacAddress, err := net.ParseMAC(macAddress)
	if err != nil {
		return -1
	}
	IfName, err := netlink.LinkByName(ifName)
	if err != nil {
		return -1
	}

	// Make Neigh
	neigh := netlink.Neigh{
		Family:       syscall.AF_BRIDGE,
		HardwareAddr: MacAddress,
		LinkIndex:    IfName.Attrs().Index,
		State:        unix.NUD_PERMANENT,
		Flags:        unix.NTF_SELF,
	}
	err = netlink.NeighDel(&neigh)
	if err != nil {
		return -1
	}
	return ret
}
