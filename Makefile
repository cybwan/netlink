#!make

.PHONY: format-c
format-c:
	find . -regex '.*\.\(c\|h\)' -exec clang-format -style=file -i {} \;

.PHONY: gonl-build
gonl-build:
	@CGO_ENABLED=1 go build -v -o ./bin/gonl ./cmd/gonl/*

.PHONY: gonl-run
gonl-run:
	@./bin/gonl

.PHONY: gonl
gonl: gonl-build gonl-run

.PHONY: nlp
nlp:
	cd nlp && make

.PHONY: nlp-run
nlp-run: nlp
	cd nlp && make run

.PHONY: nlp-clean
nlp-clean:
	cd nlp && make distclean


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