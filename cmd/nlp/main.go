package main

import (
	"fmt"
	"reflect"

	nl "github.com/cybwan/netlink/pkg/netlink"
)

func main() {
	links, err := nl.LinkList()
	if err != nil {
		return
	}
	for _, link := range links {
		lt := reflect.TypeOf(link)
		fmt.Println(lt)
	}
}
