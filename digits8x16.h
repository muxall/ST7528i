#ifndef __DIGITS8x16_H
#define __DIGITS8x16_H

#define dig8x16 &Digits8x16

static const Font_TypeDef Digits8x16 = {
		8,           // Font width
		16,          // Font height
		16,          // Bytes per character
		FONT_H,      // Vertical font scan lines
		45,          // First character: '0'
		57,          // Last character: '9'
		//48,          // Unknown character: '0'
		58,          // Unknown character: 'space'
		{
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1F,0x1F,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // '-'
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // '.'
				0x00,0x00,0x00,0x10,0x10,0x08,0x08,0x04,0x02,0x02,0x01,0x01,0x00,0x00,0x00,0x00, // '/'
				0x7E,0xFF,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0x7E, // 0
				0x08,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x08, // 1
				0x7E,0xFF,0xC0,0xC0,0xC0,0xC0,0xC0,0xFE,0x7F,0x03,0x03,0x03,0x03,0x03,0xFF,0x7E, // 2
				0x7E,0xFF,0xC0,0xC0,0xC0,0xC0,0xC0,0xFC,0xFC,0xC0,0xC0,0xC0,0xC0,0xC0,0xFF,0x7E, // 3
				0x42,0xC3,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0xFE,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x40, // 4
				0x7E,0xFF,0x03,0x03,0x03,0x03,0x03,0x7F,0xFE,0xC0,0xC0,0xC0,0xC0,0xC0,0xFF,0x7E, // 5
				0x7E,0xFF,0x03,0x03,0x03,0x03,0x03,0x7F,0xFF,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0x7E, // 6
				0x7E,0xFF,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x40, // 7
				0x7E,0xFF,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0xFF,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0x7E, // 8
				0x7E,0xFF,0xC3,0xC3,0xC3,0xC3,0xC3,0xFF,0xFE,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0x40, // 9
				0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // space
		}
};

#endif // __DIGITS8x16_H