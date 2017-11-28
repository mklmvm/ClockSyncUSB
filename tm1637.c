/*
 * tm1637.c
 *
 * Created: 27.11.2017 13:27:11
 *  Author: DiGun
 */ 
#include <avr/io.h>
#include <util/delay.h>
#include "tm1637.h"

#define	TM1637_DIO_HIGH()		(TM1637_OUT |= (TM1637_DIO_PIN))
#define	TM1637_DIO_LOW()		(TM1637_OUT &= ~(TM1637_DIO_PIN))
#define	TM1637_DIO_OUTPUT()		(TM1637_DDR |= (TM1637_DIO_PIN))
#define	TM1637_DIO_INPUT()		(TM1637_DDR &= ~(TM1637_DIO_PIN))
#define	TM1637_DIO_READ() 		(((TM1637_IN & (TM1637_DIO_PIN)) > 0) ? 1 : 0)
#define	TM1637_CLK_HIGH()		(TM1637_OUT |= (TM1637_CLK_PIN))
#define	TM1637_CLK_LOW()		(TM1637_OUT &= ~(TM1637_CLK_PIN))

#define	TM1637_FLAG_ENABLED		(1 << 0)
#define	TM1637_FLAG_SHOWCOLON		(1 << 1)


static void TM1637_configure(void);
static void TM1637_cmd(uint8_t value);
static void TM1637_start(void);
static void TM1637_stop(void);
static uint8_t TM1637_write_byte(uint8_t value);

static const uint8_t _digit2segments[] =
{
	0x3F, // 0
	0x06, // 1
	0x5B, // 2
	0x4F, // 3
	0x66, // 4
	0x6D, // 5
	0x7D, // 6
	0x07, // 7
	0x7F, // 8
	0x6F, // 9
	0x77, // A
	0x7C, // b
	0x39, // C
	0x5E, // d
	0x79, // E
	0x71, // F
	
};

static uint8_t _brightness = TM1637_DEFAULT_BRIGHTNESS;
static uint8_t _digit = 0xff;
static uint8_t _flags = 0x00;

void
TM1637_init(void)
{

	TM1637_DDR |= ((TM1637_DIO_PIN)|(TM1637_CLK_PIN));
	TM1637_OUT &= ~((TM1637_DIO_PIN)|(TM1637_CLK_PIN));
	_flags |= TM1637_FLAG_ENABLED;
	TM1637_clear();
}

void
TM1637_display_digit(const uint8_t addr, const uint8_t digit)
{
	uint8_t segments = digit < 10 ? _digit2segments[digit] : 0x00;

	if (addr == TM1637_SET_ADR_01H) {
		_digit = digit;
		if (_flags & TM1637_FLAG_SHOWCOLON) {
			segments |= 0x80;
		}
	}

	TM1637_display_segments(addr, segments);
}

void
TM1637_display_segments(const uint8_t addr, const uint8_t segments)
{

	TM1637_cmd(TM1637_CMD_SET_DATA | TM1637_SET_DATA_F_ADDR);
	TM1637_start();
	TM1637_write_byte(TM1637_CMD_SET_ADDR | addr);
	TM1637_write_byte(segments);
	TM1637_stop();
	TM1637_configure();
}

void
TM1637_display_colon(bool value)
{

	if (value) {
		_flags |= TM1637_FLAG_SHOWCOLON;
	} else {
		_flags &= ~TM1637_FLAG_SHOWCOLON;
	}
	TM1637_display_digit(TM1637_SET_ADR_01H, _digit);
}

void
TM1637_clear(void)
{

	TM1637_display_colon(false);
	TM1637_display_segments(TM1637_SET_ADR_00H, 0x00);
	TM1637_display_segments(TM1637_SET_ADR_01H, 0x00);
	TM1637_display_segments(TM1637_SET_ADR_02H, 0x00);
	TM1637_display_segments(TM1637_SET_ADR_03H, 0x00);
}

void
TM1637_set_brightness(const uint8_t brightness)
{

	_brightness = brightness & 0x07;
	TM1637_configure();
}

void
TM1637_enable(bool value)
{

	if (value) {
		_flags |= TM1637_FLAG_ENABLED;
	} else {
		_flags &= ~TM1637_FLAG_ENABLED;
	}
	TM1637_configure();
}

void
TM1637_configure(void)
{
	uint8_t cmd;

	cmd = TM1637_CMD_SET_DSIPLAY;
	cmd |= _brightness;
	if (_flags & TM1637_FLAG_ENABLED) {
		cmd |= TM1637_SET_DISPLAY_ON;
	}

	TM1637_cmd(cmd);
}

void
TM1637_cmd(uint8_t value)
{

	TM1637_start();
	TM1637_write_byte(value);
	TM1637_stop();
}

void
TM1637_start(void)
{

	TM1637_DIO_HIGH();
	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);
	TM1637_DIO_LOW();
}

void
TM1637_stop(void)
{

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_HIGH();
}

uint8_t
TM1637_write_byte(uint8_t value)
{
	uint8_t i, ack;

	for (i = 0; i < 8; ++i, value >>= 1) {
		TM1637_CLK_LOW();
		_delay_us(TM1637_DELAY_US);

		if (value & 0x01) {
			TM1637_DIO_HIGH();
		} else {
			TM1637_DIO_LOW();
		}

		TM1637_CLK_HIGH();
		_delay_us(TM1637_DELAY_US);
	}

	TM1637_CLK_LOW();
	TM1637_DIO_INPUT();
	TM1637_DIO_HIGH();
	_delay_us(TM1637_DELAY_US);

	ack = TM1637_DIO_READ();
	if (ack) {
		TM1637_DIO_OUTPUT();
		TM1637_DIO_LOW();
	}
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_HIGH();
	_delay_us(TM1637_DELAY_US);

	TM1637_CLK_LOW();
	_delay_us(TM1637_DELAY_US);

	TM1637_DIO_OUTPUT();

	return ack;
}
/*
void set2(uint8_t dig, uint8_t offset)
{
	if (dig<10)
	{
		setDigit(offset--, dig);
		setDigit(offset--, 0);
	}
	else
	{
		setDigit(offset--, dig % 10);
		dig /= 10;
		setDigit(offset--, dig);
	}
}
*/

void TM1637_setTime(uint8_t hour,uint8_t min)
{
	
}