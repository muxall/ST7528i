#include "st7528i.h"




// Display image orientation
static uint8_t scr_orientation = SCR_ORIENT_NORMAL;

// Video RAM buffer (160x129x4bit = 10320 bytes)
//static uint8_t vRAM[(SCR_W * SCR_H) >> 1] __attribute__((aligned(4)));
static uint8_t vRAM[(SCR_W * SCR_H) >> 1] __attribute__((aligned(4)));
//uint8_t  *vRAM = (uint8_t *)malloc(SCR_W * ((SCR_H + 7) / 8));

// Look-up table of pixel grayscale level
static const uint32_t GS_LUT[] = {
		0x00000000, // 0 (white)
		0x01000000, // 1
		0x00010000, // 2
		0x01010000, // 3
		0x00000100, // 4
		0x01000100, // 5
		0x00010100, // 6
		0x01010100, // 7
		0x00000001, // 8
		0x01000001, // 9
		0x00010001, // 10
		0x01010001, // 11
		0x00000101, // 12
		0x01000101, // 13
		0x00010101, // 14
		0x01010101  // 15 (black)
};

// Look-up table of masks to clear pixel
static const uint32_t POC_LUT[] = {
		0xFEFEFEFE, // 0
		0xFDFDFDFD, // 1
		0xFBFBFBFB, // 2
		0xF7F7F7F7, // 3
		0xEFEFEFEF, // 4
		0xDFDFDFDF, // 5
		0xBFBFBFBF, // 6
		0x7F7F7F7F  // 7
};

// Look-up table of mask for partial page filing
static const uint32_t LUT_PPM[] = {
		0x00000000, // 0
		0x80808080, // 1
		0xC0C0C0C0, // 2
		0xE0E0E0E0, // 3
		0xF0F0F0F0, // 4
		0xF8F8F8F8, // 5
		0xFCFCFCFC, // 6
		0xFEFEFEFE  // 7
};

// Look-up table of full page by grayscale level
static const uint32_t LUT_SBC[] = {
		0x00000000, // 0 (white)
		0xFF000000, // 1
		0x00FF0000, // 2
		0xFFFF0000, // 3
		0x0000FF00, // 4
		0xFF00FF00, // 5
		0x00FFFF00, // 6
		0xFFFFFF00, // 7
		0x000000FF, // 8
		0xFF0000FF, // 9
		0x00FF00FF, // 10
		0xFFFF00FF, // 11
		0x0000FFFF, // 12
		0xFF00FFFF, // 13
		0x00FFFFFF, // 14
		0xFFFFFFFF  // 15 (black)
};

// Grayscale palette (4 bytes for each level of gray, 4 * 14 bytes total)
static const uint8_t GrayPalette[] = {
		0x06,0x06,0x06,0x06, // level 1
		0x0b,0x0b,0x0b,0x0b, // level 2
		0x10,0x10,0x10,0x10, // level 3
		0x15,0x15,0x15,0x15, // level 4
		0x1a,0x1a,0x1a,0x1a, // level 5
		0x1e,0x1e,0x1e,0x1e, // level 6
		0x23,0x23,0x23,0x23, // level 7
		0x27,0x27,0x27,0x27, // level 8
		0x2b,0x2b,0x2b,0x2b, // level 9
		0x2f,0x2f,0x2f,0x2f, // level 10
		0x32,0x32,0x32,0x32, // level 11
		0x35,0x35,0x35,0x35, // level 12
		0x38,0x38,0x38,0x38, // level 13
		0x3a,0x3a,0x3a,0x3a  // level 14
};


ST7528i::ST7528i(){
	//Do nothing constructor.
}

ST7528i::ST7528i(uint8_t lcd_sda, uint8_t lcd_scl, uint8_t lcd_rst){
	sda = lcd_sda;
	scl = lcd_scl;
	rst = lcd_rst;
	// Foreground color
	lcd_color = 15;
	// Screen dimensions
	scr_width  = SCR_W;
	scr_height = SCR_H;
	
	//Start I2C for LCD
	Wire.begin();
	//Wire.setClock(400000); // choose 400 kHz I2C rate pg80
	Wire.setClock(ST7528i_FREQ_400K); // choose 100 kHz or 400 kHz I2C rate pg80
}


ST7528i::~ST7528i(){
/*     flush();
    if(i2c) {
        i2cRelease(i2c);
        i2c=NULL;
    } */
}


void ST7528i::SendCmd(uint8_t cmd) {
	Wire.beginTransmission(ST7528i_SLAVE_ADDR);
	Wire.write(ST7528i_CMD_WRITE);
	Wire.write(cmd);
	Wire.endTransmission(); 
	Delay_ms(1);
}

void ST7528i::SendCmd(uint8_t cmd1, uint8_t cmd2) {
	Wire.beginTransmission(ST7528i_SLAVE_ADDR);
	Wire.write(ST7528i_CMD_WRITE);
	Wire.write(cmd1);
	//Wire.write(CmdContWrite);
	Wire.write(cmd2);
	Wire.endTransmission();
	Delay_ms(1);
}	
	
void ST7528i::SendData(unsigned char dater) {
  Wire.beginTransmission(ST7528i_SLAVE_ADDR);
  Wire.write(ST7528i_DATA_WRITE);
  Wire.write(dater);
  Wire.endTransmission(); 
  //Delay_ms(1);
}

void ST7528i::SendData4(unsigned char dater1) {
  Wire.beginTransmission(ST7528i_SLAVE_ADDR);
  Wire.write(ST7528i_DATA_WRITE);
  Wire.write(dater1);
  Wire.write(dater1);
  Wire.write(dater1);
  Wire.write(dater1);
  Wire.endTransmission(); 
  //Delay_ms(1);
}

