/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_013 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 49 */
/* NLOC: 169 */
/* Subsystem: lib */
/* Includes
 * #include "../common/allocations.h"  /* ZSTD_customMalloc, ZSTD_customCalloc, ZSTD_customFree */
 * #include "../common/zstd_deps.h"  /* INT_MAX, ZSTD_memset, ZSTD_memcpy */
 * #include "../common/mem.h"
 * #include "../common/error_private.h"
 * #include "hist.h"           /* HIST_countFast_wksp */
 * #include "../common/fse.h"
 * #include "../common/huf.h"
 * #include "zstd_compress_internal.h"
 * #include "zstd_compress_sequences.h"
 * #include "zstd_compress_literals.h"
 * #include "zstd_fast.h"
 * #include "zstd_double_fast.h"
 * #include "zstd_lazy.h"
 * #include "zstd_opt.h"
 * #include "zstd_ldm.h"
 * #include "zstd_compress_superblock.h"
 * #include  "../common/bits.h"      /* ZSTD_highbit32, ZSTD_rotateRight_U64 */
 * #include "zstd_preSplit.h"
 * #include <immintrin.h>  /* AVX2 intrinsics */
 * #include "clevels.h"
 */
/* Context-Before
 *     return 0;
 * }
 * 
 * /*======   Compression   ======*/
 * 
 * static size_t ZSTD_nextInputSizeHint(const ZSTD_CCtx* cctx)
 * {
 *     if (cctx->appliedParams.inBufferMode == ZSTD_bm_stable) {
 *         return cctx->blockSizeMax - cctx->stableIn_notConsumed;
 *     }
 *     assert(cctx->appliedParams.inBufferMode == ZSTD_bm_buffered);
 *     {   size_t hintInSize = cctx->inBuffTarget - cctx->inBuffPos;
 *         if (hintInSize==0) hintInSize = cctx->blockSizeMax;
 *         return hintInSize;
 *     }
 * }
 * 
 * /* ZSTD_compressStream_generic():
 *  *  internal function for all *compressStream*() variants
 *  * @return : hint size for next input to complete ongoing block */
 */
