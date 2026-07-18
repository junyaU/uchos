#include "tests/test_cases/graphics_test.hpp"
#include "graphics/screen.hpp"
#include "tests/framework.hpp"
#include "tests/macros.hpp"

void test_put_pixel_out_of_bounds_is_ignored()
{
	using kernel::graphics::kscreen;

	ASSERT_NOT_NULL(kscreen);

	// Out-of-range coordinates used to write straight past the frame
	// buffer (issue #313); they must be clipped without touching memory
	kscreen->put_pixel({ -1, 0 }, 0xff0000);
	kscreen->put_pixel({ 0, -1 }, 0xff0000);
	kscreen->put_pixel({ kscreen->width(), 0 }, 0xff0000);
	kscreen->put_pixel({ 0, kscreen->height() }, 0xff0000);
	kscreen->put_pixel({ 1'000'000, 1'000'000 }, 0xff0000);

	// In-range corners are still valid
	kscreen->put_pixel({ 0, 0 }, 0x000000);
	kscreen->put_pixel({ kscreen->width() - 1, kscreen->height() - 1 },
					   0x000000);
}

void register_graphics_tests()
{
	test_register("put_pixel_out_of_bounds_is_ignored",
				  test_put_pixel_out_of_bounds_is_ignored);
}
