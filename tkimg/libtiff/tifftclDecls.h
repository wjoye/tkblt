/*
 * tifftclDecls.h --
 *
 *	Declarations of functions in the platform independent public TIFFTCL API.
 *
 */

#ifndef _TIFFTCLDECLS
#define _TIFFTCLDECLS

/*
 * WARNING: The contents of this file is automatically generated by the
 * genStubs.tcl script. Any modifications to the function declarations
 * below should be made in the tifftcl.decls script.
 */

#include <tcl.h>
#include <stdarg.h>

#ifdef TIFFTCLAPI
#   undef TCL_STORAGE_CLASS
#   define TCL_STORAGE_CLASS DLLEXPORT
#else
#   define TIFFTCLAPI extern
#   undef USE_TIFFTCL_STUBS
#   define USE_TIFFTCL_STUBS 1
#endif

EXTERN int Tifftcl_Init(Tcl_Interp *interp);
EXTERN int Tifftcl_SafeInit(Tcl_Interp *interp);
/*
 * The macro INLINE might be defined both in tcl.h and libtiff/port.h,
 * so better prevent a conflict here.
 */
#undef INLINE

#include "../compat/libtiff/libtiff/tiffio.h"
#include "../compat/libtiff/libtiff/tiffiop.h"
#include "../compat/libtiff/libtiff/tif_predict.h"

/* !BEGIN!: Do not edit below this line. */

/*
 * Exported function declarations:
 */

/* 0 */
TIFFTCLAPI const char *	 TIFFGetVersion(void);
/* 1 */
TIFFTCLAPI const TIFFCodec * TIFFFindCODEC(uint16 a);
/* 2 */
TIFFTCLAPI TIFFCodec *	TIFFRegisterCODEC(uint16 a, const char *b,
				TIFFInitMethod c);
/* 3 */
TIFFTCLAPI void		TIFFUnRegisterCODEC(TIFFCodec *a);
/* 4 */
TIFFTCLAPI tdata_t	_TIFFmalloc(tsize_t a);
/* 5 */
TIFFTCLAPI tdata_t	_TIFFrealloc(tdata_t a, tsize_t b);
/* 6 */
TIFFTCLAPI void		_TIFFmemset(tdata_t a, int b, tsize_t c);
/* 7 */
TIFFTCLAPI void		_TIFFmemcpy(tdata_t a, const tdata_t b, tsize_t c);
/* 8 */
TIFFTCLAPI int		_TIFFmemcmp(const tdata_t a, const tdata_t b,
				tsize_t c);
/* 9 */
TIFFTCLAPI void		_TIFFfree(tdata_t a);
/* 10 */
TIFFTCLAPI void		TIFFClose(TIFF *tiffptr);
/* 11 */
TIFFTCLAPI int		TIFFFlush(TIFF *tiffptr);
/* 12 */
TIFFTCLAPI int		TIFFFlushData(TIFF *tiffptr);
/* 13 */
TIFFTCLAPI int		TIFFGetField(TIFF *tiffptr, ttag_t a, ...);
/* 14 */
TIFFTCLAPI int		TIFFVGetField(TIFF *tiffptr, ttag_t a, va_list b);
/* 15 */
TIFFTCLAPI int		TIFFGetFieldDefaulted(TIFF *tiffptr, ttag_t a, ...);
/* 16 */
TIFFTCLAPI int		TIFFVGetFieldDefaulted(TIFF *tiffptr, ttag_t a,
				va_list b);
/* 17 */
TIFFTCLAPI int		TIFFReadDirectory(TIFF *tiffptr);
/* 18 */
TIFFTCLAPI tsize_t	TIFFScanlineSize(TIFF *tiffptr);
/* 19 */
TIFFTCLAPI tsize_t	TIFFRasterScanlineSize(TIFF *tiffptr);
/* 20 */
TIFFTCLAPI tsize_t	TIFFStripSize(TIFF *tiffptr);
/* 21 */
TIFFTCLAPI tsize_t	TIFFVStripSize(TIFF *tiffptr, uint32 a);
/* 22 */
TIFFTCLAPI tsize_t	TIFFTileRowSize(TIFF *tiffptr);
/* 23 */
TIFFTCLAPI tsize_t	TIFFTileSize(TIFF *tiffptr);
/* 24 */
TIFFTCLAPI tsize_t	TIFFVTileSize(TIFF *tiffptr, uint32 a);
/* 25 */
TIFFTCLAPI uint32	TIFFDefaultStripSize(TIFF *tiffptr, uint32 a);
/* 26 */
TIFFTCLAPI void		TIFFDefaultTileSize(TIFF *tiffptr, uint32 *a,
				uint32 *b);
/* 27 */
TIFFTCLAPI int		TIFFFileno(TIFF *tiffptr);
/* 28 */
TIFFTCLAPI int		TIFFGetMode(TIFF *tiffptr);
/* 29 */
TIFFTCLAPI int		TIFFIsTiled(TIFF *tiffptr);
/* 30 */
TIFFTCLAPI int		TIFFIsByteSwapped(TIFF *tiffptr);
/* 31 */
TIFFTCLAPI int		TIFFIsUpSampled(TIFF *tiffptr);
/* 32 */
TIFFTCLAPI int		TIFFIsMSB2LSB(TIFF *tiffptr);
/* 33 */
TIFFTCLAPI uint32	TIFFCurrentRow(TIFF *tiffptr);
/* 34 */
TIFFTCLAPI tdir_t	TIFFCurrentDirectory(TIFF *tiffptr);
/* 35 */
TIFFTCLAPI tdir_t	TIFFNumberOfDirectories(TIFF *tiffptr);
/* 36 */
TIFFTCLAPI uint32	TIFFCurrentDirOffset(TIFF *tiffptr);
/* 37 */
TIFFTCLAPI tstrip_t	TIFFCurrentStrip(TIFF *tiffptr);
/* 38 */
TIFFTCLAPI ttile_t	TIFFCurrentTile(TIFF *tiffptr);
/* 39 */
TIFFTCLAPI int		TIFFReadBufferSetup(TIFF *tiffptr, tdata_t a,
				tsize_t b);
/* 40 */
TIFFTCLAPI int		TIFFWriteBufferSetup(TIFF *tiffptr, tdata_t a,
				tsize_t b);
/* 41 */
TIFFTCLAPI int		TIFFWriteCheck(TIFF *tiffptr, int a, const char *b);
/* 42 */
TIFFTCLAPI int		TIFFCreateDirectory(TIFF *tiffptr);
/* 43 */
TIFFTCLAPI int		TIFFLastDirectory(TIFF *tiffptr);
/* 44 */
TIFFTCLAPI int		TIFFSetDirectory(TIFF *tiffptr, tdir_t a);
/* 45 */
TIFFTCLAPI int		TIFFSetSubDirectory(TIFF *tiffptr, uint32 a);
/* 46 */
TIFFTCLAPI int		TIFFUnlinkDirectory(TIFF *tiffptr, tdir_t a);
/* 47 */
TIFFTCLAPI int		TIFFSetField(TIFF *tiffptr, ttag_t a, ...);
/* 48 */
TIFFTCLAPI int		TIFFVSetField(TIFF *tiffptr, ttag_t a, va_list b);
/* 49 */
TIFFTCLAPI int		TIFFWriteDirectory(TIFF *tiffptr);
/* 50 */
TIFFTCLAPI int		TIFFReassignTagToIgnore(enum TIFFIgnoreSense a,
				int b);