static size_t ZSTD_compressStream_generic(ZSTD_CStream* zcs,
                                          ZSTD_outBuffer* output,
                                          ZSTD_inBuffer* input,
                                          ZSTD_EndDirective const flushMode)
{
    const char* const istart = (assert(input != NULL), (const char*)input->src);
    const char* const iend = (istart != NULL) ? istart + input->size : istart;
    const char* ip = (istart != NULL) ? istart + input->pos : istart;
    char* const ostart = (assert(output != NULL), (char*)output->dst);
    char* const oend = (ostart != NULL) ? ostart + output->size : ostart;
    char* op = (ostart != NULL) ? ostart + output->pos : ostart;
    U32 someMoreWork = 1;

    /* check expectations */
    DEBUGLOG(5, "ZSTD_compressStream_generic, flush=%i, srcSize = %zu", (int)flushMode, input->size - input->pos);
    assert(zcs != NULL);
    if (zcs->appliedParams.inBufferMode == ZSTD_bm_stable) {
        assert(input->pos >= zcs->stableIn_notConsumed);
        input->pos -= zcs->stableIn_notConsumed;
        if (ip) ip -= zcs->stableIn_notConsumed;
        zcs->stableIn_notConsumed = 0;
    }
    if (zcs->appliedParams.inBufferMode == ZSTD_bm_buffered) {
        assert(zcs->inBuff != NULL);
        assert(zcs->inBuffSize > 0);
    }
    if (zcs->appliedParams.outBufferMode == ZSTD_bm_buffered) {
        assert(zcs->outBuff !=  NULL);
        assert(zcs->outBuffSize > 0);
    }
    if (input->src == NULL) assert(input->size == 0);
    assert(input->pos <= input->size);
    if (output->dst == NULL) assert(output->size == 0);
    assert(output->pos <= output->size);
    assert((U32)flushMode <= (U32)ZSTD_e_end);

    while (someMoreWork) {
        switch(zcs->streamStage)
        {
        case zcss_init:
            RETURN_ERROR(init_missing, "call ZSTD_initCStream() first!");

        case zcss_load:
            if ( (flushMode == ZSTD_e_end)
              && ( (size_t)(oend-op) >= ZSTD_compressBound((size_t)(iend-ip))     /* Enough output space */
                || zcs->appliedParams.outBufferMode == ZSTD_bm_stable)  /* OR we are allowed to return dstSizeTooSmall */
              && (zcs->inBuffPos == 0) ) {
                /* shortcut to compression pass directly into output buffer */
                size_t const cSize = ZSTD_compressEnd_public(zcs,
                                                op, (size_t)(oend-op),
                                                ip, (size_t)(iend-ip));
                DEBUGLOG(4, "ZSTD_compressEnd : cSize=%u", (unsigned)cSize);
                FORWARD_IF_ERROR(cSize, "ZSTD_compressEnd failed");
                ip = iend;
                op += cSize;
                zcs->frameEnded = 1;
                ZSTD_CCtx_reset(zcs, ZSTD_reset_session_only);
                someMoreWork = 0; break;
            }
            /* complete loading into inBuffer in buffered mode */
            if (zcs->appliedParams.inBufferMode == ZSTD_bm_buffered) {
                size_t const toLoad = zcs->inBuffTarget - zcs->inBuffPos;
                size_t const loaded = ZSTD_limitCopy(
                                        zcs->inBuff + zcs->inBuffPos, toLoad,
                                        ip, (size_t)(iend-ip));
                zcs->inBuffPos += loaded;
                if (ip) ip += loaded;
                if ( (flushMode == ZSTD_e_continue)
                  && (zcs->inBuffPos < zcs->inBuffTarget) ) {
                    /* not enough input to fill full block : stop here */
                    someMoreWork = 0; break;
                }
                if ( (flushMode == ZSTD_e_flush)
                  && (zcs->inBuffPos == zcs->inToCompress) ) {
                    /* empty */
                    someMoreWork = 0; break;
                }
            } else {
                assert(zcs->appliedParams.inBufferMode == ZSTD_bm_stable);
                if ( (flushMode == ZSTD_e_continue)
                  && ( (size_t)(iend - ip) < zcs->blockSizeMax) ) {
                    /* can't compress a full block : stop here */
                    zcs->stableIn_notConsumed = (size_t)(iend - ip);
                    ip = iend;  /* pretend to have consumed input */
                    someMoreWork = 0; break;
                }
                if ( (flushMode == ZSTD_e_flush)
                  && (ip == iend) ) {
                    /* empty */
                    someMoreWork = 0; break;
                }
            }
            /* compress current block (note : this stage cannot be stopped in the middle) */
            DEBUGLOG(5, "stream compression stage (flushMode==%u)", flushMode);
            {   int const inputBuffered = (zcs->appliedParams.inBufferMode == ZSTD_bm_buffered);
                void* cDst;
                size_t cSize;
                size_t oSize = (size_t)(oend-op);
                size_t const iSize = inputBuffered ? zcs->inBuffPos - zcs->inToCompress
                                                   : MIN((size_t)(iend - ip), zcs->blockSizeMax);
                if (oSize >= ZSTD_compressBound(iSize) || zcs->appliedParams.outBufferMode == ZSTD_bm_stable)
                    cDst = op;   /* compress into output buffer, to skip flush stage */
                else
                    cDst = zcs->outBuff, oSize = zcs->outBuffSize;
                if (inputBuffered) {
                    unsigned const lastBlock = (flushMode == ZSTD_e_end) && (ip==iend);
                    cSize = lastBlock ?
                            ZSTD_compressEnd_public(zcs, cDst, oSize,
                                        zcs->inBuff + zcs->inToCompress, iSize) :
                            ZSTD_compressContinue_public(zcs, cDst, oSize,
                                        zcs->inBuff + zcs->inToCompress, iSize);
                    FORWARD_IF_ERROR(cSize, "%s", lastBlock ? "ZSTD_compressEnd failed" : "ZSTD_compressContinue failed");
                    zcs->frameEnded = lastBlock;
                    /* prepare next block */
                    zcs->inBuffTarget = zcs->inBuffPos + zcs->blockSizeMax;
                    if (zcs->inBuffTarget > zcs->inBuffSize)
                        zcs->inBuffPos = 0, zcs->inBuffTarget = zcs->blockSizeMax;
                    DEBUGLOG(5, "inBuffTarget:%u / inBuffSize:%u",
                            (unsigned)zcs->inBuffTarget, (unsigned)zcs->inBuffSize);
                    if (!lastBlock)
                        assert(zcs->inBuffTarget <= zcs->inBuffSize);
                    zcs->inToCompress = zcs->inBuffPos;
                } else { /* !inputBuffered, hence ZSTD_bm_stable */
                    unsigned const lastBlock = (flushMode == ZSTD_e_end) && (ip + iSize == iend);
                    cSize = lastBlock ?
                            ZSTD_compressEnd_public(zcs, cDst, oSize, ip, iSize) :
                            ZSTD_compressContinue_public(zcs, cDst, oSize, ip, iSize);
                    /* Consume the input prior to error checking to mirror buffered mode. */
                    if (ip) ip += iSize;
                    FORWARD_IF_ERROR(cSize, "%s", lastBlock ? "ZSTD_compressEnd failed" : "ZSTD_compressContinue failed");
                    zcs->frameEnded = lastBlock;
                    if (lastBlock) assert(ip == iend);
                }
                if (cDst == op) {  /* no need to flush */
                    op += cSize;
                    if (zcs->frameEnded) {
                        DEBUGLOG(5, "Frame completed directly in outBuffer");
                        someMoreWork = 0;
                        ZSTD_CCtx_reset(zcs, ZSTD_reset_session_only);
                    }
                    break;
                }
                zcs->outBuffContentSize = cSize;
                zcs->outBuffFlushedSize = 0;
                zcs->streamStage = zcss_flush; /* pass-through to flush stage */
            }
	    ZSTD_FALLTHROUGH;
        case zcss_flush:
            DEBUGLOG(5, "flush stage");
            assert(zcs->appliedParams.outBufferMode == ZSTD_bm_buffered);
            {   size_t const toFlush = zcs->outBuffContentSize - zcs->outBuffFlushedSize;
                size_t const flushed = ZSTD_limitCopy(op, (size_t)(oend-op),
                            zcs->outBuff + zcs->outBuffFlushedSize, toFlush);
                DEBUGLOG(5, "toFlush: %u into %u ==> flushed: %u",
                            (unsigned)toFlush, (unsigned)(oend-op), (unsigned)flushed);
                if (flushed)
                    op += flushed;
                zcs->outBuffFlushedSize += flushed;
                if (toFlush!=flushed) {
                    /* flush not fully completed, presumably because dst is too small */
                    assert(op==oend);
                    someMoreWork = 0;
                    break;
                }
                zcs->outBuffContentSize = zcs->outBuffFlushedSize = 0;
                if (zcs->frameEnded) {
                    DEBUGLOG(5, "Frame completed on flush");
                    someMoreWork = 0;
                    ZSTD_CCtx_reset(zcs, ZSTD_reset_session_only);
                    break;
                }
                zcs->streamStage = zcss_load;
                break;
            }

        default: /* impossible */
            assert(0);
        }
    }

    input->pos = (size_t)(ip - istart);
    output->pos = (size_t)(op - ostart);
    if (zcs->frameEnded) return 0;
    return ZSTD_nextInputSizeHint(zcs);
}
/* Context-After
 * static size_t ZSTD_nextInputSizeHint_MTorST(const ZSTD_CCtx* cctx)
 * {
 *     return ZSTD_nextInputSizeHint(cctx);
 * 
 * }
 * 
 * size_t ZSTD_compressStream(ZSTD_CStream* zcs, ZSTD_outBuffer* output, ZSTD_inBuffer* input)
 * {
 *     FORWARD_IF_ERROR( ZSTD_compressStream2(zcs, output, input, ZSTD_e_continue) , "");
 *     return ZSTD_nextInputSizeHint_MTorST(zcs);
 * }
 * 
 * /* After a compression call set the expected input/output buffer.
 *  * This is validated at the start of the next compression call.
 *  */
 * static void
 * ZSTD_setBufferExpectations(ZSTD_CCtx* cctx, const ZSTD_outBuffer* output, const ZSTD_inBuffer* input)
 * {
 *     DEBUGLOG(5, "ZSTD_setBufferExpectations (for advanced stable in/out modes)");
 */