void ST7528i::ClearScreen() {
  //Serial.println("Clear LCD Screene...");
  Clear();					//Clear vRAM buffer.
  int n,i;
  char page=0xB0;         //0xBx Set page. 0xB0 is first page pg28
  for(i=0;i<13;i++){      //100 pixels = 12.5 pages to 
	//Serial.print("ComSend Start: Sending page: ");
	//Serial.println(page,HEX);
	SendCmd(page);
	SendCmd(0x10);      //column address Y9:Y6 0x1x is set colum address MSB 0x10 is first column MSB addr. pg29
	SendCmd(0x00);      //column address Y5:Y2 0x0x is set column addr LSB; 0x01 is first column lsb addr??
	for(n=0;n<160;n++){     // up to 160
	  SendData4(0x00);
	}    
	page++;          //move to next page
  }
	//Serial.println("ClearScreen End.");
}

void ST7528i::ClearPartial(uint8_t X, uint8_t Y, uint8_t W, uint8_t H ){
	uint8_t pX;
	uint8_t pY;
	//uint8_t tmpCh;
	uint8_t bL;

	pY = Y;
	while (pY < Y + H) {
		pX = X;
		while (pX < X + W) {
			bL = 0;
			while (bL < 8) {
				SetPixel(pX,pY + bL,1);
				bL++;
			}
			pX++;
		}
		pY += 8;
	}
}

// Clears the vRAM memory (fill with zeros)
// note: memset() here will be faster, but needs "string.h" include
// void ST7528i::ClearPartial(uint8_t column_x, uint8_t row_y, uint8_t width_x, uint8_t height_y ){
	// uint8_t *ptr = vRAM;
	// uint8_t page;
	// uint8_t startPg = (row_y >> 3);
	// uint8_t endPg = ((row_y + height_y) >> 3);
	// //uint8_t col_lsb = ST7528i_COLM_LSB;
	
	// // Column LSB
	// // if (scr_orientation & (SCR_ORIENT_180 | SCR_ORIENT_CW)) {
		// // // The display controller actually have 132 columns but the display
		// // // itself have only 128, therefore must shift for 4 nonexistent columns
		// // col_lsb = ST7528i_COLM_LSB + 4;
	// // } else {
		// // col_lsb = ST7528i_COLM_LSB;
	// // }
	
	// ptr +=  ((row_y / 8) * (SCR_PAGE_WIDTH * 4)) + (column_x * 4);
	// // Send vRAM to display by pages
	// for(page = startPg; page <= endPg; page++){  
		// SetPage(page);	
		// SetColumn(column_x);
		// //Serial.print("\n page="); Serial.println(page, HEX);
		// //printf("ptr= %p\n", (void *)ptr);
				
		// //Clear one page
		// ClearBuf(ptr, width_x * 4);
		// //Serial.println("Sending Buffer...");
		// // Transmit one page
		// SendBuf(ptr, width_x * 4);

		// // Move vRAM pointer to the next page and increase display page number
		// ptr += SCR_PAGE_WIDTH * 4;
		// //page++;
	// }

	// //FlushPartial(column_x, row_y, width_x, height_y);
// }

// void ST7528i::ClearBuf(uint8_t *pBuf, uint32_t count) {
	// //Serial.println("SendBuff called...");
  // while (count--){

	// *pBuf = 0x00;
	 // //Serial.print("*pBuf = 0x");
	 // //Serial.print(*pBuf,HEX); 
	// // Serial.print("*pBuf = ");
	// // printf("Address of x is %p\n", (void *)pBuf);
	// // Serial.println("");
    // pBuf++;
  // }
// }



/* Sets the Column Address of display RAM from the microprocessor into the column address register.
 Along with the Column Address, the Column Address defines the address of the display RAM 
 to write or read display data. When the microprocessor reads or writes display data
 to or from display RAM, Column Addresses are automatically increased. 
 SEG0-159 Columns  Pg 52*/
void ST7528i::SetColumn(uint8_t column){
	//Serial.print("Setting Column:  ");
	//Serial.println(column);
		// Column address LSB
	SendCmd(ST7528i_COLM_LSB | (column & 0x0f));
	// Column address MSB
	SendCmd(ST7528i_COLM_MSB | (column >> 4));
	
}

/* Sets the line address of display RAM to determine the initial 
display line using 2-byte instruction. The RAM display data is
displayed at the top of row (COM0) of LCD panel.
COM0-COM99 Rows,  */
void ST7528i::SetRow(uint8_t row_y){
	SendCmd(ST7528i_CMD_COM0,(row_y & 0x7f));
}

void ST7528i::SetPage(uint8_t page){
	uint8_t newPage = (ST7528i_FIRST_PAGE | page);
	//Serial.print("Setting Page:  ");
	//Serial.println(newPage, HEX);
	// Page address - 8 rows per page
	SendCmd(newPage);
}

void ST7528i::SetModifyRead(bool state){
	if (state == true){
		SendCmd(ST7528i_CMD_MR_ON);
	} else {
		SendCmd(ST7528i_CMD_MR_OFF);
	}
}


// Send vRAM buffer into display
void ST7528i::FlushPartial(uint8_t column_x, uint8_t row_y, uint8_t width_x, uint8_t height_y ) {
	uint8_t *ptr = vRAM;
	uint8_t page;
	uint8_t startPg = (row_y >> 3);
	uint8_t endPg = ((row_y + height_y) >> 3);
	//uint8_t col_lsb = ST7528i_COLM_LSB;
	
	// Column LSB
	// if (scr_orientation & (SCR_ORIENT_180 | SCR_ORIENT_CW)) {
		// // The display controller actually have 132 columns but the display
		// // itself have only 128, therefore must shift for 4 nonexistent columns
		// col_lsb = ST7528i_COLM_LSB + 4;
	// } else {
		// col_lsb = ST7528i_COLM_LSB;
	// }
	
	ptr +=  ((row_y / 8) * (SCR_PAGE_WIDTH * 4)) + (column_x * 4);
	// Send vRAM to display by pages
	for(page = startPg; page <= endPg; page++){  
		SetPage(page);	
		SetColumn(column_x);
		//Serial.print("\n page="); Serial.println(page, HEX);
		//printf("ptr= %p\n", (void *)ptr);
		// Transmit one page
		SendBuf(ptr, width_x * 4);

		// Move vRAM pointer to the next page and increase display page number
		ptr += SCR_PAGE_WIDTH * 4;
		//page++;
	}
}

	