/* 51 */
TIFFTCLAPI void		TIFFPrintDirectory(TIFF *tiffptr, FILE *a, long b);
/* 52 */
TIFFTCLAPI int		TIFFReadScanline(TIFF *tiffptr, tdata_t a, uint32 b,
				tsample_t c);
/* 53 */
TIFFTCLAPI int		TIFFWriteScanline(TIFF *tiffptr, tdata_t a, uint32 b,
				tsample_t c);
/* 54 */
TIFFTCLAPI int		TIFFReadRGBAImage(TIFF *tiffptr, uint32 a, uint32 b,
				uint32 *c, int d);
/* 55 */
TIFFTCLAPI int		TIFFReadRGBAStrip(TIFF *tiffptr, tstrip_t a,
				uint32 *b);
/* 56 */
TIFFTCLAPI int		TIFFReadRGBATile(TIFF *tiffptr, uint32 a, uint32 b,
				uint32 *c);
/* 57 */
TIFFTCLAPI int		TIFFRGBAImageOK(TIFF *tiffptr, char *a);
/* 58 */
TIFFTCLAPI int		TIFFRGBAImageBegin(TIFFRGBAImage *a, TIFF *tiffptr,
				int b, char *c);
/* 59 */
TIFFTCLAPI int		TIFFRGBAImageGet(TIFFRGBAImage *d, uint32 *c,
				uint32 b, uint32 a);
/* 60 */
TIFFTCLAPI void		TIFFRGBAImageEnd(TIFFRGBAImage *a);
/* 61 */
TIFFTCLAPI TIFF *	TIFFOpen(const char *b, const char *a);
/* 62 */
TIFFTCLAPI TIFF *	TIFFFdOpen(int a, const char *b, const char *c);
/* 63 */
TIFFTCLAPI TIFF *	TIFFClientOpen(const char *a, const char *b,
				thandle_t c, TIFFReadWriteProc d,
				TIFFReadWriteProc e, TIFFSeekProc f,
				TIFFCloseProc g, TIFFSizeProc h,
				TIFFMapFileProc i, TIFFUnmapFileProc j);
/* 64 */
TIFFTCLAPI const char *	 TIFFFileName(TIFF *tiffptr);
/* 65 */
TIFFTCLAPI void		TIFFError(const char *a, const char *b, ...);
/* 66 */
TIFFTCLAPI void		TIFFWarning(const char *a, const char *b, ...);
/* 67 */
TIFFTCLAPI TIFFErrorHandler TIFFSetErrorHandler(TIFFErrorHandler a);
/* 68 */
TIFFTCLAPI TIFFErrorHandler TIFFSetWarningHandler(TIFFErrorHandler a);
/* 69 */
TIFFTCLAPI TIFFExtendProc TIFFSetTagExtender(TIFFExtendProc a);
/* 70 */
TIFFTCLAPI ttile_t	TIFFComputeTile(TIFF *tiffptr, uint32 a, uint32 b,
				uint32 c, tsample_t d);
/* 71 */
TIFFTCLAPI int		TIFFCheckTile(TIFF *tiffptr, uint32 d, uint32 c,
				uint32 b, tsample_t a);
/* 72 */
TIFFTCLAPI ttile_t	TIFFNumberOfTiles(TIFF *tiffptr);
/* 73 */
TIFFTCLAPI tsize_t	TIFFReadTile(TIFF *tiffptr, tdata_t a, uint32 b,
				uint32 c, uint32 d, tsample_t e);
/* 74 */
TIFFTCLAPI tsize_t	TIFFWriteTile(TIFF *tiffptr, tdata_t e, uint32 d,
				uint32 c, uint32 b, tsample_t a);
/* 75 */
TIFFTCLAPI tstrip_t	TIFFComputeStrip(TIFF *tiffptr, uint32 a,
				tsample_t b);
/* 76 */
TIFFTCLAPI tstrip_t	TIFFNumberOfStrips(TIFF *tiffptr);
/* 77 */
TIFFTCLAPI tsize_t	TIFFReadEncodedStrip(TIFF *tiffptr, tstrip_t a,
				tdata_t b, tsize_t c);
/* 78 */
TIFFTCLAPI tsize_t	TIFFReadRawStrip(TIFF *tiffptr, tstrip_t a,
				tdata_t b, tsize_t c);
/* 79 */
TIFFTCLAPI tsize_t	TIFFReadEncodedTile(TIFF *tiffptr, ttile_t a,
				tdata_t b, tsize_t c);
/* 80 */
TIFFTCLAPI tsize_t	TIFFReadRawTile(TIFF *tiffptr, ttile_t c, tdata_t b,
				tsize_t a);
/* 81 */
TIFFTCLAPI tsize_t	TIFFWriteEncodedStrip(TIFF *tiffptr, tstrip_t a,
				tdata_t b, tsize_t c);
/* 82 */
TIFFTCLAPI tsize_t	TIFFWriteRawStrip(TIFF *tiffptr, tstrip_t a,
				tdata_t b, tsize_t c);
/* 83 */
TIFFTCLAPI tsize_t	TIFFWriteEncodedTile(TIFF *tiffptr, ttile_t a,
				tdata_t b, tsize_t c);
/* 84 */
TIFFTCLAPI tsize_t	TIFFWriteRawTile(TIFF *tiffptr, ttile_t c, tdata_t b,
				tsize_t a);
/* 85 */
TIFFTCLAPI void		TIFFSetWriteOffset(TIFF *tiffptr, toff_t a);
/* 86 */
TIFFTCLAPI void		TIFFSwabShort(uint16 *a);
/* 87 */
TIFFTCLAPI void		TIFFSwabLong(uint32 *a);
/* 88 */
TIFFTCLAPI void		TIFFSwabDouble(double *a);
/* 89 */
TIFFTCLAPI void		TIFFSwabArrayOfShort(uint16 *a, unsigned long b);
/* 90 */
TIFFTCLAPI void		TIFFSwabArrayOfLong(uint32 *b, unsigned long a);
/* 91 */
TIFFTCLAPI void		TIFFSwabArrayOfDouble(double *a, unsigned long b);
/* 92 */
TIFFTCLAPI void		TIFFReverseBits(unsigned char *a, unsigned long b);
/* 93 */
TIFFTCLAPI const unsigned char * TIFFGetBitRevTable(int a);
/* Slot 94 is reserved */
/* Slot 95 is reserved */
/* Slot 96 is reserved */
/* Slot 97 is reserved */
/* Slot 98 is reserved */
/* Slot 99 is reserved */
/* 100 */
TIFFTCLAPI int		TIFFPredictorInit(TIFF *tiffptr);
/* Slot 101 is reserved */
/* Slot 102 is reserved */
/* Slot 103 is reserved */
/* Slot 104 is reserved */
/* Slot 105 is reserved */
/* Slot 106 is reserved */
/* Slot 107 is reserved */
/* Slot 108 is reserved */
/* Slot 109 is reserved */
/* 110 */
TIFFTCLAPI void		_TIFFSetupFieldInfo(TIFF *tiffptr,
				const TIFFFieldInfo a[], size_t b);
