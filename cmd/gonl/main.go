package main

import (
	"github.com/cybwan/netlink/pkg/nlp"
)

func main() {
	nlp.PortList()

	wait := make(chan int, 1)
	<-wait
}