// Set LCD contrast
// input:
//   res_ratio - internal regulator resistor ratio (one of ST7528_RREG_XX 0-7)
//   lcd_bias - LCD bias (one of ST7528_BIAS_XX 0-7)
//   el_vol - electronic volume [0..63]
void ST7528i::SetContrast(uint8_t res_ratio, uint8_t lcd_bias, uint8_t el_vol) {	
	// Set internal resistance ratio of the regulator resistor
	SendCmd(ST7528i_CMD_RREG | (res_ratio & 0x07));
	// Set LCD bias
	SendCmd(ST7528i_CMD_BIAS | (lcd_bias & 0x07));
	// Electronic volume
	//Set Reference Voltage Select Mode pg60
	//VEV called the voltage of electronic volume = (1-(63-alpha) x 2.1V pg40
	SendCmd(ST7528i_CMD_ELVOL,(el_vol & 0x3f));

}

// Toggle display on/off
// input:
//   disp_state - new display state (SCR_ON or SCR_OFF)
// note: doesn't affect the display memory
void ST7528i::SetDisplayState(uint8_t disp_state) {
	SendCmd(disp_state ? ST7528i_CMD_DISPON : ST7528i_CMD_DISPOFF);
}

// Configure LCD partial display pg37
// input:
//   phy_line - partial display physical starting top line(Row) (COM0) [0..99]
//   log_line - partial display logical starting line [0 .. 100] ??
//   lines_num - number of lines for partial display [16 .. 100]
void ST7528i::SetPartialDisplay(uint8_t phy_line, uint8_t log_line, uint8_t lines_num) {
	// Set initial display line
	SendCmd(ST7528i_CMD_LINE,log_line & 0x7f);
	// Set initial COM0
	SendCmd(ST7528i_CMD_COM0,phy_line & 0x7f);
	// Set partial display duty
	SendCmd(ST7528i_CMD_PD_DUTY,lines_num);
}

// Control display power save mode
// input:
//   pm_state - set new power save display mode (SCR_ON or SCR_OFF)
// note: SCR_OFF puts display to sleep mode, SCR_ON returns to normal mode
void ST7528i::PowerSave(uint8_t pm_state) {
	SendCmd(pm_state ? ST7528i_CMD_PM_OFF : ST7528i_CMD_PM_ON);
}

// Set X coordinate mapping (normal or mirrored)
// input:
//   x_map - new mapping of X coordinate (one of SCR_INVERT_XXX values)
// note: SCR_INVERT_OFF means normal COM scan direction
// note: doesn't affect on display memory contents
void ST7528i::SetXDir(uint8_t x_map) {
	// Configure SEG scan direction
	SendCmd(x_map ? ST7528i_CMD_ADC_OFF : ST7528i_CMD_ADC_ON);
}

// Set Y coordinate mapping (normal or mirrored)
// input:
//   y_map - new mapping of Y coordinate (one of SCR_INVERT_XXX values)
// note: SCR_INVERT_OFF means normal SEG scan direction
// note: it is necessary to rewrite the display data RAM after calling this function
void ST7528i::SetYDir(uint8_t y_map) {
	// Configure COM scan direction
	SendCmd(y_map ? ST7528i_CMD_SHL_OFF : ST7528i_CMD_SHL_ON);
}

/* Set display column:page according to a specified coordinates of pixel
Page Address - Rows
In mode 1
It incorporates 4-bit Page Address register changed by only the “Set Page” 
instruction. Page Address 16 is a special RAM area for the icons and display 
data DB0 is only valid. The page address is set  from 0 to 12, and Page 16 is for Icon page.
Line Address - Rows

Column Address Circuit
When set Column Address MSB / LSB instruction is issued, 8-bit [Y9:Y2] are set and 
lowest 2 bit, Y[1:0] is set to “00”.this address is increased by 1 each a read or write data instruction,

Set Page Address instruction can not be used to set the page address to “16”. Use ICON control register ON/OFF
instruction to set the page address to “16”.

COM0-COM99 Rows, SEG0-159 Columns
	input:
	X, Y - pixel coordinates */
void ST7528i::SetAddr(uint8_t X, uint8_t Y) {
	// Column address LSB
	SendCmd(ST7528i_COLM_LSB | (X & 0x0f));
	// Column address MSB
	SendCmd(ST7528i_COLM_MSB | (X >> 4));
	// Page address - 8 rows per page
	SendCmd(ST7528i_FIRST_PAGE | (Y >> 4));
}

/* Set scroll line
This circuit assigns DDRAM a Line Address corresponding to the first line (COM0) of the display.
 Therefore, by setting Line Address repeatedly, it is possible to realize the screen scrolling 
 and page switching without changing the contents of on-chip RAM.
 The 7-bit Line Address (Rows) register is set from 0 ~ 99,
// input:
//   line - start row number [0..99] */
void ST7528i::SetScrollLine(uint8_t line) {
	// Set initial display line
	SendCmd(ST7528i_CMD_LINE,line & 0x7f);
}