/* 111 */
TIFFTCLAPI int		_TIFFMergeFieldInfo(TIFF *tiffptr,
				const TIFFFieldInfo *a, int b);
/* 112 */
TIFFTCLAPI void		_TIFFPrintFieldInfo(TIFF *tiffptr, FILE *a);
/* 113 */
TIFFTCLAPI const TIFFFieldInfo * TIFFFindFieldInfo(TIFF *tiffptr, ttag_t a,
				TIFFDataType b);
/* 114 */
TIFFTCLAPI const TIFFFieldInfo * TIFFFieldWithTag(TIFF *tiffptr, ttag_t a);
/* 115 */
TIFFTCLAPI TIFFDataType	 _TIFFSampleToTagType(TIFF *tiffptr);
/* Slot 116 is reserved */
/* Slot 117 is reserved */
/* Slot 118 is reserved */
/* Slot 119 is reserved */
/* 120 */
TIFFTCLAPI int		_TIFFgetMode(const char *a, const char *b);
/* 121 */
TIFFTCLAPI int		_TIFFNoRowEncode(TIFF *tiffptr, tidata_t a,
				tsize_t b, tsample_t c);
/* 122 */
TIFFTCLAPI int		_TIFFNoStripEncode(TIFF *tiffptr, tidata_t c,
				tsize_t b, tsample_t a);
/* 123 */
TIFFTCLAPI int		_TIFFNoTileEncode(TIFF *tiffptr, tidata_t a,
				tsize_t b, tsample_t c);
/* 124 */
TIFFTCLAPI int		_TIFFNoRowDecode(TIFF *tiffptr, tidata_t c,
				tsize_t b, tsample_t a);
/* 125 */
TIFFTCLAPI int		_TIFFNoStripDecode(TIFF *tiffptr, tidata_t a,
				tsize_t b, tsample_t c);
/* 126 */
TIFFTCLAPI int		_TIFFNoTileDecode(TIFF *tiffptr, tidata_t c,
				tsize_t b, tsample_t a);
/* 127 */
TIFFTCLAPI void		_TIFFNoPostDecode(TIFF *tiffptr, tidata_t a,
				tsize_t b);
/* 128 */
TIFFTCLAPI int		_TIFFNoPreCode(TIFF *tiffptr, tsample_t a);
/* 129 */
TIFFTCLAPI int		_TIFFNoSeek(TIFF *tiffptr, uint32 a);
/* 130 */
TIFFTCLAPI void		_TIFFSwab16BitData(TIFF *tiffptr, tidata_t a,
				tsize_t b);
/* 131 */
TIFFTCLAPI void		_TIFFSwab32BitData(TIFF *tiffptr, tidata_t b,
				tsize_t a);
/* 132 */
TIFFTCLAPI void		_TIFFSwab64BitData(TIFF *tiffptr, tidata_t a,
				tsize_t b);
/* 133 */
TIFFTCLAPI int		TIFFFlushData1(TIFF *tiffptr);
/* 134 */
TIFFTCLAPI void		TIFFFreeDirectory(TIFF *tiffptr);
/* 135 */
TIFFTCLAPI int		TIFFDefaultDirectory(TIFF *tiffptr);
/* 136 */
TIFFTCLAPI int		TIFFSetCompressionScheme(TIFF *tiffptr, int a);
/* 137 */
TIFFTCLAPI void		_TIFFSetDefaultCompressionState(TIFF *tiffptr);
/* 138 */
TIFFTCLAPI uint32	_TIFFDefaultStripSize(TIFF *tiffptr, uint32 a);
/* 139 */
TIFFTCLAPI void		_TIFFDefaultTileSize(TIFF *tiffptr, uint32 *a,
				uint32 *b);
/* 140 */
TIFFTCLAPI void		_TIFFsetByteArray(void **a, void *b, uint32 c);
/* 141 */
TIFFTCLAPI void		_TIFFsetString(char **a, char *b);
/* 142 */
TIFFTCLAPI void		_TIFFsetShortArray(uint16 **a, uint16 *b, uint32 c);
/* 143 */
TIFFTCLAPI void		_TIFFsetLongArray(uint32 **a, uint32 *b, uint32 c);
/* 144 */
TIFFTCLAPI void		_TIFFsetFloatArray(float **a, float *b, uint32 c);
/* 145 */
TIFFTCLAPI void		_TIFFsetDoubleArray(double **a, double *b, uint32 c);
/* 146 */
TIFFTCLAPI void		_TIFFprintAscii(FILE *a, const char *b);
/* 147 */
TIFFTCLAPI void		_TIFFprintAsciiTag(FILE *a, const char *b,
				const char *c);
/* 148 */
TIFFTCLAPI int		TIFFInitDumpMode(TIFF *tiffptr, int a);
/* 149 */
TIFFTCLAPI int		TIFFInitPackBits(TIFF *tiffptr, int a);
/* 150 */
TIFFTCLAPI int		TIFFInitCCITTRLE(TIFF *tiffptr, int a);
/* 151 */
TIFFTCLAPI int		TIFFInitCCITTRLEW(TIFF *tiffptr, int a);
/* 152 */
TIFFTCLAPI int		TIFFInitCCITTFax3(TIFF *tiffptr, int a);
/* 153 */
TIFFTCLAPI int		TIFFInitCCITTFax4(TIFF *tiffptr, int a);
/* 154 */
TIFFTCLAPI int		TIFFInitThunderScan(TIFF *tiffptr, int a);
/* 155 */
TIFFTCLAPI int		TIFFInitNeXT(TIFF *tiffptr, int a);
/* 156 */
TIFFTCLAPI int		TIFFInitLZW(TIFF *tiffptr, int a);
/* 157 */
TIFFTCLAPI int		TIFFInitOJPEG(TIFF *tiffptr, int a);
/* 158 */
TIFFTCLAPI int		TIFFInitJPEG(TIFF *tiffptr, int a);
/* 159 */
TIFFTCLAPI int		TIFFInitJBIG(TIFF *tiffptr, int a);
/* 160 */
TIFFTCLAPI int		TIFFInitZIP(TIFF *tiffptr, int a);
/* 161 */
TIFFTCLAPI int		TIFFInitPixarLog(TIFF *tiffptr, int a);
/* 162 */
TIFFTCLAPI int		TIFFInitSGILog(TIFF *tiffptr, int a);

