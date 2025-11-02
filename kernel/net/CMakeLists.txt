set(NET_SOURCE_FILES
	arp.cpp
	checksum.cpp
	ethernet.cpp
	icmp.cpp
	ipv4.cpp
	packet_handler.cpp
)

add_library(UchosNet ${NET_SOURCE_FILES})
