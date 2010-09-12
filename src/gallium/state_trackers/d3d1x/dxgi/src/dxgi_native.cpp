/**************************************************************************
 *
 * Copyright 2010 Luca Barbieri
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "dxgi_private.h"
extern "C" {
#include "native.h"
#include <util/u_format.h>
#include <util/u_inlines.h>
#include <util/u_simple_shaders.h>
#include <pipe/p_shader_tokens.h>
}
#include <iostream>
#include <memory>

struct GalliumDXGIOutput;
struct GalliumDXGIAdapter;
struct GalliumDXGISwapChain;
struct GalliumDXGIFactory;

static HRESULT GalliumDXGISwapChainCreate(GalliumDXGIFactory* factory, IUnknown* device, const DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain** ppSwapChain);
static HRESULT GalliumDXGIAdapterCreate(GalliumDXGIFactory* adapter, const struct native_platform* platform, void* dpy, IDXGIAdapter1** ppAdapter);
static HRESULT GalliumDXGIOutputCreate(GalliumDXGIAdapter* adapter, const std::string& name, const struct native_connector* connector, IDXGIOutput** ppOutput);
static void GalliumDXGISwapChainRevalidate(IDXGISwapChain* swap_chain);

template<typename Base = IDXGIObject, typename Parent = IDXGIObject>
struct GalliumDXGIObject : public GalliumPrivateDataComObject<Base>
{
	ComPtr<Parent> parent;

	GalliumDXGIObject(Parent* p_parent = 0)
	{
		this->parent = p_parent;
	}

        virtual HRESULT STDMETHODCALLTYPE GetParent(
            __in  REFIID riid,
            __out  void **ppParent)
        {
        	return parent->QueryInterface(riid, ppParent);
        }
};

static void* STDMETHODCALLTYPE identity_resolver(void* cookie, HWND hwnd)
{
	return (void*)hwnd;
}

struct GalliumDXGIFactory : public GalliumDXGIObject<IDXGIFactory1, IUnknown>
{
        HWND associated_window;
	const struct native_platform* platform;
	void* display;
	PFNHWNDRESOLVER resolver;
	void* resolver_cookie;

        GalliumDXGIFactory(const struct native_platform* platform, void* display, PFNHWNDRESOLVER resolver, void* resolver_cookie)
        : GalliumDXGIObject((IUnknown*)NULL), platform(platform), display(display), resolver(resolver ? resolver : identity_resolver), resolver_cookie(resolver_cookie)
        {}

        virtual HRESULT STDMETHODCALLTYPE EnumAdapters(
        	UINT Adapter,
        	__out  IDXGIAdapter **ppAdapter)
	{
		return EnumAdapters1(Adapter, (IDXGIAdapter1**)ppAdapter);
	}

        virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(
        	UINT Adapter,
        	__out  IDXGIAdapter1 **ppAdapter)
        {
        	*ppAdapter = 0;
		if(Adapter == 0)
		{
			return GalliumDXGIAdapterCreate(this, platform, display, ppAdapter);
		}
#if 0
		// TODO: enable this
        	if(platform == native_get_x11_platform())
        	{
        		unsigned nscreens = ScreenCount((Display*)display);
        		if(Adapter < nscreens)
        		{
        			unsigned def_screen = DefaultScreen(display);
        			if(Adapter <= def_screen)
        				--Adapter;
        			*ppAdapter = GalliumDXGIAdapterCreate(this, platform, display, Adapter);
        			return S_OK;
        		}
        	}
#endif
		return DXGI_ERROR_NOT_FOUND;
        }

        /* TODO: this is a mysterious underdocumented magic API
         * Can we have multiple windows associated?
         * Can we have multiple windows associated if we use multiple factories?
         * If so, what should GetWindowAssociation return?
         * If not, does a new swapchain steal the association?
         * Does this act for existing swapchains? For new swapchains?
         */
        virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(
        	HWND WindowHandle,
        	UINT Flags)
        {
        	/* TODO: actually implement, for Wine, X11 and KMS*/
        	associated_window = WindowHandle;
        	return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(
        	__out  HWND *pWindowHandle)
        {
        	*pWindowHandle = associated_window;
        	return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(
        	__in  IUnknown *pDevice,
        	__in  DXGI_SWAP_CHAIN_DESC *pDesc,
        	__out  IDXGISwapChain **ppSwapChain)
        {
        	return GalliumDXGISwapChainCreate(this, pDevice, *pDesc, ppSwapChain);
        }

        virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(
            HMODULE Module,
            __out  IDXGIAdapter **ppAdapter)
        {
        	/* TODO: ignore the module, and just create a Gallium software screen */
        	*ppAdapter = 0;
        	return E_NOTIMPL;
        }

        /* TODO: support hotplug */
        virtual BOOL STDMETHODCALLTYPE IsCurrent( void)
        {
        	return TRUE;
        }
};

struct GalliumDXGIAdapter
	: public GalliumMultiComObject<
		  GalliumDXGIObject<IDXGIAdapter1, GalliumDXGIFactory>,
		  IGalliumAdapter>
{
	struct native_display* display;
	const struct native_config** configs;
	std::unordered_multimap<unsigned, unsigned> configs_by_pipe_format;
	std::unordered_map<unsigned, unsigned> configs_by_native_visual_id;
	const struct native_connector** connectors;
	unsigned num_configs;
	DXGI_ADAPTER_DESC1 desc;
	std::vector<ComPtr<IDXGIOutput> > outputs;
	int num_outputs;
	struct native_event_handler handler;

	GalliumDXGIAdapter(GalliumDXGIFactory* factory, const struct native_platform* platform, void* dpy)
	{
		this->parent = factory;

		handler.invalid_surface = handle_invalid_surface;
		handler.new_drm_screen = dxgi_loader_create_drm_screen;
		handler.new_sw_screen = dxgi_loader_create_sw_screen;
		display = platform->create_display(dpy, &handler, this);
		if(!display)
			throw E_FAIL;
		memset(&desc, 0, sizeof(desc));
		std::string s = std::string("GalliumD3D on ") + display->screen->get_name(display->screen) + " by " + display->screen->get_vendor(display->screen);

		/* hopefully no one will decide to use UTF-8 in Gallium name/vendor strings */
		for(int i = 0; i < std::min((int)s.size(), 127); ++i)
			desc.Description[i] = (WCHAR)s[i];

		// TODO: add an interface to get these; for now, return mid/low values
		desc.DedicatedVideoMemory = 256 << 20;
		desc.DedicatedSystemMemory = 256 << 20;
		desc.SharedSystemMemory = 1024 << 20;

		// TODO: we should actually use an unique ID instead
		*(void**)&desc.AdapterLuid = dpy;

		configs = display->get_configs(display, (int*)&num_configs);
		for(unsigned i = 0; i < num_configs; ++i)
		{
			if(configs[i]->window_bit)
			{
				configs_by_pipe_format.insert(std::make_pair(configs[i]->color_format, i));
				configs_by_native_visual_id[configs[i]->native_visual_id] = i;
			}
		}

		connectors = 0;
		num_outputs = 0;

		if(display->modeset)
		{
			int num_crtcs;

			connectors = display->modeset->get_connectors(display, &num_outputs, &num_crtcs);
			if(!connectors)
				num_outputs = 0;
			else if(!num_outputs)
			{
				free(connectors);
				connectors = 0;
			}
		}
		if(!num_outputs)
			num_outputs = 1;
	}

	static void handle_invalid_surface(struct native_display *ndpy, struct native_surface *nsurf, unsigned int seq_num)
	{
		GalliumDXGISwapChainRevalidate((IDXGISwapChain*)nsurf->user_data);
	}

	~GalliumDXGIAdapter()
	{
		free(configs);
		free(connectors);
	}

        virtual HRESULT STDMETHODCALLTYPE EnumOutputs(
        	UINT Output,
        	__out  IDXGIOutput **ppOutput)
	{
        	if(Output >= (unsigned)num_outputs)
        		return DXGI_ERROR_NOT_FOUND;

        	if(connectors)
        	{
        		std::ostringstream ss;
        		ss << "Output #" << Output;
			return GalliumDXGIOutputCreate(this, ss.str(), connectors[Output], ppOutput);
        	}
        	else
        		return GalliumDXGIOutputCreate(this, "Unique output", NULL, ppOutput);
	}

        virtual HRESULT STDMETHODCALLTYPE GetDesc(
        	__out  DXGI_ADAPTER_DESC *pDesc)
        {
        	memcpy(pDesc, &desc, sizeof(*pDesc));
        	return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetDesc1(
        	__out  DXGI_ADAPTER_DESC1 *pDesc)
        {
        	memcpy(pDesc, &desc, sizeof(*pDesc));
        	return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(
            __in  REFGUID InterfaceName,
            __out  LARGE_INTEGER *pUMDVersion)
        {
        	// these number was taken from Windows 7 with Catalyst 10.8: its meaning is unclear
        	if(InterfaceName == IID_ID3D11Device || InterfaceName == IID_ID3D10Device1 || InterfaceName == IID_ID3D10Device)
        	{
        		pUMDVersion->QuadPart = 0x00080011000a0411ULL;
        		return S_OK;
        	}
        	return DXGI_ERROR_UNSUPPORTED;
        }

        pipe_screen* STDMETHODCALLTYPE GetGalliumScreen()
        {
        	return display->screen;
        }

        pipe_screen* STDMETHODCALLTYPE GetGalliumReferenceSoftwareScreen()
        {
        	// TODO: give a softpipe screen
        	return display->screen;
        }

        pipe_screen* STDMETHODCALLTYPE GetGalliumFastSoftwareScreen()
	{
		// TODO: give an llvmpipe screen
		return display->screen;
	}
};


struct GalliumDXGIOutput : public GalliumDXGIObject<IDXGIOutput, GalliumDXGIAdapter>
{
	DXGI_OUTPUT_DESC desc;
	const struct native_mode** modes;
	DXGI_MODE_DESC* dxgi_modes;
	unsigned num_modes;
	const struct native_connector* connector;
	DXGI_GAMMA_CONTROL* gamma;

	GalliumDXGIOutput(GalliumDXGIAdapter* adapter, std::string name, const struct native_connector* connector = 0)
	: GalliumDXGIObject(adapter), connector(connector)
	{
		memset(&desc, 0, sizeof(desc));
		for(unsigned i = 0; i < std::min(name.size(), sizeof(desc.DeviceName) - 1); ++i)
			desc.DeviceName[i] = name[i];
		desc.AttachedToDesktop = TRUE;
		/* TODO: should put an HMONITOR in desc.Monitor */

		gamma = 0;
		num_modes = 0;
		modes = 0;
		if(connector)
		{
			modes = parent->display->modeset->get_modes(parent->display, connector, (int*)&num_modes);
			if(modes && num_modes)
			{
				dxgi_modes = new DXGI_MODE_DESC[num_modes];
				for(unsigned i = 0; i < num_modes; ++i)
				{
					dxgi_modes[i].Width = modes[i]->width;
					dxgi_modes[i].Height = modes[i]->height;
					dxgi_modes[i].RefreshRate.Numerator = modes[i]->refresh_rate;
					dxgi_modes[i].RefreshRate.Denominator = 1;
					dxgi_modes[i].Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
					dxgi_modes[i].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
				}
			}
			else
			{
				if(modes)
				{
					free(modes);
					modes = 0;
				}
				goto use_fake_mode;
			}
		}
		else
		{
use_fake_mode:
			dxgi_modes = new DXGI_MODE_DESC[1];
			dxgi_modes[0].Width = 1920;
			dxgi_modes[0].Height = 1200;
			dxgi_modes[0].RefreshRate.Numerator = 60;
			dxgi_modes[0].RefreshRate.Denominator = 1;
			dxgi_modes[0].Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
			dxgi_modes[0].ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		}
	}

	~GalliumDXGIOutput()
	{
		delete [] dxgi_modes;
		free(modes);
		if(gamma)
			delete gamma;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		__out  DXGI_OUTPUT_DESC *pDesc)
	{
		*pDesc = desc;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList(
		DXGI_FORMAT EnumFormat,
		UINT Flags,
		__inout  UINT *pNumModes,
		__out_ecount_part_opt(*pNumModes,*pNumModes)  DXGI_MODE_DESC *pDesc)
	{
		/* TODO: should we return DXGI_ERROR_NOT_CURRENTLY_AVAILABLE when we don't
		 * support modesetting instead of fake modes?
		 */
		pipe_format format = dxgi_to_pipe_format[EnumFormat];
		if(parent->configs_by_pipe_format.count(format))
		{
			if(!pDesc)
			{
				*pNumModes = num_modes;
				return S_OK;
			}

			unsigned copy_modes = std::min(num_modes, *pNumModes);
			for(unsigned i = 0; i < copy_modes; ++i)
			{
				pDesc[i] = dxgi_modes[i];
				pDesc[i].Format = EnumFormat;
			}
			*pNumModes = num_modes;

			if(copy_modes < num_modes)
				return DXGI_ERROR_MORE_DATA;
			else
				return S_OK;
		}
		else
		{
			*pNumModes = 0;
			return S_OK;
		}
	}

	virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode(
		__in  const DXGI_MODE_DESC *pModeToMatch,
		__out  DXGI_MODE_DESC *pClosestMatch,
		__in_opt  IUnknown *pConcernedDevice)
	{
		/* TODO: actually implement this */
		DXGI_FORMAT dxgi_format = pModeToMatch->Format;
		enum pipe_format format = dxgi_to_pipe_format[dxgi_format];
		init_pipe_to_dxgi_format();
		if(!parent->configs_by_pipe_format.count(format))
		{
			if(!pConcernedDevice)
				return E_FAIL;
			else
			{
				format = parent->configs[0]->color_format;
				dxgi_format = pipe_to_dxgi_format[format];
			}
		}

		*pClosestMatch = dxgi_modes[0];
		pClosestMatch->Format = dxgi_format;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE WaitForVBlank( void)
	{
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE TakeOwnership(
		__in  IUnknown *pDevice,
		BOOL Exclusive)
	{
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE ReleaseOwnership( void)
	{
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities(
		__out  DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps)
	{
		memset(pGammaCaps, 0, sizeof(*pGammaCaps));
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetGammaControl(
			__in  const DXGI_GAMMA_CONTROL *pArray)
	{
		if(!gamma)
			gamma = new DXGI_GAMMA_CONTROL;
		*gamma = *pArray;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetGammaControl(
			__out  DXGI_GAMMA_CONTROL *pArray)
	{
		if(gamma)
			*pArray = *gamma;
		else
		{
			pArray->Scale.Red = 1;
			pArray->Scale.Green = 1;
			pArray->Scale.Blue = 1;
			pArray->Offset.Red = 0;
			pArray->Offset.Green = 0;
			pArray->Offset.Blue = 0;
			for(unsigned i = 0; i <= 1024; ++i)
				pArray->GammaCurve[i].Red = pArray->GammaCurve[i].Green = pArray->GammaCurve[i].Blue = (float)i / 1024.0;
		}
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface(
		__in  IDXGISurface *pScanoutSurface)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData(
		__in  IDXGISurface *pDestination)
	{
		return E_NOTIMPL;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
		__out  DXGI_FRAME_STATISTICS *pStats)
	{
		memset(pStats, 0, sizeof(*pStats));
#ifdef _WIN32
		QueryPerformanceCounter(&pStats->SyncQPCTime);
#endif
		return E_NOTIMPL;
	}
};

/* Swap chain are rather complex, and Microsoft's documentation is rather
 * lacking. As far as I know, this is the most thorough publicly available
 * description of how swap chains work, based on multiple sources and
 * experimentation.
 *
 * There are two modes (called "swap effects") that a swap chain can operate in:
 * discard and sequential.
 *
 * In discard mode, things always look as if there is a single buffer, which
 * you can get with GetBuffers(0).
 * The 2D texture returned by GetBuffers(0) and can only be
 * used as a render target view and for resource copies, since no CPU access
 * flags are set and only the D3D11_BIND_RENDER_TARGET bind flag is set.
 * On Present, it is copied to the actual display
 * surface and the contents become undefined.
 * D3D may internally use multiple buffers, but you can't observe this, except
 * by looking at the buffer contents after Present (but those are undefined).
 * If it uses multiple buffers internally, then it will normally use BufferCount buffers
 * (this has latency implications).
 * Discard mode seems to internally use a single buffer in windowed mode,
 * even if DWM is enabled, and BufferCount buffers in fullscreen mode.
 *
 * In sequential mode, the runtime alllocates BufferCount buffers.
 * You can get each with GetBuffers(n).
 * GetBuffers(0) ALWAYS points to the backbuffer to be presented and has the
 * same usage constraints as the discard mode.
 * GetBuffer(n) with n > 0 points to resources that are identical to buffer 0, but
 * are classified as "read-only resources" (due to DXGI_USAGE_READ_ONLY),
 * meaning that you can't create render target views on them, or use them as
 * a CopyResource/CopySubresourceRegion  destination.
 * It appears the only valid operation is to use them as a source for CopyResource
 * and CopySubresourceRegion as well as just waiting for them to become
 * buffer 0 again.
 * Buffer n - 1 is always displayed on screen.
 * When you call Present(), the contents of the buffers are rotated, so that buffer 0
 * goes to buffer n - 1, and is thus displayed, and buffer 1 goes to buffer 0, becomes
 * the accessible back buffer.
 * The resources themselves are NOT rotated, so that you can still render on the
 * same ID3D11Texture2D*, and views based on it, that you got before Present().
 *
 * Present seems to happen by either copying the relevant buffer into the window,
 * or alternatively making it the current one, either by programming the CRTC or
 * by sending the resource name to the DWM compositor.
 *
 * Hence, you can call GetBuffer(0) once and keep using the same ID3D11Texture2D*
 * and ID3D11RenderTargetView* (and other views if needed) you got from it.
 *
 * If the window gets resized, DXGI will then "emulate" all successive presentations,
 * by using a stretched blit automatically.
 * Thus, you should handle WM_SIZE and call ResizeBuffers to update the DXGI
 * swapchain buffers size to the new window size.
 * Doing so requires you to release all GetBuffers() results and anything referencing
 * them, including views and Direct3D11 deferred context command lists (this is
 * documented).
 *
 * How does Microsoft implement the rotation behavior?
 * It turns out that it does it by calling RotateResourceIdentitiesDXGI in the user-mode
 * DDI driver.
 * This will rotate the kernel buffer handle, or possibly rotate the GPU virtual memory
 * mappings.
 *
 * The reason this is done by driver instead of by the runtime appears to be that
 * this is necessary to support driver-provided command list support, since otherwise
 * the command list would not always target the current backbuffer, since it would
 * be done at the driver level, while only the runtime knows about the rotation.
 *
 * OK, so how do we implement this in Gallium?
 *
 * There are three strategies:
 * 1. Use a single buffer, and always copy it to a window system provided buffer, or
 *    just give the buffer to the window system if it supports that
 * 2. Rotate the buffers in the D3D1x implementation, and recreate and rebind the views.
 *     Don't support driver-provided command lists
 * 3. Add this rotation functionality to the Gallium driver, with the idea that it would rotate
 *    remap GPU virtual memory, so that virtual address are unchanged, but the physical
 *    ones are rotated (so that pushbuffers remain valid).
 *    If the driver does not support this, either fall back to (1), or have a layer doing this,
 *    putting a deferred context layer over this intermediate layer.
 *
 * (2) is not acceptable since it prevents an optimal implementation.
 *  (3) is the ideal solution, but it is complicated.
 *
 * Hence, we implement (1) for now, and will switch to (3) later.
 *
 * Note that (1) doesn't really work for DXGI_SWAP_EFFECT_SEQUENTIAL with more
 * than one buffer, so we just pretend we got asked for a single buffer in that case
 * Fortunately, no one seems to rely on that, so we'll just not implement it at first, and
 * later perform the rotation with blits.
 * Once we switch to (3), we'll just use real rotation to do it..
 *
 * DXGI_SWAP_EFFECT_SEQUENTIAL with more than one buffer is of dubious use
 * anyway, since you can only render or write to buffer 0, and other buffers can apparently
 * be used only as sources for copies.
 * I was unable to find any code using it either in DirectX SDK examples, or on the web.
 *
 * It seems the only reason you would use it is to not have to redraw from scratch, while
 * also possibly avoid a copy compared to BufferCount == 1, assuming that your
 * application is OK with having to redraw starting not from the last frame, but from
 * one/two/more frames behind it.
 *
 * A better design would forbid the user specifying BufferCount explicitly, and
 * would instead let the application give an upper bound on how old the buffer can
 * become after presentation, with "infinite" being equivalent to discard.
 * The runtime would then tell the application with frame number the buffer switched to
 * after present.
 * In addition, in a better design, the application would be allowed to specify the
 * number of buffers available, having all them usable for rendering, so that things
 * like video players could efficiently decode frames in parallel.
 * Present would in such a better design gain a way to specify the number of buffers
 * to present.
 *
 * Other miscellaneous info:
 * DXGI_PRESENT_DO_NOT_SEQUENCE causes DXGI to hold the frame for another
 * vblank interval without rotating the resource data.
 *
 * References:
 * "DXGI Overview" in MSDN
 * IDXGISwapChain documentation on MSDN
 * "RotateResourceIdentitiesDXGI" on MSDN
 * http://forums.xna.com/forums/p/42362/266016.aspx
 */

static float quad_data[] = {
	-1, -1, 0, 0,
	-1, 1, 0, 1,
	1, 1, 1, 1,
	1, -1, 1, 0,
};

struct dxgi_blitter
{
	pipe_context* pipe;
	bool normalized;
	void* fs;
	void* vs;
	void* sampler[2];
	void* elements;
	void* blend;
	void* rasterizer;
	void* zsa;
	struct pipe_clip_state clip;
	struct pipe_vertex_buffer vbuf;
	struct pipe_draw_info draw;

	dxgi_blitter(pipe_context* pipe)
	: pipe(pipe)
	{
		//normalized = !!pipe->screen->get_param(pipe, PIPE_CAP_NPOT_TEXTURES);
		// TODO: need to update buffer in unnormalized case
		normalized = true;

		struct pipe_rasterizer_state rs_state;
		memset(&rs_state, 0, sizeof(rs_state));
		rs_state.cull_face = PIPE_FACE_NONE;
		rs_state.gl_rasterization_rules = 1;
		rs_state.flatshade = 1;
		rasterizer = pipe->create_rasterizer_state(pipe, &rs_state);

		struct pipe_blend_state blendd;
		blendd.rt[0].colormask = PIPE_MASK_RGBA;
		blend = pipe->create_blend_state(pipe, &blendd);

		struct pipe_depth_stencil_alpha_state zsad;
		memset(&zsad, 0, sizeof(zsad));
		zsa = pipe->create_depth_stencil_alpha_state(pipe, &zsad);

		struct pipe_vertex_element velem[2];
		memset(&velem[0], 0, sizeof(velem[0]) * 2);
		velem[0].src_offset = 0;
		velem[0].src_format = PIPE_FORMAT_R32G32_FLOAT;
		velem[1].src_offset = 8;
		velem[1].src_format = PIPE_FORMAT_R32G32_FLOAT;
		elements = pipe->create_vertex_elements_state(pipe, 2, &velem[0]);

		for(unsigned stretch = 0; stretch < 2; ++stretch)
		{
			struct pipe_sampler_state sampler_state;
			memset(&sampler_state, 0, sizeof(sampler_state));
			sampler_state.min_img_filter = stretch ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
			sampler_state.mag_img_filter = stretch ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
			sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
			sampler_state.normalized_coords = normalized;

			sampler[stretch] = pipe->create_sampler_state(pipe, &sampler_state);
		}

		fs = util_make_fragment_tex_shader(pipe, normalized ? TGSI_TEXTURE_2D : TGSI_TEXTURE_RECT, TGSI_INTERPOLATE_LINEAR);

		const unsigned semantic_names[] = { TGSI_SEMANTIC_POSITION, TGSI_SEMANTIC_GENERIC };
		const unsigned semantic_indices[] = { 0, 0 };
		vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names, semantic_indices);

		vbuf.buffer = pipe_buffer_create(pipe->screen, PIPE_BIND_VERTEX_BUFFER, sizeof(quad_data));
		vbuf.buffer_offset = 0;
		vbuf.max_index = ~0;
		vbuf.stride = 4 * sizeof(float);
		pipe_buffer_write(pipe, vbuf.buffer, 0, sizeof(quad_data), quad_data);

		memset(&clip, 0, sizeof(clip));

		memset(&draw, 0, sizeof(draw));
		draw.mode = PIPE_PRIM_QUADS;
		draw.count = 4;
		draw.instance_count = 1;
		draw.max_index = ~0;
	}

	void blit(struct pipe_surface* surf, struct pipe_sampler_view* view, unsigned x, unsigned y, unsigned w, unsigned h)
	{
		struct pipe_framebuffer_state fb;
		memset(&fb, 0, sizeof(fb));
		fb.nr_cbufs = 1;
		fb.cbufs[0] = surf;
		fb.width = surf->width;
		fb.height = surf->height;

		struct pipe_viewport_state viewport;
		float half_width = w * 0.5f;
		float half_height = h * 0.5f;
		viewport.scale[0] = half_width;
		viewport.scale[1] = half_height;
		viewport.scale[2] = 1.0f;
		viewport.scale[3] = 1.0f;
		viewport.translate[0] = x + half_width;
		viewport.translate[1] = y + half_height;
		viewport.translate[2] = 0.0f;
		viewport.translate[3] = 1.0f;

		bool stretch = view->texture->width0 != w || view->texture->height0 != h;
		if(pipe->render_condition)
			pipe->render_condition(pipe, 0, 0);
		pipe->set_framebuffer_state(pipe, &fb);
		pipe->bind_fragment_sampler_states(pipe, 1, &sampler[stretch]);
		pipe->set_viewport_state(pipe, &viewport);
		pipe->set_clip_state(pipe, &clip);
		pipe->bind_rasterizer_state(pipe, rasterizer);
		pipe->bind_depth_stencil_alpha_state(pipe, zsa);
		pipe->bind_blend_state(pipe, blend);
		pipe->bind_vertex_elements_state(pipe, elements);
		pipe->set_vertex_buffers(pipe, 1, &vbuf);
		pipe->bind_fs_state(pipe, fs);
		pipe->bind_vs_state(pipe, vs);
		if(pipe->bind_gs_state)
			pipe->bind_gs_state(pipe, 0);
		if(pipe->bind_stream_output_state)
			pipe->bind_stream_output_state(pipe, 0);
		pipe->set_fragment_sampler_views(pipe, 1, &view);

		pipe->draw_vbo(pipe, &draw);
	}

	~dxgi_blitter()
	{
		pipe->delete_blend_state(pipe, blend);
		pipe->delete_rasterizer_state(pipe, rasterizer);
		pipe->delete_depth_stencil_alpha_state(pipe, zsa);
		pipe->delete_sampler_state(pipe, sampler[0]);
		pipe->delete_sampler_state(pipe, sampler[1]);
		pipe->delete_vertex_elements_state(pipe, elements);
		pipe->delete_vs_state(pipe, vs);
		pipe->delete_fs_state(pipe, fs);
		pipe->screen->resource_destroy(pipe->screen, vbuf.buffer);
	}
};

struct GalliumDXGISwapChain : public GalliumDXGIObject<IDXGISwapChain, GalliumDXGIFactory>
{
	ComPtr<IDXGIDevice>dxgi_device;
	ComPtr<IGalliumDevice>gallium_device;
	ComPtr<GalliumDXGIAdapter> adapter;
	ComPtr<IDXGIOutput> target;

	struct native_surface* surface;
	const struct native_config* config;

	struct pipe_resource* resources[NUM_NATIVE_ATTACHMENTS];
	int width;
	int height;
	unsigned seq_num;
	bool ever_validated;
	bool needs_validation;
	unsigned present_count;

	ComPtr<IDXGISurface> buffer0;
	struct pipe_resource* gallium_buffer0;
	struct pipe_sampler_view* gallium_buffer0_view;

	DXGI_SWAP_CHAIN_DESC desc;

	struct pipe_context* pipe;
	bool owns_pipe;

	BOOL fullscreen;

	std::auto_ptr<dxgi_blitter> blitter;
	bool formats_compatible;

	GalliumDXGISwapChain(GalliumDXGIFactory* factory, IUnknown* p_device, const DXGI_SWAP_CHAIN_DESC& p_desc)
	: GalliumDXGIObject(factory), desc(p_desc)
	{
		HRESULT hr;

		hr = p_device->QueryInterface(IID_IGalliumDevice, (void**)&gallium_device);
		if(!SUCCEEDED(hr))
			throw hr;

		hr = p_device->QueryInterface(IID_IDXGIDevice, (void**)&dxgi_device);
		if(!SUCCEEDED(hr))
			throw hr;

		hr = dxgi_device->GetAdapter((IDXGIAdapter**)&adapter);
		if(!SUCCEEDED(hr))
			throw hr;

		void* win = factory->resolver(factory->resolver_cookie, desc.OutputWindow);

		unsigned config_num;
		if(!strcmp(factory->platform->name, "X11"))
		{
			XWindowAttributes xwa;
			XGetWindowAttributes((Display*)factory->display, (Window)win, &xwa);
			config_num = adapter->configs_by_native_visual_id[xwa.visual->visualid];
		}
		else
		{
			enum pipe_format format = dxgi_to_pipe_format[desc.BufferDesc.Format];
			if(!adapter->configs_by_pipe_format.count(format))
			{
				if(adapter->configs_by_pipe_format.empty())
					throw E_FAIL;
				// TODO: choose the best match
				format = (pipe_format)adapter->configs_by_pipe_format.begin()->first;
			}
			// TODO: choose the best config
			config_num = adapter->configs_by_pipe_format.find(format)->second;
		}

		config = adapter->configs[config_num];
		surface = adapter->display->create_window_surface(adapter->display, (EGLNativeWindowType)win, config);
		surface->user_data = this;

		width = 0;
		height = 0;
		seq_num = 0;
		present_count = 0;
		needs_validation = true;
		ever_validated = false;

		if(desc.SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL && desc.BufferCount != 1)
		{
			std::cerr << "Gallium DXGI: if DXGI_SWAP_EFFECT_SEQUENTIAL is specified, only BufferCount == 1 is implemented, but " << desc.BufferCount  << " was specified: ignoring this" << std::endl;
			// change the returned desc, so that the application might perhaps notice what we did and react well
			desc.BufferCount = 1;
		}

		pipe = gallium_device->GetGalliumContext();
		owns_pipe = false;
		if(!pipe)
		{
			pipe = adapter->display->screen->context_create(adapter->display->screen, 0);
			owns_pipe = true;
		}

		blitter.reset(new dxgi_blitter(pipe));

		formats_compatible = util_is_format_compatible(
				util_format_description(dxgi_to_pipe_format[desc.BufferDesc.Format]),
				util_format_description(config->color_format));
	}

	~GalliumDXGISwapChain()
	{
		if(owns_pipe)
			pipe->destroy(pipe);
	}

        virtual HRESULT STDMETHODCALLTYPE GetDevice(
            __in  REFIID riid,
            __out  void **ppDevice)
        {
        	return dxgi_device->QueryInterface(riid, ppDevice);
        }

        HRESULT create_buffer0()
        {
        	HRESULT hr;
		ComPtr<IDXGISurface> new_buffer0;
		DXGI_USAGE usage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if(desc.SwapEffect == DXGI_SWAP_EFFECT_DISCARD)
			usage |= DXGI_USAGE_DISCARD_ON_PRESENT;
		// for our blitter
		usage |= DXGI_USAGE_SHADER_INPUT;

		DXGI_SURFACE_DESC surface_desc;
		surface_desc.Format = desc.BufferDesc.Format;
		surface_desc.Width = desc.BufferDesc.Width;
		surface_desc.Height = desc.BufferDesc.Height;
		surface_desc.SampleDesc = desc.SampleDesc;
		hr = dxgi_device->CreateSurface(&surface_desc, 1, usage, 0, &new_buffer0);
		if(!SUCCEEDED(hr))
			return hr;

		ComPtr<IGalliumResource> gallium_resource;
		hr = new_buffer0->QueryInterface(IID_IGalliumResource, (void**)&gallium_resource);
		if(!SUCCEEDED(hr))
			return hr;

		struct pipe_resource* new_gallium_buffer0 = gallium_resource->GetGalliumResource();
		if(!new_gallium_buffer0)
			return E_FAIL;

		buffer0.reset(new_buffer0.steal());
		gallium_buffer0 = new_gallium_buffer0;
		struct pipe_sampler_view templat;
		memset(&templat, 0, sizeof(templat));
		templat.texture = gallium_buffer0;
		templat.swizzle_r = 0;
		templat.swizzle_g = 1;
		templat.swizzle_b = 2;
		templat.swizzle_a = 3;
		templat.format = gallium_buffer0->format;
		gallium_buffer0_view = pipe->create_sampler_view(pipe, gallium_buffer0, &templat);
		return S_OK;
        }

	bool validate()
	{
		unsigned new_seq_num;
		needs_validation = false;

		if(!surface->validate(surface, 1 << NATIVE_ATTACHMENT_BACK_LEFT, &new_seq_num, resources, &width, &height))
			return false;

		if(!ever_validated || seq_num != new_seq_num)
		{
			seq_num = new_seq_num;
			ever_validated = true;
		}
		return true;
	}

	virtual HRESULT STDMETHODCALLTYPE Present(
		UINT SyncInterval,
		UINT Flags)
	{
		if(Flags & DXGI_PRESENT_TEST)
			return S_OK;

		if(!buffer0)
		{
			HRESULT hr = create_buffer0();
			if(!SUCCEEDED(hr))
				return hr;
		}

		if(needs_validation)
		{
			if(!validate())
				return DXGI_ERROR_DEVICE_REMOVED;
		}

		bool db = !!(config->buffer_mask & NATIVE_ATTACHMENT_BACK_LEFT);
		struct pipe_resource* dst = resources[db ? NATIVE_ATTACHMENT_BACK_LEFT : NATIVE_ATTACHMENT_FRONT_LEFT];
		struct pipe_resource* src = gallium_buffer0;
		struct pipe_surface* dst_surface = 0;

		/* TODO: sharing the context for blitting won't work correctly if queries are active
		 * Hopefully no one is crazy enough to keep queries active while presenting, expecting
		 * sensible results.
		 * We could alternatively force using another context, but that might cause inefficiency issues
		 */

		/* Windows DXGI does not scale in an aspect-preserving way, but we do this
		 * by default, since we can and it's usually what you want
		 */
		unsigned blit_x, blit_y, blit_w, blit_h;
		float black[4] = {0, 0, 0, 0};

		if(!formats_compatible || src->width0 != dst->width0 || dst->width0 != src->width0)
			dst_surface = pipe->screen->get_tex_surface(pipe->screen, dst, 0, 0, 0, PIPE_BIND_RENDER_TARGET);

		int delta = src->width0 * dst->height0 - dst->width0 * src->height0;
		if(delta > 0)
		{
			blit_w = dst->width0;
			blit_h = dst->width0 * src->height0 / src->width0;
		}
		else if(delta < 0)
		{
			blit_w = dst->height0 * src->width0 / src->height0;
			blit_h = dst->height0;
		}
		else
		{
			blit_w = dst->width0;
			blit_h = dst->height0;
		}

		blit_x = (dst->width0 - blit_w) >> 1;
		blit_y = (dst->height0 - blit_h) >> 1;

		if(blit_x)
			pipe->clear_render_target(pipe, dst_surface, black, 0, 0, blit_x, dst->height0);
		if(blit_y)
			pipe->clear_render_target(pipe, dst_surface, black, 0, 0, dst->width0, blit_y);

		if(formats_compatible && blit_w == src->width0 && blit_h == src->height0)
		{
			pipe_subresource sr;
			sr.face = 0;
			sr.level = 0;
			pipe->resource_copy_region(pipe, dst, sr, 0, 0, 0, src, sr, 0, 0, 0, blit_w, blit_h);
		}
		else
		{
			blitter->blit(dst_surface, gallium_buffer0_view, blit_x, blit_y, blit_w, blit_h);
			if(!owns_pipe)
				gallium_device->RestoreGalliumState();
		}

		if(blit_w != dst->width0)
			pipe->clear_render_target(pipe, dst_surface, black, blit_x + blit_w, 0, dst->width0 - blit_x - blit_w, dst->height0);
		if(blit_h != dst->height0)
			pipe->clear_render_target(pipe, dst_surface, black, 0, blit_y + blit_h, dst->width0, dst->height0 - blit_y - blit_h);

		if(dst_surface)
			pipe->screen->tex_surface_destroy(dst_surface);

		if(db)
		{
			if(!surface->swap_buffers(surface))
				return DXGI_ERROR_DEVICE_REMOVED;
		}
		else
		{
			if(!surface->flush_frontbuffer(surface))
				return DXGI_ERROR_DEVICE_REMOVED;
		}

		++present_count;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(
		    UINT Buffer,
		    __in  REFIID riid,
		    __out  void **ppSurface)
	{
		if(Buffer > 0)
		{
			if(desc.SwapEffect == DXGI_SWAP_EFFECT_SEQUENTIAL)
				std::cerr << "DXGI unimplemented: GetBuffer(n) with n > 0 not supported, returning buffer 0 instead!" << std::endl;
			else
				std::cerr << "DXGI error: in GetBuffer(n), n must be 0 for DXGI_SWAP_EFFECT_DISCARD\n" << std::endl;
		}

		if(!buffer0)
		{
			HRESULT hr = create_buffer0();
			if(!SUCCEEDED(hr))
				return hr;
		}
		return buffer0->QueryInterface(riid, ppSurface);
	}

	/* TODO: implement somehow */
	virtual HRESULT STDMETHODCALLTYPE SetFullscreenState(
		BOOL Fullscreen,
		__in_opt  IDXGIOutput *pTarget)
	{
		fullscreen = Fullscreen;
		target = pTarget;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFullscreenState(
		__out  BOOL *pFullscreen,
		__out  IDXGIOutput **ppTarget)
	{
		if(pFullscreen)
			*pFullscreen = fullscreen;
		if(ppTarget)
			*ppTarget = target.ref();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDesc(
		__out  DXGI_SWAP_CHAIN_DESC *pDesc)
	{
		*pDesc = desc;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeBuffers(
		UINT BufferCount,
		UINT Width,
		UINT Height,
		DXGI_FORMAT NewFormat,
		UINT SwapChainFlags)
	{
		if(buffer0)
		{
			buffer0.p->AddRef();
			ULONG v = buffer0.p->Release();
			// we must fail if there are any references to buffer0 other than ours
			if(v > 1)
				return E_FAIL;
			pipe_sampler_view_reference(&gallium_buffer0_view, 0);
			buffer0 = (IUnknown*)NULL;
			gallium_buffer0 = 0;
		}

		if(desc.SwapEffect != DXGI_SWAP_EFFECT_SEQUENTIAL)
			desc.BufferCount = BufferCount;
		desc.BufferDesc.Format = NewFormat;
		desc.BufferDesc.Width = Width;
		desc.BufferDesc.Height = Height;
		desc.Flags = SwapChainFlags;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ResizeTarget(
		const DXGI_MODE_DESC *pNewTargetParameters)
	{
		/* TODO: implement */
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetContainingOutput(
			__out  IDXGIOutput **ppOutput)
	{
		*ppOutput = adapter->outputs[0].ref();
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(
			__out  DXGI_FRAME_STATISTICS *pStats)
	{
		memset(pStats, 0, sizeof(*pStats));
#ifdef _WIN32
		QueryPerformanceCounter(&pStats->SyncQPCTime);
#endif
		pStats->PresentCount = present_count;
		pStats->PresentRefreshCount = present_count;
		pStats->SyncRefreshCount = present_count;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetLastPresentCount(
		__out  UINT *pLastPresentCount)
	{
		*pLastPresentCount = present_count;
		return S_OK;
	}
};

static void GalliumDXGISwapChainRevalidate(IDXGISwapChain* swap_chain)
{
	((GalliumDXGISwapChain*)swap_chain)->needs_validation = true;
}

static HRESULT GalliumDXGIAdapterCreate(GalliumDXGIFactory* factory, const struct native_platform* platform, void* dpy, IDXGIAdapter1** pAdapter)
{
	try
	{
		*pAdapter = new GalliumDXGIAdapter(factory, platform, dpy);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

static HRESULT GalliumDXGIOutputCreate(GalliumDXGIAdapter* adapter, const std::string& name, const struct native_connector* connector, IDXGIOutput** pOutput)
{
	try
	{
		*pOutput = new GalliumDXGIOutput(adapter, name, connector);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

static HRESULT GalliumDXGISwapChainCreate(GalliumDXGIFactory* factory, IUnknown* device, const DXGI_SWAP_CHAIN_DESC& desc, IDXGISwapChain** pSwapChain)
{
	try
	{
		*pSwapChain = new GalliumDXGISwapChain(factory, device, desc);
		return S_OK;
	}
	catch(HRESULT hr)
	{
		return hr;
	}
}

struct dxgi_binding
{
	const struct native_platform* platform;
	void* display;
	PFNHWNDRESOLVER resolver;
	void* resolver_cookie;
};

static dxgi_binding dxgi_default_binding;
static __thread dxgi_binding dxgi_thread_binding;

void STDMETHODCALLTYPE GalliumDXGIUseNothing()
{
	dxgi_thread_binding.platform = 0;
	dxgi_thread_binding.display = 0;
	dxgi_thread_binding.resolver = 0;
	dxgi_thread_binding.resolver_cookie = 0;
}

#ifdef GALLIUM_DXGI_USE_X11
void STDMETHODCALLTYPE GalliumDXGIUseX11Display(Display* dpy, PFNHWNDRESOLVER resolver, void* resolver_cookie)
{
	dxgi_thread_binding.platform = native_get_x11_platform();
	dxgi_thread_binding.display = dpy;
	dxgi_thread_binding.resolver = resolver;
	dxgi_thread_binding.resolver_cookie = resolver_cookie;
}
#endif

#ifdef GALLIUM_DXGI_USE_DRM
void STDMETHODCALLTYPE GalliumDXGIUseDRMCard(int fd)
{
	dxgi_thread_binding.platform = native_get_drm_platform();
	dxgi_thread_binding.display = (void*)fd;
	dxgi_thread_binding.resolver = 0;
	dxgi_thread_binding.resolver_cookie = 0;
}
#endif

#ifdef GALLIUM_DXGI_USE_FBDEV
void STDMETHODCALLTYPE GalliumDXGIUseFBDev(int fd)
{
	dxgi_thread_binding.platform = native_get_fbdev_platform();
	dxgi_thread_binding.display = (void*)fd;
	dxgi_thread_binding.resolver = 0;
	dxgi_thread_binding.resolver_cookie = 0;
}
#endif

#ifdef GALLIUM_DXGI_USE_GDI
void STDMETHODCALLTYPE GalliumDXGIUseHDC(HDC hdc, PFNHWNDRESOLVER resolver, void* resolver_cookie)
{
	dxgi_thread_binding.platform = native_get_gdi_platform();
	dxgi_thread_binding.display = (void*)hdc;
	dxgi_thread_binding.resolver = resolver;
	dxgi_thread_binding.resolver_cookie = resolver_cookie;
}
#endif

void STDMETHODCALLTYPE GalliumDXGIMakeDefault()
{
	dxgi_default_binding = dxgi_thread_binding;
}

 /* TODO: why did Microsoft add this? should we do something different for DXGI 1.0 and 1.1?
  * Or perhaps what they actually mean is "only create a single factory in your application"?
  * TODO: should we use a singleton here, so we never have multiple DXGI objects for the same thing? */
 HRESULT STDMETHODCALLTYPE  CreateDXGIFactory1(
		__in   REFIID riid,
		__out  void **ppFactory
)
 {
	 GalliumDXGIFactory* factory;
	 *ppFactory = 0;
	 if(dxgi_thread_binding.platform)
		 factory = new GalliumDXGIFactory(dxgi_thread_binding.platform, dxgi_thread_binding.display, dxgi_thread_binding.resolver, dxgi_thread_binding.resolver_cookie);
	 else if(dxgi_default_binding.platform)
		 factory = new GalliumDXGIFactory(dxgi_default_binding.platform, dxgi_default_binding.display, dxgi_default_binding.resolver, dxgi_default_binding.resolver_cookie);
	 else
		 factory = new GalliumDXGIFactory(native_get_x11_platform(), NULL, NULL, NULL);
	 HRESULT hres = factory->QueryInterface(riid, ppFactory);
	 factory->Release();
	 return hres;
 }

 HRESULT STDMETHODCALLTYPE  CreateDXGIFactory(
		 __in   REFIID riid,
		 __out  void **ppFactory
)
 {
	 return CreateDXGIFactory1(riid, ppFactory);
 }