typedef struct TifftclStubs {
    int magic;
    const struct TifftclStubHooks *hooks;

    const char * (*tIFFGetVersion) (void); /* 0 */
    const TIFFCodec * (*tIFFFindCODEC) (uint16 a); /* 1 */
    TIFFCodec * (*tIFFRegisterCODEC) (uint16 a, const char *b, TIFFInitMethod c); /* 2 */
    void (*tIFFUnRegisterCODEC) (TIFFCodec *a); /* 3 */
    tdata_t (*_TIFFmallocPtr) (tsize_t a); /* 4 */
    tdata_t (*_TIFFreallocPtr) (tdata_t a, tsize_t b); /* 5 */
    void (*_TIFFmemsetPtr) (tdata_t a, int b, tsize_t c); /* 6 */
    void (*_TIFFmemcpyPtr) (tdata_t a, const tdata_t b, tsize_t c); /* 7 */
    int (*_TIFFmemcmpPtr) (const tdata_t a, const tdata_t b, tsize_t c); /* 8 */
    void (*_TIFFfreePtr) (tdata_t a); /* 9 */
    void (*tIFFClose) (TIFF *tiffptr); /* 10 */
    int (*tIFFFlush) (TIFF *tiffptr); /* 11 */
    int (*tIFFFlushData) (TIFF *tiffptr); /* 12 */
    int (*tIFFGetField) (TIFF *tiffptr, ttag_t a, ...); /* 13 */
    int (*tIFFVGetField) (TIFF *tiffptr, ttag_t a, va_list b); /* 14 */
    int (*tIFFGetFieldDefaulted) (TIFF *tiffptr, ttag_t a, ...); /* 15 */
    int (*tIFFVGetFieldDefaulted) (TIFF *tiffptr, ttag_t a, va_list b); /* 16 */
    int (*tIFFReadDirectory) (TIFF *tiffptr); /* 17 */
    tsize_t (*tIFFScanlineSize) (TIFF *tiffptr); /* 18 */
    tsize_t (*tIFFRasterScanlineSize) (TIFF *tiffptr); /* 19 */
    tsize_t (*tIFFStripSize) (TIFF *tiffptr); /* 20 */
    tsize_t (*tIFFVStripSize) (TIFF *tiffptr, uint32 a); /* 21 */
    tsize_t (*tIFFTileRowSize) (TIFF *tiffptr); /* 22 */
    tsize_t (*tIFFTileSize) (TIFF *tiffptr); /* 23 */
    tsize_t (*tIFFVTileSize) (TIFF *tiffptr, uint32 a); /* 24 */
    uint32 (*tIFFDefaultStripSize) (TIFF *tiffptr, uint32 a); /* 25 */
    void (*tIFFDefaultTileSize) (TIFF *tiffptr, uint32 *a, uint32 *b); /* 26 */
    int (*tIFFFileno) (TIFF *tiffptr); /* 27 */
    int (*tIFFGetMode) (TIFF *tiffptr); /* 28 */
    int (*tIFFIsTiled) (TIFF *tiffptr); /* 29 */
    int (*tIFFIsByteSwapped) (TIFF *tiffptr); /* 30 */
    int (*tIFFIsUpSampled) (TIFF *tiffptr); /* 31 */
    int (*tIFFIsMSB2LSB) (TIFF *tiffptr); /* 32 */
    uint32 (*tIFFCurrentRow) (TIFF *tiffptr); /* 33 */
    tdir_t (*tIFFCurrentDirectory) (TIFF *tiffptr); /* 34 */
    tdir_t (*tIFFNumberOfDirectories) (TIFF *tiffptr); /* 35 */
    uint32 (*tIFFCurrentDirOffset) (TIFF *tiffptr); /* 36 */
    tstrip_t (*tIFFCurrentStrip) (TIFF *tiffptr); /* 37 */
    ttile_t (*tIFFCurrentTile) (TIFF *tiffptr); /* 38 */
    int (*tIFFReadBufferSetup) (TIFF *tiffptr, tdata_t a, tsize_t b); /* 39 */
    int (*tIFFWriteBufferSetup) (TIFF *tiffptr, tdata_t a, tsize_t b); /* 40 */
    int (*tIFFWriteCheck) (TIFF *tiffptr, int a, const char *b); /* 41 */
    int (*tIFFCreateDirectory) (TIFF *tiffptr); /* 42 */
    int (*tIFFLastDirectory) (TIFF *tiffptr); /* 43 */
    int (*tIFFSetDirectory) (TIFF *tiffptr, tdir_t a); /* 44 */
    int (*tIFFSetSubDirectory) (TIFF *tiffptr, uint32 a); /* 45 */
    int (*tIFFUnlinkDirectory) (TIFF *tiffptr, tdir_t a); /* 46 */
    int (*tIFFSetField) (TIFF *tiffptr, ttag_t a, ...); /* 47 */
    int (*tIFFVSetField) (TIFF *tiffptr, ttag_t a, va_list b); /* 48 */
    int (*tIFFWriteDirectory) (TIFF *tiffptr); /* 49 */
    int (*tIFFReassignTagToIgnore) (enum TIFFIgnoreSense a, int b); /* 50 */
    void (*tIFFPrintDirectory) (TIFF *tiffptr, FILE *a, long b); /* 51 */
    int (*tIFFReadScanline) (TIFF *tiffptr, tdata_t a, uint32 b, tsample_t c); /* 52 */
    int (*tIFFWriteScanline) (TIFF *tiffptr, tdata_t a, uint32 b, tsample_t c); /* 53 */
    int (*tIFFReadRGBAImage) (TIFF *tiffptr, uint32 a, uint32 b, uint32 *c, int d); /* 54 */
    int (*tIFFReadRGBAStrip) (TIFF *tiffptr, tstrip_t a, uint32 *b); /* 55 */
    int (*tIFFReadRGBATile) (TIFF *tiffptr, uint32 a, uint32 b, uint32 *c); /* 56 */
    int (*tIFFRGBAImageOK) (TIFF *tiffptr, char *a); /* 57 */
    int (*tIFFRGBAImageBegin) (TIFFRGBAImage *a, TIFF *tiffptr, int b, char *c); /* 58 */
    int (*tIFFRGBAImageGet) (TIFFRGBAImage *d, uint32 *c, uint32 b, uint32 a); /* 59 */
    void (*tIFFRGBAImageEnd) (TIFFRGBAImage *a); /* 60 */
    TIFF * (*tIFFOpen) (const char *b, const char *a); /* 61 */
    TIFF * (*tIFFFdOpen) (int a, const char *b, const char *c); /* 62 */
    TIFF * (*tIFFClientOpen) (const char *a, const char *b, thandle_t c, TIFFReadWriteProc d, TIFFReadWriteProc e, TIFFSeekProc f, TIFFCloseProc g, TIFFSizeProc h, TIFFMapFileProc i, TIFFUnmapFileProc j); /* 63 */
    const char * (*tIFFFileName) (TIFF *tiffptr); /* 64 */
    void (*tIFFError) (const char *a, const char *b, ...); /* 65 */
    void (*tIFFWarning) (const char *a, const char *b, ...); /* 66 */
    TIFFErrorHandler (*tIFFSetErrorHandler) (TIFFErrorHandler a); /* 67 */
    TIFFErrorHandler (*tIFFSetWarningHandler) (TIFFErrorHandler a); /* 68 */
    TIFFExtendProc (*tIFFSetTagExtender) (TIFFExtendProc a); /* 69 */
    ttile_t (*tIFFComputeTile) (TIFF *tiffptr, uint32 a, uint32 b, uint32 c, tsample_t d); /* 70 */
    int (*tIFFCheckTile) (TIFF *tiffptr, uint32 d, uint32 c, uint32 b, tsample_t a); /* 71 */
    ttile_t (*tIFFNumberOfTiles) (TIFF *tiffptr); /* 72 */
    tsize_t (*tIFFReadTile) (TIFF *tiffptr, tdata_t a, uint32 b, uint32 c, uint32 d, tsample_t e); /* 73 */
    tsize_t (*tIFFWriteTile) (TIFF *tiffptr, tdata_t e, uint32 d, uint32 c, uint32 b, tsample_t a); /* 74 */
    tstrip_t (*tIFFComputeStrip) (TIFF *tiffptr, uint32 a, tsample_t b); /* 75 */
    tstrip_t (*tIFFNumberOfStrips) (TIFF *tiffptr); /* 76 */
    tsize_t (*tIFFReadEncodedStrip) (TIFF *tiffptr, tstrip_t a, tdata_t b, tsize_t c); /* 77 */
    tsize_t (*tIFFReadRawStrip) (TIFF *tiffptr, tstrip_t a, tdata_t b, tsize_t c); /* 78 */
    tsize_t (*tIFFReadEncodedTile) (TIFF *tiffptr, ttile_t a, tdata_t b, tsize_t c); /* 79 */
    tsize_t (*tIFFReadRawTile) (TIFF *tiffptr, ttile_t c, tdata_t b, tsize_t a); /* 80 */
    tsize_t (*tIFFWriteEncodedStrip) (TIFF *tiffptr, tstrip_t a, tdata_t b, tsize_t c); /* 81 */
    tsize_t (*tIFFWriteRawStrip) (TIFF *tiffptr, tstrip_t a, tdata_t b, tsize_t c); /* 82 */
    tsize_t (*tIFFWriteEncodedTile) (TIFF *tiffptr, ttile_t a, tdata_t b, tsize_t c); /* 83 */
    tsize_t (*tIFFWriteRawTile) (TIFF *tiffptr, ttile_t c, tdata_t b, tsize_t a); /* 84 */
    void (*tIFFSetWriteOffset) (TIFF *tiffptr, toff_t a); /* 85 */
    void (*tIFFSwabShort) (uint16 *a); /* 86 */
    void (*tIFFSwabLong) (uint32 *a); /* 87 */
    void (*tIFFSwabDouble) (double *a); /* 88 */
    void (*tIFFSwabArrayOfShort) (uint16 *a, unsigned long b); /* 89 */
    void (*tIFFSwabArrayOfLong) (uint32 *b, unsigned long a); /* 90 */
    void (*tIFFSwabArrayOfDouble) (double *a, unsigned long b); /* 91 */
    void (*tIFFReverseBits) (unsigned char *a, unsigned long b); /* 92 */
    const unsigned char * (*tIFFGetBitRevTable) (int a); /* 93 */
    void (*reserved94)(void);
    void (*reserved95)(void);
    void (*reserved96)(void);
    void (*reserved97)(void);
    void (*reserved98)(void);
    void (*reserved99)(void);
    int (*tIFFPredictorInit) (TIFF *tiffptr); /* 100 */
    void (*reserved101)(void);
    void (*reserved102)(void);
    void (*reserved103)(void);
    void (*reserved104)(void);
    void (*reserved105)(void);
    void (*reserved106)(void);
    void (*reserved107)(void);
    void (*reserved108)(void);
    void (*reserved109)(void);
    void (*_TIFFSetupFieldInfoPtr) (TIFF *tiffptr, const TIFFFieldInfo a[], size_t b); /* 110 */
    int (*_TIFFMergeFieldInfoPtr) (TIFF *tiffptr, const TIFFFieldInfo *a, int b); /* 111 */
    void (*_TIFFPrintFieldInfoPtr) (TIFF *tiffptr, FILE *a); /* 112 */
    const TIFFFieldInfo * (*tIFFFindFieldInfo) (TIFF *tiffptr, ttag_t a, TIFFDataType b); /* 113 */
    const TIFFFieldInfo * (*tIFFFieldWithTag) (TIFF *tiffptr, ttag_t a); /* 114 */
    TIFFDataType (*_TIFFSampleToTagTypePtr) (TIFF *tiffptr); /* 115 */
    void (*reserved116)(void);
    void (*reserved117)(void);
    void (*reserved118)(void);
    void (*reserved119)(void);
    int (*_TIFFgetModePtr) (const char *a, const char *b); /* 120 */
    int (*_TIFFNoRowEncodePtr) (TIFF *tiffptr, tidata_t a, tsize_t b, tsample_t c); /* 121 */
    int (*_TIFFNoStripEncodePtr) (TIFF *tiffptr, tidata_t c, tsize_t b, tsample_t a); /* 122 */
    int (*_TIFFNoTileEncodePtr) (TIFF *tiffptr, tidata_t a, tsize_t b, tsample_t c); /* 123 */
    int (*_TIFFNoRowDecodePtr) (TIFF *tiffptr, tidata_t c, tsize_t b, tsample_t a); /* 124 */
    int (*_TIFFNoStripDecodePtr) (TIFF *tiffptr, tidata_t a, tsize_t b, tsample_t c); /* 125 */
    int (*_TIFFNoTileDecodePtr) (TIFF *tiffptr, tidata_t c, tsize_t b, tsample_t a); /* 126 */
    void (*_TIFFNoPostDecodePtr) (TIFF *tiffptr, tidata_t a, tsize_t b); /* 127 */
    int (*_TIFFNoPreCodePtr) (TIFF *tiffptr, tsample_t a); /* 128 */
    int (*_TIFFNoSeekPtr) (TIFF *tiffptr, uint32 a); /* 129 */
    void (*_TIFFSwab16BitDataPtr) (TIFF *tiffptr, tidata_t a, tsize_t b); /* 130 */
    void (*_TIFFSwab32BitDataPtr) (TIFF *tiffptr, tidata_t b, tsize_t a); /* 131 */
    void (*_TIFFSwab64BitDataPtr) (TIFF *tiffptr, tidata_t a, tsize_t b); /* 132 */
    int (*tIFFFlushData1) (TIFF *tiffptr); /* 133 */
    void (*tIFFFreeDirectory) (TIFF *tiffptr); /* 134 */
    int (*tIFFDefaultDirectory) (TIFF *tiffptr); /* 135 */
    int (*tIFFSetCompressionScheme) (TIFF *tiffptr, int a); /* 136 */
    void (*_TIFFSetDefaultCompressionStatePtr) (TIFF *tiffptr); /* 137 */
    uint32 (*_TIFFDefaultStripSizePtr) (TIFF *tiffptr, uint32 a); /* 138 */
    void (*_TIFFDefaultTileSizePtr) (TIFF *tiffptr, uint32 *a, uint32 *b); /* 139 */
    void (*_TIFFsetByteArrayPtr) (void **a, void *b, uint32 c); /* 140 */
    void (*_TIFFsetStringPtr) (char **a, char *b); /* 141 */
    void (*_TIFFsetShortArrayPtr) (uint16 **a, uint16 *b, uint32 c); /* 142 */
    void (*_TIFFsetLongArrayPtr) (uint32 **a, uint32 *b, uint32 c); /* 143 */
    void (*_TIFFsetFloatArrayPtr) (float **a, float *b, uint32 c); /* 144 */
    void (*_TIFFsetDoubleArrayPtr) (double **a, double *b, uint32 c); /* 145 */
    void (*_TIFFprintAsciiPtr) (FILE *a, const char *b); /* 146 */
    void (*_TIFFprintAsciiTagPtr) (FILE *a, const char *b, const char *c); /* 147 */
    int (*tIFFInitDumpMode) (TIFF *tiffptr, int a); /* 148 */
    int (*tIFFInitPackBits) (TIFF *tiffptr, int a); /* 149 */
    int (*tIFFInitCCITTRLE) (TIFF *tiffptr, int a); /* 150 */
    int (*tIFFInitCCITTRLEW) (TIFF *tiffptr, int a); /* 151 */
    int (*tIFFInitCCITTFax3) (TIFF *tiffptr, int a); /* 152 */
    int (*tIFFInitCCITTFax4) (TIFF *tiffptr, int a); /* 153 */
    int (*tIFFInitThunderScan) (TIFF *tiffptr, int a); /* 154 */
    int (*tIFFInitNeXT) (TIFF *tiffptr, int a); /* 155 */
    int (*tIFFInitLZW) (TIFF *tiffptr, int a); /* 156 */
    int (*tIFFInitOJPEG) (TIFF *tiffptr, int a); /* 157 */
    int (*tIFFInitJPEG) (TIFF *tiffptr, int a); /* 158 */
    int (*tIFFInitJBIG) (TIFF *tiffptr, int a); /* 159 */
    int (*tIFFInitZIP) (TIFF *tiffptr, int a); /* 160 */
    int (*tIFFInitPixarLog) (TIFF *tiffptr, int a); /* 161 */
    int (*tIFFInitSGILog) (TIFF *tiffptr, int a); /* 162 */
} TifftclStubs;

