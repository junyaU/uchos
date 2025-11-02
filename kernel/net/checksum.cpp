#include "net/checksum.hpp"

namespace kernel::net
{
uint16_t calculate_checksum(const void* data, size_t len)
{
	const uint16_t* ptr = static_cast<const uint16_t*>(data);
	uint32_t sum = 0;

	// 16ビット単位で加算
	for (size_t i = 0; i < len / 2; i++) {
		sum += ptr[i];
	}

	// 奇数バイトの処理
	if (len % 2 != 0) {
		sum += static_cast<const uint8_t*>(data)[len - 1] << 8;
	}

	// キャリーの折り返し
	while (sum >> 16 != 0) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}

	return ~sum;
}
} // namespace kernel::net
