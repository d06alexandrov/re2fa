/*
 * Regular expressions parser's inner functions.
 *
 * Authors: Dmitriy Alexandrov <d06alexandrov@gmail.com>
 */

#ifndef REFA_PARSER_INNER_H
#define REFA_PARSER_INNER_H

/* types of backslash expressions */
enum bs_type {
	BS_UNKNOWN = 0x00,	/* not implemented or unrecognized */
	BS_CHAR    = 0x01,	/* equal to some character */
	BS_CHARSET = 0x02,	/* equal to charset */
	BS_OCTET   = 0x04,	/* begin of octet */
	BS_HEX     = 0x08	/* begin of hex */
};

/* table to connect \<smth> and meaning in regular expression (NOT in []) */
extern const enum bs_type	backslash_table_re[];
extern const unsigned char	backslash_replace_table_re[];

extern void set_charset_bits_re(unsigned char *, unsigned char);

/* table to connect \<smth> and meaning in charclass */
extern const enum bs_type	backslash_table_cc[];
extern const unsigned char	backslash_replace_table_cc[];

extern void set_charset_bits_cc(unsigned char *, unsigned char);

#endif /* REFA_PARSER_INNER_H */