#ifdef __cplusplus
extern "C" {
#endif
TIFFTCLAPI const TifftclStubs *tifftclStubsPtr;
#ifdef __cplusplus
}
#endif

#if defined(USE_TIFFTCL_STUBS)

/*
 * Inline function declarations:
 */

#define TIFFGetVersion \
	(tifftclStubsPtr->tIFFGetVersion) /* 0 */
#define TIFFFindCODEC \
	(tifftclStubsPtr->tIFFFindCODEC) /* 1 */
#define TIFFRegisterCODEC \
	(tifftclStubsPtr->tIFFRegisterCODEC) /* 2 */
#define TIFFUnRegisterCODEC \
	(tifftclStubsPtr->tIFFUnRegisterCODEC) /* 3 */
#define _TIFFmalloc \
	(tifftclStubsPtr->_TIFFmallocPtr) /* 4 */
#define _TIFFrealloc \
	(tifftclStubsPtr->_TIFFreallocPtr) /* 5 */
#define _TIFFmemset \
	(tifftclStubsPtr->_TIFFmemsetPtr) /* 6 */
#define _TIFFmemcpy \
	(tifftclStubsPtr->_TIFFmemcpyPtr) /* 7 */
#define _TIFFmemcmp \
	(tifftclStubsPtr->_TIFFmemcmpPtr) /* 8 */
