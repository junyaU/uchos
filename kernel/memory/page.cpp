#include "page.hpp"

page pages[MAX_PAGES];

int page_index = 0;
page* alloc_page()
{
	if (page_index >= MAX_PAGES) {
		return nullptr;
	}

	return &pages[page_index++];
}
