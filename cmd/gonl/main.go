package main

import (
	"fmt"

	nl "github.com/vishvananda/netlink"
)

func main() {
	//nlp.PortList()
	links, _ := nl.LinkList()
	for _, link := range links {
		//lt := reflect.TypeOf(link)
		if vxlan, ok := link.(*nl.Vxlan); ok {
			fmt.Printf("VxlanId:%d VtepDevIndex:%d\n", vxlan.VxlanId, vxlan.VtepDevIndex)
		}
		//fmt.Printf("%v = %v\n", lt, link)
	}
}