// Set display orientation
// input:
//   orientation - display image rotation (one of SCR_ORIENT_XXX values)
void ST7528i::SetOrientation(uint8_t orientation) {
	// Configure display SEG/COM scan direction
	switch(orientation) {
		case SCR_ORIENT_CW:
			// Clockwise rotation
			scr_width  = SCR_H;
			scr_height = SCR_W;
			SetXDir(SCR_INVERT_OFF);
			SetYDir(SCR_INVERT_OFF);
			break;
		case SCR_ORIENT_CCW:
			// Counter-clockwise rotation
			scr_width  = SCR_H;
			scr_height = SCR_W;
			SetXDir(SCR_INVERT_ON);
			SetYDir(SCR_INVERT_ON);
			break;
		case SCR_ORIENT_180:
			// 180 degree rotation
			scr_width  = SCR_W;
			scr_height = SCR_H;
			SetXDir(SCR_INVERT_OFF);
			SetYDir(SCR_INVERT_ON);
			break;
		default:
			// Normal orientation
			scr_width  = SCR_W;
			scr_height = SCR_H;
			SetXDir(SCR_INVERT_ON);
			SetYDir(SCR_INVERT_OFF);
			break;
	}

	// Store orientation
	scr_orientation = orientation;
}


// Initialize SPI peripheral and ST7528i display
// note: SPI peripheral must be initialized before
void ST7528i::Init(void) {

	// Initial display configuration

	// ICON disable
	SendCmd(ST7528i_CMD_ICON_OFF);

	// Display OFF
	SendCmd(ST7528i_CMD_DISPOFF);

	// Set duty cycle: 100 [16-100 Mode 1]
	SendCmd(ST7528i_CMD_PD_DUTY,100);

	// Set initial COM0: 0
	SendCmd(ST7528i_CMD_COM0,0);

	// Set initial display line: 0
	SendCmd(ST7528i_CMD_LINE,0);

	// Set COM scan direction (SHL bit): normal
	SendCmd(ST7528i_CMD_SHL_ON);

	// Set SEG scan direction (ADC bit): normal
	SendCmd(ST7528i_CMD_ADC_OFF);

	// Enable built-in oscillator circuit
	SendCmd(ST7528i_CMD_OSCON);

	// Set internal resistance ratio of the regulator resistor
	SendCmd(ST7528i_CMD_RREG | ST7528i_RREG_58);

	// Set LCD bias
	SendCmd(ST7528i_CMD_BIAS | ST7528i_BIAS_8);

	// Electronic volume
	//Set Reference Voltage Select Mode pg60
	//VEV called the voltage of electronic volume = (1-(63-alpha) x 2.1V pg40
	SendCmd(ST7528i_CMD_ELVOL,17);

	// Configure FRC/PWM mode
	SendCmd(ST7528i_CMD_FRC_PWM | ST7528i_FRC_4 | ST7528i_PWM_60);

	// DC-DC DC[1:0]=00 booster 3X
	SendCmd(ST7528i_CMD_DCDC | ST7528i_BOOST_3X);
	// Delay 200ms
	Delay_ms(200);

	// Power control (VC=1,VR=0,VF=0): turn on internal voltage converter circuit
	SendCmd(ST7528i_CMD_PWR | ST7528i_PWR_VC);

	// DC-DC DC[1:0]=11 booster 6X
	SendCmd(ST7528i_CMD_DCDC | ST7528i_BOOST_6X);
	// Delay 200ms
	Delay_ms(200);

	// Power control (VC=1,VR=1,VF=0): turn on internal voltage regulator circuit
	SendCmd(ST7528i_CMD_PWR | ST7528i_PWR_VC | ST7528i_PWR_VR);
	// Delay 10ms
	Delay_ms(10);

	// Power control (VC=1,VR=1,VF=1): turn on internal voltage follower circuit
	SendCmd(ST7528i_CMD_PWR | ST7528i_PWR_VC | ST7528i_PWR_VR | ST7528i_PWR_VF);

	// Set EXT=1 mode
	SendCmd(ST7528i_CMD_MODE,ST7528i_MODE_EXT1);

	// Set white mode and 1st frame
	SendCmd(ST7528i_CMD1_W1F,0x00);
	// Set white mode and 2nd frame
	SendCmd(ST7528i_CMD1_W2F,0x00);
	// Set white mode and 3rd frame
	SendCmd(ST7528i_CMD1_W3F,0x00);
	// Set white mode and 4th frame (not used in 3FRC mode)
	SendCmd(ST7528i_CMD1_W4F,0x00);

	//Load grayscale palette
	for (uint8_t i = 0; i < sizeof(GrayPalette); i++) {
		SendCmd(ST7528i_CMD1_GRAYPAL + i,GrayPalette[i]);
	}
	
	// Set dark mode and 1st frame
	SendCmd(ST7528i_CMD1_D1F,0x3c);
	// Set dark mode and 2nd frame
	SendCmd(ST7528i_CMD1_D2F,0x3c);
	// Set dark mode and 3rd frame
	SendCmd(ST7528i_CMD1_D3F,0x3c);
	// Set dark mode and 4th frame (not used in 3FRC mode)
	SendCmd(ST7528i_CMD1_D4F,0x3c);

	// Set EXT=0 mode, booster efficiency level and frame frequency
	SendCmd(ST7528i_CMD_MODE,ST7528i_MODE_EXT0 | ST7528i_MODE_BE1 | ST7528i_FF_77);

	// Configure N-line inversion to reduce display pixels crosstalk
	// Recommended to used the odd number from range of 3..33
	//SendCmd(ST7528i_CMD_NLINE_INV,15);

	// Display ON
	SendCmd(ST7528i_CMD_DISPON);
}

