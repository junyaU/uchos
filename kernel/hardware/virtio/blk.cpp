#include "hardware/virtio/blk.hpp"
#include "libs/common/types.hpp"

error_t write_to_blk_device(uint8_t* buffer, uint64_t sector, uint32_t len)
{
	return OK;
}

error_t read_from_blk_device(uint8_t* buffer, uint64_t sector, uint32_t len)
{
	return OK;
}
