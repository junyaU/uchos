GRAPHICS_PATH = graphics
OBJS += $(GRAPHICS_PATH)/color.o $(GRAPHICS_PATH)/screen.o $(GRAPHICS_PATH)/hankaku.o $(GRAPHICS_PATH)/font.o $(GRAPHICS_PATH)/system_logger.o

include common.mk

hankaku.bin: hankaku.txt
	go run ../tools/font_to_binary.go -o $@ -i $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@