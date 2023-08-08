TARGET = kernel.elf
OBJS = main.o graphics/color.o graphics/screen.o point2d.o graphics/hankaku.o graphics/font.o graphics/system_logger.o newlib_support.o interrupt/handler.o interrupt/idt.o timers/acpi.o timers/local_apic.o asm_utils.o
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

LIBS_PATH = $(HOME)/osbook/devenv/x86_64-elf
EDK2DIR = $(HOME)/edk2

CPPFLAGS += -I. -I$(LIBS_PATH)/include/c++/v1 -I$(LIBS_PATH)/include -I$(LIBS_PATH)/include/freetype2 \
            -I$(EDK2DIR)/MdePkg/Include -I$(EDK2DIR)/MdePkg/Include/X64 \
            -nostdlibinc -D__ELF__ -D_LDBL_EQ_DBL -D_GNU_SOURCE -D_POSIX_TIMERS \
            -DEFIAPI='__attribute__((ms_abi))'
CFLAGS   += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone
CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone \
            -fno-exceptions -fno-rtti -std=c++17
LDFLAGS  += --entry KernelMain -z norelro --image-base 0x100000 --static -z separate-code -L$(LIBS_PATH)/lib

INTERRUPT_PATH = interrupt

INTERRUPT_SRCS = $(wildcard interrupt/*.cpp)
INTERRUPT_OBJS = $(INTERRUPT_SRC:.cpp=.o)

.PHONY: all clean
all: $(TARGET)

clean:
	rm -rf *.o

kernel.elf: $(OBJS)
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc -lc++ -lc++abi -lm -lfreetype

%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

interrupt/%.o: interrupt/%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

graphics/%.o: graphics/%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

times/%.o: times/%.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: interrupt/%.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: graphics/%.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: times/%.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed -I '' -e 's|$(notdir $(OBJ))|$(OBJ)|' $@


%.o: %.asm Makefile
	nasm -f elf64 -o $@ $<

graphics/hankaku.bin: graphics/hankaku.txt
	go run ../tools/font_to_binary.go -o $@ -i $<

graphics/hankaku.o: graphics/hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

graphics/.%.d: graphics/%.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)