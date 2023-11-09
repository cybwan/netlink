#!make

.PHONY: default
default: run

.PHONY: format-c
format-c:
	@find . -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \;

.PHONY: gonl-build
gonl-build: nlp-so
	@CGO_ENABLED=1 go build -v -o ./bin/gonl ./cmd/gonl/*

.PHONY: gonl-run
gonl-run:
	@LD_LIBRARY_PATH=./nlp ./bin/gonl

.PHONY: gonl-clean
gonl-clean:
	@rm -rf ./bin/gonl

.PHONY: gonl
gonl: gonl-build gonl-run

.PHONY: nlp
nlp:
	@cd nlp && make

.PHONY: nlp-so
nlp-so:
	@cd nlp && make nlp.so

.PHONY: nlp-run
nlp-run: nlp
	@cd nlp && make run

.PHONY: nlp-clean
nlp-clean:
	@cd nlp && make distclean

.PHONY: clean
clean: gonl-clean nlp-clean

.PHONY: build
build:
	@rm -rf build && mkdir build && cd build && cmake ../nlp && make

.PHONY: run
run: build
	@cd build && ./nlp

.PHONY: test-trie
test-trie: build
	@cd build && ./trie

.PHONY: net
net:
	ip link add eth0 type veth
	ip link add eth1 type veth
	ip tuntap add tap1 mode tun
	ip tuntap add tap2 mode tun
	ip link add br0 type bridge
	ip link set eth0 master br0
	ip link set eth1 master br0
	ip link set tap1 master br0
	ip link add bond1 type bond miimon 100 mode active-backup
	ip link set tap1 master bond1
	ip link set tap2 master bond1
	ip link add eth3 type veth
	ip link add link eth3 name eth3.2 type vlan id 2
	ip link add link eth3 name eth3.3 type vlan id 3
	ip link add eth4 type veth
	ip link add vx0 type vxlan id 100 local 1.1.1.1 remote 2.2.2.2 dev eth4 dstport 4789
	ip tunnel add sixbone mode sit remote 145.100.1.5 local 145.100.24.181 ttl 255
	ip link set sixbone up
	ip addr add 3FFE:604:6:7::2/126 dev sixbone
	ip route add 3ffe::0/16 dev sixbone
	ip link add ip6tnl6 type ip6tnl external
	ip link set up dev ip6tnl6
	ip -6 route replace 3000::/96 dev ip6tnl6 encap ip6 dst 1000::1:192.168.2.221
	ip route replace 220.0.0.0/24 dev ip6tnl6 encap ip6 dst 1000::1:192.168.2.221