// Draw string
// input:
//   X,Y - top left coordinates of first character
//   str - pointer to zero-terminated string
//   Font - pointer to font
// return: string width in pixels
uint16_t ST7528i::PutStr(uint8_t X, uint8_t Y, const char *str, const Font_TypeDef *Font) {
	uint8_t pX = X;
	uint8_t eX = scr_width - Font->font_Width - 1;

	while (*str) {
		//Serial.print("PutStr *str = ");
		//Serial.println(*str);
		pX += PutChar(pX,Y,*str++,Font);
		if (pX > eX) break;
	}

	return (pX - X);
}

// Draw signed integer value
// input:
//   X,Y - top left coordinates of first symbol
//   num - signed integer value
//   Font - pointer to font
// return: number width in pixels
uint8_t ST7528i::PutInt(uint8_t X, uint8_t Y, int32_t num, const Font_TypeDef *Font) {
	uint8_t str[11]; // 10 chars max for INT32_MIN..INT32_MAX (without sign)
	uint8_t *pStr = str;
	uint8_t pX = X;
	uint8_t neg = 0;

	// String termination character
	*pStr++ = '\0';

	// Convert number to characters
	if (num < 0) {
		neg = 1;
		num *= -1;
	}
	do { *pStr++ = (num % 10) + '0'; } while (num /= 10);
	if (neg) *pStr++ = '-';

	// Draw a number
	while (*--pStr) pX += PutChar(pX,Y,*pStr,Font);

	return (pX - X);
}

// Draw signed integer value with decimal point
// input:
//   X,Y - top left coordinates of first symbol
//   num - unsigned integer value
//   decimals - number of digits after decimal point
//   Font - pointer to font
// return: number width in pixels
uint8_t ST7528i::PutIntF(uint8_t X, uint8_t Y, int32_t num, uint8_t decimals, const Font_TypeDef *Font) {
	uint8_t str[11]; // 10 chars max for INT32_MIN..INT32_MAX (without sign)
	uint8_t *pStr = str;
	uint8_t pX = X;
	uint8_t neg = 0;
	uint8_t strLen = 0;

	// Convert number to characters
	*pStr++ = '\0'; // String termination character
	if (num < 0) {
		neg = 1;
		num *= -1;
	}
	do {
		*pStr++ = (num % 10) + '0';
		strLen++;
	} while (num /= 10);

	// Add leading zeroes
	if (strLen <= decimals) {
		while (strLen <= decimals) {
			*pStr++ = '0';
			strLen++;
		}
	}

	// Minus sign?
	if (neg) {
		*pStr++ = '-';
		strLen++;
	}

	// Draw a number
	while (*--pStr) {
		pX += PutChar(pX,Y,*pStr,Font);
		if (decimals && (--strLen == decimals)) {
			// Draw decimal point
			DrawRect(pX,Y + Font->font_Height - 2,pX + 1,Y + Font->font_Height - 1,lcd_color);
			pX += 3;
		}
	}

	return (pX - X);
}


// Draw monochrome bitmap with lcd_color
// input:
//   X, Y - top left corner coordinates of bitmap
//   W, H - width and height of bitmap in pixels
//   pBMP - pointer to array containing bitmap
// note:
//   each SET bit in the bitmap means a pixel with lcd_color color
//   each RESET bit in the bitmap means no drawing (transparent)
// bitmap coding:
//   1 byte represents 8 vertical pixels
//   scan top->bottom, left->right
//   LSB top
//   bottom bits truncated
void ST7528i::DrawBitmap(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP) {
	uint8_t pX;
	uint8_t pY;
	uint8_t tmpCh;
	uint8_t bL;

	pY = Y;
	while (pY < Y + H) {
		pX = X;
		while (pX < X + W) {
			bL = 0;
			tmpCh = *pBMP++;
			if (tmpCh) {
				while (bL < 8) {
					if (tmpCh & 0x01) SetPixel(pX,pY + bL,lcd_color);
					tmpCh >>= 1;
					if (tmpCh) {
						bL++;
					} else {
						pX++;
						break;
					}
				}
			} else {
				pX++;
			}
		}
		pY += 8;
	}
}

// Draw grayscale bitmap (4-bit)
// input:
//   X, Y - top left corner coordinates of bitmap
//   W, H - width and height of bitmap in pixels
//   pBMP - pointer to array containing bitmap
// note:
//   this function uses SetPixel for drawing, in terms of performance it is a bad idea
//   but such way respects screen rotation
// bitmap coding:
//   1 byte represents 2 horizontal pixels (color coded in byte nibbles)
//   scan left->right, top->bottom
//   LSB left
void ST7528i::DrawBitmapGS(uint8_t X, uint8_t Y, uint8_t W, uint8_t H, const uint8_t* pBMP) {
	uint8_t pX;
	uint8_t pY;
	uint8_t tmpCh;

	pY = Y;
	while (pY < Y + H) {
		pX = X;
		while (pX < X + W) {
			tmpCh = *pBMP++;

			// First nibble
			SetPixel(pX,pY,tmpCh >> 4);
			pX++;

			// Seconds nibble
			if (pX < X + W) {
				SetPixel(pX,pY,tmpCh & 0x0F);
				pX++;
			}
		}
		pY++;
	}
}

// Optimized draw horizontal line
// input:
//   X - horizontal coordinate of the left pixel
//   Y - vertical coordinate of line
//   W - line width (in pixels)
//   GS - grayscale pixel color
void ST7528i::DrawHLineInt(uint8_t X, uint8_t Y, uint8_t W, uint8_t GS) {
	register uint32_t bmap = GS_LUT[GS] << (Y & 0x07);
	register uint32_t bmsk = POC_LUT[Y & 0x07];
	//register uint32_t *ptr = (uint32_t *)&vRAM[((Y >> 3) << 9) + (X << 2)];
	register uint32_t vIdx = ((Y / 8) * (SCR_PAGE_WIDTH * 4)) + (X * 4);
	register uint32_t *ptr = (uint32_t *)&vRAM[vIdx];
	

	do {
		*ptr   &= bmsk;
		*ptr++ |= bmap;
	} while (W--);
}

