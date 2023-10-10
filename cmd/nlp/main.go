package main

import (
	"fmt"
	"reflect"

	nl "github.com/vishvananda/netlink"

	"github.com/cybwan/netlink/pkg/nlp"
)

func main() {
	nlp.PortList()
	links, _ := nl.LinkList()
	for _, link := range links {
		lt := reflect.TypeOf(link)
		fmt.Printf("%v = %v\n", lt, link)
	}
}