#define _TIFFfree \
	(tifftclStubsPtr->_TIFFfreePtr) /* 9 */
#define TIFFClose \
	(tifftclStubsPtr->tIFFClose) /* 10 */
#define TIFFFlush \
	(tifftclStubsPtr->tIFFFlush) /* 11 */
#define TIFFFlushData \
	(tifftclStubsPtr->tIFFFlushData) /* 12 */
#define TIFFGetField \
	(tifftclStubsPtr->tIFFGetField) /* 13 */
#define TIFFVGetField \
	(tifftclStubsPtr->tIFFVGetField) /* 14 */
#define TIFFGetFieldDefaulted \
	(tifftclStubsPtr->tIFFGetFieldDefaulted) /* 15 */
#define TIFFVGetFieldDefaulted \
	(tifftclStubsPtr->tIFFVGetFieldDefaulted) /* 16 */
#define TIFFReadDirectory \
	(tifftclStubsPtr->tIFFReadDirectory) /* 17 */
#define TIFFScanlineSize \
	(tifftclStubsPtr->tIFFScanlineSize) /* 18 */
#define TIFFRasterScanlineSize \
	(tifftclStubsPtr->tIFFRasterScanlineSize) /* 19 */
#define TIFFStripSize \
	(tifftclStubsPtr->tIFFStripSize) /* 20 */
#define TIFFVStripSize \
	(tifftclStubsPtr->tIFFVStripSize) /* 21 */
#define TIFFTileRowSize \
	(tifftclStubsPtr->tIFFTileRowSize) /* 22 */
#define TIFFTileSize \
	(tifftclStubsPtr->tIFFTileSize) /* 23 */
#define TIFFVTileSize \
	(tifftclStubsPtr->tIFFVTileSize) /* 24 */
#define TIFFDefaultStripSize \
	(tifftclStubsPtr->tIFFDefaultStripSize) /* 25 */
#define TIFFDefaultTileSize \
	(tifftclStubsPtr->tIFFDefaultTileSize) /* 26 */
#define TIFFFileno \
	(tifftclStubsPtr->tIFFFileno) /* 27 */
#define TIFFGetMode \
	(tifftclStubsPtr->tIFFGetMode) /* 28 */
#define TIFFIsTiled \
	(tifftclStubsPtr->tIFFIsTiled) /* 29 */
#define TIFFIsByteSwapped \
	(tifftclStubsPtr->tIFFIsByteSwapped) /* 30 */
#define TIFFIsUpSampled \
	(tifftclStubsPtr->tIFFIsUpSampled) /* 31 */
#define TIFFIsMSB2LSB \
	(tifftclStubsPtr->tIFFIsMSB2LSB) /* 32 */
#define TIFFCurrentRow \
	(tifftclStubsPtr->tIFFCurrentRow) /* 33 */
#define TIFFCurrentDirectory \
	(tifftclStubsPtr->tIFFCurrentDirectory) /* 34 */
