#include "base.hpp"

namespace usb
{
class_driver::class_driver(device* dev) : device_(dev) {}

class_driver::~class_driver() {}
} // namespace usb
