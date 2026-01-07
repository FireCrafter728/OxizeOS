static volatile struct limine_framebuffer_request framebuffer_request = {
	.id = LIMINE_FRAMEBUFFER_REQUEST_ID,
	.revision = 0,
	.response = 0
};

extern "C" void kernel_main()
{
	if(framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count == 0) while(1);

	struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
	uint32_t width = fb->width;
	uint32_t height = fb->height;
	uint32_t pitch = fb->pitch;
	uint8_t* fbmem = (uint8_t*)fb->address;

	for(uint32_t y = 0; y < height; y++)
		for(uint32_t x = 0; x < width; x++)
		{
			uint32_t pixel = 0x0030FFFF;
			((uint32_t*)(fbmem + y * pitch))[x] = pixel;
		}

	while(1);
}