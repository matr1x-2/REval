/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_027 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 21 */
/* NLOC: 116 */
/* Subsystem: lib */
/* Includes
 * #include "../common/compiler.h"
 * #include "../common/mem.h"        /* U32, U16, etc. */
 * #include "../common/debug.h"      /* assert, DEBUGLOG */
 * #include "hist.h"       /* HIST_count_wksp */
 * #include "../common/bitstream.h"
 * #include "../common/fse.h"
 * #include "../common/error_private.h"
 * #include "../common/zstd_deps.h"  /* ZSTD_memset */
 * #include "../common/bits.h" /* ZSTD_highbit32 */
 */
/* Context-Before
 * #ifndef FSE_FUNCTION_EXTENSION
 * #  error "FSE_FUNCTION_EXTENSION must be defined"
 * #endif
 * #ifndef FSE_FUNCTION_TYPE
 * #  error "FSE_FUNCTION_TYPE must be defined"
 * #endif
 * 
 * /* Function names */
 * #define FSE_CAT(X,Y) X##Y
 * #define FSE_FUNCTION_NAME(X,Y) FSE_CAT(X,Y)
 * #define FSE_TYPE_NAME(X,Y) FSE_CAT(X,Y)
 * 
 * 
 * /* Function templates */
 * 
 * /* FSE_buildCTable_wksp() :
 *  * Same as FSE_buildCTable(), but using an externally allocated scratch buffer (`workSpace`).
 *  * wkspSize should be sized to handle worst case situation, which is `1<<max_tableLog * sizeof(FSE_FUNCTION_TYPE)`
 *  * workSpace must also be properly aligned with FSE_FUNCTION_TYPE requirements
 *  */
 */