#define TIFFNumberOfDirectories \
	(tifftclStubsPtr->tIFFNumberOfDirectories) /* 35 */
#define TIFFCurrentDirOffset \
	(tifftclStubsPtr->tIFFCurrentDirOffset) /* 36 */
#define TIFFCurrentStrip \
	(tifftclStubsPtr->tIFFCurrentStrip) /* 37 */
#define TIFFCurrentTile \
	(tifftclStubsPtr->tIFFCurrentTile) /* 38 */
#define TIFFReadBufferSetup \
	(tifftclStubsPtr->tIFFReadBufferSetup) /* 39 */
#define TIFFWriteBufferSetup \
	(tifftclStubsPtr->tIFFWriteBufferSetup) /* 40 */
#define TIFFWriteCheck \
	(tifftclStubsPtr->tIFFWriteCheck) /* 41 */
#define TIFFCreateDirectory \
	(tifftclStubsPtr->tIFFCreateDirectory) /* 42 */
#define TIFFLastDirectory \
	(tifftclStubsPtr->tIFFLastDirectory) /* 43 */
#define TIFFSetDirectory \
	(tifftclStubsPtr->tIFFSetDirectory) /* 44 */
#define TIFFSetSubDirectory \
	(tifftclStubsPtr->tIFFSetSubDirectory) /* 45 */
#define TIFFUnlinkDirectory \
	(tifftclStubsPtr->tIFFUnlinkDirectory) /* 46 */
#define TIFFSetField \
	(tifftclStubsPtr->tIFFSetField) /* 47 */
#define TIFFVSetField \
	(tifftclStubsPtr->tIFFVSetField) /* 48 */
#define TIFFWriteDirectory \
	(tifftclStubsPtr->tIFFWriteDirectory) /* 49 */
#define TIFFReassignTagToIgnore \
	(tifftclStubsPtr->tIFFReassignTagToIgnore) /* 50 */
#define TIFFPrintDirectory \
	(tifftclStubsPtr->tIFFPrintDirectory) /* 51 */
#define TIFFReadScanline \
	(tifftclStubsPtr->tIFFReadScanline) /* 52 */
#define TIFFWriteScanline \
	(tifftclStubsPtr->tIFFWriteScanline) /* 53 */
#define TIFFReadRGBAImage \
	(tifftclStubsPtr->tIFFReadRGBAImage) /* 54 */
#define TIFFReadRGBAStrip \
	(tifftclStubsPtr->tIFFReadRGBAStrip) /* 55 */
#define TIFFReadRGBATile \
	(tifftclStubsPtr->tIFFReadRGBATile) /* 56 */
#define TIFFRGBAImageOK \
	(tifftclStubsPtr->tIFFRGBAImageOK) /* 57 */
#define TIFFRGBAImageBegin \
	(tifftclStubsPtr->tIFFRGBAImageBegin) /* 58 */
#define TIFFRGBAImageGet \
	(tifftclStubsPtr->tIFFRGBAImageGet) /* 59 */
#define TIFFRGBAImageEnd \
	(tifftclStubsPtr->tIFFRGBAImageEnd) /* 60 */
#define TIFFOpen \
	(tifftclStubsPtr->tIFFOpen) /* 61 */
#define TIFFFdOpen \
	(tifftclStubsPtr->tIFFFdOpen) /* 62 */
#define TIFFClientOpen \
	(tifftclStubsPtr->tIFFClientOpen) /* 63 */
#define TIFFFileName \
	(tifftclStubsPtr->tIFFFileName) /* 64 */
#define TIFFError \
	(tifftclStubsPtr->tIFFError) /* 65 */
#define TIFFWarning \
	(tifftclStubsPtr->tIFFWarning) /* 66 */
#define TIFFSetErrorHandler \
	(tifftclStubsPtr->tIFFSetErrorHandler) /* 67 */
#define TIFFSetWarningHandler \
	(tifftclStubsPtr->tIFFSetWarningHandler) /* 68 */
#define TIFFSetTagExtender \
	(tifftclStubsPtr->tIFFSetTagExtender) /* 69 */
#define TIFFComputeTile \
	(tifftclStubsPtr->tIFFComputeTile) /* 70 */
#define TIFFCheckTile \
	(tifftclStubsPtr->tIFFCheckTile) /* 71 */
#define TIFFNumberOfTiles \
	(tifftclStubsPtr->tIFFNumberOfTiles) /* 72 */
#define TIFFReadTile \
	(tifftclStubsPtr->tIFFReadTile) /* 73 */
#define TIFFWriteTile \
	(tifftclStubsPtr->tIFFWriteTile) /* 74 */
#define TIFFComputeStrip \
	(tifftclStubsPtr->tIFFComputeStrip) /* 75 */
#define TIFFNumberOfStrips \
	(tifftclStubsPtr->tIFFNumberOfStrips) /* 76 */
#define TIFFReadEncodedStrip \
	(tifftclStubsPtr->tIFFReadEncodedStrip) /* 77 */
#define TIFFReadRawStrip \
	(tifftclStubsPtr->tIFFReadRawStrip) /* 78 */
#define TIFFReadEncodedTile \
	(tifftclStubsPtr->tIFFReadEncodedTile) /* 79 */
#define TIFFReadRawTile \
	(tifftclStubsPtr->tIFFReadRawTile) /* 80 */
#define TIFFWriteEncodedStrip \
	(tifftclStubsPtr->tIFFWriteEncodedStrip) /* 81 */
#define TIFFWriteRawStrip \
	(tifftclStubsPtr->tIFFWriteRawStrip) /* 82 */
#define TIFFWriteEncodedTile \
	(tifftclStubsPtr->tIFFWriteEncodedTile) /* 83 */
#define TIFFWriteRawTile \
	(tifftclStubsPtr->tIFFWriteRawTile) /* 84 */
#define TIFFSetWriteOffset \
	(tifftclStubsPtr->tIFFSetWriteOffset) /* 85 */
#define TIFFSwabShort \
	(tifftclStubsPtr->tIFFSwabShort) /* 86 */
#define TIFFSwabLong \
	(tifftclStubsPtr->tIFFSwabLong) /* 87 */
#define TIFFSwabDouble \
	(tifftclStubsPtr->tIFFSwabDouble) /* 88 */
#define TIFFSwabArrayOfShort \
	(tifftclStubsPtr->tIFFSwabArrayOfShort) /* 89 */
#define TIFFSwabArrayOfLong \
	(tifftclStubsPtr->tIFFSwabArrayOfLong) /* 90 */
#define TIFFSwabArrayOfDouble \
	(tifftclStubsPtr->tIFFSwabArrayOfDouble) /* 91 */
#define TIFFReverseBits \
	(tifftclStubsPtr->tIFFReverseBits) /* 92 */
#define TIFFGetBitRevTable \
	(tifftclStubsPtr->tIFFGetBitRevTable) /* 93 */
