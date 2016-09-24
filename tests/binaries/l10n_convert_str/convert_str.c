/* Copyright 2005  Sony Corporation */
/*
 * SCE CONFIDENTIAL
 * PlayStation(R)3 Programmer Tool Runtime Library 400.001
 * Copyright (C) 2009 Sony Computer Entertainment Inc.
 * All Rights Reserved.
 */
#include	"sample.h"

#include        <sys/process.h>
SYS_PROCESS_PARAM(1001, 0x10000);

int	main( int argc, char *argv[] )
{
    L10nCode	 sc, dc;
    l10n_conv_t	 cd;
    void	*src = NULL, *dst = NULL;
    size_t	 src_len, dst_len;
    L10nResult	 result;
    uint8_t	 utf8[10], eucjp[3], sjis[3], sbcs[3];
    uint16_t	 ucs2[3], utf16[3];
    uint32_t	 utf32[3];
    size_t	 n;

    l10n_sysmodule_init() ;

    /* Argument check */
    if( argc != 3 ){
	puts( "The arguments are insufficient" );
	return( 1 );
    }

    for( n = 0; CodeType[n].name != NULL; n++ ) {
	if( strcmp( argv[1], CodeType[n].name ) == 0 ) {
	    break;
	}
    }
    if( CodeType[n].name == NULL ) {
	puts( "Specified converter name is unknown" );
	return( 1 );
    }
    sc = CodeType[n].code;

    for( n = 0; CodeType[n].name != NULL; n++ ) {
	if( strcmp( argv[2], CodeType[n].name ) == 0 ) {
	    break;
	}
    }
    if( CodeType[n].name == NULL ) {
	puts( "Specified converter name is unknown" );
	return( 1 );
    }
    dc = CodeType[n].code;

    /* Parameter set */
    src_len = 3;
    switch( sc ) {
      case L10N_UTF8:
	utf8[0] = 'a';
	utf8[1] = 'b';
	utf8[2] = 'c';
	src = utf8;
	break;

      case L10N_UCS2:
	ucs2[0] = 'a';
	ucs2[1] = 'b';
	ucs2[2] = 'c';
	src = ucs2;
	break;

      case L10N_UTF16:
	utf16[0] = 'a';
	utf16[1] = 'b';
	utf16[2] = 'c';
	src = utf16;
	break;

      case L10N_UTF32:
	utf32[0] = 'a';
	utf32[1] = 'b';
	utf32[2] = 'c';
	src = utf32;
	break;

      case L10N_CODEPAGE_437:
      case L10N_CODEPAGE_737:
      case L10N_CODEPAGE_775:
      case L10N_CODEPAGE_850:
      case L10N_CODEPAGE_852:
      case L10N_CODEPAGE_855:
      case L10N_CODEPAGE_857:
      case L10N_CODEPAGE_858:
      case L10N_CODEPAGE_860:
      case L10N_CODEPAGE_861:
      case L10N_CODEPAGE_863:
      case L10N_CODEPAGE_865:
      case L10N_CODEPAGE_866:
      case L10N_CODEPAGE_869:
      case L10N_CODEPAGE_1250:
      case L10N_CODEPAGE_1251:
      case L10N_CODEPAGE_1252:
      case L10N_CODEPAGE_1253:
      case L10N_CODEPAGE_1254:
      case L10N_CODEPAGE_1257:
	sbcs[0] = 224;
	sbcs[1] = 225;
	sbcs[2] = 226;
	src = sbcs;
	break;

      case L10N_CODEPAGE_932:
	sjis[0] = 'a';
	sjis[1] = 'b';
	sjis[2] = 'c';
	src = sjis;
	break;

      case L10N_EUC_JP:
	eucjp[0] = 'a';
	eucjp[1] = 'b';
	eucjp[2] = 'c';
	src = eucjp;
	break;

      default:
	puts( "Specified converter name is unknown" );
	return( 1 );
    }

    dst_len = 10;
    switch( dc ) {
      case L10N_UTF8:
	dst = utf8;
	break;

      case L10N_UCS2:
	dst = ucs2;
	break;

      case L10N_UTF16:
        dst = utf16;
        break;

      case L10N_UTF32:
	dst = utf32;
	break;

      case L10N_CODEPAGE_437:
      case L10N_CODEPAGE_850:
      case L10N_CODEPAGE_863:
      case L10N_CODEPAGE_866:
      case L10N_CODEPAGE_1251:
      case L10N_CODEPAGE_1252:
	dst = &sbcs;
	break;

      case L10N_CODEPAGE_932:
	dst = sjis;
	break;

      case L10N_EUC_JP:
	dst = eucjp;
	break;

      default:
	puts( "Specified converter name is unknown" );
	return( 1 );
    }

    if( (cd = l10n_get_converter( sc, dc )) == -1 ) {
	puts( "l10n_get_converter: no such converter" );
	return( 1 );
    }

    /***** l10n_convert_str *****/
    result = l10n_convert_str( cd, src, &src_len, dst, &dst_len );

    if( result != ConversionOK ) {
	printf( "l10n_convert_str: %s\n", Result[result] );
	return( 1 );
    }

    printf( "l10n_convert_str: %s", argv[1] );
    if( sc == L10N_UTF32 ) {
	for( n = 0; n < src_len; n++ )
	    printf( " 0x%08x", ((uint32_t *)src)[n] );
    }
    else if( sc == L10N_UCS2  ||  sc == L10N_UTF16 ) {
	for( n = 0; n < src_len; n++ )
	    printf( " 0x%04x", ((uint16_t *)src)[n] );
    }
    else {
	for( n = 0; n < src_len; n++ )
	    printf( " 0x%02x", ((uint8_t *)src)[n] );
    }

    printf( " => %s", argv[2] );
    if( dc == L10N_UTF32 ) {
	for( n = 0; n < dst_len; n++ )
	    printf( " 0x%08x", ((uint32_t *)dst)[n] );
    }
    else if( dc == L10N_UCS2  ||  dc == L10N_UTF16 ) {
	for( n = 0; n < dst_len; n++ )
	    printf( " 0x%04x", ((uint16_t *)dst)[n] );
    }
    else {
	for( n = 0; n < dst_len; n++ )
	    printf( " 0x%02x", ((uint8_t *)dst)[n] );
    }
    putchar( '\n' );

    l10n_sysmodule_quit() ;

    return( 0 );
}