size_t FSE_buildCTable_wksp(FSE_CTable* ct,
                      const short* normalizedCounter, unsigned maxSymbolValue, unsigned tableLog,
                            void* workSpace, size_t wkspSize)
{
    U32 const tableSize = 1 << tableLog;
    U32 const tableMask = tableSize - 1;
    void* const ptr = ct;
    U16* const tableU16 = ( (U16*) ptr) + 2;
    void* const FSCT = ((U32*)ptr) + 1 /* header */ + (tableLog ? tableSize>>1 : 1) ;
    FSE_symbolCompressionTransform* const symbolTT = (FSE_symbolCompressionTransform*) (FSCT);
    U32 const step = FSE_TABLESTEP(tableSize);
    U32 const maxSV1 = maxSymbolValue+1;

    U16* cumul = (U16*)workSpace;   /* size = maxSV1 */
    FSE_FUNCTION_TYPE* const tableSymbol = (FSE_FUNCTION_TYPE*)(cumul + (maxSV1+1));  /* size = tableSize */

    U32 highThreshold = tableSize-1;

    assert(((size_t)workSpace & 1) == 0);  /* Must be 2 bytes-aligned */
    if (FSE_BUILD_CTABLE_WORKSPACE_SIZE(maxSymbolValue, tableLog) > wkspSize) return ERROR(tableLog_tooLarge);
    /* CTable header */
    tableU16[-2] = (U16) tableLog;
    tableU16[-1] = (U16) maxSymbolValue;
    assert(tableLog < 16);   /* required for threshold strategy to work */

    /* For explanations on how to distribute symbol values over the table :
     * https://fastcompression.blogspot.fr/2014/02/fse-distributing-symbol-values.html */

     #ifdef __clang_analyzer__
     ZSTD_memset(tableSymbol, 0, sizeof(*tableSymbol) * tableSize);   /* useless initialization, just to keep scan-build happy */
     #endif

    /* symbol start positions */
    {   U32 u;
        cumul[0] = 0;
        for (u=1; u <= maxSV1; u++) {
            if (normalizedCounter[u-1]==-1) {  /* Low proba symbol */
                cumul[u] = cumul[u-1] + 1;
                tableSymbol[highThreshold--] = (FSE_FUNCTION_TYPE)(u-1);
            } else {
                assert(normalizedCounter[u-1] >= 0);
                cumul[u] = cumul[u-1] + (U16)normalizedCounter[u-1];
                assert(cumul[u] >= cumul[u-1]);  /* no overflow */
        }   }
        cumul[maxSV1] = (U16)(tableSize+1);
    }

    /* Spread symbols */
    if (highThreshold == tableSize - 1) {
        /* Case for no low prob count symbols. Lay down 8 bytes at a time
         * to reduce branch misses since we are operating on a small block
         */
        BYTE* const spread = tableSymbol + tableSize; /* size = tableSize + 8 (may write beyond tableSize) */
        {   U64 const add = 0x0101010101010101ull;
            size_t pos = 0;
            U64 sv = 0;
            U32 s;
            for (s=0; s<maxSV1; ++s, sv += add) {
                int i;
                int const n = normalizedCounter[s];
                MEM_write64(spread + pos, sv);
                for (i = 8; i < n; i += 8) {
                    MEM_write64(spread + pos + i, sv);
                }
                assert(n>=0);
                pos += (size_t)n;
            }
        }
        /* Spread symbols across the table. Lack of lowprob symbols means that
         * we don't need variable sized inner loop, so we can unroll the loop and
         * reduce branch misses.
         */
        {   size_t position = 0;
            size_t s;
            size_t const unroll = 2; /* Experimentally determined optimal unroll */
            assert(tableSize % unroll == 0); /* FSE_MIN_TABLELOG is 5 */
            for (s = 0; s < (size_t)tableSize; s += unroll) {
                size_t u;
                for (u = 0; u < unroll; ++u) {
                    size_t const uPosition = (position + (u * step)) & tableMask;
                    tableSymbol[uPosition] = spread[s + u];
                }
                position = (position + (unroll * step)) & tableMask;
            }
            assert(position == 0);   /* Must have initialized all positions */
        }
    } else {
        U32 position = 0;
        U32 symbol;
        for (symbol=0; symbol<maxSV1; symbol++) {
            int nbOccurrences;
            int const freq = normalizedCounter[symbol];
            for (nbOccurrences=0; nbOccurrences<freq; nbOccurrences++) {
                tableSymbol[position] = (FSE_FUNCTION_TYPE)symbol;
                position = (position + step) & tableMask;
                while (position > highThreshold)
                    position = (position + step) & tableMask;   /* Low proba area */
        }   }
        assert(position==0);  /* Must have initialized all positions */
    }

    /* Build table */
    {   U32 u; for (u=0; u<tableSize; u++) {
        FSE_FUNCTION_TYPE s = tableSymbol[u];   /* note : static analyzer may not understand tableSymbol is properly initialized */
        tableU16[cumul[s]++] = (U16) (tableSize+u);   /* TableU16 : sorted by symbol order; gives next state value */
    }   }

    /* Build Symbol Transformation Table */
    {   unsigned total = 0;
        unsigned s;
        for (s=0; s<=maxSymbolValue; s++) {
            switch (normalizedCounter[s])
            {
            case  0:
                /* filling nonetheless, for compatibility with FSE_getMaxNbBits() */
                symbolTT[s].deltaNbBits = ((tableLog+1) << 16) - (1<<tableLog);
                break;

            case -1:
            case  1:
                symbolTT[s].deltaNbBits = (tableLog << 16) - (1<<tableLog);
                assert(total <= INT_MAX);
                symbolTT[s].deltaFindState = (int)(total - 1);
                total ++;
                break;
            default :
                assert(normalizedCounter[s] > 1);
                {   U32 const maxBitsOut = tableLog - ZSTD_highbit32 ((U32)normalizedCounter[s]-1);
                    U32 const minStatePlus = (U32)normalizedCounter[s] << maxBitsOut;
                    symbolTT[s].deltaNbBits = (maxBitsOut << 16) - minStatePlus;
                    symbolTT[s].deltaFindState = (int)(total - (unsigned)normalizedCounter[s]);
                    total +=  (unsigned)normalizedCounter[s];
    }   }   }   }

#if 0  /* debug : symbol costs */
    DEBUGLOG(5, "\n --- table statistics : ");
    {   U32 symbol;
        for (symbol=0; symbol<=maxSymbolValue; symbol++) {
            DEBUGLOG(5, "%3u: w=%3i,   maxBits=%u, fracBits=%.2f",
                symbol, normalizedCounter[symbol],
                FSE_getMaxNbBits(symbolTT, symbol),
                (double)FSE_bitCost(symbolTT, tableLog, symbol, 8) / 256);
    }   }
#endif

    return 0;
}
/* Context-After
 * #ifndef FSE_COMMONDEFS_ONLY
 * 
 * /*-**************************************************************
 * *  FSE NCount encoding
 * ****************************************************************/
 * size_t FSE_NCountWriteBound(unsigned maxSymbolValue, unsigned tableLog)
 * {
 *     size_t const maxHeaderSize = (((maxSymbolValue+1) * tableLog
 *                                    + 4 /* bitCount initialized at 4 */
 *                                    + 2 /* first two symbols may use one additional bit each */) / 8)
 *                                    + 1 /* round up to whole nb bytes */
 *                                    + 2 /* additional two bytes for bitstream flush */;
 *     return maxSymbolValue ? maxHeaderSize : FSE_NCOUNTBOUND;  /* maxSymbolValue==0 ? use default */
 * }
 * 
 * static size_t
 * FSE_writeNCount_generic (void* header, size_t headerBufferSize,
 */
