package main

import (
	"github.com/cybwan/netlink/pkg/nlp"
)

func main() {
	nlp.NetlinkMonitor()

	wait := make(chan int, 1)
	<-wait
}
