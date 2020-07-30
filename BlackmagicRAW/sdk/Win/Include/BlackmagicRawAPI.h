

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.00.0603 */
/* at Fri Jul 24 22:42:27 2020
 */
/* Compiler settings for BlackmagicRawAPI.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.00.0603 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__


#ifndef __BlackmagickRawAPI_h__
#define __BlackmagickRawAPI_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IBlackmagicRaw_FWD_DEFINED__
#define __IBlackmagicRaw_FWD_DEFINED__
typedef interface IBlackmagicRaw IBlackmagicRaw;

#endif 	/* __IBlackmagicRaw_FWD_DEFINED__ */


#ifndef __IBlackmagicRawFactory_FWD_DEFINED__
#define __IBlackmagicRawFactory_FWD_DEFINED__
typedef interface IBlackmagicRawFactory IBlackmagicRawFactory;

#endif 	/* __IBlackmagicRawFactory_FWD_DEFINED__ */


#ifndef __IBlackmagicRawPipelineIterator_FWD_DEFINED__
#define __IBlackmagicRawPipelineIterator_FWD_DEFINED__
typedef interface IBlackmagicRawPipelineIterator IBlackmagicRawPipelineIterator;

#endif 	/* __IBlackmagicRawPipelineIterator_FWD_DEFINED__ */


#ifndef __IBlackmagicRawPipelineDeviceIterator_FWD_DEFINED__
#define __IBlackmagicRawPipelineDeviceIterator_FWD_DEFINED__
typedef interface IBlackmagicRawPipelineDeviceIterator IBlackmagicRawPipelineDeviceIterator;

#endif 	/* __IBlackmagicRawPipelineDeviceIterator_FWD_DEFINED__ */


#ifndef __IBlackmagicRawOpenGLInteropHelper_FWD_DEFINED__
#define __IBlackmagicRawOpenGLInteropHelper_FWD_DEFINED__
typedef interface IBlackmagicRawOpenGLInteropHelper IBlackmagicRawOpenGLInteropHelper;

#endif 	/* __IBlackmagicRawOpenGLInteropHelper_FWD_DEFINED__ */


#ifndef __IBlackmagicRawPipelineDevice_FWD_DEFINED__
#define __IBlackmagicRawPipelineDevice_FWD_DEFINED__
typedef interface IBlackmagicRawPipelineDevice IBlackmagicRawPipelineDevice;

#endif 	/* __IBlackmagicRawPipelineDevice_FWD_DEFINED__ */


#ifndef __IBlackmagicRawToneCurve_FWD_DEFINED__
#define __IBlackmagicRawToneCurve_FWD_DEFINED__
typedef interface IBlackmagicRawToneCurve IBlackmagicRawToneCurve;

#endif 	/* __IBlackmagicRawToneCurve_FWD_DEFINED__ */


#ifndef __IBlackmagicRawConstants_FWD_DEFINED__
#define __IBlackmagicRawConstants_FWD_DEFINED__
typedef interface IBlackmagicRawConstants IBlackmagicRawConstants;

#endif 	/* __IBlackmagicRawConstants_FWD_DEFINED__ */


#ifndef __IBlackmagicRawConfiguration_FWD_DEFINED__
#define __IBlackmagicRawConfiguration_FWD_DEFINED__
typedef interface IBlackmagicRawConfiguration IBlackmagicRawConfiguration;

#endif 	/* __IBlackmagicRawConfiguration_FWD_DEFINED__ */


#ifndef __IBlackmagicRawConfigurationEx_FWD_DEFINED__
#define __IBlackmagicRawConfigurationEx_FWD_DEFINED__
typedef interface IBlackmagicRawConfigurationEx IBlackmagicRawConfigurationEx;

#endif 	/* __IBlackmagicRawConfigurationEx_FWD_DEFINED__ */


#ifndef __IBlackmagicRawResourceManager_FWD_DEFINED__
#define __IBlackmagicRawResourceManager_FWD_DEFINED__
typedef interface IBlackmagicRawResourceManager IBlackmagicRawResourceManager;

#endif 	/* __IBlackmagicRawResourceManager_FWD_DEFINED__ */


#ifndef __IBlackmagicRawMetadataIterator_FWD_DEFINED__
#define __IBlackmagicRawMetadataIterator_FWD_DEFINED__
typedef interface IBlackmagicRawMetadataIterator IBlackmagicRawMetadataIterator;

#endif 	/* __IBlackmagicRawMetadataIterator_FWD_DEFINED__ */


#ifndef __IBlackmagicRawClipProcessingAttributes_FWD_DEFINED__
#define __IBlackmagicRawClipProcessingAttributes_FWD_DEFINED__
typedef interface IBlackmagicRawClipProcessingAttributes IBlackmagicRawClipProcessingAttributes;

#endif 	/* __IBlackmagicRawClipProcessingAttributes_FWD_DEFINED__ */


#ifndef __IBlackmagicRawPost3DLUT_FWD_DEFINED__
#define __IBlackmagicRawPost3DLUT_FWD_DEFINED__
typedef interface IBlackmagicRawPost3DLUT IBlackmagicRawPost3DLUT;

#endif 	/* __IBlackmagicRawPost3DLUT_FWD_DEFINED__ */


#ifndef __IBlackmagicRawFrameProcessingAttributes_FWD_DEFINED__
#define __IBlackmagicRawFrameProcessingAttributes_FWD_DEFINED__
typedef interface IBlackmagicRawFrameProcessingAttributes IBlackmagicRawFrameProcessingAttributes;

#endif 	/* __IBlackmagicRawFrameProcessingAttributes_FWD_DEFINED__ */


#ifndef __IBlackmagicRawProcessedImage_FWD_DEFINED__
#define __IBlackmagicRawProcessedImage_FWD_DEFINED__
typedef interface IBlackmagicRawProcessedImage IBlackmagicRawProcessedImage;

#endif 	/* __IBlackmagicRawProcessedImage_FWD_DEFINED__ */


#ifndef __IBlackmagicRawJob_FWD_DEFINED__
#define __IBlackmagicRawJob_FWD_DEFINED__
typedef interface IBlackmagicRawJob IBlackmagicRawJob;

#endif 	/* __IBlackmagicRawJob_FWD_DEFINED__ */


#ifndef __IBlackmagicRawCallback_FWD_DEFINED__
#define __IBlackmagicRawCallback_FWD_DEFINED__
typedef interface IBlackmagicRawCallback IBlackmagicRawCallback;

#endif 	/* __IBlackmagicRawCallback_FWD_DEFINED__ */


#ifndef __IBlackmagicRawClipAudio_FWD_DEFINED__
#define __IBlackmagicRawClipAudio_FWD_DEFINED__
typedef interface IBlackmagicRawClipAudio IBlackmagicRawClipAudio;

#endif 	/* __IBlackmagicRawClipAudio_FWD_DEFINED__ */


#ifndef __IBlackmagicRawFrame_FWD_DEFINED__
#define __IBlackmagicRawFrame_FWD_DEFINED__
typedef interface IBlackmagicRawFrame IBlackmagicRawFrame;

#endif 	/* __IBlackmagicRawFrame_FWD_DEFINED__ */


#ifndef __IBlackmagicRawFrameEx_FWD_DEFINED__
#define __IBlackmagicRawFrameEx_FWD_DEFINED__
typedef interface IBlackmagicRawFrameEx IBlackmagicRawFrameEx;

#endif 	/* __IBlackmagicRawFrameEx_FWD_DEFINED__ */


#ifndef __IBlackmagicRawManualDecoderFlow1_FWD_DEFINED__
#define __IBlackmagicRawManualDecoderFlow1_FWD_DEFINED__
typedef interface IBlackmagicRawManualDecoderFlow1 IBlackmagicRawManualDecoderFlow1;

#endif 	/* __IBlackmagicRawManualDecoderFlow1_FWD_DEFINED__ */


#ifndef __IBlackmagicRawManualDecoderFlow2_FWD_DEFINED__
#define __IBlackmagicRawManualDecoderFlow2_FWD_DEFINED__
typedef interface IBlackmagicRawManualDecoderFlow2 IBlackmagicRawManualDecoderFlow2;

#endif 	/* __IBlackmagicRawManualDecoderFlow2_FWD_DEFINED__ */


#ifndef __IBlackmagicRawClip_FWD_DEFINED__
#define __IBlackmagicRawClip_FWD_DEFINED__
typedef interface IBlackmagicRawClip IBlackmagicRawClip;

#endif 	/* __IBlackmagicRawClip_FWD_DEFINED__ */


#ifndef __IBlackmagicRawClipEx_FWD_DEFINED__
#define __IBlackmagicRawClipEx_FWD_DEFINED__
typedef interface IBlackmagicRawClipEx IBlackmagicRawClipEx;

#endif 	/* __IBlackmagicRawClipEx_FWD_DEFINED__ */


#ifndef __CBlackmagicRawFactory_FWD_DEFINED__
#define __CBlackmagicRawFactory_FWD_DEFINED__

#ifdef __cplusplus
typedef class CBlackmagicRawFactory CBlackmagicRawFactory;
#else
typedef struct CBlackmagicRawFactory CBlackmagicRawFactory;
#endif /* __cplusplus */

#endif 	/* __CBlackmagicRawFactory_FWD_DEFINED__ */


/* header files for imported files */
#include "unknwn.h"

