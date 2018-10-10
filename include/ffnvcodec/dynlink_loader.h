/*
 * This copyright notice applies to this header file only:
 *
 * Copyright (c) 2016
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the software, and to permit persons to whom the
 * software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef FFNV_CUDA_DYNLINK_LOADER_H
#define FFNV_CUDA_DYNLINK_LOADER_H

#include <stdlib.h>

#include "dynlink_cuda.h"
#include "dynlink_nvcuvid.h"
#include "nvEncodeAPI.h"

#if defined(_WIN32) && (!defined(FFNV_LOAD_FUNC) || !defined(FFNV_SYM_FUNC) || !defined(FFNV_LIB_HANDLE))
# include <windows.h>
#endif

#ifndef FFNV_LIB_HANDLE
# if defined(_WIN32)
#  define FFNV_LIB_HANDLE HMODULE
# else
#  define FFNV_LIB_HANDLE void*
# endif
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
# define CUDA_LIBNAME "nvcuda.dll"
# define NVCUVID_LIBNAME "nvcuvid.dll"
# if defined(_WIN64) || defined(__CYGWIN64__)
#  define NVENC_LIBNAME "nvEncodeAPI64.dll"
# else
#  define NVENC_LIBNAME "nvEncodeAPI.dll"
# endif
#else
# define CUDA_LIBNAME "libcuda.so.1"
# define NVCUVID_LIBNAME "libnvcuvid.so.1"
# define NVENC_LIBNAME "libnvidia-encode.so.1"
#endif

#if !defined(FFNV_LOAD_FUNC) || !defined(FFNV_SYM_FUNC)
# ifdef _WIN32
#  define FFNV_LOAD_FUNC(path) LoadLibrary(TEXT(path))
#  define FFNV_SYM_FUNC(lib, sym) GetProcAddress((lib), TEXT(sym))
#  define FFNV_FREE_FUNC(lib) FreeLibrary(lib)
# else
#  include <dlfcn.h>
#  define FFNV_LOAD_FUNC(path) dlopen((path), RTLD_LAZY)
#  define FFNV_SYM_FUNC(lib, sym) dlsym((lib), (sym))
#  define FFNV_FREE_FUNC(lib) dlclose(lib)
# endif
#endif

#if !defined(FFNV_LOG_FUNC) || !defined(FFNV_DEBUG_LOG_FUNC)
# include <stdio.h>
# define FFNV_LOG_FUNC(logctx, msg, ...) fprintf(stderr, (msg), __VA_ARGS__)
# define FFNV_DEBUG_LOG_FUNC(logctx, msg, ...)
#endif

#define LOAD_LIBRARY(l, path)                                  \
    do {                                                       \
        if (!((l) = FFNV_LOAD_FUNC(path))) {                   \
            FFNV_LOG_FUNC(logctx, "Cannot load %s\n", path);   \
            ret = -1;                                          \
            goto error;                                        \
        }                                                      \
        FFNV_DEBUG_LOG_FUNC(logctx, "Loaded lib: %s\n", path); \
    } while (0)

#define LOAD_SYMBOL(fun, tp, symbol)                             \
    do {                                                         \
        if (!((f->fun) = (tp*)FFNV_SYM_FUNC(f->lib, symbol))) {  \
            FFNV_LOG_FUNC(logctx, "Cannot load %s\n", symbol);   \
            ret = -1;                                            \
            goto error;                                          \
        }                                                        \
        FFNV_DEBUG_LOG_FUNC(logctx, "Loaded sym: %s\n", symbol); \
    } while (0)

#define LOAD_SYMBOL_OPT(fun, tp, symbol)                                      \
    do {                                                                      \
        if (!((f->fun) = (tp*)FFNV_SYM_FUNC(f->lib, symbol))) {               \
            FFNV_DEBUG_LOG_FUNC(logctx, "Cannot load optional %s\n", symbol); \
        } else {                                                              \
            FFNV_DEBUG_LOG_FUNC(logctx, "Loaded sym: %s\n", symbol);          \
        }                                                                     \
    } while (0)

#define GENERIC_LOAD_FUNC_PREAMBLE(T, n, N)     \
    T *f;                                       \
    int ret;                                    \
                                                \
    n##_free_functions(functions);              \
                                                \
    f = *functions = (T*)calloc(1, sizeof(*f)); \
    if (!f)                                     \
        return -1;                              \
                                                \
    LOAD_LIBRARY(f->lib, N);

#define GENERIC_LOAD_FUNC_FINALE(n) \
    return 0;                       \
error:                              \
    n##_free_functions(functions);  \
    return ret;

#define GENERIC_FREE_FUNC()                \
    if (!functions)                        \
        return;                            \
    if (*functions && (*functions)->lib)   \
        FFNV_FREE_FUNC((*functions)->lib); \
    free(*functions);                      \
    *functions = NULL;

#ifdef FFNV_DYNLINK_CUDA_H
typedef struct CudaFunctions {
    tcuInit *cuInit;
    tcuDeviceGetCount *cuDeviceGetCount;
    tcuDeviceGet *cuDeviceGet;
    tcuDeviceGetName *cuDeviceGetName;
    tcuDeviceGetUuid *cuDeviceGetUuid;
    tcuDeviceComputeCapability *cuDeviceComputeCapability;
    tcuCtxCreate_v2 *cuCtxCreate;
    tcuCtxSetLimit *cuCtxSetLimit;
    tcuCtxPushCurrent_v2 *cuCtxPushCurrent;
    tcuCtxPopCurrent_v2 *cuCtxPopCurrent;
    tcuCtxDestroy_v2 *cuCtxDestroy;
    tcuMemAlloc_v2 *cuMemAlloc;
    tcuMemFree_v2 *cuMemFree;
    tcuMemcpy2D_v2 *cuMemcpy2D;
    tcuMemcpy2DAsync_v2 *cuMemcpy2DAsync;
    tcuGetErrorName *cuGetErrorName;
    tcuGetErrorString *cuGetErrorString;

    tcuStreamCreate *cuStreamCreate;
    tcuStreamQuery *cuStreamQuery;
    tcuStreamSynchronize *cuStreamSynchronize;
    tcuStreamDestroy_v2 *cuStreamDestroy;
    tcuStreamAddCallback *cuStreamAddCallback;
    tcuEventCreate *cuEventCreate;
    tcuEventDestroy_v2 *cuEventDestroy;
    tcuEventSynchronize *cuEventSynchronize;
    tcuEventQuery *cuEventQuery;
    tcuEventRecord *cuEventRecord;

    tcuGLGetDevices_v2 *cuGLGetDevices;
    tcuGraphicsGLRegisterImage *cuGraphicsGLRegisterImage;
    tcuGraphicsUnregisterResource *cuGraphicsUnregisterResource;
    tcuGraphicsMapResources *cuGraphicsMapResources;
    tcuGraphicsUnmapResources *cuGraphicsUnmapResources;
    tcuGraphicsSubResourceGetMappedArray *cuGraphicsSubResourceGetMappedArray;

    tcuImportExternalMemory *cuImportExternalMemory;
    tcuDestroyExternalMemory *cuDestroyExternalMemory;
    tcuExternalMemoryGetMappedBuffer *cuExternalMemoryGetMappedBuffer;
    tcuExternalMemoryGetMappedMipmappedArray *cuExternalMemoryGetMappedMipmappedArray;

    tcuMipmappedArrayGetLevel *cuMipmappedArrayGetLevel;

    FFNV_LIB_HANDLE lib;
} CudaFunctions;
#else
typedef struct CudaFunctions CudaFunctions;
#endif

typedef struct CuvidFunctions {
    tcuvidGetDecoderCaps *cuvidGetDecoderCaps;
    tcuvidCreateDecoder *cuvidCreateDecoder;
    tcuvidDestroyDecoder *cuvidDestroyDecoder;
    tcuvidDecodePicture *cuvidDecodePicture;
    tcuvidGetDecodeStatus *cuvidGetDecodeStatus;
    tcuvidReconfigureDecoder *cuvidReconfigureDecoder;
    tcuvidMapVideoFrame *cuvidMapVideoFrame;
    tcuvidUnmapVideoFrame *cuvidUnmapVideoFrame;
    tcuvidCtxLockCreate *cuvidCtxLockCreate;
    tcuvidCtxLockDestroy *cuvidCtxLockDestroy;
    tcuvidCtxLock *cuvidCtxLock;
    tcuvidCtxUnlock *cuvidCtxUnlock;

    tcuvidCreateVideoSource *cuvidCreateVideoSource;
    tcuvidCreateVideoSourceW *cuvidCreateVideoSourceW;
    tcuvidDestroyVideoSource *cuvidDestroyVideoSource;
    tcuvidSetVideoSourceState *cuvidSetVideoSourceState;
    tcuvidGetVideoSourceState *cuvidGetVideoSourceState;
    tcuvidGetSourceVideoFormat *cuvidGetSourceVideoFormat;
    tcuvidGetSourceAudioFormat *cuvidGetSourceAudioFormat;
    tcuvidCreateVideoParser *cuvidCreateVideoParser;
    tcuvidParseVideoData *cuvidParseVideoData;
    tcuvidDestroyVideoParser *cuvidDestroyVideoParser;

    FFNV_LIB_HANDLE lib;
} CuvidFunctions;

typedef NVENCSTATUS NVENCAPI tNvEncodeAPICreateInstance(NV_ENCODE_API_FUNCTION_LIST *functionList);
typedef NVENCSTATUS NVENCAPI tNvEncodeAPIGetMaxSupportedVersion(uint32_t* version);

typedef struct NvencFunctions {
    tNvEncodeAPICreateInstance *NvEncodeAPICreateInstance;
    tNvEncodeAPIGetMaxSupportedVersion *NvEncodeAPIGetMaxSupportedVersion;

    FFNV_LIB_HANDLE lib;
} NvencFunctions;

#ifdef FFNV_DYNLINK_CUDA_H
static inline void cuda_free_functions(CudaFunctions **functions)
{
    GENERIC_FREE_FUNC();
}
#endif

static inline void cuvid_free_functions(CuvidFunctions **functions)
{
    GENERIC_FREE_FUNC();
}

static inline void nvenc_free_functions(NvencFunctions **functions)
{
    GENERIC_FREE_FUNC();
}

#ifdef FFNV_DYNLINK_CUDA_H
static inline int cuda_load_functions(CudaFunctions **functions, void *logctx)
{
    GENERIC_LOAD_FUNC_PREAMBLE(CudaFunctions, cuda, CUDA_LIBNAME);

    LOAD_SYMBOL(cuInit, tcuInit, "cuInit");
    LOAD_SYMBOL(cuDeviceGetCount, tcuDeviceGetCount, "cuDeviceGetCount");
    LOAD_SYMBOL(cuDeviceGet, tcuDeviceGet, "cuDeviceGet");
    LOAD_SYMBOL(cuDeviceGetName, tcuDeviceGetName, "cuDeviceGetName");
    LOAD_SYMBOL(cuDeviceGetUuid, tcuDeviceGetUuid, "cuDeviceGetUuid");
    LOAD_SYMBOL(cuDeviceComputeCapability, tcuDeviceComputeCapability, "cuDeviceComputeCapability");
    LOAD_SYMBOL(cuCtxCreate, tcuCtxCreate_v2, "cuCtxCreate_v2");
    LOAD_SYMBOL(cuCtxSetLimit, tcuCtxSetLimit, "cuCtxSetLimit");
    LOAD_SYMBOL(cuCtxPushCurrent, tcuCtxPushCurrent_v2, "cuCtxPushCurrent_v2");
    LOAD_SYMBOL(cuCtxPopCurrent, tcuCtxPopCurrent_v2, "cuCtxPopCurrent_v2");
    LOAD_SYMBOL(cuCtxDestroy, tcuCtxDestroy_v2, "cuCtxDestroy_v2");
    LOAD_SYMBOL(cuMemAlloc, tcuMemAlloc_v2, "cuMemAlloc_v2");
    LOAD_SYMBOL(cuMemFree, tcuMemFree_v2, "cuMemFree_v2");
    LOAD_SYMBOL(cuMemcpy2D, tcuMemcpy2D_v2, "cuMemcpy2D_v2");
    LOAD_SYMBOL(cuMemcpy2DAsync, tcuMemcpy2DAsync_v2, "cuMemcpy2DAsync_v2");
    LOAD_SYMBOL(cuGetErrorName, tcuGetErrorName, "cuGetErrorName");
    LOAD_SYMBOL(cuGetErrorString, tcuGetErrorString, "cuGetErrorString");

    LOAD_SYMBOL(cuStreamCreate, tcuStreamCreate, "cuStreamCreate");
    LOAD_SYMBOL(cuStreamQuery, tcuStreamQuery, "cuStreamQuery");
    LOAD_SYMBOL(cuStreamSynchronize, tcuStreamSynchronize, "cuStreamSynchronize");
    LOAD_SYMBOL(cuStreamDestroy, tcuStreamDestroy_v2, "cuStreamDestroy_v2");
    LOAD_SYMBOL(cuStreamAddCallback, tcuStreamAddCallback, "cuStreamAddCallback");
    LOAD_SYMBOL(cuEventCreate, tcuEventCreate, "cuEventCreate");
    LOAD_SYMBOL(cuEventDestroy, tcuEventDestroy_v2, "cuEventDestroy_v2");
    LOAD_SYMBOL(cuEventSynchronize, tcuEventSynchronize, "cuEventSynchronize");
    LOAD_SYMBOL(cuEventQuery, tcuEventQuery, "cuEventQuery");
    LOAD_SYMBOL(cuEventRecord, tcuEventRecord, "cuEventRecord");

    LOAD_SYMBOL(cuGLGetDevices, tcuGLGetDevices_v2, "cuGLGetDevices_v2");
    LOAD_SYMBOL(cuGraphicsGLRegisterImage, tcuGraphicsGLRegisterImage, "cuGraphicsGLRegisterImage");
    LOAD_SYMBOL(cuGraphicsUnregisterResource, tcuGraphicsUnregisterResource, "cuGraphicsUnregisterResource");
    LOAD_SYMBOL(cuGraphicsMapResources, tcuGraphicsMapResources, "cuGraphicsMapResources");
    LOAD_SYMBOL(cuGraphicsUnmapResources, tcuGraphicsUnmapResources, "cuGraphicsUnmapResources");
    LOAD_SYMBOL(cuGraphicsSubResourceGetMappedArray, tcuGraphicsSubResourceGetMappedArray, "cuGraphicsSubResourceGetMappedArray");

    LOAD_SYMBOL_OPT(cuImportExternalMemory, tcuImportExternalMemory, "cuImportExternalMemory");
    LOAD_SYMBOL_OPT(cuDestroyExternalMemory, tcuDestroyExternalMemory, "cuDestroyExternalMemory");
    LOAD_SYMBOL_OPT(cuExternalMemoryGetMappedBuffer, tcuExternalMemoryGetMappedBuffer, "cuExternalMemoryGetMappedBuffer");
    LOAD_SYMBOL_OPT(cuExternalMemoryGetMappedMipmappedArray, tcuExternalMemoryGetMappedMipmappedArray, "cuExternalMemoryGetMappedMipmappedArray");
    LOAD_SYMBOL_OPT(cuMipmappedArrayGetLevel, tcuMipmappedArrayGetLevel, "cuMipmappedArrayGetLevel");

    GENERIC_LOAD_FUNC_FINALE(cuda);
}
#endif

static inline int cuvid_load_functions(CuvidFunctions **functions, void *logctx)
{
    GENERIC_LOAD_FUNC_PREAMBLE(CuvidFunctions, cuvid, NVCUVID_LIBNAME);

    LOAD_SYMBOL_OPT(cuvidGetDecoderCaps, tcuvidGetDecoderCaps, "cuvidGetDecoderCaps");
    LOAD_SYMBOL(cuvidCreateDecoder, tcuvidCreateDecoder, "cuvidCreateDecoder");
    LOAD_SYMBOL(cuvidDestroyDecoder, tcuvidDestroyDecoder, "cuvidDestroyDecoder");
    LOAD_SYMBOL(cuvidDecodePicture, tcuvidDecodePicture, "cuvidDecodePicture");
    LOAD_SYMBOL(cuvidGetDecodeStatus, tcuvidGetDecodeStatus, "cuvidGetDecodeStatus");
    LOAD_SYMBOL(cuvidReconfigureDecoder, tcuvidReconfigureDecoder, "cuvidReconfigureDecoder");
#ifdef __CUVID_DEVPTR64
    LOAD_SYMBOL(cuvidMapVideoFrame, tcuvidMapVideoFrame, "cuvidMapVideoFrame64");
    LOAD_SYMBOL(cuvidUnmapVideoFrame, tcuvidUnmapVideoFrame, "cuvidUnmapVideoFrame64");
#else
    LOAD_SYMBOL(cuvidMapVideoFrame, tcuvidMapVideoFrame, "cuvidMapVideoFrame");
    LOAD_SYMBOL(cuvidUnmapVideoFrame, tcuvidUnmapVideoFrame, "cuvidUnmapVideoFrame");
#endif
    LOAD_SYMBOL(cuvidCtxLockCreate, tcuvidCtxLockCreate, "cuvidCtxLockCreate");
    LOAD_SYMBOL(cuvidCtxLockDestroy, tcuvidCtxLockDestroy, "cuvidCtxLockDestroy");
    LOAD_SYMBOL(cuvidCtxLock, tcuvidCtxLock, "cuvidCtxLock");
    LOAD_SYMBOL(cuvidCtxUnlock, tcuvidCtxUnlock, "cuvidCtxUnlock");

    LOAD_SYMBOL(cuvidCreateVideoSource, tcuvidCreateVideoSource, "cuvidCreateVideoSource");
    LOAD_SYMBOL(cuvidCreateVideoSourceW, tcuvidCreateVideoSourceW, "cuvidCreateVideoSourceW");
    LOAD_SYMBOL(cuvidDestroyVideoSource, tcuvidDestroyVideoSource, "cuvidDestroyVideoSource");
    LOAD_SYMBOL(cuvidSetVideoSourceState, tcuvidSetVideoSourceState, "cuvidSetVideoSourceState");
    LOAD_SYMBOL(cuvidGetVideoSourceState, tcuvidGetVideoSourceState, "cuvidGetVideoSourceState");
    LOAD_SYMBOL(cuvidGetSourceVideoFormat, tcuvidGetSourceVideoFormat, "cuvidGetSourceVideoFormat");
    LOAD_SYMBOL(cuvidGetSourceAudioFormat, tcuvidGetSourceAudioFormat, "cuvidGetSourceAudioFormat");
    LOAD_SYMBOL(cuvidCreateVideoParser, tcuvidCreateVideoParser, "cuvidCreateVideoParser");
    LOAD_SYMBOL(cuvidParseVideoData, tcuvidParseVideoData, "cuvidParseVideoData");
    LOAD_SYMBOL(cuvidDestroyVideoParser, tcuvidDestroyVideoParser, "cuvidDestroyVideoParser");

    GENERIC_LOAD_FUNC_FINALE(cuvid);
}

static inline int nvenc_load_functions(NvencFunctions **functions, void *logctx)
{
    GENERIC_LOAD_FUNC_PREAMBLE(NvencFunctions, nvenc, NVENC_LIBNAME);

    LOAD_SYMBOL(NvEncodeAPICreateInstance, tNvEncodeAPICreateInstance, "NvEncodeAPICreateInstance");
    LOAD_SYMBOL(NvEncodeAPIGetMaxSupportedVersion, tNvEncodeAPIGetMaxSupportedVersion, "NvEncodeAPIGetMaxSupportedVersion");

    GENERIC_LOAD_FUNC_FINALE(nvenc);
}

#undef GENERIC_LOAD_FUNC_PREAMBLE
#undef LOAD_LIBRARY
#undef LOAD_SYMBOL
#undef GENERIC_LOAD_FUNC_FINALE
#undef GENERIC_FREE_FUNC
#undef CUDA_LIBNAME
#undef NVCUVID_LIBNAME
#undef NVENC_LIBNAME

#endif

