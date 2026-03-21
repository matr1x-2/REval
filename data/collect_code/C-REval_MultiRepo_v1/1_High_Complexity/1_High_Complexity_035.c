/* C-REval Sample */
/* Sample-ID: 1_High_Complexity_035 */
/* Category: 1_High_Complexity */
/* Repo: linux */
/* Cyclomatic Complexity: 40 */
/* NLOC: 124 */
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
 *         if (value!=0)    /* 0 ==> default */
 *             BOUNDCHECK(ZSTD_c_maxBlockSize, value);
 *         assert(value>=0);
 *         CCtxParams->maxBlockSize = (size_t)value;
 *         return CCtxParams->maxBlockSize;
 * 
 *     case ZSTD_c_repcodeResolution:
 *         BOUNDCHECK(ZSTD_c_repcodeResolution, value);
 *         CCtxParams->searchForExternalRepcodes = (ZSTD_ParamSwitch_e)value;
 *         return CCtxParams->searchForExternalRepcodes;
 * 
 *     default: RETURN_ERROR(parameter_unsupported, "unknown parameter");
 *     }
 * }
 * 
 * size_t ZSTD_CCtx_getParameter(ZSTD_CCtx const* cctx, ZSTD_cParameter param, int* value)
 * {
 *     return ZSTD_CCtxParams_getParameter(&cctx->requestedParams, param, value);
 * }
 */
size_t ZSTD_CCtxParams_getParameter(
        ZSTD_CCtx_params const* CCtxParams, ZSTD_cParameter param, int* value)
{
    switch(param)
    {
    case ZSTD_c_format :
        *value = (int)CCtxParams->format;
        break;
    case ZSTD_c_compressionLevel :
        *value = CCtxParams->compressionLevel;
        break;
    case ZSTD_c_windowLog :
        *value = (int)CCtxParams->cParams.windowLog;
        break;
    case ZSTD_c_hashLog :
        *value = (int)CCtxParams->cParams.hashLog;
        break;
    case ZSTD_c_chainLog :
        *value = (int)CCtxParams->cParams.chainLog;
        break;
    case ZSTD_c_searchLog :
        *value = (int)CCtxParams->cParams.searchLog;
        break;
    case ZSTD_c_minMatch :
        *value = (int)CCtxParams->cParams.minMatch;
        break;
    case ZSTD_c_targetLength :
        *value = (int)CCtxParams->cParams.targetLength;
        break;
    case ZSTD_c_strategy :
        *value = (int)CCtxParams->cParams.strategy;
        break;
    case ZSTD_c_contentSizeFlag :
        *value = CCtxParams->fParams.contentSizeFlag;
        break;
    case ZSTD_c_checksumFlag :
        *value = CCtxParams->fParams.checksumFlag;
        break;
    case ZSTD_c_dictIDFlag :
        *value = !CCtxParams->fParams.noDictIDFlag;
        break;
    case ZSTD_c_forceMaxWindow :
        *value = CCtxParams->forceWindow;
        break;
    case ZSTD_c_forceAttachDict :
        *value = (int)CCtxParams->attachDictPref;
        break;
    case ZSTD_c_literalCompressionMode :
        *value = (int)CCtxParams->literalCompressionMode;
        break;
    case ZSTD_c_nbWorkers :
        assert(CCtxParams->nbWorkers == 0);
        *value = CCtxParams->nbWorkers;
        break;
    case ZSTD_c_jobSize :
        RETURN_ERROR(parameter_unsupported, "not compiled with multithreading");
    case ZSTD_c_overlapLog :
        RETURN_ERROR(parameter_unsupported, "not compiled with multithreading");
    case ZSTD_c_rsyncable :
        RETURN_ERROR(parameter_unsupported, "not compiled with multithreading");
    case ZSTD_c_enableDedicatedDictSearch :
        *value = CCtxParams->enableDedicatedDictSearch;
        break;
    case ZSTD_c_enableLongDistanceMatching :
        *value = (int)CCtxParams->ldmParams.enableLdm;
        break;
    case ZSTD_c_ldmHashLog :
        *value = (int)CCtxParams->ldmParams.hashLog;
        break;
    case ZSTD_c_ldmMinMatch :
        *value = (int)CCtxParams->ldmParams.minMatchLength;
        break;
    case ZSTD_c_ldmBucketSizeLog :
        *value = (int)CCtxParams->ldmParams.bucketSizeLog;
        break;
    case ZSTD_c_ldmHashRateLog :
        *value = (int)CCtxParams->ldmParams.hashRateLog;
        break;
    case ZSTD_c_targetCBlockSize :
        *value = (int)CCtxParams->targetCBlockSize;
        break;
    case ZSTD_c_srcSizeHint :
        *value = (int)CCtxParams->srcSizeHint;
        break;
    case ZSTD_c_stableInBuffer :
        *value = (int)CCtxParams->inBufferMode;
        break;
    case ZSTD_c_stableOutBuffer :
        *value = (int)CCtxParams->outBufferMode;
        break;
    case ZSTD_c_blockDelimiters :
        *value = (int)CCtxParams->blockDelimiters;
        break;
    case ZSTD_c_validateSequences :
        *value = (int)CCtxParams->validateSequences;
        break;
    case ZSTD_c_splitAfterSequences :
        *value = (int)CCtxParams->postBlockSplitter;
        break;
    case ZSTD_c_blockSplitterLevel :
        *value = CCtxParams->preBlockSplitter_level;
        break;
    case ZSTD_c_useRowMatchFinder :
        *value = (int)CCtxParams->useRowMatchFinder;
        break;
    case ZSTD_c_deterministicRefPrefix:
        *value = (int)CCtxParams->deterministicRefPrefix;
        break;
    case ZSTD_c_prefetchCDictTables:
        *value = (int)CCtxParams->prefetchCDictTables;
        break;
    case ZSTD_c_enableSeqProducerFallback:
        *value = CCtxParams->enableMatchFinderFallback;
        break;
    case ZSTD_c_maxBlockSize:
        *value = (int)CCtxParams->maxBlockSize;
        break;
    case ZSTD_c_repcodeResolution:
        *value = (int)CCtxParams->searchForExternalRepcodes;
        break;
    default: RETURN_ERROR(parameter_unsupported, "unknown parameter");
    }
    return 0;
}
/* Context-After
 * /* ZSTD_CCtx_setParametersUsingCCtxParams() :
 *  *  just applies `params` into `cctx`
 *  *  no action is performed, parameters are merely stored.
 *  *  If ZSTDMT is enabled, parameters are pushed to cctx->mtctx.
 *  *    This is possible even if a compression is ongoing.
 *  *    In which case, new parameters will be applied on the fly, starting with next compression job.
 *  */
 * size_t ZSTD_CCtx_setParametersUsingCCtxParams(
 *         ZSTD_CCtx* cctx, const ZSTD_CCtx_params* params)
 * {
 *     DEBUGLOG(4, "ZSTD_CCtx_setParametersUsingCCtxParams");
 *     RETURN_ERROR_IF(cctx->streamStage != zcss_init, stage_wrong,
 *                     "The context is in the wrong stage!");
 *     RETURN_ERROR_IF(cctx->cdict, stage_wrong,
 *                     "Can't override parameters with cdict attached (some must "
 *                     "be inherited from the cdict).");
 * 
 *     cctx->requestedParams = *params;
 *     return 0;
 */
