/**
 * obex.c
 * GroundLift OBEX implementation.
 *
 * @author Nathan Campos <nathan@innoveworkshop.com>
 */

#include "obex.h"



/**
 * Prints the contents of a header in a human-readable, debug-friendly, way.
 *
 * @param header Header to have its contents printed out.
 */
void obex_print_header(const obex_header_t *header) {
	/* Print table header. */
	printf("HI\tEncoding\tCode\tValue\n");

	/* Print header identifier and its various bits separated. */
	printf("0x%02X\t", header->identifier.id);
	obex_print_header_encoding(header);
	printf("\t0x%02X\t", header->identifier.meaning);

	/* Print the header value. */
	obex_print_header_value(header);

	printf("\n");
}

/**
 * Prints the encoding type of a header in a human-readable way.
 *
 * @param header Header object to be inspected.
 */
void obex_print_header_encoding(const obex_header_t *header) {
	switch (header->identifier.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			printf("UTF-16 (0x%02X)", header->identifier.encoding);
			break;
		case OBEX_HEADER_ENCODING_STRING:
			printf("String (0x%02X)", header->identifier.encoding);
			break;
		case OBEX_HEADER_ENCODING_BYTE:
			printf("Byte (0x%02X)", header->identifier.encoding);
			break;
		case OBEX_HEADER_ENCODING_WORD64:
			printf("4 Bytes (0x%02X)", header->identifier.encoding);
			break;
		default:
			printf("Unknown (0x%02X)", header->identifier.encoding);
			break;
	}
}

/**
 * Prints the value of a header in a human-readable way.
 *
 * @param header Header object to be inspected.
 */
void obex_print_header_value(const obex_header_t *header) {
	switch (header->identifier.encoding) {
		case OBEX_HEADER_ENCODING_UTF16:
			wprintf(L"(%u) L\"%ls\"", header->value.wstring.fhlength,
					header->value.wstring.text);
			break;
		case OBEX_HEADER_ENCODING_STRING:
			printf("(%u) \"%s\"", header->value.string.fhlength,
				   header->value.string.text);
			break;
		case OBEX_HEADER_ENCODING_BYTE:
			printf("0x%02X", header->value.byte);
			break;
		case OBEX_HEADER_ENCODING_WORD64:
			printf("%llu", header->value.word64);
			break;
		default:
			printf("Printing the contents of an unknown header encoding is "
				   "currently not supported.");
			break;
	}
}