// Optimized draw vertical line
// input:
//   X - horizontal coordinate of the line
//   Y - vertical coordinate of the top pixel
//   H - line height (in pixels)
//   GS - grayscale pixel color
void ST7528i::DrawVLineInt(uint8_t X, uint8_t Y, uint8_t H, uint8_t GS) {
	register uint32_t bmap = LUT_SBC[GS];
	register uint32_t vofs = (Y & 0x07);
	//register uint32_t *ptr = (uint32_t *)&vRAM[((Y >> 3) << 9) + (X << 2)];
	register uint32_t vIdx = ((Y / 8) * (SCR_PAGE_WIDTH * 4)) + (X * 4);
	register uint32_t *ptr = (uint32_t *)&vRAM[vIdx];

	// If the vertical line is not aligned to the screen page, the drawing function must
	// mask a bits in the bitmap corresponding to the pixels above the top of the line
	// Another PITA is what the end of the line can be at the same page, thus the drawing
	// function must also mask a bits in the bitmap corresponding to the pixels below the line
	if (vofs) {
		vofs = 8 - vofs;

		if (vofs > H) {
			// The line ends at the same page where it begins
			// Mask pixels in the bitmap, apply them to the vRAM and bail out
			*ptr &= ~LUT_PPM[vofs] |  LUT_PPM[vofs - H];
			*ptr |=  LUT_PPM[vofs] & ~LUT_PPM[vofs - H] & bmap;

			return;
		}

		// The line continued at the next page
		*ptr &= ~LUT_PPM[vofs];
		*ptr |=  LUT_PPM[vofs] & bmap;
		ptr  += SCR_PAGE_WIDTH;
		H    -= vofs;
	}

	// Draw part of the line which aligned to the screen pages (8 pixels at once)
	if (H > 7) {
		do {
			*ptr = bmap;
			ptr += SCR_PAGE_WIDTH;
			H -= 8;
		} while (H > 7);
	}

	// If end of the line is not aligned to the screen page mask a bits
	// corresponding to the pixels below the line
	if (H) {
		vofs = LUT_PPM[8 - H];
		*ptr &=  vofs;
		*ptr |= ~vofs & bmap;
	}
}

// Draw horizontal line
// input:
//   X1 - horizontal coordinate of line begin
//   X2 - horizontal coordinate of line end
//   Y - vertical coordinate of line
//   GS - grayscale pixel color
void ST7528i::DrawHLine(uint8_t X1, uint8_t X2, uint8_t Y, uint8_t GS) {
	uint8_t X,W;

	if (X1 > X2) {
		X = X2; W = X1 - X2;
	} else {
		X = X1; W = X2 - X1;
	}

	if (scr_orientation & (SCR_ORIENT_CW | SCR_ORIENT_CCW)) {
		DrawVLineInt(Y,X,W + 1,GS);
	} else {
		DrawHLineInt(X,Y,W,GS);
	}
}

// Draw vertical line
// input:
//   X - horizontal coordinate of line
//   Y1 - vertical coordinate of line begin
//   Y2 - vertical coordinate of line end
//   GS - grayscale pixel color
void ST7528i::DrawVLine(uint8_t X, uint8_t Y1, uint8_t Y2, uint8_t GS) {
	uint8_t Y,H;

	if (Y1 > Y2) {
		Y = Y2; H = Y1 - Y2;
	} else {
		Y = Y1; H = Y2 - Y1;
	}

	if (scr_orientation & (SCR_ORIENT_CW | SCR_ORIENT_CCW)) {
		DrawHLineInt(Y,X,H,GS);
	} else {
		DrawVLineInt(X,Y,H + 1,GS);
	}
}

// Draw rectangle
// input:
//   X1,X2 - horizontal coordinates of the opposite rectangle corners
//   Y1,Y2 - vertical coordinates of the opposite rectangle corners
//   GS - grayscale pixel color
void ST7528i::DrawRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, uint8_t GS) {
	DrawHLine(X1,X2,Y1,GS);
	DrawHLine(X1,X2,Y2,GS);
	DrawVLine(X1,Y1,Y2,GS);
	DrawVLine(X2,Y1,Y2,GS);
}

// Draw filled rectangle
// input:
//   X1,Y1 - top left coordinates
//   X2,Y2 - bottom right coordinates
void ST7528i::FillRect(uint8_t X1, uint8_t Y1, uint8_t X2, uint8_t Y2, uint8_t GS) {
	uint8_t Z,E,T,L;

	// Most optimal way to draw a filled rectangle draw it by optimized
	// vertical line function

	// Calculate coordinates based on screen rotation
	if (scr_orientation & (SCR_ORIENT_CW | SCR_ORIENT_CCW)) {
		if (X1 > X2) {
			T = X2; L = X1 - X2;
		} else {
			T = X1; L = X2 - X1;
		}
		if (Y1 > Y2) {
			Z = Y1; E = Y2;
		} else {
			Z = Y2; E = Y1;
		}
	} else {
		if (Y1 > Y2) {
			T = Y2; L = Y1 - Y2;
		} else {
			T = Y1; L = Y2 - Y1;
		}
		if (X1 > X2) {
			Z = X1; E = X2;
		} else {
			Z = X2; E = X1;
		}
	}
	L++;

	// Fill a rectangle
	do {
		DrawVLineInt(Z,T,L,GS);
	} while (Z-- > E);
}