#ifdef __cplusplus
extern "C"{
#endif 



#ifndef __BlackmagicRawAPI_LIBRARY_DEFINED__
#define __BlackmagicRawAPI_LIBRARY_DEFINED__

/* library BlackmagicRawAPI */
/* [helpstring][version][uuid] */ 

typedef SAFEARRAY SafeArray;

typedef SAFEARRAYBOUND SafeArrayBound;

#if 0
#endif
typedef /* [v1_enum] */ 
enum _BlackmagicRawVariantType
    {
        blackmagicRawVariantTypeEmpty	= VT_EMPTY,
        blackmagicRawVariantTypeU8	= VT_UI1,
        blackmagicRawVariantTypeS16	= VT_I2,
        blackmagicRawVariantTypeU16	= VT_UI2,
        blackmagicRawVariantTypeS32	= VT_I4,
        blackmagicRawVariantTypeU32	= VT_UI4,
        blackmagicRawVariantTypeFloat32	= VT_R4,
        blackmagicRawVariantTypeString	= VT_BSTR,
        blackmagicRawVariantTypeSafeArray	= VT_SAFEARRAY
    } 	BlackmagicRawVariantType;

typedef /* [v1_enum] */ 
enum _BlackmagicRawResourceType
    {
        blackmagicRawResourceTypeBufferCPU	= 0x63707562,
        blackmagicRawResourceTypeBufferMetal	= 0x6d657462,
        blackmagicRawResourceTypeBufferCUDA	= 0x63756462,
        blackmagicRawResourceTypeBufferOpenCL	= 0x6f636c62
    } 	BlackmagicRawResourceType;

typedef /* [v1_enum] */ 
enum _BlackmagicRawResourceFormat
    {
        blackmagicRawResourceFormatRGBAU8	= 0x72676261,
        blackmagicRawResourceFormatBGRAU8	= 0x62677261,
        blackmagicRawResourceFormatRGBU16	= 0x3136696c,
        blackmagicRawResourceFormatRGBAU16	= 0x3136616c,
        blackmagicRawResourceFormatBGRAU16	= 0x31366c61,
        blackmagicRawResourceFormatRGBU16Planar	= 0x3136706c,
        blackmagicRawResourceFormatRGBF32	= 0x66333273,
        blackmagicRawResourceFormatRGBF32Planar	= 0x66333270,
        blackmagicRawResourceFormatBGRAF32	= 0x66333261
    } 	BlackmagicRawResourceFormat;

typedef /* [v1_enum] */ 
enum _BlackmagicRawResourceUsage
    {
        blackmagicRawResourceUsageReadCPUWriteCPU	= 0x72637763,
        blackmagicRawResourceUsageReadGPUWriteGPU	= 0x72677767,
        blackmagicRawResourceUsageReadGPUWriteCPU	= 0x72677763,
        blackmagicRawResourceUsageReadCPUWriteGPU	= 0x72637767
    } 	BlackmagicRawResourceUsage;

typedef /* [v1_enum] */ 
enum _BlackmagicRawPipeline
    {
        blackmagicRawPipelineCPU	= 0x63707562,
        blackmagicRawPipelineCUDA	= 0x63756461,
        blackmagicRawPipelineMetal	= 0x6d65746c,
        blackmagicRawPipelineOpenCL	= 0x6f70636c
    } 	BlackmagicRawPipeline;

typedef /* [v1_enum] */ 
enum _BlackmagicRawInstructionSet
    {
        blackmagicRawInstructionSetSSE41	= 0x73653431,
        blackmagicRawInstructionSetAVX	= 0x6176785f,
        blackmagicRawInstructionSetAVX2	= 0x61767832
    } 	BlackmagicRawInstructionSet;

typedef /* [v1_enum] */ 
enum _BlackmagicRawAudioFormat
    {
        blackmagicRawAudioFormatPCMLittleEndian	= 0x70636d6c
    } 	BlackmagicRawAudioFormat;

typedef /* [v1_enum] */ 
enum _BlackmagicRawResolutionScale
    {
        blackmagicRawResolutionScaleFull	= 0x66756c6c,
        blackmagicRawResolutionScaleHalf	= 0x68616c66,
        blackmagicRawResolutionScaleQuarter	= 0x71727472,
        blackmagicRawResolutionScaleEighth	= 0x65697468,
        blackmagicRawResolutionScaleFullUpsideDown	= 0x6c6c7566,
        blackmagicRawResolutionScaleHalfUpsideDown	= 0x666c6168,
        blackmagicRawResolutionScaleQuarterUpsideDown	= 0x72747271,
        blackmagicRawResolutionScaleEighthUpsideDown	= 0x68746965
    } 	BlackmagicRawResolutionScale;

typedef /* [v1_enum] */ 
enum _BlackmagicRawClipProcessingAttribute
    {
        blackmagicRawClipProcessingAttributeColorScienceGen	= 0x6373676e,
        blackmagicRawClipProcessingAttributeGamma	= 0x67616d61,
        blackmagicRawClipProcessingAttributeGamut	= 0x67616d74,
        blackmagicRawClipProcessingAttributeToneCurveContrast	= 0x74636f6e,
        blackmagicRawClipProcessingAttributeToneCurveSaturation	= 0x74736174,
        blackmagicRawClipProcessingAttributeToneCurveMidpoint	= 0x746d6964,
        blackmagicRawClipProcessingAttributeToneCurveHighlights	= 0x74686968,
        blackmagicRawClipProcessingAttributeToneCurveShadows	= 0x74736861,
        blackmagicRawClipProcessingAttributeToneCurveVideoBlackLevel	= 0x7476626c,
        blackmagicRawClipProcessingAttributeToneCurveBlackLevel	= 0x74626c6b,
        blackmagicRawClipProcessingAttributeToneCurveWhiteLevel	= 0x74776974,
        blackmagicRawClipProcessingAttributeHighlightRecovery	= 0x686c7279,
        blackmagicRawClipProcessingAttributeAnalogGainIsConstant	= 0x61676963,
        blackmagicRawClipProcessingAttributeAnalogGain	= 0x6761696e,
        blackmagicRawClipProcessingAttributePost3DLUTMode	= 0x6c75746d,
        blackmagicRawClipProcessingAttributeEmbeddedPost3DLUTName	= 0x656d6c6e,
        blackmagicRawClipProcessingAttributeEmbeddedPost3DLUTTitle	= 0x656d6c74,
        blackmagicRawClipProcessingAttributeEmbeddedPost3DLUTSize	= 0x656d6c73,
        blackmagicRawClipProcessingAttributeEmbeddedPost3DLUTData	= 0x656d6c64,
        blackmagicRawClipProcessingAttributeSidecarPost3DLUTName	= 0x73636c6e,
        blackmagicRawClipProcessingAttributeSidecarPost3DLUTTitle	= 0x73636c74,
        blackmagicRawClipProcessingAttributeSidecarPost3DLUTSize	= 0x73636c73,
        blackmagicRawClipProcessingAttributeSidecarPost3DLUTData	= 0x73636c64
    } 	BlackmagicRawClipProcessingAttribute;

typedef /* [v1_enum] */ 
enum _BlackmagicRawFrameProcessingAttribute
    {
        blackmagicRawFrameProcessingAttributeWhiteBalanceKelvin	= 0x77626b76,
        blackmagicRawFrameProcessingAttributeWhiteBalanceTint	= 0x7762746e,
        blackmagicRawFrameProcessingAttributeExposure	= 0x6578706f,
        blackmagicRawFrameProcessingAttributeISO	= 0x6669736f,
        blackmagicRawFrameProcessingAttributeAnalogGain	= 0x61677066
    } 	BlackmagicRawFrameProcessingAttribute;

typedef /* [v1_enum] */ 
enum _BlackmagicRawInterop
    {
        blackmagicRawInteropNone	= 0x6e6f6e65,
        blackmagicRawInteropOpenGL	= 0x6f70676c
    } 	BlackmagicRawInterop;



























EXTERN_C const IID LIBID_BlackmagicRawAPI;

#ifndef __IBlackmagicRaw_INTERFACE_DEFINED__
#define __IBlackmagicRaw_INTERFACE_DEFINED__

/* interface IBlackmagicRaw */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRaw;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("5A540A06-1B62-4224-ACB0-A2385C6ED649")
    IBlackmagicRaw : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OpenClip( 
            /* [in] */ BSTR fileName,
            /* [out] */ IBlackmagicRawClip **clip) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCallback( 
            /* [in] */ IBlackmagicRawCallback *callback) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PreparePipeline( 
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ void *pipelineContext,
            /* [in] */ void *pipelineCommandQueue,
            /* [in] */ void *userData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE PreparePipelineForDevice( 
            /* [in] */ IBlackmagicRawPipelineDevice *pipelineDevice,
            /* [in] */ void *userData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE FlushJobs( void) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRaw * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRaw * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRaw * This);
        
        HRESULT ( STDMETHODCALLTYPE *OpenClip )( 
            IBlackmagicRaw * This,
            /* [in] */ BSTR fileName,
            /* [out] */ IBlackmagicRawClip **clip);
        
        HRESULT ( STDMETHODCALLTYPE *SetCallback )( 
            IBlackmagicRaw * This,
            /* [in] */ IBlackmagicRawCallback *callback);
        
        HRESULT ( STDMETHODCALLTYPE *PreparePipeline )( 
            IBlackmagicRaw * This,
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ void *pipelineContext,
            /* [in] */ void *pipelineCommandQueue,
            /* [in] */ void *userData);
        
        HRESULT ( STDMETHODCALLTYPE *PreparePipelineForDevice )( 
            IBlackmagicRaw * This,
            /* [in] */ IBlackmagicRawPipelineDevice *pipelineDevice,
            /* [in] */ void *userData);
        
        HRESULT ( STDMETHODCALLTYPE *FlushJobs )( 
            IBlackmagicRaw * This);
        
        END_INTERFACE
    } IBlackmagicRawVtbl;

    interface IBlackmagicRaw
    {
        CONST_VTBL struct IBlackmagicRawVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRaw_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRaw_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRaw_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRaw_OpenClip(This,fileName,clip)	\
    ( (This)->lpVtbl -> OpenClip(This,fileName,clip) ) 

#define IBlackmagicRaw_SetCallback(This,callback)	\
    ( (This)->lpVtbl -> SetCallback(This,callback) ) 

#define IBlackmagicRaw_PreparePipeline(This,pipeline,pipelineContext,pipelineCommandQueue,userData)	\
    ( (This)->lpVtbl -> PreparePipeline(This,pipeline,pipelineContext,pipelineCommandQueue,userData) ) 

#define IBlackmagicRaw_PreparePipelineForDevice(This,pipelineDevice,userData)	\
    ( (This)->lpVtbl -> PreparePipelineForDevice(This,pipelineDevice,userData) ) 

#define IBlackmagicRaw_FlushJobs(This)	\
    ( (This)->lpVtbl -> FlushJobs(This) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRaw_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawFactory_INTERFACE_DEFINED__
#define __IBlackmagicRawFactory_INTERFACE_DEFINED__

/* interface IBlackmagicRawFactory */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("74FEBEDC-12D6-490D-9A77-48F19E8F60CB")
    IBlackmagicRawFactory : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateCodec( 
            /* [out] */ IBlackmagicRaw **codec) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePipelineIterator( 
            /* [in] */ BlackmagicRawInterop interop,
            /* [out] */ IBlackmagicRawPipelineIterator **pipelineIterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreatePipelineDeviceIterator( 
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ BlackmagicRawInterop interop,
            /* [out] */ IBlackmagicRawPipelineDeviceIterator **deviceIterator) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawFactoryVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawFactory * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawFactory * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawFactory * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateCodec )( 
            IBlackmagicRawFactory * This,
            /* [out] */ IBlackmagicRaw **codec);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePipelineIterator )( 
            IBlackmagicRawFactory * This,
            /* [in] */ BlackmagicRawInterop interop,
            /* [out] */ IBlackmagicRawPipelineIterator **pipelineIterator);
        
        HRESULT ( STDMETHODCALLTYPE *CreatePipelineDeviceIterator )( 
            IBlackmagicRawFactory * This,
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ BlackmagicRawInterop interop,
            /* [out] */ IBlackmagicRawPipelineDeviceIterator **deviceIterator);
        
        END_INTERFACE
    } IBlackmagicRawFactoryVtbl;

    interface IBlackmagicRawFactory
    {
        CONST_VTBL struct IBlackmagicRawFactoryVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawFactory_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawFactory_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawFactory_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawFactory_CreateCodec(This,codec)	\
    ( (This)->lpVtbl -> CreateCodec(This,codec) ) 

#define IBlackmagicRawFactory_CreatePipelineIterator(This,interop,pipelineIterator)	\
    ( (This)->lpVtbl -> CreatePipelineIterator(This,interop,pipelineIterator) ) 

#define IBlackmagicRawFactory_CreatePipelineDeviceIterator(This,pipeline,interop,deviceIterator)	\
    ( (This)->lpVtbl -> CreatePipelineDeviceIterator(This,pipeline,interop,deviceIterator) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawFactory_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawPipelineIterator_INTERFACE_DEFINED__
#define __IBlackmagicRawPipelineIterator_INTERFACE_DEFINED__

/* interface IBlackmagicRawPipelineIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawPipelineIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("051ED792-3D9D-4ED0-BB1F-3873E08773CB")
    IBlackmagicRawPipelineIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *pipelineName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterop( 
            /* [out] */ BlackmagicRawInterop *interop) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPipeline( 
            /* [out] */ BlackmagicRawPipeline *pipeline) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawPipelineIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawPipelineIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawPipelineIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawPipelineIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IBlackmagicRawPipelineIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IBlackmagicRawPipelineIterator * This,
            /* [out] */ BSTR *pipelineName);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterop )( 
            IBlackmagicRawPipelineIterator * This,
            /* [out] */ BlackmagicRawInterop *interop);
        
        HRESULT ( STDMETHODCALLTYPE *GetPipeline )( 
            IBlackmagicRawPipelineIterator * This,
            /* [out] */ BlackmagicRawPipeline *pipeline);
        
        END_INTERFACE
    } IBlackmagicRawPipelineIteratorVtbl;

    interface IBlackmagicRawPipelineIterator
    {
        CONST_VTBL struct IBlackmagicRawPipelineIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawPipelineIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawPipelineIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawPipelineIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawPipelineIterator_Next(This)	\
    ( (This)->lpVtbl -> Next(This) ) 

#define IBlackmagicRawPipelineIterator_GetName(This,pipelineName)	\
    ( (This)->lpVtbl -> GetName(This,pipelineName) ) 

#define IBlackmagicRawPipelineIterator_GetInterop(This,interop)	\
    ( (This)->lpVtbl -> GetInterop(This,interop) ) 

#define IBlackmagicRawPipelineIterator_GetPipeline(This,pipeline)	\
    ( (This)->lpVtbl -> GetPipeline(This,pipeline) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawPipelineIterator_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawPipelineDeviceIterator_INTERFACE_DEFINED__
#define __IBlackmagicRawPipelineDeviceIterator_INTERFACE_DEFINED__

/* interface IBlackmagicRawPipelineDeviceIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawPipelineDeviceIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("32D3385F-06EE-4260-82EB-2BABFFFACED8")
    IBlackmagicRawPipelineDeviceIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPipeline( 
            /* [out] */ BlackmagicRawPipeline *pipeline) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterop( 
            /* [out] */ BlackmagicRawInterop *interop) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateDevice( 
            /* [out] */ IBlackmagicRawPipelineDevice **pipelineDevice) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawPipelineDeviceIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawPipelineDeviceIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawPipelineDeviceIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawPipelineDeviceIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IBlackmagicRawPipelineDeviceIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPipeline )( 
            IBlackmagicRawPipelineDeviceIterator * This,
            /* [out] */ BlackmagicRawPipeline *pipeline);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterop )( 
            IBlackmagicRawPipelineDeviceIterator * This,
            /* [out] */ BlackmagicRawInterop *interop);
        
        HRESULT ( STDMETHODCALLTYPE *CreateDevice )( 
            IBlackmagicRawPipelineDeviceIterator * This,
            /* [out] */ IBlackmagicRawPipelineDevice **pipelineDevice);
        
        END_INTERFACE
    } IBlackmagicRawPipelineDeviceIteratorVtbl;

    interface IBlackmagicRawPipelineDeviceIterator
    {
        CONST_VTBL struct IBlackmagicRawPipelineDeviceIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawPipelineDeviceIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawPipelineDeviceIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawPipelineDeviceIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawPipelineDeviceIterator_Next(This)	\
    ( (This)->lpVtbl -> Next(This) ) 

#define IBlackmagicRawPipelineDeviceIterator_GetPipeline(This,pipeline)	\
    ( (This)->lpVtbl -> GetPipeline(This,pipeline) ) 

#define IBlackmagicRawPipelineDeviceIterator_GetInterop(This,interop)	\
    ( (This)->lpVtbl -> GetInterop(This,interop) ) 

#define IBlackmagicRawPipelineDeviceIterator_CreateDevice(This,pipelineDevice)	\
    ( (This)->lpVtbl -> CreateDevice(This,pipelineDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawPipelineDeviceIterator_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawOpenGLInteropHelper_INTERFACE_DEFINED__
#define __IBlackmagicRawOpenGLInteropHelper_INTERFACE_DEFINED__

/* interface IBlackmagicRawOpenGLInteropHelper */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawOpenGLInteropHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86444C8A-4398-4364-9166-7D10F41C315E")
    IBlackmagicRawOpenGLInteropHelper : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetPreferredResourceFormat( 
            /* [out] */ BlackmagicRawResourceFormat *preferredFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetImage( 
            /* [in] */ IBlackmagicRawProcessedImage *processedImage,
            /* [out] */ unsigned int *openGLTextureName,
            /* [out] */ int *openGLTextureTarget) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawOpenGLInteropHelperVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawOpenGLInteropHelper * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawOpenGLInteropHelper * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawOpenGLInteropHelper * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetPreferredResourceFormat )( 
            IBlackmagicRawOpenGLInteropHelper * This,
            /* [out] */ BlackmagicRawResourceFormat *preferredFormat);
        
        HRESULT ( STDMETHODCALLTYPE *SetImage )( 
            IBlackmagicRawOpenGLInteropHelper * This,
            /* [in] */ IBlackmagicRawProcessedImage *processedImage,
            /* [out] */ unsigned int *openGLTextureName,
            /* [out] */ int *openGLTextureTarget);
        
        END_INTERFACE
    } IBlackmagicRawOpenGLInteropHelperVtbl;

    interface IBlackmagicRawOpenGLInteropHelper
    {
        CONST_VTBL struct IBlackmagicRawOpenGLInteropHelperVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawOpenGLInteropHelper_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawOpenGLInteropHelper_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawOpenGLInteropHelper_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawOpenGLInteropHelper_GetPreferredResourceFormat(This,preferredFormat)	\
    ( (This)->lpVtbl -> GetPreferredResourceFormat(This,preferredFormat) ) 

#define IBlackmagicRawOpenGLInteropHelper_SetImage(This,processedImage,openGLTextureName,openGLTextureTarget)	\
    ( (This)->lpVtbl -> SetImage(This,processedImage,openGLTextureName,openGLTextureTarget) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawOpenGLInteropHelper_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawPipelineDevice_INTERFACE_DEFINED__
#define __IBlackmagicRawPipelineDevice_INTERFACE_DEFINED__

/* interface IBlackmagicRawPipelineDevice */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawPipelineDevice;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2B0D350D-8C17-431F-88AD-1E7945DF2F9F")
    IBlackmagicRawPipelineDevice : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetBestInstructionSet( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInstructionSet( 
            /* [in] */ BlackmagicRawInstructionSet instructionSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstructionSet( 
            /* [out] */ BlackmagicRawInstructionSet *instructionSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetIndex( 
            /* [out] */ unsigned int *deviceIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *deviceName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInterop( 
            /* [out] */ BlackmagicRawInterop *interop) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPipeline( 
            /* [out] */ BlackmagicRawPipeline *pipeline,
            /* [out] */ void **context,
            /* [out] */ void **commandQueue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPipelineName( 
            /* [out] */ BSTR *pipelineName) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetOpenGLInteropHelper( 
            /* [out] */ IBlackmagicRawOpenGLInteropHelper **interopHelper) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawPipelineDeviceVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawPipelineDevice * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawPipelineDevice * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawPipelineDevice * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetBestInstructionSet )( 
            IBlackmagicRawPipelineDevice * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetInstructionSet )( 
            IBlackmagicRawPipelineDevice * This,
            /* [in] */ BlackmagicRawInstructionSet instructionSet);
        
        HRESULT ( STDMETHODCALLTYPE *GetInstructionSet )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ BlackmagicRawInstructionSet *instructionSet);
        
        HRESULT ( STDMETHODCALLTYPE *GetIndex )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ unsigned int *deviceIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ BSTR *deviceName);
        
        HRESULT ( STDMETHODCALLTYPE *GetInterop )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ BlackmagicRawInterop *interop);
        
        HRESULT ( STDMETHODCALLTYPE *GetPipeline )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ BlackmagicRawPipeline *pipeline,
            /* [out] */ void **context,
            /* [out] */ void **commandQueue);
        
        HRESULT ( STDMETHODCALLTYPE *GetPipelineName )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ BSTR *pipelineName);
        
        HRESULT ( STDMETHODCALLTYPE *GetOpenGLInteropHelper )( 
            IBlackmagicRawPipelineDevice * This,
            /* [out] */ IBlackmagicRawOpenGLInteropHelper **interopHelper);
        
        END_INTERFACE
    } IBlackmagicRawPipelineDeviceVtbl;

    interface IBlackmagicRawPipelineDevice
    {
        CONST_VTBL struct IBlackmagicRawPipelineDeviceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawPipelineDevice_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawPipelineDevice_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawPipelineDevice_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawPipelineDevice_SetBestInstructionSet(This)	\
    ( (This)->lpVtbl -> SetBestInstructionSet(This) ) 

#define IBlackmagicRawPipelineDevice_SetInstructionSet(This,instructionSet)	\
    ( (This)->lpVtbl -> SetInstructionSet(This,instructionSet) ) 

#define IBlackmagicRawPipelineDevice_GetInstructionSet(This,instructionSet)	\
    ( (This)->lpVtbl -> GetInstructionSet(This,instructionSet) ) 

#define IBlackmagicRawPipelineDevice_GetIndex(This,deviceIndex)	\
    ( (This)->lpVtbl -> GetIndex(This,deviceIndex) ) 

#define IBlackmagicRawPipelineDevice_GetName(This,deviceName)	\
    ( (This)->lpVtbl -> GetName(This,deviceName) ) 

#define IBlackmagicRawPipelineDevice_GetInterop(This,interop)	\
    ( (This)->lpVtbl -> GetInterop(This,interop) ) 

#define IBlackmagicRawPipelineDevice_GetPipeline(This,pipeline,context,commandQueue)	\
    ( (This)->lpVtbl -> GetPipeline(This,pipeline,context,commandQueue) ) 

#define IBlackmagicRawPipelineDevice_GetPipelineName(This,pipelineName)	\
    ( (This)->lpVtbl -> GetPipelineName(This,pipelineName) ) 

#define IBlackmagicRawPipelineDevice_GetOpenGLInteropHelper(This,interopHelper)	\
    ( (This)->lpVtbl -> GetOpenGLInteropHelper(This,interopHelper) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawPipelineDevice_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawToneCurve_INTERFACE_DEFINED__
#define __IBlackmagicRawToneCurve_INTERFACE_DEFINED__

/* interface IBlackmagicRawToneCurve */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawToneCurve;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("7E40C13D-3575-46B5-B2B7-85DAE1EEFD77")
    IBlackmagicRawToneCurve : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetToneCurve( 
            /* [in] */ BSTR cameraType,
            /* [in] */ BSTR gamma,
            /* [in] */ unsigned short gen,
            /* [out] */ float *contrast,
            /* [out] */ float *saturation,
            /* [out] */ float *midpoint,
            /* [out] */ float *highlights,
            /* [out] */ float *shadows,
            /* [out] */ float *blackLevel,
            /* [out] */ float *whiteLevel,
            /* [out] */ unsigned short *videoBlackLevel) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE EvaluateToneCurve( 
            /* [in] */ BSTR cameraType,
            /* [in] */ unsigned short gen,
            /* [in] */ float contrast,
            /* [in] */ float saturation,
            /* [in] */ float midpoint,
            /* [in] */ float highlights,
            /* [in] */ float shadows,
            /* [in] */ float blackLevel,
            /* [in] */ float whiteLevel,
            /* [in] */ unsigned short videoBlackLevel,
            /* [out] */ float *array,
            /* [in] */ unsigned int arrayElementCount) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawToneCurveVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawToneCurve * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawToneCurve * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawToneCurve * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetToneCurve )( 
            IBlackmagicRawToneCurve * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ BSTR gamma,
            /* [in] */ unsigned short gen,
            /* [out] */ float *contrast,
            /* [out] */ float *saturation,
            /* [out] */ float *midpoint,
            /* [out] */ float *highlights,
            /* [out] */ float *shadows,
            /* [out] */ float *blackLevel,
            /* [out] */ float *whiteLevel,
            /* [out] */ unsigned short *videoBlackLevel);
        
        HRESULT ( STDMETHODCALLTYPE *EvaluateToneCurve )( 
            IBlackmagicRawToneCurve * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ unsigned short gen,
            /* [in] */ float contrast,
            /* [in] */ float saturation,
            /* [in] */ float midpoint,
            /* [in] */ float highlights,
            /* [in] */ float shadows,
            /* [in] */ float blackLevel,
            /* [in] */ float whiteLevel,
            /* [in] */ unsigned short videoBlackLevel,
            /* [out] */ float *array,
            /* [in] */ unsigned int arrayElementCount);
        
        END_INTERFACE
    } IBlackmagicRawToneCurveVtbl;

    interface IBlackmagicRawToneCurve
    {
        CONST_VTBL struct IBlackmagicRawToneCurveVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawToneCurve_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawToneCurve_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawToneCurve_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawToneCurve_GetToneCurve(This,cameraType,gamma,gen,contrast,saturation,midpoint,highlights,shadows,blackLevel,whiteLevel,videoBlackLevel)	\
    ( (This)->lpVtbl -> GetToneCurve(This,cameraType,gamma,gen,contrast,saturation,midpoint,highlights,shadows,blackLevel,whiteLevel,videoBlackLevel) ) 

#define IBlackmagicRawToneCurve_EvaluateToneCurve(This,cameraType,gen,contrast,saturation,midpoint,highlights,shadows,blackLevel,whiteLevel,videoBlackLevel,array,arrayElementCount)	\
    ( (This)->lpVtbl -> EvaluateToneCurve(This,cameraType,gen,contrast,saturation,midpoint,highlights,shadows,blackLevel,whiteLevel,videoBlackLevel,array,arrayElementCount) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawToneCurve_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawConstants_INTERFACE_DEFINED__
#define __IBlackmagicRawConstants_INTERFACE_DEFINED__

/* interface IBlackmagicRawConstants */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawConstants;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DE64F929-93A1-45A9-BA69-166A7DEF9FB3")
    IBlackmagicRawConstants : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClipProcessingAttributeRange( 
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *valueMin,
            /* [out] */ VARIANT *valueMax,
            /* [out] */ BOOL *isReadOnly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetClipProcessingAttributeList( 
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *array,
            /* [out] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameProcessingAttributeRange( 
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *valueMin,
            /* [out] */ VARIANT *valueMax,
            /* [out] */ BOOL *isReadOnly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameProcessingAttributeList( 
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *array,
            /* [out] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetISOListForAnalogGain( 
            /* [in] */ BSTR cameraType,
            /* [in] */ float analogGain,
            /* [in] */ BOOL analogGainIsConstant,
            /* [out] */ unsigned int *array,
            /* [out][in] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawConstantsVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawConstants * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawConstants * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawConstants * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClipProcessingAttributeRange )( 
            IBlackmagicRawConstants * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *valueMin,
            /* [out] */ VARIANT *valueMax,
            /* [out] */ BOOL *isReadOnly);
        
        HRESULT ( STDMETHODCALLTYPE *GetClipProcessingAttributeList )( 
            IBlackmagicRawConstants * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *array,
            /* [out] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameProcessingAttributeRange )( 
            IBlackmagicRawConstants * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *valueMin,
            /* [out] */ VARIANT *valueMax,
            /* [out] */ BOOL *isReadOnly);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameProcessingAttributeList )( 
            IBlackmagicRawConstants * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *array,
            /* [out] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly);
        
        HRESULT ( STDMETHODCALLTYPE *GetISOListForAnalogGain )( 
            IBlackmagicRawConstants * This,
            /* [in] */ BSTR cameraType,
            /* [in] */ float analogGain,
            /* [in] */ BOOL analogGainIsConstant,
            /* [out] */ unsigned int *array,
            /* [out][in] */ unsigned int *arrayElementCount,
            /* [out] */ BOOL *isReadOnly);
        
        END_INTERFACE
    } IBlackmagicRawConstantsVtbl;

    interface IBlackmagicRawConstants
    {
        CONST_VTBL struct IBlackmagicRawConstantsVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawConstants_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawConstants_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawConstants_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawConstants_GetClipProcessingAttributeRange(This,cameraType,attribute,valueMin,valueMax,isReadOnly)	\
    ( (This)->lpVtbl -> GetClipProcessingAttributeRange(This,cameraType,attribute,valueMin,valueMax,isReadOnly) ) 

#define IBlackmagicRawConstants_GetClipProcessingAttributeList(This,cameraType,attribute,array,arrayElementCount,isReadOnly)	\
    ( (This)->lpVtbl -> GetClipProcessingAttributeList(This,cameraType,attribute,array,arrayElementCount,isReadOnly) ) 

#define IBlackmagicRawConstants_GetFrameProcessingAttributeRange(This,cameraType,attribute,valueMin,valueMax,isReadOnly)	\
    ( (This)->lpVtbl -> GetFrameProcessingAttributeRange(This,cameraType,attribute,valueMin,valueMax,isReadOnly) ) 

#define IBlackmagicRawConstants_GetFrameProcessingAttributeList(This,cameraType,attribute,array,arrayElementCount,isReadOnly)	\
    ( (This)->lpVtbl -> GetFrameProcessingAttributeList(This,cameraType,attribute,array,arrayElementCount,isReadOnly) ) 

#define IBlackmagicRawConstants_GetISOListForAnalogGain(This,cameraType,analogGain,analogGainIsConstant,array,arrayElementCount,isReadOnly)	\
    ( (This)->lpVtbl -> GetISOListForAnalogGain(This,cameraType,analogGain,analogGainIsConstant,array,arrayElementCount,isReadOnly) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawConstants_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawConfiguration_INTERFACE_DEFINED__
#define __IBlackmagicRawConfiguration_INTERFACE_DEFINED__

/* interface IBlackmagicRawConfiguration */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F8588A3D-E31F-45BD-96C7-A5640EA8B8E7")
    IBlackmagicRawConfiguration : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE SetPipeline( 
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ void *pipelineContext,
            /* [in] */ void *pipelineCommandQueue) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPipeline( 
            /* [out] */ BlackmagicRawPipeline *pipeline,
            /* [out] */ void **pipelineContextOut,
            /* [out] */ void **pipelineCommandQueueOut) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsPipelineSupported( 
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [out] */ BOOL *pipelineSupported) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetCPUThreads( 
            /* [in] */ unsigned int threadCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCPUThreads( 
            /* [out] */ unsigned int *threadCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMaxCPUThreadCount( 
            /* [out] */ unsigned int *threadCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetWriteMetadataPerFrame( 
            /* [in] */ BOOL writePerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWriteMetadataPerFrame( 
            /* [out] */ BOOL *writePerFrame) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFromDevice( 
            /* [in] */ IBlackmagicRawPipelineDevice *pipelineDevice) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawConfigurationVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawConfiguration * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawConfiguration * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetPipeline )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [in] */ void *pipelineContext,
            /* [in] */ void *pipelineCommandQueue);
        
        HRESULT ( STDMETHODCALLTYPE *GetPipeline )( 
            IBlackmagicRawConfiguration * This,
            /* [out] */ BlackmagicRawPipeline *pipeline,
            /* [out] */ void **pipelineContextOut,
            /* [out] */ void **pipelineCommandQueueOut);
        
        HRESULT ( STDMETHODCALLTYPE *IsPipelineSupported )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ BlackmagicRawPipeline pipeline,
            /* [out] */ BOOL *pipelineSupported);
        
        HRESULT ( STDMETHODCALLTYPE *SetCPUThreads )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ unsigned int threadCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetCPUThreads )( 
            IBlackmagicRawConfiguration * This,
            /* [out] */ unsigned int *threadCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxCPUThreadCount )( 
            IBlackmagicRawConfiguration * This,
            /* [out] */ unsigned int *threadCount);
        
        HRESULT ( STDMETHODCALLTYPE *SetWriteMetadataPerFrame )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ BOOL writePerFrame);
        
        HRESULT ( STDMETHODCALLTYPE *GetWriteMetadataPerFrame )( 
            IBlackmagicRawConfiguration * This,
            /* [out] */ BOOL *writePerFrame);
        
        HRESULT ( STDMETHODCALLTYPE *SetFromDevice )( 
            IBlackmagicRawConfiguration * This,
            /* [in] */ IBlackmagicRawPipelineDevice *pipelineDevice);
        
        END_INTERFACE
    } IBlackmagicRawConfigurationVtbl;

    interface IBlackmagicRawConfiguration
    {
        CONST_VTBL struct IBlackmagicRawConfigurationVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawConfiguration_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawConfiguration_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawConfiguration_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawConfiguration_SetPipeline(This,pipeline,pipelineContext,pipelineCommandQueue)	\
    ( (This)->lpVtbl -> SetPipeline(This,pipeline,pipelineContext,pipelineCommandQueue) ) 

#define IBlackmagicRawConfiguration_GetPipeline(This,pipeline,pipelineContextOut,pipelineCommandQueueOut)	\
    ( (This)->lpVtbl -> GetPipeline(This,pipeline,pipelineContextOut,pipelineCommandQueueOut) ) 

#define IBlackmagicRawConfiguration_IsPipelineSupported(This,pipeline,pipelineSupported)	\
    ( (This)->lpVtbl -> IsPipelineSupported(This,pipeline,pipelineSupported) ) 

#define IBlackmagicRawConfiguration_SetCPUThreads(This,threadCount)	\
    ( (This)->lpVtbl -> SetCPUThreads(This,threadCount) ) 

#define IBlackmagicRawConfiguration_GetCPUThreads(This,threadCount)	\
    ( (This)->lpVtbl -> GetCPUThreads(This,threadCount) ) 

#define IBlackmagicRawConfiguration_GetMaxCPUThreadCount(This,threadCount)	\
    ( (This)->lpVtbl -> GetMaxCPUThreadCount(This,threadCount) ) 

#define IBlackmagicRawConfiguration_SetWriteMetadataPerFrame(This,writePerFrame)	\
    ( (This)->lpVtbl -> SetWriteMetadataPerFrame(This,writePerFrame) ) 

#define IBlackmagicRawConfiguration_GetWriteMetadataPerFrame(This,writePerFrame)	\
    ( (This)->lpVtbl -> GetWriteMetadataPerFrame(This,writePerFrame) ) 

#define IBlackmagicRawConfiguration_SetFromDevice(This,pipelineDevice)	\
    ( (This)->lpVtbl -> SetFromDevice(This,pipelineDevice) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawConfiguration_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawConfigurationEx_INTERFACE_DEFINED__
#define __IBlackmagicRawConfigurationEx_INTERFACE_DEFINED__

/* interface IBlackmagicRawConfigurationEx */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawConfigurationEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("ACE9078F-ABA0-4B26-A954-EDA108DADA5A")
    IBlackmagicRawConfigurationEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetResourceManager( 
            /* [out] */ IBlackmagicRawResourceManager **resourceManager) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetResourceManager( 
            /* [in] */ IBlackmagicRawResourceManager *resourceManager) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetInstructionSet( 
            /* [out] */ BlackmagicRawInstructionSet *instructionSet) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetInstructionSet( 
            /* [in] */ BlackmagicRawInstructionSet instructionSet) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawConfigurationExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawConfigurationEx * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawConfigurationEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawConfigurationEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceManager )( 
            IBlackmagicRawConfigurationEx * This,
            /* [out] */ IBlackmagicRawResourceManager **resourceManager);
        
        HRESULT ( STDMETHODCALLTYPE *SetResourceManager )( 
            IBlackmagicRawConfigurationEx * This,
            /* [in] */ IBlackmagicRawResourceManager *resourceManager);
        
        HRESULT ( STDMETHODCALLTYPE *GetInstructionSet )( 
            IBlackmagicRawConfigurationEx * This,
            /* [out] */ BlackmagicRawInstructionSet *instructionSet);
        
        HRESULT ( STDMETHODCALLTYPE *SetInstructionSet )( 
            IBlackmagicRawConfigurationEx * This,
            /* [in] */ BlackmagicRawInstructionSet instructionSet);
        
        END_INTERFACE
    } IBlackmagicRawConfigurationExVtbl;

    interface IBlackmagicRawConfigurationEx
    {
        CONST_VTBL struct IBlackmagicRawConfigurationExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawConfigurationEx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawConfigurationEx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawConfigurationEx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawConfigurationEx_GetResourceManager(This,resourceManager)	\
    ( (This)->lpVtbl -> GetResourceManager(This,resourceManager) ) 

#define IBlackmagicRawConfigurationEx_SetResourceManager(This,resourceManager)	\
    ( (This)->lpVtbl -> SetResourceManager(This,resourceManager) ) 

#define IBlackmagicRawConfigurationEx_GetInstructionSet(This,instructionSet)	\
    ( (This)->lpVtbl -> GetInstructionSet(This,instructionSet) ) 

#define IBlackmagicRawConfigurationEx_SetInstructionSet(This,instructionSet)	\
    ( (This)->lpVtbl -> SetInstructionSet(This,instructionSet) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawConfigurationEx_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawResourceManager_INTERFACE_DEFINED__
#define __IBlackmagicRawResourceManager_INTERFACE_DEFINED__

/* interface IBlackmagicRawResourceManager */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawResourceManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("3C5C3C4A-812C-4AF0-99F0-06C6E197C189")
    IBlackmagicRawResourceManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE CreateResource( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ unsigned int sizeBytes,
            /* [in] */ BlackmagicRawResourceType type,
            /* [in] */ BlackmagicRawResourceUsage usage,
            /* [out] */ void **resource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReleaseResource( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *resource,
            /* [in] */ BlackmagicRawResourceType type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CopyResource( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *source,
            /* [in] */ BlackmagicRawResourceType sourceType,
            /* [in] */ void *destination,
            /* [in] */ BlackmagicRawResourceType destinationType,
            /* [in] */ unsigned int sizeBytes,
            /* [in] */ BOOL copyAsync) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceHostPointer( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *resource,
            /* [in] */ BlackmagicRawResourceType resourceType,
            /* [out] */ void **hostPointer) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawResourceManagerVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawResourceManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawResourceManager * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawResourceManager * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateResource )( 
            IBlackmagicRawResourceManager * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ unsigned int sizeBytes,
            /* [in] */ BlackmagicRawResourceType type,
            /* [in] */ BlackmagicRawResourceUsage usage,
            /* [out] */ void **resource);
        
        HRESULT ( STDMETHODCALLTYPE *ReleaseResource )( 
            IBlackmagicRawResourceManager * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *resource,
            /* [in] */ BlackmagicRawResourceType type);
        
        HRESULT ( STDMETHODCALLTYPE *CopyResource )( 
            IBlackmagicRawResourceManager * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *source,
            /* [in] */ BlackmagicRawResourceType sourceType,
            /* [in] */ void *destination,
            /* [in] */ BlackmagicRawResourceType destinationType,
            /* [in] */ unsigned int sizeBytes,
            /* [in] */ BOOL copyAsync);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceHostPointer )( 
            IBlackmagicRawResourceManager * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *resource,
            /* [in] */ BlackmagicRawResourceType resourceType,
            /* [out] */ void **hostPointer);
        
        END_INTERFACE
    } IBlackmagicRawResourceManagerVtbl;

    interface IBlackmagicRawResourceManager
    {
        CONST_VTBL struct IBlackmagicRawResourceManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawResourceManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawResourceManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawResourceManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawResourceManager_CreateResource(This,context,commandQueue,sizeBytes,type,usage,resource)	\
    ( (This)->lpVtbl -> CreateResource(This,context,commandQueue,sizeBytes,type,usage,resource) ) 

#define IBlackmagicRawResourceManager_ReleaseResource(This,context,commandQueue,resource,type)	\
    ( (This)->lpVtbl -> ReleaseResource(This,context,commandQueue,resource,type) ) 

#define IBlackmagicRawResourceManager_CopyResource(This,context,commandQueue,source,sourceType,destination,destinationType,sizeBytes,copyAsync)	\
    ( (This)->lpVtbl -> CopyResource(This,context,commandQueue,source,sourceType,destination,destinationType,sizeBytes,copyAsync) ) 

#define IBlackmagicRawResourceManager_GetResourceHostPointer(This,context,commandQueue,resource,resourceType,hostPointer)	\
    ( (This)->lpVtbl -> GetResourceHostPointer(This,context,commandQueue,resource,resourceType,hostPointer) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawResourceManager_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawMetadataIterator_INTERFACE_DEFINED__
#define __IBlackmagicRawMetadataIterator_INTERFACE_DEFINED__

/* interface IBlackmagicRawMetadataIterator */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawMetadataIterator;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F85AE78D-5DC2-40BC-8C1D-D0D805523ADA")
    IBlackmagicRawMetadataIterator : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Next( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetKey( 
            /* [out] */ BSTR *key) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetData( 
            /* [out] */ VARIANT *data) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawMetadataIteratorVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawMetadataIterator * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawMetadataIterator * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawMetadataIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *Next )( 
            IBlackmagicRawMetadataIterator * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetKey )( 
            IBlackmagicRawMetadataIterator * This,
            /* [out] */ BSTR *key);
        
        HRESULT ( STDMETHODCALLTYPE *GetData )( 
            IBlackmagicRawMetadataIterator * This,
            /* [out] */ VARIANT *data);
        
        END_INTERFACE
    } IBlackmagicRawMetadataIteratorVtbl;

    interface IBlackmagicRawMetadataIterator
    {
        CONST_VTBL struct IBlackmagicRawMetadataIteratorVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawMetadataIterator_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawMetadataIterator_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawMetadataIterator_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawMetadataIterator_Next(This)	\
    ( (This)->lpVtbl -> Next(This) ) 

#define IBlackmagicRawMetadataIterator_GetKey(This,key)	\
    ( (This)->lpVtbl -> GetKey(This,key) ) 

#define IBlackmagicRawMetadataIterator_GetData(This,data)	\
    ( (This)->lpVtbl -> GetData(This,data) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawMetadataIterator_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawClipProcessingAttributes_INTERFACE_DEFINED__
#define __IBlackmagicRawClipProcessingAttributes_INTERFACE_DEFINED__

/* interface IBlackmagicRawClipProcessingAttributes */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawClipProcessingAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AA3F216F-123F-4617-B741-6B6F497E94AA")
    IBlackmagicRawClipProcessingAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetClipAttribute( 
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetClipAttribute( 
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [in] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPost3DLUT( 
            /* [out] */ IBlackmagicRawPost3DLUT **lut) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawClipProcessingAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawClipProcessingAttributes * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawClipProcessingAttributes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawClipProcessingAttributes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetClipAttribute )( 
            IBlackmagicRawClipProcessingAttributes * This,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [out] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *SetClipAttribute )( 
            IBlackmagicRawClipProcessingAttributes * This,
            /* [in] */ BlackmagicRawClipProcessingAttribute attribute,
            /* [in] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetPost3DLUT )( 
            IBlackmagicRawClipProcessingAttributes * This,
            /* [out] */ IBlackmagicRawPost3DLUT **lut);
        
        END_INTERFACE
    } IBlackmagicRawClipProcessingAttributesVtbl;

    interface IBlackmagicRawClipProcessingAttributes
    {
        CONST_VTBL struct IBlackmagicRawClipProcessingAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawClipProcessingAttributes_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawClipProcessingAttributes_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawClipProcessingAttributes_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawClipProcessingAttributes_GetClipAttribute(This,attribute,value)	\
    ( (This)->lpVtbl -> GetClipAttribute(This,attribute,value) ) 

#define IBlackmagicRawClipProcessingAttributes_SetClipAttribute(This,attribute,value)	\
    ( (This)->lpVtbl -> SetClipAttribute(This,attribute,value) ) 

#define IBlackmagicRawClipProcessingAttributes_GetPost3DLUT(This,lut)	\
    ( (This)->lpVtbl -> GetPost3DLUT(This,lut) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawClipProcessingAttributes_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawPost3DLUT_INTERFACE_DEFINED__
#define __IBlackmagicRawPost3DLUT_INTERFACE_DEFINED__

/* interface IBlackmagicRawPost3DLUT */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawPost3DLUT;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("72A53B50-BB40-4C69-83FB-58CF58AF35B6")
    IBlackmagicRawPost3DLUT : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetName( 
            /* [out] */ BSTR *name) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTitle( 
            /* [out] */ BSTR *title) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSize( 
            /* [out] */ unsigned int *size) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceGPU( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [out] */ BlackmagicRawResourceType *type,
            /* [out] */ void **resource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceCPU( 
            /* [out] */ void **resource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceSizeBytes( 
            /* [out] */ unsigned int *sizeBytes) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawPost3DLUTVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawPost3DLUT * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawPost3DLUT * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawPost3DLUT * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetName )( 
            IBlackmagicRawPost3DLUT * This,
            /* [out] */ BSTR *name);
        
        HRESULT ( STDMETHODCALLTYPE *GetTitle )( 
            IBlackmagicRawPost3DLUT * This,
            /* [out] */ BSTR *title);
        
        HRESULT ( STDMETHODCALLTYPE *GetSize )( 
            IBlackmagicRawPost3DLUT * This,
            /* [out] */ unsigned int *size);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceGPU )( 
            IBlackmagicRawPost3DLUT * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [out] */ BlackmagicRawResourceType *type,
            /* [out] */ void **resource);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceCPU )( 
            IBlackmagicRawPost3DLUT * This,
            /* [out] */ void **resource);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceSizeBytes )( 
            IBlackmagicRawPost3DLUT * This,
            /* [out] */ unsigned int *sizeBytes);
        
        END_INTERFACE
    } IBlackmagicRawPost3DLUTVtbl;

    interface IBlackmagicRawPost3DLUT
    {
        CONST_VTBL struct IBlackmagicRawPost3DLUTVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawPost3DLUT_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawPost3DLUT_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawPost3DLUT_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawPost3DLUT_GetName(This,name)	\
    ( (This)->lpVtbl -> GetName(This,name) ) 

#define IBlackmagicRawPost3DLUT_GetTitle(This,title)	\
    ( (This)->lpVtbl -> GetTitle(This,title) ) 

#define IBlackmagicRawPost3DLUT_GetSize(This,size)	\
    ( (This)->lpVtbl -> GetSize(This,size) ) 

#define IBlackmagicRawPost3DLUT_GetResourceGPU(This,context,commandQueue,type,resource)	\
    ( (This)->lpVtbl -> GetResourceGPU(This,context,commandQueue,type,resource) ) 

#define IBlackmagicRawPost3DLUT_GetResourceCPU(This,resource)	\
    ( (This)->lpVtbl -> GetResourceCPU(This,resource) ) 

#define IBlackmagicRawPost3DLUT_GetResourceSizeBytes(This,sizeBytes)	\
    ( (This)->lpVtbl -> GetResourceSizeBytes(This,sizeBytes) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawPost3DLUT_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawFrameProcessingAttributes_INTERFACE_DEFINED__
#define __IBlackmagicRawFrameProcessingAttributes_INTERFACE_DEFINED__

/* interface IBlackmagicRawFrameProcessingAttributes */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawFrameProcessingAttributes;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("659756A6-215B-47A7-A1A5-F5D6CD79D450")
    IBlackmagicRawFrameProcessingAttributes : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameAttribute( 
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetFrameAttribute( 
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [in] */ VARIANT *value) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawFrameProcessingAttributesVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawFrameProcessingAttributes * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawFrameProcessingAttributes * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawFrameProcessingAttributes * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameAttribute )( 
            IBlackmagicRawFrameProcessingAttributes * This,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [out] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *SetFrameAttribute )( 
            IBlackmagicRawFrameProcessingAttributes * This,
            /* [in] */ BlackmagicRawFrameProcessingAttribute attribute,
            /* [in] */ VARIANT *value);
        
        END_INTERFACE
    } IBlackmagicRawFrameProcessingAttributesVtbl;

    interface IBlackmagicRawFrameProcessingAttributes
    {
        CONST_VTBL struct IBlackmagicRawFrameProcessingAttributesVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawFrameProcessingAttributes_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawFrameProcessingAttributes_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawFrameProcessingAttributes_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawFrameProcessingAttributes_GetFrameAttribute(This,attribute,value)	\
    ( (This)->lpVtbl -> GetFrameAttribute(This,attribute,value) ) 

#define IBlackmagicRawFrameProcessingAttributes_SetFrameAttribute(This,attribute,value)	\
    ( (This)->lpVtbl -> SetFrameAttribute(This,attribute,value) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawFrameProcessingAttributes_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawProcessedImage_INTERFACE_DEFINED__
#define __IBlackmagicRawProcessedImage_INTERFACE_DEFINED__

/* interface IBlackmagicRawProcessedImage */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawProcessedImage;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D87A0F72-A883-42BB-8488-0089411C5035")
    IBlackmagicRawProcessedImage : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetWidth( 
            /* [out] */ unsigned int *width) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeight( 
            /* [out] */ unsigned int *height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResource( 
            /* [out] */ void **resource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceType( 
            /* [out] */ BlackmagicRawResourceType *type) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceFormat( 
            /* [out] */ BlackmagicRawResourceFormat *format) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceSizeBytes( 
            /* [out] */ unsigned int *sizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceContextAndCommandQueue( 
            /* [out] */ void **context,
            /* [out] */ void **commandQueue) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawProcessedImageVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawProcessedImage * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawProcessedImage * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawProcessedImage * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetWidth )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ unsigned int *width);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeight )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ unsigned int *height);
        
        HRESULT ( STDMETHODCALLTYPE *GetResource )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ void **resource);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceType )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ BlackmagicRawResourceType *type);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceFormat )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ BlackmagicRawResourceFormat *format);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceSizeBytes )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ unsigned int *sizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceContextAndCommandQueue )( 
            IBlackmagicRawProcessedImage * This,
            /* [out] */ void **context,
            /* [out] */ void **commandQueue);
        
        END_INTERFACE
    } IBlackmagicRawProcessedImageVtbl;

    interface IBlackmagicRawProcessedImage
    {
        CONST_VTBL struct IBlackmagicRawProcessedImageVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawProcessedImage_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawProcessedImage_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawProcessedImage_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawProcessedImage_GetWidth(This,width)	\
    ( (This)->lpVtbl -> GetWidth(This,width) ) 

#define IBlackmagicRawProcessedImage_GetHeight(This,height)	\
    ( (This)->lpVtbl -> GetHeight(This,height) ) 

#define IBlackmagicRawProcessedImage_GetResource(This,resource)	\
    ( (This)->lpVtbl -> GetResource(This,resource) ) 

#define IBlackmagicRawProcessedImage_GetResourceType(This,type)	\
    ( (This)->lpVtbl -> GetResourceType(This,type) ) 

#define IBlackmagicRawProcessedImage_GetResourceFormat(This,format)	\
    ( (This)->lpVtbl -> GetResourceFormat(This,format) ) 

#define IBlackmagicRawProcessedImage_GetResourceSizeBytes(This,sizeBytes)	\
    ( (This)->lpVtbl -> GetResourceSizeBytes(This,sizeBytes) ) 

#define IBlackmagicRawProcessedImage_GetResourceContextAndCommandQueue(This,context,commandQueue)	\
    ( (This)->lpVtbl -> GetResourceContextAndCommandQueue(This,context,commandQueue) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawProcessedImage_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawJob_INTERFACE_DEFINED__
#define __IBlackmagicRawJob_INTERFACE_DEFINED__

/* interface IBlackmagicRawJob */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawJob;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("34C05ACF-7118-45EA-8B71-887E0515395D")
    IBlackmagicRawJob : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Submit( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE Abort( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetUserData( 
            /* [in] */ void *userData) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetUserData( 
            /* [out] */ void **userData) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawJobVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawJob * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawJob * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *Submit )( 
            IBlackmagicRawJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *Abort )( 
            IBlackmagicRawJob * This);
        
        HRESULT ( STDMETHODCALLTYPE *SetUserData )( 
            IBlackmagicRawJob * This,
            /* [in] */ void *userData);
        
        HRESULT ( STDMETHODCALLTYPE *GetUserData )( 
            IBlackmagicRawJob * This,
            /* [out] */ void **userData);
        
        END_INTERFACE
    } IBlackmagicRawJobVtbl;

    interface IBlackmagicRawJob
    {
        CONST_VTBL struct IBlackmagicRawJobVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawJob_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawJob_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawJob_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawJob_Submit(This)	\
    ( (This)->lpVtbl -> Submit(This) ) 

#define IBlackmagicRawJob_Abort(This)	\
    ( (This)->lpVtbl -> Abort(This) ) 

#define IBlackmagicRawJob_SetUserData(This,userData)	\
    ( (This)->lpVtbl -> SetUserData(This,userData) ) 

#define IBlackmagicRawJob_GetUserData(This,userData)	\
    ( (This)->lpVtbl -> GetUserData(This,userData) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawJob_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawCallback_INTERFACE_DEFINED__
#define __IBlackmagicRawCallback_INTERFACE_DEFINED__

/* interface IBlackmagicRawCallback */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("E9F98FAC-33DB-4A65-BB94-8A82B027AED0")
    IBlackmagicRawCallback : public IUnknown
    {
    public:
        virtual void STDMETHODCALLTYPE ReadComplete( 
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result,
            /* [in] */ IBlackmagicRawFrame *frame) = 0;
        
        virtual void STDMETHODCALLTYPE DecodeComplete( 
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result) = 0;
        
        virtual void STDMETHODCALLTYPE ProcessComplete( 
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result,
            /* [in] */ IBlackmagicRawProcessedImage *processedImage) = 0;
        
        virtual void STDMETHODCALLTYPE TrimProgress( 
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ float progress) = 0;
        
        virtual void STDMETHODCALLTYPE TrimComplete( 
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result) = 0;
        
        virtual void STDMETHODCALLTYPE SidecarMetadataParseWarning( 
            /* [in] */ IBlackmagicRawClip *clip,
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned int lineNumber,
            /* [in] */ BSTR info) = 0;
        
        virtual void STDMETHODCALLTYPE SidecarMetadataParseError( 
            /* [in] */ IBlackmagicRawClip *clip,
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned int lineNumber,
            /* [in] */ BSTR info) = 0;
        
        virtual void STDMETHODCALLTYPE PreparePipelineComplete( 
            /* [in] */ void *userData,
            /* [in] */ HRESULT result) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawCallback * This);
        
        void ( STDMETHODCALLTYPE *ReadComplete )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result,
            /* [in] */ IBlackmagicRawFrame *frame);
        
        void ( STDMETHODCALLTYPE *DecodeComplete )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result);
        
        void ( STDMETHODCALLTYPE *ProcessComplete )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result,
            /* [in] */ IBlackmagicRawProcessedImage *processedImage);
        
        void ( STDMETHODCALLTYPE *TrimProgress )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ float progress);
        
        void ( STDMETHODCALLTYPE *TrimComplete )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawJob *job,
            /* [in] */ HRESULT result);
        
        void ( STDMETHODCALLTYPE *SidecarMetadataParseWarning )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawClip *clip,
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned int lineNumber,
            /* [in] */ BSTR info);
        
        void ( STDMETHODCALLTYPE *SidecarMetadataParseError )( 
            IBlackmagicRawCallback * This,
            /* [in] */ IBlackmagicRawClip *clip,
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned int lineNumber,
            /* [in] */ BSTR info);
        
        void ( STDMETHODCALLTYPE *PreparePipelineComplete )( 
            IBlackmagicRawCallback * This,
            /* [in] */ void *userData,
            /* [in] */ HRESULT result);
        
        END_INTERFACE
    } IBlackmagicRawCallbackVtbl;

    interface IBlackmagicRawCallback
    {
        CONST_VTBL struct IBlackmagicRawCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawCallback_ReadComplete(This,job,result,frame)	\
    ( (This)->lpVtbl -> ReadComplete(This,job,result,frame) ) 

#define IBlackmagicRawCallback_DecodeComplete(This,job,result)	\
    ( (This)->lpVtbl -> DecodeComplete(This,job,result) ) 

#define IBlackmagicRawCallback_ProcessComplete(This,job,result,processedImage)	\
    ( (This)->lpVtbl -> ProcessComplete(This,job,result,processedImage) ) 

#define IBlackmagicRawCallback_TrimProgress(This,job,progress)	\
    ( (This)->lpVtbl -> TrimProgress(This,job,progress) ) 

#define IBlackmagicRawCallback_TrimComplete(This,job,result)	\
    ( (This)->lpVtbl -> TrimComplete(This,job,result) ) 

#define IBlackmagicRawCallback_SidecarMetadataParseWarning(This,clip,fileName,lineNumber,info)	\
    ( (This)->lpVtbl -> SidecarMetadataParseWarning(This,clip,fileName,lineNumber,info) ) 

#define IBlackmagicRawCallback_SidecarMetadataParseError(This,clip,fileName,lineNumber,info)	\
    ( (This)->lpVtbl -> SidecarMetadataParseError(This,clip,fileName,lineNumber,info) ) 

#define IBlackmagicRawCallback_PreparePipelineComplete(This,userData,result)	\
    ( (This)->lpVtbl -> PreparePipelineComplete(This,userData,result) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawCallback_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawClipAudio_INTERFACE_DEFINED__
#define __IBlackmagicRawClipAudio_INTERFACE_DEFINED__

/* interface IBlackmagicRawClipAudio */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawClipAudio;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("76D4ACED-E0D6-45BB-B547-56B7435B2A1D")
    IBlackmagicRawClipAudio : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetAudioFormat( 
            /* [out] */ BlackmagicRawAudioFormat *format) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioBitDepth( 
            /* [out] */ unsigned int *bitDepth) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioChannelCount( 
            /* [out] */ unsigned int *channelCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioSampleRate( 
            /* [out] */ unsigned int *sampleRate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioSampleCount( 
            /* [out] */ unsigned long long *sampleCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetAudioSamples( 
            /* [in] */ long long sampleFrameIndex,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int bufferSizeBytes,
            /* [in] */ unsigned int maxSampleCount,
            /* [out] */ unsigned int *samplesRead,
            /* [out] */ unsigned int *bytesRead) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawClipAudioVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawClipAudio * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawClipAudio * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawClipAudio * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioFormat )( 
            IBlackmagicRawClipAudio * This,
            /* [out] */ BlackmagicRawAudioFormat *format);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioBitDepth )( 
            IBlackmagicRawClipAudio * This,
            /* [out] */ unsigned int *bitDepth);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioChannelCount )( 
            IBlackmagicRawClipAudio * This,
            /* [out] */ unsigned int *channelCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioSampleRate )( 
            IBlackmagicRawClipAudio * This,
            /* [out] */ unsigned int *sampleRate);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioSampleCount )( 
            IBlackmagicRawClipAudio * This,
            /* [out] */ unsigned long long *sampleCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetAudioSamples )( 
            IBlackmagicRawClipAudio * This,
            /* [in] */ long long sampleFrameIndex,
            /* [in] */ void *buffer,
            /* [in] */ unsigned int bufferSizeBytes,
            /* [in] */ unsigned int maxSampleCount,
            /* [out] */ unsigned int *samplesRead,
            /* [out] */ unsigned int *bytesRead);
        
        END_INTERFACE
    } IBlackmagicRawClipAudioVtbl;

    interface IBlackmagicRawClipAudio
    {
        CONST_VTBL struct IBlackmagicRawClipAudioVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawClipAudio_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawClipAudio_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawClipAudio_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawClipAudio_GetAudioFormat(This,format)	\
    ( (This)->lpVtbl -> GetAudioFormat(This,format) ) 

#define IBlackmagicRawClipAudio_GetAudioBitDepth(This,bitDepth)	\
    ( (This)->lpVtbl -> GetAudioBitDepth(This,bitDepth) ) 

#define IBlackmagicRawClipAudio_GetAudioChannelCount(This,channelCount)	\
    ( (This)->lpVtbl -> GetAudioChannelCount(This,channelCount) ) 

#define IBlackmagicRawClipAudio_GetAudioSampleRate(This,sampleRate)	\
    ( (This)->lpVtbl -> GetAudioSampleRate(This,sampleRate) ) 

#define IBlackmagicRawClipAudio_GetAudioSampleCount(This,sampleCount)	\
    ( (This)->lpVtbl -> GetAudioSampleCount(This,sampleCount) ) 

#define IBlackmagicRawClipAudio_GetAudioSamples(This,sampleFrameIndex,buffer,bufferSizeBytes,maxSampleCount,samplesRead,bytesRead)	\
    ( (This)->lpVtbl -> GetAudioSamples(This,sampleFrameIndex,buffer,bufferSizeBytes,maxSampleCount,samplesRead,bytesRead) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawClipAudio_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawFrame_INTERFACE_DEFINED__
#define __IBlackmagicRawFrame_INTERFACE_DEFINED__

/* interface IBlackmagicRawFrame */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawFrame;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("226A7BC7-16EE-4D78-B724-D3D0E073ADE7")
    IBlackmagicRawFrame : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetFrameIndex( 
            /* [out] */ unsigned long long *frameIndex) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecode( 
            /* [out] */ BSTR *timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetadataIterator( 
            /* [out] */ IBlackmagicRawMetadataIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetadata( 
            /* [in] */ BSTR key,
            /* [out] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMetadata( 
            /* [in] */ BSTR key,
            /* [in] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloneFrameProcessingAttributes( 
            /* [out] */ IBlackmagicRawFrameProcessingAttributes **frameProcessingAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetResolutionScale( 
            /* [in] */ BlackmagicRawResolutionScale resolutionScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResolutionScale( 
            /* [out] */ BlackmagicRawResolutionScale *resolutionScale) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetResourceFormat( 
            /* [in] */ BlackmagicRawResourceFormat resourceFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetResourceFormat( 
            /* [out] */ BlackmagicRawResourceFormat *resourceFormat) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobDecodeAndProcessFrame( 
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawFrameVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawFrame * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawFrame * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawFrame * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameIndex )( 
            IBlackmagicRawFrame * This,
            /* [out] */ unsigned long long *frameIndex);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecode )( 
            IBlackmagicRawFrame * This,
            /* [out] */ BSTR *timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadataIterator )( 
            IBlackmagicRawFrame * This,
            /* [out] */ IBlackmagicRawMetadataIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IBlackmagicRawFrame * This,
            /* [in] */ BSTR key,
            /* [out] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *SetMetadata )( 
            IBlackmagicRawFrame * This,
            /* [in] */ BSTR key,
            /* [in] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *CloneFrameProcessingAttributes )( 
            IBlackmagicRawFrame * This,
            /* [out] */ IBlackmagicRawFrameProcessingAttributes **frameProcessingAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *SetResolutionScale )( 
            IBlackmagicRawFrame * This,
            /* [in] */ BlackmagicRawResolutionScale resolutionScale);
        
        HRESULT ( STDMETHODCALLTYPE *GetResolutionScale )( 
            IBlackmagicRawFrame * This,
            /* [out] */ BlackmagicRawResolutionScale *resolutionScale);
        
        HRESULT ( STDMETHODCALLTYPE *SetResourceFormat )( 
            IBlackmagicRawFrame * This,
            /* [in] */ BlackmagicRawResourceFormat resourceFormat);
        
        HRESULT ( STDMETHODCALLTYPE *GetResourceFormat )( 
            IBlackmagicRawFrame * This,
            /* [out] */ BlackmagicRawResourceFormat *resourceFormat);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobDecodeAndProcessFrame )( 
            IBlackmagicRawFrame * This,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ IBlackmagicRawJob **job);
        
        END_INTERFACE
    } IBlackmagicRawFrameVtbl;

    interface IBlackmagicRawFrame
    {
        CONST_VTBL struct IBlackmagicRawFrameVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawFrame_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawFrame_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawFrame_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawFrame_GetFrameIndex(This,frameIndex)	\
    ( (This)->lpVtbl -> GetFrameIndex(This,frameIndex) ) 

#define IBlackmagicRawFrame_GetTimecode(This,timecode)	\
    ( (This)->lpVtbl -> GetTimecode(This,timecode) ) 

#define IBlackmagicRawFrame_GetMetadataIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetMetadataIterator(This,iterator) ) 

#define IBlackmagicRawFrame_GetMetadata(This,key,value)	\
    ( (This)->lpVtbl -> GetMetadata(This,key,value) ) 

#define IBlackmagicRawFrame_SetMetadata(This,key,value)	\
    ( (This)->lpVtbl -> SetMetadata(This,key,value) ) 

#define IBlackmagicRawFrame_CloneFrameProcessingAttributes(This,frameProcessingAttributes)	\
    ( (This)->lpVtbl -> CloneFrameProcessingAttributes(This,frameProcessingAttributes) ) 

#define IBlackmagicRawFrame_SetResolutionScale(This,resolutionScale)	\
    ( (This)->lpVtbl -> SetResolutionScale(This,resolutionScale) ) 

#define IBlackmagicRawFrame_GetResolutionScale(This,resolutionScale)	\
    ( (This)->lpVtbl -> GetResolutionScale(This,resolutionScale) ) 

#define IBlackmagicRawFrame_SetResourceFormat(This,resourceFormat)	\
    ( (This)->lpVtbl -> SetResourceFormat(This,resourceFormat) ) 

#define IBlackmagicRawFrame_GetResourceFormat(This,resourceFormat)	\
    ( (This)->lpVtbl -> GetResourceFormat(This,resourceFormat) ) 

#define IBlackmagicRawFrame_CreateJobDecodeAndProcessFrame(This,clipProcessingAttributes,frameProcessingAttributes,job)	\
    ( (This)->lpVtbl -> CreateJobDecodeAndProcessFrame(This,clipProcessingAttributes,frameProcessingAttributes,job) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawFrame_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawFrameEx_INTERFACE_DEFINED__
#define __IBlackmagicRawFrameEx_INTERFACE_DEFINED__

/* interface IBlackmagicRawFrameEx */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawFrameEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("F8C6C374-D7FB-4BD3-AD0B-C533464FF450")
    IBlackmagicRawFrameEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetBitStreamSizeBytes( 
            /* [out] */ unsigned int *bitStreamSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessedImageResolution( 
            /* [out] */ unsigned int *width,
            /* [out] */ unsigned int *height) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawFrameExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawFrameEx * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawFrameEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawFrameEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitStreamSizeBytes )( 
            IBlackmagicRawFrameEx * This,
            /* [out] */ unsigned int *bitStreamSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessedImageResolution )( 
            IBlackmagicRawFrameEx * This,
            /* [out] */ unsigned int *width,
            /* [out] */ unsigned int *height);
        
        END_INTERFACE
    } IBlackmagicRawFrameExVtbl;

    interface IBlackmagicRawFrameEx
    {
        CONST_VTBL struct IBlackmagicRawFrameExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawFrameEx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawFrameEx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawFrameEx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawFrameEx_GetBitStreamSizeBytes(This,bitStreamSizeBytes)	\
    ( (This)->lpVtbl -> GetBitStreamSizeBytes(This,bitStreamSizeBytes) ) 

#define IBlackmagicRawFrameEx_GetProcessedImageResolution(This,width,height)	\
    ( (This)->lpVtbl -> GetProcessedImageResolution(This,width,height) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawFrameEx_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawManualDecoderFlow1_INTERFACE_DEFINED__
#define __IBlackmagicRawManualDecoderFlow1_INTERFACE_DEFINED__

/* interface IBlackmagicRawManualDecoderFlow1 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawManualDecoderFlow1;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("278815A6-A3C1-47C7-A0A6-6754DEAE5E7A")
    IBlackmagicRawManualDecoderFlow1 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PopulateFrameStateBuffer( 
            /* [in] */ IBlackmagicRawFrame *frame,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ void *frameState,
            /* [in] */ unsigned int frameStateSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameStateSizeBytes( 
            /* [out] */ unsigned int *frameStateSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDecodedSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *decodedSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessedSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *processedSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPost3DLUTSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *post3DLUTSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobDecode( 
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *bitStreamBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobProcess( 
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [in] */ void *processedBufferCPU,
            /* [in] */ void *post3DLUTBufferCPU,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawManualDecoderFlow1Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawManualDecoderFlow1 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawManualDecoderFlow1 * This);
        
        HRESULT ( STDMETHODCALLTYPE *PopulateFrameStateBuffer )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ IBlackmagicRawFrame *frame,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ void *frameState,
            /* [in] */ unsigned int frameStateSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStateSizeBytes )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [out] */ unsigned int *frameStateSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecodedSizeBytes )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *decodedSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessedSizeBytes )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *processedSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetPost3DLUTSizeBytes )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *post3DLUTSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobDecode )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *bitStreamBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [out] */ IBlackmagicRawJob **job);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobProcess )( 
            IBlackmagicRawManualDecoderFlow1 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [in] */ void *processedBufferCPU,
            /* [in] */ void *post3DLUTBufferCPU,
            /* [out] */ IBlackmagicRawJob **job);
        
        END_INTERFACE
    } IBlackmagicRawManualDecoderFlow1Vtbl;

    interface IBlackmagicRawManualDecoderFlow1
    {
        CONST_VTBL struct IBlackmagicRawManualDecoderFlow1Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawManualDecoderFlow1_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawManualDecoderFlow1_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawManualDecoderFlow1_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawManualDecoderFlow1_PopulateFrameStateBuffer(This,frame,clipProcessingAttributes,frameProcessingAttributes,frameState,frameStateSizeBytes)	\
    ( (This)->lpVtbl -> PopulateFrameStateBuffer(This,frame,clipProcessingAttributes,frameProcessingAttributes,frameState,frameStateSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow1_GetFrameStateSizeBytes(This,frameStateSizeBytes)	\
    ( (This)->lpVtbl -> GetFrameStateSizeBytes(This,frameStateSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow1_GetDecodedSizeBytes(This,frameStateBufferCPU,decodedSizeBytes)	\
    ( (This)->lpVtbl -> GetDecodedSizeBytes(This,frameStateBufferCPU,decodedSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow1_GetProcessedSizeBytes(This,frameStateBufferCPU,processedSizeBytes)	\
    ( (This)->lpVtbl -> GetProcessedSizeBytes(This,frameStateBufferCPU,processedSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow1_GetPost3DLUTSizeBytes(This,frameStateBufferCPU,post3DLUTSizeBytes)	\
    ( (This)->lpVtbl -> GetPost3DLUTSizeBytes(This,frameStateBufferCPU,post3DLUTSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow1_CreateJobDecode(This,frameStateBufferCPU,bitStreamBufferCPU,decodedBufferCPU,job)	\
    ( (This)->lpVtbl -> CreateJobDecode(This,frameStateBufferCPU,bitStreamBufferCPU,decodedBufferCPU,job) ) 

#define IBlackmagicRawManualDecoderFlow1_CreateJobProcess(This,frameStateBufferCPU,decodedBufferCPU,processedBufferCPU,post3DLUTBufferCPU,job)	\
    ( (This)->lpVtbl -> CreateJobProcess(This,frameStateBufferCPU,decodedBufferCPU,processedBufferCPU,post3DLUTBufferCPU,job) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawManualDecoderFlow1_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawManualDecoderFlow2_INTERFACE_DEFINED__
#define __IBlackmagicRawManualDecoderFlow2_INTERFACE_DEFINED__

/* interface IBlackmagicRawManualDecoderFlow2 */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawManualDecoderFlow2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("DBEC4C39-B4C2-4A65-AA8C-2B3C7F4777E3")
    IBlackmagicRawManualDecoderFlow2 : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE PopulateFrameStateBuffer( 
            /* [in] */ IBlackmagicRawFrame *frame,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ void *frameState,
            /* [in] */ unsigned int frameStateSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameStateSizeBytes( 
            /* [out] */ unsigned int *frameStateSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDecodedSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *decodedSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetWorkingSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *workingSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetProcessedSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *processedSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetPost3DLUTSizeBytes( 
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *post3DLUTSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobDecode( 
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *bitStreamBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobProcess( 
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *decodedBufferGPU,
            /* [in] */ void *workingBufferGPU,
            /* [in] */ void *processedBufferGPU,
            /* [in] */ void *post3DLUTBufferGPU,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawManualDecoderFlow2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawManualDecoderFlow2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawManualDecoderFlow2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *PopulateFrameStateBuffer )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ IBlackmagicRawFrame *frame,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ void *frameState,
            /* [in] */ unsigned int frameStateSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameStateSizeBytes )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [out] */ unsigned int *frameStateSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetDecodedSizeBytes )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *decodedSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetWorkingSizeBytes )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *workingSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetProcessedSizeBytes )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *processedSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetPost3DLUTSizeBytes )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [out] */ unsigned int *post3DLUTSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobDecode )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *bitStreamBufferCPU,
            /* [in] */ void *decodedBufferCPU,
            /* [out] */ IBlackmagicRawJob **job);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobProcess )( 
            IBlackmagicRawManualDecoderFlow2 * This,
            /* [in] */ void *context,
            /* [in] */ void *commandQueue,
            /* [in] */ void *frameStateBufferCPU,
            /* [in] */ void *decodedBufferGPU,
            /* [in] */ void *workingBufferGPU,
            /* [in] */ void *processedBufferGPU,
            /* [in] */ void *post3DLUTBufferGPU,
            /* [out] */ IBlackmagicRawJob **job);
        
        END_INTERFACE
    } IBlackmagicRawManualDecoderFlow2Vtbl;

    interface IBlackmagicRawManualDecoderFlow2
    {
        CONST_VTBL struct IBlackmagicRawManualDecoderFlow2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawManualDecoderFlow2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawManualDecoderFlow2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawManualDecoderFlow2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawManualDecoderFlow2_PopulateFrameStateBuffer(This,frame,clipProcessingAttributes,frameProcessingAttributes,frameState,frameStateSizeBytes)	\
    ( (This)->lpVtbl -> PopulateFrameStateBuffer(This,frame,clipProcessingAttributes,frameProcessingAttributes,frameState,frameStateSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_GetFrameStateSizeBytes(This,frameStateSizeBytes)	\
    ( (This)->lpVtbl -> GetFrameStateSizeBytes(This,frameStateSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_GetDecodedSizeBytes(This,frameStateBufferCPU,decodedSizeBytes)	\
    ( (This)->lpVtbl -> GetDecodedSizeBytes(This,frameStateBufferCPU,decodedSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_GetWorkingSizeBytes(This,frameStateBufferCPU,workingSizeBytes)	\
    ( (This)->lpVtbl -> GetWorkingSizeBytes(This,frameStateBufferCPU,workingSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_GetProcessedSizeBytes(This,frameStateBufferCPU,processedSizeBytes)	\
    ( (This)->lpVtbl -> GetProcessedSizeBytes(This,frameStateBufferCPU,processedSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_GetPost3DLUTSizeBytes(This,frameStateBufferCPU,post3DLUTSizeBytes)	\
    ( (This)->lpVtbl -> GetPost3DLUTSizeBytes(This,frameStateBufferCPU,post3DLUTSizeBytes) ) 

#define IBlackmagicRawManualDecoderFlow2_CreateJobDecode(This,frameStateBufferCPU,bitStreamBufferCPU,decodedBufferCPU,job)	\
    ( (This)->lpVtbl -> CreateJobDecode(This,frameStateBufferCPU,bitStreamBufferCPU,decodedBufferCPU,job) ) 

#define IBlackmagicRawManualDecoderFlow2_CreateJobProcess(This,context,commandQueue,frameStateBufferCPU,decodedBufferGPU,workingBufferGPU,processedBufferGPU,post3DLUTBufferGPU,job)	\
    ( (This)->lpVtbl -> CreateJobProcess(This,context,commandQueue,frameStateBufferCPU,decodedBufferGPU,workingBufferGPU,processedBufferGPU,post3DLUTBufferGPU,job) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawManualDecoderFlow2_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawClip_INTERFACE_DEFINED__
#define __IBlackmagicRawClip_INTERFACE_DEFINED__

/* interface IBlackmagicRawClip */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawClip;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("408F758F-347F-4CDA-BA9B-89B6F15603CF")
    IBlackmagicRawClip : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetWidth( 
            /* [out] */ unsigned int *width) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetHeight( 
            /* [out] */ unsigned int *height) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameRate( 
            /* [out] */ float *frameRate) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetFrameCount( 
            /* [out] */ unsigned long long *frameCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetTimecodeForFrame( 
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ BSTR *timecode) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetadataIterator( 
            /* [out] */ IBlackmagicRawMetadataIterator **iterator) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMetadata( 
            /* [in] */ BSTR key,
            /* [out] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SetMetadata( 
            /* [in] */ BSTR key,
            /* [in] */ VARIANT *value) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetCameraType( 
            /* [out] */ BSTR *cameraType) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CloneClipProcessingAttributes( 
            /* [out] */ IBlackmagicRawClipProcessingAttributes **clipProcessingAttributes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetMulticardFileCount( 
            /* [out] */ unsigned int *multicardFileCount) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsMulticardFilePresent( 
            /* [in] */ unsigned int index,
            /* [out] */ BOOL *isMulticardFilePresent) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetSidecarFileAttached( 
            /* [out] */ BOOL *isSidecarFileAttached) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE SaveSidecarFile( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE ReloadSidecarFile( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobReadFrame( 
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobTrim( 
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned long long frameIndex,
            /* [in] */ unsigned long long frameCount,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawClipVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawClip * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawClip * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawClip * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetWidth )( 
            IBlackmagicRawClip * This,
            /* [out] */ unsigned int *width);
        
        HRESULT ( STDMETHODCALLTYPE *GetHeight )( 
            IBlackmagicRawClip * This,
            /* [out] */ unsigned int *height);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameRate )( 
            IBlackmagicRawClip * This,
            /* [out] */ float *frameRate);
        
        HRESULT ( STDMETHODCALLTYPE *GetFrameCount )( 
            IBlackmagicRawClip * This,
            /* [out] */ unsigned long long *frameCount);
        
        HRESULT ( STDMETHODCALLTYPE *GetTimecodeForFrame )( 
            IBlackmagicRawClip * This,
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ BSTR *timecode);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadataIterator )( 
            IBlackmagicRawClip * This,
            /* [out] */ IBlackmagicRawMetadataIterator **iterator);
        
        HRESULT ( STDMETHODCALLTYPE *GetMetadata )( 
            IBlackmagicRawClip * This,
            /* [in] */ BSTR key,
            /* [out] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *SetMetadata )( 
            IBlackmagicRawClip * This,
            /* [in] */ BSTR key,
            /* [in] */ VARIANT *value);
        
        HRESULT ( STDMETHODCALLTYPE *GetCameraType )( 
            IBlackmagicRawClip * This,
            /* [out] */ BSTR *cameraType);
        
        HRESULT ( STDMETHODCALLTYPE *CloneClipProcessingAttributes )( 
            IBlackmagicRawClip * This,
            /* [out] */ IBlackmagicRawClipProcessingAttributes **clipProcessingAttributes);
        
        HRESULT ( STDMETHODCALLTYPE *GetMulticardFileCount )( 
            IBlackmagicRawClip * This,
            /* [out] */ unsigned int *multicardFileCount);
        
        HRESULT ( STDMETHODCALLTYPE *IsMulticardFilePresent )( 
            IBlackmagicRawClip * This,
            /* [in] */ unsigned int index,
            /* [out] */ BOOL *isMulticardFilePresent);
        
        HRESULT ( STDMETHODCALLTYPE *GetSidecarFileAttached )( 
            IBlackmagicRawClip * This,
            /* [out] */ BOOL *isSidecarFileAttached);
        
        HRESULT ( STDMETHODCALLTYPE *SaveSidecarFile )( 
            IBlackmagicRawClip * This);
        
        HRESULT ( STDMETHODCALLTYPE *ReloadSidecarFile )( 
            IBlackmagicRawClip * This);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobReadFrame )( 
            IBlackmagicRawClip * This,
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ IBlackmagicRawJob **job);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobTrim )( 
            IBlackmagicRawClip * This,
            /* [in] */ BSTR fileName,
            /* [in] */ unsigned long long frameIndex,
            /* [in] */ unsigned long long frameCount,
            /* [in] */ IBlackmagicRawClipProcessingAttributes *clipProcessingAttributes,
            /* [in] */ IBlackmagicRawFrameProcessingAttributes *frameProcessingAttributes,
            /* [out] */ IBlackmagicRawJob **job);
        
        END_INTERFACE
    } IBlackmagicRawClipVtbl;

    interface IBlackmagicRawClip
    {
        CONST_VTBL struct IBlackmagicRawClipVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawClip_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawClip_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawClip_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawClip_GetWidth(This,width)	\
    ( (This)->lpVtbl -> GetWidth(This,width) ) 

#define IBlackmagicRawClip_GetHeight(This,height)	\
    ( (This)->lpVtbl -> GetHeight(This,height) ) 

#define IBlackmagicRawClip_GetFrameRate(This,frameRate)	\
    ( (This)->lpVtbl -> GetFrameRate(This,frameRate) ) 

#define IBlackmagicRawClip_GetFrameCount(This,frameCount)	\
    ( (This)->lpVtbl -> GetFrameCount(This,frameCount) ) 

#define IBlackmagicRawClip_GetTimecodeForFrame(This,frameIndex,timecode)	\
    ( (This)->lpVtbl -> GetTimecodeForFrame(This,frameIndex,timecode) ) 

#define IBlackmagicRawClip_GetMetadataIterator(This,iterator)	\
    ( (This)->lpVtbl -> GetMetadataIterator(This,iterator) ) 

#define IBlackmagicRawClip_GetMetadata(This,key,value)	\
    ( (This)->lpVtbl -> GetMetadata(This,key,value) ) 

#define IBlackmagicRawClip_SetMetadata(This,key,value)	\
    ( (This)->lpVtbl -> SetMetadata(This,key,value) ) 

#define IBlackmagicRawClip_GetCameraType(This,cameraType)	\
    ( (This)->lpVtbl -> GetCameraType(This,cameraType) ) 

#define IBlackmagicRawClip_CloneClipProcessingAttributes(This,clipProcessingAttributes)	\
    ( (This)->lpVtbl -> CloneClipProcessingAttributes(This,clipProcessingAttributes) ) 

#define IBlackmagicRawClip_GetMulticardFileCount(This,multicardFileCount)	\
    ( (This)->lpVtbl -> GetMulticardFileCount(This,multicardFileCount) ) 

#define IBlackmagicRawClip_IsMulticardFilePresent(This,index,isMulticardFilePresent)	\
    ( (This)->lpVtbl -> IsMulticardFilePresent(This,index,isMulticardFilePresent) ) 

#define IBlackmagicRawClip_GetSidecarFileAttached(This,isSidecarFileAttached)	\
    ( (This)->lpVtbl -> GetSidecarFileAttached(This,isSidecarFileAttached) ) 

#define IBlackmagicRawClip_SaveSidecarFile(This)	\
    ( (This)->lpVtbl -> SaveSidecarFile(This) ) 

#define IBlackmagicRawClip_ReloadSidecarFile(This)	\
    ( (This)->lpVtbl -> ReloadSidecarFile(This) ) 

#define IBlackmagicRawClip_CreateJobReadFrame(This,frameIndex,job)	\
    ( (This)->lpVtbl -> CreateJobReadFrame(This,frameIndex,job) ) 

#define IBlackmagicRawClip_CreateJobTrim(This,fileName,frameIndex,frameCount,clipProcessingAttributes,frameProcessingAttributes,job)	\
    ( (This)->lpVtbl -> CreateJobTrim(This,fileName,frameIndex,frameCount,clipProcessingAttributes,frameProcessingAttributes,job) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawClip_INTERFACE_DEFINED__ */


#ifndef __IBlackmagicRawClipEx_INTERFACE_DEFINED__
#define __IBlackmagicRawClipEx_INTERFACE_DEFINED__

/* interface IBlackmagicRawClipEx */
/* [helpstring][uuid][object] */ 


EXTERN_C const IID IID_IBlackmagicRawClipEx;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("D260C7D0-93BD-4D68-B600-93B4CAB7F870")
    IBlackmagicRawClipEx : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetMaxBitStreamSizeBytes( 
            /* [out] */ unsigned int *maxBitStreamSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBitStreamSizeBytes( 
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ unsigned int *bitStreamSizeBytes) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateJobReadFrame( 
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ void *bitStream,
            /* [in] */ unsigned int bitStreamSizeBytes,
            /* [out] */ IBlackmagicRawJob **job) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE QueryTimecodeInfo( 
            /* [out] */ unsigned int *baseFrameIndex,
            /* [out] */ BOOL *isDropFrameTimecode) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBlackmagicRawClipExVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBlackmagicRawClipEx * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBlackmagicRawClipEx * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBlackmagicRawClipEx * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetMaxBitStreamSizeBytes )( 
            IBlackmagicRawClipEx * This,
            /* [out] */ unsigned int *maxBitStreamSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *GetBitStreamSizeBytes )( 
            IBlackmagicRawClipEx * This,
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ unsigned int *bitStreamSizeBytes);
        
        HRESULT ( STDMETHODCALLTYPE *CreateJobReadFrame )( 
            IBlackmagicRawClipEx * This,
            /* [in] */ unsigned long long frameIndex,
            /* [out] */ void *bitStream,
            /* [in] */ unsigned int bitStreamSizeBytes,
            /* [out] */ IBlackmagicRawJob **job);
        
        HRESULT ( STDMETHODCALLTYPE *QueryTimecodeInfo )( 
            IBlackmagicRawClipEx * This,
            /* [out] */ unsigned int *baseFrameIndex,
            /* [out] */ BOOL *isDropFrameTimecode);
        
        END_INTERFACE
    } IBlackmagicRawClipExVtbl;

    interface IBlackmagicRawClipEx
    {
        CONST_VTBL struct IBlackmagicRawClipExVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBlackmagicRawClipEx_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBlackmagicRawClipEx_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBlackmagicRawClipEx_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBlackmagicRawClipEx_GetMaxBitStreamSizeBytes(This,maxBitStreamSizeBytes)	\
    ( (This)->lpVtbl -> GetMaxBitStreamSizeBytes(This,maxBitStreamSizeBytes) ) 

#define IBlackmagicRawClipEx_GetBitStreamSizeBytes(This,frameIndex,bitStreamSizeBytes)	\
    ( (This)->lpVtbl -> GetBitStreamSizeBytes(This,frameIndex,bitStreamSizeBytes) ) 

#define IBlackmagicRawClipEx_CreateJobReadFrame(This,frameIndex,bitStream,bitStreamSizeBytes,job)	\
    ( (This)->lpVtbl -> CreateJobReadFrame(This,frameIndex,bitStream,bitStreamSizeBytes,job) ) 

#define IBlackmagicRawClipEx_QueryTimecodeInfo(This,baseFrameIndex,isDropFrameTimecode)	\
    ( (This)->lpVtbl -> QueryTimecodeInfo(This,baseFrameIndex,isDropFrameTimecode) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBlackmagicRawClipEx_INTERFACE_DEFINED__ */


EXTERN_C const CLSID CLSID_CBlackmagicRawFactory;

#ifdef __cplusplus

class DECLSPEC_UUID("D630D04C-1434-4BF8-8801-E2AC1F56C6BA")
CBlackmagicRawFactory;
#endif
#endif /* __BlackmagicRawAPI_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