/* Slot 94 is reserved */
/* Slot 95 is reserved */
/* Slot 96 is reserved */
/* Slot 97 is reserved */
/* Slot 98 is reserved */
/* Slot 99 is reserved */
#define TIFFPredictorInit \
	(tifftclStubsPtr->tIFFPredictorInit) /* 100 */
/* Slot 101 is reserved */
/* Slot 102 is reserved */
/* Slot 103 is reserved */
/* Slot 104 is reserved */
/* Slot 105 is reserved */
/* Slot 106 is reserved */
/* Slot 107 is reserved */
/* Slot 108 is reserved */
/* Slot 109 is reserved */
#define _TIFFSetupFieldInfo \
	(tifftclStubsPtr->_TIFFSetupFieldInfoPtr) /* 110 */
#define _TIFFMergeFieldInfo \
	(tifftclStubsPtr->_TIFFMergeFieldInfoPtr) /* 111 */
#define _TIFFPrintFieldInfo \
	(tifftclStubsPtr->_TIFFPrintFieldInfoPtr) /* 112 */
#define TIFFFindFieldInfo \
	(tifftclStubsPtr->tIFFFindFieldInfo) /* 113 */
#define TIFFFieldWithTag \
	(tifftclStubsPtr->tIFFFieldWithTag) /* 114 */
#define _TIFFSampleToTagType \
	(tifftclStubsPtr->_TIFFSampleToTagTypePtr) /* 115 */
/* Slot 116 is reserved */
/* Slot 117 is reserved */
/* Slot 118 is reserved */
/* Slot 119 is reserved */
#define _TIFFgetMode \
	(tifftclStubsPtr->_TIFFgetModePtr) /* 120 */
#define _TIFFNoRowEncode \
	(tifftclStubsPtr->_TIFFNoRowEncodePtr) /* 121 */
#define _TIFFNoStripEncode \
	(tifftclStubsPtr->_TIFFNoStripEncodePtr) /* 122 */
#define _TIFFNoTileEncode \
	(tifftclStubsPtr->_TIFFNoTileEncodePtr) /* 123 */
#define _TIFFNoRowDecode \
	(tifftclStubsPtr->_TIFFNoRowDecodePtr) /* 124 */
#define _TIFFNoStripDecode \
	(tifftclStubsPtr->_TIFFNoStripDecodePtr) /* 125 */
#define _TIFFNoTileDecode \
	(tifftclStubsPtr->_TIFFNoTileDecodePtr) /* 126 */
#define _TIFFNoPostDecode \
	(tifftclStubsPtr->_TIFFNoPostDecodePtr) /* 127 */
#define _TIFFNoPreCode \
	(tifftclStubsPtr->_TIFFNoPreCodePtr) /* 128 */
#define _TIFFNoSeek \
	(tifftclStubsPtr->_TIFFNoSeekPtr) /* 129 */
#define _TIFFSwab16BitData \
	(tifftclStubsPtr->_TIFFSwab16BitDataPtr) /* 130 */
#define _TIFFSwab32BitData \
	(tifftclStubsPtr->_TIFFSwab32BitDataPtr) /* 131 */
#define _TIFFSwab64BitData \
	(tifftclStubsPtr->_TIFFSwab64BitDataPtr) /* 132 */
#define TIFFFlushData1 \
	(tifftclStubsPtr->tIFFFlushData1) /* 133 */
#define TIFFFreeDirectory \
	(tifftclStubsPtr->tIFFFreeDirectory) /* 134 */
#define TIFFDefaultDirectory \
	(tifftclStubsPtr->tIFFDefaultDirectory) /* 135 */
#define TIFFSetCompressionScheme \
	(tifftclStubsPtr->tIFFSetCompressionScheme) /* 136 */
#define _TIFFSetDefaultCompressionState \
	(tifftclStubsPtr->_TIFFSetDefaultCompressionStatePtr) /* 137 */
#define _TIFFDefaultStripSize \
	(tifftclStubsPtr->_TIFFDefaultStripSizePtr) /* 138 */
#define _TIFFDefaultTileSize \
	(tifftclStubsPtr->_TIFFDefaultTileSizePtr) /* 139 */
#define _TIFFsetByteArray \
	(tifftclStubsPtr->_TIFFsetByteArrayPtr) /* 140 */
#define _TIFFsetString \
	(tifftclStubsPtr->_TIFFsetStringPtr) /* 141 */
#define _TIFFsetShortArray \
	(tifftclStubsPtr->_TIFFsetShortArrayPtr) /* 142 */
#define _TIFFsetLongArray \
	(tifftclStubsPtr->_TIFFsetLongArrayPtr) /* 143 */
#define _TIFFsetFloatArray \
	(tifftclStubsPtr->_TIFFsetFloatArrayPtr) /* 144 */
#define _TIFFsetDoubleArray \
	(tifftclStubsPtr->_TIFFsetDoubleArrayPtr) /* 145 */
#define _TIFFprintAscii \
	(tifftclStubsPtr->_TIFFprintAsciiPtr) /* 146 */
#define _TIFFprintAsciiTag \
	(tifftclStubsPtr->_TIFFprintAsciiTagPtr) /* 147 */
#define TIFFInitDumpMode \
	(tifftclStubsPtr->tIFFInitDumpMode) /* 148 */
#define TIFFInitPackBits \
	(tifftclStubsPtr->tIFFInitPackBits) /* 149 */
#define TIFFInitCCITTRLE \
	(tifftclStubsPtr->tIFFInitCCITTRLE) /* 150 */
#define TIFFInitCCITTRLEW \
	(tifftclStubsPtr->tIFFInitCCITTRLEW) /* 151 */
#define TIFFInitCCITTFax3 \
	(tifftclStubsPtr->tIFFInitCCITTFax3) /* 152 */
#define TIFFInitCCITTFax4 \
	(tifftclStubsPtr->tIFFInitCCITTFax4) /* 153 */
#define TIFFInitThunderScan \
	(tifftclStubsPtr->tIFFInitThunderScan) /* 154 */
#define TIFFInitNeXT \
	(tifftclStubsPtr->tIFFInitNeXT) /* 155 */
#define TIFFInitLZW \
	(tifftclStubsPtr->tIFFInitLZW) /* 156 */
#define TIFFInitOJPEG \
	(tifftclStubsPtr->tIFFInitOJPEG) /* 157 */
#define TIFFInitJPEG \
	(tifftclStubsPtr->tIFFInitJPEG) /* 158 */
#define TIFFInitJBIG \
	(tifftclStubsPtr->tIFFInitJBIG) /* 159 */
#define TIFFInitZIP \
	(tifftclStubsPtr->tIFFInitZIP) /* 160 */
#define TIFFInitPixarLog \
	(tifftclStubsPtr->tIFFInitPixarLog) /* 161 */
#define TIFFInitSGILog \
	(tifftclStubsPtr->tIFFInitSGILog) /* 162 */

#endif /* defined(USE_TIFFTCL_STUBS) */

/* !END!: Do not edit above this line. */

#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TIFFTCLDECLS */