// Draw circle
// input:
//   Xc,Yc - circle center coordinates
//   R - circle radius
//   GS - grayscale pixel color
void ST7528i::DrawCircle(int16_t Xc, int16_t Yc, uint8_t R, uint8_t GS) {
	int16_t err = 1 - R;
	int16_t dx  = 0;
	int16_t dy  = -2 * R;
	int16_t x   = 0;
	int16_t y   = R;
	// Screen width and height for less calculations
	int16_t sh  = scr_height - 1;
	int16_t sw  = scr_width  - 1;

	// Draw pixels with respect of screen boundaries
	while (x < y) {
		if (err >= 0) {
			dy  += 2;
			err += dy;
			y--;
		}
		dx  += 2;
		err += dx + 1;
		x++;

		// Draw eight pixels of each octant
		if (Xc + x < sw) {
			if (Yc + y < sh) SetPixel(Xc + x,Yc + y,GS);
			if (Yc - y > -1) SetPixel(Xc + x,Yc - y,GS);
		}
		if (Xc - x > -1) {
			if (Yc + y < sh) SetPixel(Xc - x,Yc + y,GS);
			if (Yc - y > -1) SetPixel(Xc - x,Yc - y,GS);
		}
		if (Xc + y < sw) {
			if (Yc + x < sh) SetPixel(Xc + y,Yc + x,GS);
			if (Yc - x > -1) SetPixel(Xc + y,Yc - x,GS);
		}
		if (Xc - y > -1) {
			if (Yc + x < sh) SetPixel(Xc - y,Yc + x,GS);
			if (Yc - x > -1) SetPixel(Xc - y,Yc - x,GS);
		}
	}

	// Vertical and horizontal points
	if (Xc + R < sw) SetPixel(Xc + R,Yc,GS);
	if (Xc - R > -1) SetPixel(Xc - R,Yc,GS);
	if (Yc + R < sh) SetPixel(Xc,Yc + R,GS);
	if (Yc - R > -1) SetPixel(Xc,Yc - R,GS);
}

// Draw ellipse
// input:
//   Xc,Yc - ellipse center coordinates
//   Ra,Rb - horizontal and vertical radiuses
//   GS - grayscale pixel color
void ST7528i::DrawEllipse(uint16_t Xc, uint16_t Yc, uint16_t Ra, uint16_t Rb, uint8_t GS) {
	int16_t x  = 0;
	int16_t y  = Rb;
	int32_t A2 = Ra * Ra;
	int32_t B2 = Rb * Rb;
	int32_t C1 = -((A2 >> 2) + (Ra & 0x01) + B2);
	int32_t C2 = -((B2 >> 2) + (Rb & 0x01) + A2);
	int32_t C3 = -((B2 >> 2) + (Rb & 0x01));
	int32_t t  = -A2 * y;
	int32_t dX = B2 * x * 2;
	int32_t dY = -A2 * y * 2;
	int32_t dXt2 = B2 * 2;
	int32_t dYt2 = A2 * 2;
	// Screen width and height for less calculations
	int16_t sh = scr_height - 1;
	int16_t sw = scr_width  - 1;

	// Draw pixels with respect of screen boundaries
	while ((y >= 0) && (x <= Ra)) {
		if ((Xc + x < sw) && (Yc + y < sh)) {
			SetPixel(Xc + x,Yc + y,GS);
		}
		if (x || y) {
			if ((Xc - x > -1) && (Yc - y > -1)) {
				SetPixel(Xc - x,Yc - y,GS);
			}
		}
		if (x && y) {
			if ((Xc + x < sw) && (Yc - y > - 1)) {
				SetPixel(Xc + x,Yc - y,GS);
			}
			if ((Xc - x > -1) && (Yc + y < sh)) {
				SetPixel(Xc - x,Yc + y,GS);
			}
		}

		if ((t + x*B2 <= C1) || (t + y*A2 <= C3)) {
			dX += dXt2;
			t  += dX;
			x++;
		} else if (t - y*A2 > C2) {
			dY += dYt2;
			t  += dY;
			y--;
		} else {
			dX += dXt2;
			dY += dYt2;
			t  += dX;
			t  += dY;
			x++;
			y--;
		}
	}
}


void ST7528i::SendBuf(uint8_t *pBuf, uint32_t count) {
	//Serial.println("SendBuff called...");
  //Wire.beginTransmission(ST7528i_SLAVE_ADDR);
  //Wire.write(ST7528i_DATA_WRITE);
  while (count--){
    //Wire.write(*pBuf);
	SendData(*pBuf);
	 //Serial.print("*pBuf = 0x");
	 //Serial.print(*pBuf,HEX); 
	// Serial.print("*pBuf = ");
	// printf("Address of x is %p\n", (void *)pBuf);
	// Serial.println("");
    pBuf++;
	//Delay_ms(1);
  }
  //Wire.endTransmission();
}


