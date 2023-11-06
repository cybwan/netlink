CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(libnlp C)

INCLUDE_DIRECTORIES(headers)

ADD_DEFINITIONS(-g -Og -W -Wall -Wextra -Wno-unused-parameter)

STRING(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")

SET(SOURCES
	log/log.c
	netlink/attr.c
	netlink/cache.c
	netlink/cache_mngt.c
	netlink/error.c
	netlink/genl.c
	netlink/genl_ctrl.c
	netlink/genl_family.c
	netlink/genl_mngt.c
	netlink/handlers.c
	netlink/msg.c
	netlink/nl.c
	netlink/object.c
	netlink/socket.c
	netlink/unl.c
	nlp/addr.c
	nlp/filter.c
	nlp/link.c
	nlp/neigh.c
	nlp/net_api.c
	nlp/nlops.c
	nlp/route.c
	lbrt/counter.c
	lbrt/layer2.c
	lbrt/layer3.c
	lbrt/mirror.c
	lbrt/neigh.c
	lbrt/port.c
	lbrt/qospol.c
	lbrt/route.c
	lbrt/rule.c
	lbrt/session.c
	lbrt/vlan.c
	lbrt/zone.c
	lbrt/net.c
)

#ADD_LIBRARY(nlp SHARED ${SOURCES})

ADD_EXECUTABLE(nlp main.c ${SOURCES})