#include "color.hpp"
#include <cstdint>

namespace kernel::graphics {

uint32_t Color::GetCode() const
{
	uint32_t code = 0;
	code += r_;
	code <<= 8;
	code += g_;
	code <<= 8;
	code += b_;
	return code;
}

} // namespace kernel::graphics