// Draw a single character
// input:
//   X,Y - character top left corner coordinates
//   Char - character to be drawn
//   Font - pointer to font
// return: character width in pixels
uint8_t ST7528i::PutChar(uint8_t X, uint8_t Y, uint8_t Char, const Font_TypeDef *Font) {
	uint8_t pX;
	uint8_t pY;
	uint8_t tmpCh;
	uint8_t bL;
	const uint8_t *pCh;

	// If the specified character code is out of bounds should substitute the code of the "unknown" character
	if ((Char < Font->font_MinChar) || (Char > Font->font_MaxChar)) Char = Font->font_UnknownChar;
		

	// Pointer to the first byte of character in font data array
	pCh = &Font->font_Data[(Char - Font->font_MinChar) * Font->font_BPC];
	//Serial.print("pCh = 0x");
	//Serial.println(*pCh,HEX);

	// Draw character
	if (Font->font_Scan == FONT_V) {
		// Vertical pixels order
		if (Font->font_Height < 9) {
			// Height is 8 pixels or less (one byte per column)
			pX = X;
			while (pX < X + Font->font_Width) {
				pY = Y;
				tmpCh = *pCh++;
				while (tmpCh) {
					if (tmpCh & 0x01) SetPixel(pX,pY,lcd_color);
					tmpCh >>= 1;
					pY++;
				}
				pX++;
			}
		} else {
			// Height is more than 8 pixels (several bytes per column)
			pX = X;
			while (pX < X + Font->font_Width) {
				pY = Y;
				while (pY < Y + Font->font_Height) {
					bL = 8;
					tmpCh = *pCh++;
					if (tmpCh) {
						while (bL) {
							if (tmpCh & 0x01) SetPixel(pX,pY,lcd_color);
							tmpCh >>= 1;
							if (tmpCh) {
								pY++;
								bL--;
							} else {
								pY += bL;
								break;
							}
						}
					} else {
						pY += bL;
					}
				}
				pX++;
			}
		}
	} else {
		// Horizontal pixels order
		if (Font->font_Width < 9) {
			// Width is 8 pixels or less (one byte per row)
			pY = Y;
			while (pY < Y + Font->font_Height) {
				pX = X;
				tmpCh = *pCh++;
				while (tmpCh) {
					if (tmpCh & 0x01) SetPixel(pX,pY,lcd_color);
					tmpCh >>= 1;
					pX++;
				}
				pY++;
			}
		} else {
			// Width is more than 8 pixels (several bytes per row)
			pY = Y;
			while (pY < Y + Font->font_Height) {
				pX = X;
				while (pX < X + Font->font_Width) {
					bL = 8;
					tmpCh = *pCh++;
					if (tmpCh) {
						while (bL) {
							if (tmpCh & 0x01) SetPixel(pX,pY,lcd_color);
							tmpCh >>= 1;
							if (tmpCh) {
								pX++;
								bL--;
							} else {
								pX += bL;
								break;
							}
						}
					} else {
						pX += bL;
					}
				}
				pY++;
			}
		}
	}

	return Font->font_Width + 1;
}

// Set pixel color in vRAM buffer
// input:
//   X, Y - pixel coordinates
//   GS - grayscale pixel color [0..15]
void ST7528i::SetPixel(uint8_t X, uint8_t Y, uint8_t GS) {
	// Pointer to the pixel byte in video buffer computed by
	// formula ((Y / 8) * (SCR_PAGE_WIDTH * 4)) + (X * 4)
	register uint32_t *ptr;
	// Vertical offset of pixel in display page
	register uint32_t voffs;
	// Bitmap for one pixel (4 consecutive bytes), get it from
	// the look-up table (it is better in terms of performance)
	register uint32_t bmap = GS_LUT[GS];

	// X and Y coordinate values must be swapped if screen rotated either clockwise or counter-clockwise
	
	if (scr_orientation & (SCR_ORIENT_CW | SCR_ORIENT_CCW)) {		
		voffs = X & 0x07;
		uint32_t vIdx = ((X / 8) * (SCR_PAGE_WIDTH * 4)) + (Y * 4);
		// Serial.print("vIdx = 0x");
		// Serial.println(vIdx,HEX);
		//ptr = (uint32_t *)&vRAM[((X >> 3) << 9) + (Y << 2)];
		ptr = (uint32_t *)&vRAM[vIdx];
	} else {
		voffs = Y & 0x07;
		uint32_t vIdx = ((Y / 8) * (SCR_PAGE_WIDTH * 4)) + (X * 4);
		//Serial.print("vIdx = 0x");
		//Serial.println(vIdx,HEX);
		//ptr = (uint32_t *)&vRAM[((Y >> 3) << 9) + (X << 2)];
		ptr = (uint32_t *)&vRAM[vIdx];
	}

	// Vertical shift of the pixel bitmap
	bmap <<= voffs;


	// Clear pixel in vRAM (take mask from look-up table)
	*ptr &= POC_LUT[voffs];

	// Finally write new pixel color in vRAM
	*ptr |= bmap;
}

// Send vRAM buffer into display
void ST7528i::Flush(void) {
	uint8_t *ptr = vRAM;
	char page = ST7528i_FIRST_PAGE;
	//uint8_t col_lsb = ST7528i_COLM_LSB;
	
	// Column LSB
	// if (scr_orientation & (SCR_ORIENT_180 | SCR_ORIENT_CW)) {
		// // The display controller actually have 132 columns but the display
		// // itself have only 128, therefore must shift for 4 nonexistent columns
		// col_lsb = ST7528i_COLM_LSB + 4;
	// } else {
		// col_lsb = ST7528i_COLM_LSB;
	// }
	
	
	// Send vRAM to display by pages
	for (int i = 0; i < ST7528i_MAX_PAGES; i++) {
		SendCmd(page);	
		SendCmd(ST7528i_COLM_MSB);
		SendCmd(ST7528i_COLM_LSB);
		
		//Serial.print("\n page="); Serial.println(page, HEX);
		//printf("ptr= %p\n", (void *)ptr);
		
		// Transmit one page
		SendBuf(ptr, SCR_PAGE_WIDTH * 4);

		// Move vRAM pointer to the next page and increase display page number
		ptr += SCR_PAGE_WIDTH * 4;
		page++;
	}
}

// Clears the vRAM memory (fill with zeros)
// note: memset() here will be faster, but needs "string.h" include
void ST7528i::Clear(void) {
	register uint32_t *ptr = (uint32_t *)vRAM;
	register uint32_t i = sizeof(vRAM) >> 2;

	while (i--) {
		*ptr++ = 0x00000000;
	}
}



void Delay_ms(uint16_t dt){            /*Delay*/
  while(dt-- > 0){
	  delay(1);
  }
  
}
