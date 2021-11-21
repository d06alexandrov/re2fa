/*
 * Regular expressions parser's inner functions.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#include <string.h> /* memset() */
#include "parser_inner.h"

/*
 * ASCII table
 *
 *     .0  .1  .2  .3  .4  .5  .6  .7  .8  .9  .A  .B  .C  .D  .E  .F
 *  0. NUL SOH STX ETX EOT ENQ ACK BEL BS  TAB LF  VT  FF  CR  SO  SI
 *  1. DLE DC1 DC2 DC3 DC4 NAK SYN ETB CAN EM  SUB ESC FS  GS  RS  US
 *  2.     !   "   #   $   %   &   '   (   )   *   +   ,   —   .   /
 *  3. 0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
 *  4. @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
 *  5. P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
 *  6. `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
 *  7. p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~   DEL
 */

const enum bs_type backslash_table_re[256] = {
/*	[0 ... 255] = BS_UNKNOWN,*/
	['{'] = BS_CHAR, ['}'] = BS_CHAR, ['['] = BS_CHAR, [']'] = BS_CHAR,
	['('] = BS_CHAR, [')'] = BS_CHAR, ['^'] = BS_CHAR, ['$'] = BS_CHAR,
	['.'] = BS_CHAR, ['|'] = BS_CHAR, ['*'] = BS_CHAR, ['+'] = BS_CHAR,
	['?'] = BS_CHAR, ['\\']= BS_CHAR, ['\'']= BS_CHAR, ['%'] = BS_CHAR,
	['='] = BS_CHAR,

	['a'] = BS_CHAR, ['e'] = BS_CHAR, ['f'] = BS_CHAR, ['n'] = BS_CHAR,
	['r'] = BS_CHAR, ['t'] = BS_CHAR,

/* things above are from practice */
	['/'] = BS_CHAR, ['#'] = BS_CHAR,
/* */

	['x']  = BS_HEX,
	['0']  = BS_OCTET,   ['1'] = BS_OCTET,
	['d']  = BS_CHARSET, ['s'] = BS_CHARSET, ['S'] = BS_CHARSET,
	['w']  = BS_CHARSET, ['W'] = BS_CHARSET,

};

const unsigned char backslash_replace_table_re[256] = {
/*	[0 ... 255] = '\0',*/

	['{'] = '{',
	['}'] = '}',
	['['] = '[',
	[']'] = ']',
	['('] = '(',
	[')'] = ')',
	['^'] = '^',
	['$'] = '$',
	['.'] = '.',
	['|'] = '|',
	['*'] = '*',
	['+'] = '+',
	['?'] = '?',
	['\\']= '\\',
	['\'']= '\'',
	['%'] = '%',
	['='] = '=',

	['a'] = 0x07, ['e'] = 0x1B,
	['f'] = 0x0C, ['n'] = 0x0A, ['r'] = 0x0D,
	['t'] = 0x09,

	['/'] = '/',  ['#'] = '#'


};

void set_charset_bits_re(unsigned char *data, unsigned char cs)
{
	switch (cs) {
	case 'd':	/* digits */
		data[6] |= 0xFF;
		data[7] |= 0x03;
		break;
	case 's':	/* whitespace [\t\n\f\r ] */
		data[1] |= 0x36;
		data[4] |= 0x01;
		break;
	case 'S':
		memset(data, 0xFF, 256/8);
		data[1] ^= 0x36;
		data[4] ^= 0x01;
		break;
	case 'w':
		data[6]  |= 0xFF;
		data[7]  |= 0x03;
		data[8]  |= 0xFE;
		data[9]  |= 0xFF;
		data[10] |= 0xFF;
		data[11] |= 0x87;
		data[12] |= 0xFE;
		data[13] |= 0xFF;
		data[14] |= 0xFF;
		data[15] |= 0x07;
		break;
	case 'W':
		memset(data, 0xFF, 256/8);
		data[6]  ^= 0xFF;
		data[7]  ^= 0x03;
		data[8]  ^= 0xFE;
		data[9]  ^= 0xFF;
		data[10] ^= 0xFF;
		data[11] ^= 0x87;
		data[12] ^= 0xFE;
		data[13] ^= 0xFF;
		data[14] ^= 0xFF;
		data[15] ^= 0x07;
		break;
	};
}

/*
enum bs_type {
	BS_UNKNOWN = 0x00,	not implemented or unrecognized
	BS_CHAR    = 0x01,	equal to some character
	BS_CHARSET = 0x02,	equal to charset
	BS_OCTET   = 0x04,	begin of octet
	BS_HEX     = 0x08	begin of hex
};
*/

const enum bs_type backslash_table_cc[256] = {
/*	[0 ... 255] = 0x00,*/
	[']'] = BS_CHAR,
	['a'] = BS_CHAR, ['b'] = BS_CHAR, ['e'] = BS_CHAR,
	['f'] = BS_CHAR, ['n'] = BS_CHAR, ['r'] = BS_CHAR,
	['t'] = BS_CHAR,
/* things above are from practice */
	['/'] = BS_CHAR, ['&'] = BS_CHAR, ['.'] = BS_CHAR,
	['\\']= BS_CHAR, ['-'] = BS_CHAR,
/**/
	['d'] = BS_CHARSET, ['s'] = BS_CHARSET, ['w'] = BS_CHARSET,
	['0'] = BS_OCTET, ['1'] = BS_OCTET,
	['x'] = BS_HEX
};

const unsigned char backslash_replace_table_cc[256] = {
/*	[0 ... 255] = '\0',*/
	[']'] = ']',
	['a'] = 0x07, ['b'] = 0x08, ['e'] = 0x1B,
	['f'] = 0x0C, ['n'] = 0x0A, ['r'] = 0x0D,
	['t'] = 0x09,
	['/'] = '/',  ['&'] = '&' , ['.'] = '.',
	['\\']= '\\', ['-'] = '-'
};

void set_charset_bits_cc(unsigned char *data, unsigned char cs)
{
	switch (cs) {
	case 'd':	/* digits */
		data[6] |= 0xFF;
		data[7] |= 0x03;
		break;
	case 's':	/* whitespace [\t\n\f\r ] */
		data[1] |= 0x36;
		data[4] |= 0x01;
		break;
	case 'w':
		set_charset_bits_cc(data, 'd');
		data[8]  |= 0xFE;
		data[9]  |= 0xFF;
		data[10] |= 0xFF;
		data[11] |= 0x87;
		data[12] |= 0xFE;
		data[13] |= 0xFF;
		data[14] |= 0xFF;
		data[15] |= 0x07;
		break;
	};
}
