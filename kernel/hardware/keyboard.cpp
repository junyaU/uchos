#include "usb/class_driver/keyboard.hpp"
#include "../system_event_queue.hpp"
#include "keyboad.hpp"

namespace
{
const char keycode_map[256] = {
	0,	  0,	0,	  0,	'a',  'b', 'c', 'd', // 0
	'e',  'f',	'g',  'h',	'i',  'j', 'k', 'l', // 8
	'm',  'n',	'o',  'p',	'q',  'r', 's', 't', // 16
	'u',  'v',	'w',  'x',	'y',  'z', '1', '2', // 24
	'3',  '4',	'5',  '6',	'7',  '8', '9', '0', // 32
	'\n', '\b', 0x08, '\t', ' ',  '-', '=', '[', // 40
	']',  '\\', '#',  ';',	'\'', '`', ',', '.', // 48
	'/',  0,	0,	  0,	0,	  0,   0,	0,	 // 56
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 64
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 72
	0,	  0,	0,	  0,	'/',  '*', '-', '+', // 80
	'\n', '1',	'2',  '3',	'4',  '5', '6', '7', // 88
	'8',  '9',	'0',  '.',	'\\', 0,   0,	'=', // 96
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 104
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 112
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 120
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 128
	0,	  '\\', 0,	  0,	0,	  0,   0,	0,	 // 136
};

const char keycode_map_shifted[256] = {
	0,	  0,	0,	  0,	'A',  'B', 'C', 'D', // 0
	'E',  'F',	'G',  'H',	'I',  'J', 'K', 'L', // 8
	'M',  'N',	'O',  'P',	'Q',  'R', 'S', 'T', // 16
	'U',  'V',	'W',  'X',	'Y',  'Z', '!', '@', // 24
	'#',  '$',	'%',  '^',	'&',  '*', '(', ')', // 32
	'\n', '\b', 0x08, '\t', ' ',  '_', '+', '{', // 40
	'}',  '|',	'~',  ':',	'"',  '~', '<', '>', // 48
	'?',  0,	0,	  0,	0,	  0,   0,	0,	 // 56
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 64
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 72
	0,	  0,	0,	  0,	'/',  '*', '-', '+', // 80
	'\n', '1',	'2',  '3',	'4',  '5', '6', '7', // 88
	'8',  '9',	'0',  '.',	'\\', 0,   0,	'=', // 96
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 104
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 112
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 120
	0,	  0,	0,	  0,	0,	  0,   0,	0,	 // 128
	0,	  '|',	0,	  0,	0,	  0,   0,	0,	 // 136
};
} // namespace

void initialize_keyboard()
{
	usb::keyboard_driver::default_observer = [](uint8_t modifier, uint8_t keycode,
												bool press) {
		const bool shift = (modifier & (L_SHIFT | R_SHIFT)) != 0;
		const char ascii =
				shift ? keycode_map_shifted[keycode] : keycode_map[keycode];

		system_event event{ system_event::KEY_PUSH };
		event.args_.keyboard.press = static_cast<int>(press);
		event.args_.keyboard.keycode = keycode;
		event.args_.keyboard.ascii = ascii;
		event.args_.keyboard.modifier = modifier;

		kevent_queue->queue(event);
	};
}