package main

import (
	"fmt"

	nl "github.com/cybwan/netlink/pkg/netlink"
)

func main() {
	links, err := nl.LinkList()
	if err != nil {
		return
	}
	for _, link := range links {
		if link.Attrs().Index == 2 {
			fmt.Println(link.Attrs().Name)
			routes, _ := nl.RouteList(link, nl.FAMILY_ALL)
			for _, route := range routes {
				fmt.Println(route)
			}
		}
	}
}
