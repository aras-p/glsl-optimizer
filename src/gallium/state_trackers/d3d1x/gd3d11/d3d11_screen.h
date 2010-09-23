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

/* These cap sets are much more correct than the ones in u_caps.c */
/* TODO: it seems cube levels should be the same as 2D levels */

/* DX 9_1 */
static unsigned caps_dx_9_1[] = {
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),	/* 2048 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 8),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};

/* DX 9_2 */
static unsigned caps_dx_9_2[] = {
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_CAP(TWO_SIDED_STENCIL),
	UTIL_CHECK_CAP(TEXTURE_MIRROR_CLAMP),
	UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 1),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 12),	/* 2048 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};

/* DX 9_3 */
static unsigned caps_dx_9_3[] = {
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_CAP(TWO_SIDED_STENCIL),
	UTIL_CHECK_CAP(TEXTURE_MIRROR_CLAMP),
	UTIL_CHECK_CAP(BLEND_EQUATION_SEPARATE),
	UTIL_CHECK_CAP(SM3),
	//UTIL_CHECK_CAP(INSTANCING),
	UTIL_CHECK_CAP(OCCLUSION_QUERY),
	UTIL_CHECK_INT(MAX_RENDER_TARGETS, 4),
	UTIL_CHECK_INT(MAX_TEXTURE_2D_LEVELS, 13),	/* 4096 */
	UTIL_CHECK_INT(MAX_TEXTURE_3D_LEVELS, 9),	 /* 256 */
	UTIL_CHECK_INT(MAX_TEXTURE_CUBE_LEVELS, 10), /* 512 */
	UTIL_CHECK_TERMINATE
};


// this is called "screen" because in the D3D10 case it's only part of the device
template<bool threadsafe>
struct GalliumD3D11ScreenImpl : public GalliumD3D11Screen
{
	D3D_FEATURE_LEVEL feature_level;
	int format_support[PIPE_FORMAT_COUNT];
	unsigned creation_flags;
	unsigned exception_mode;
	maybe_mutex_t<threadsafe> mutex;

/* TODO: Direct3D 11 specifies that fine-grained locking should be used if the driver supports it.
 * Right now, I don't trust Gallium drivers to get this right.
 */
#define SYNCHRONIZED lock_t<maybe_mutex_t<threadsafe> > lock_(mutex)

	GalliumD3D11ScreenImpl(struct pipe_screen* screen, struct pipe_context* immediate_pipe, BOOL owns_immediate_pipe,unsigned creation_flags, IDXGIAdapter* adapter)
	: GalliumD3D11Screen(screen, immediate_pipe, adapter), creation_flags(creation_flags)
	{
		memset(&screen_caps, 0, sizeof(screen_caps));
		screen_caps.gs = screen->get_shader_param(screen, PIPE_SHADER_GEOMETRY, PIPE_SHADER_CAP_MAX_INSTRUCTIONS) > 0;
		screen_caps.so = !!screen->get_param(screen, PIPE_CAP_STREAM_OUTPUT);
		screen_caps.queries = screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY);
		screen_caps.render_condition = screen_caps.queries;
		for(unsigned i = 0; i < PIPE_SHADER_TYPES; ++i)
			screen_caps.constant_buffers[i] = screen->get_shader_param(screen, i, PIPE_SHADER_CAP_MAX_CONST_BUFFERS);
		screen_caps.stages = 0;
		for(unsigned i = 0; i < PIPE_SHADER_TYPES; ++i)
		{
			if(!screen->get_shader_param(screen, i, PIPE_SHADER_CAP_MAX_INSTRUCTIONS))
				break;
			screen_caps.stages = i + 1;
		}

		memset(format_support, 0xff, sizeof(format_support));

		float default_level;
		/* don't even attempt to autodetect D3D10 level support, since it's just not fully implemented yet */
		if(util_check_caps(screen, caps_dx_9_3))
			default_level = 9.3;
		else if(util_check_caps(screen, caps_dx_9_2))
			default_level = 9.2;
		else if(util_check_caps(screen, caps_dx_9_1))
			default_level = 9.1;
		else
		{
			_debug_printf("Warning: driver does not even meet D3D_FEATURE_LEVEL_9_1 features, advertising it anyway!\n");
			default_level = 9.1;
		}

		char default_level_name[64];
		sprintf(default_level_name, "%.1f", default_level);
		float feature_level_number = atof(debug_get_option("D3D11_FEATURE_LEVEL", default_level_name));
		if(!feature_level_number)
			feature_level_number = default_level;

#if API >= 11
		if(feature_level_number >= 11.0f)
			feature_level = D3D_FEATURE_LEVEL_11_0;
		else
#endif
		if(feature_level_number >= 10.1f)
			feature_level = D3D_FEATURE_LEVEL_10_1;
		else if(feature_level_number >= 10.0f)
			feature_level = D3D_FEATURE_LEVEL_10_0;
		else if(feature_level_number >= 9.3f)
			feature_level = D3D_FEATURE_LEVEL_9_3;
		else if(feature_level_number >= 9.2f)
			feature_level = D3D_FEATURE_LEVEL_9_2;
		else
			feature_level = D3D_FEATURE_LEVEL_9_1;

#if API >= 11
		immediate_context = GalliumD3D11ImmediateDeviceContext_Create(this, immediate_pipe, owns_immediate_pipe);
#endif
	}

	~GalliumD3D11ScreenImpl()
	{
#if API >= 11
		GalliumD3D11ImmediateDeviceContext_Destroy(immediate_context);
#endif
	}

	virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel(void)
	{
		return feature_level;
	}

	virtual unsigned STDMETHODCALLTYPE GetCreationFlags(void)
	{
		return creation_flags;
	}

	virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason(void)
	{
		return S_OK;
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE GetImmediateContext(
		ID3D11DeviceContext **ppImmediateContext)
	{
		immediate_context->AddRef();
		*ppImmediateContext = immediate_context;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(unsigned RaiseFlags)
	{
		exception_mode = RaiseFlags;
		return S_OK;
	}

	virtual unsigned STDMETHODCALLTYPE GetExceptionMode(void)
	{
		return exception_mode;
	}

	virtual HRESULT STDMETHODCALLTYPE CheckCounter(
		const D3D11_COUNTER_DESC *pDesc,
		D3D11_COUNTER_TYPE *pType,
		unsigned *pActiveCounters,
		LPSTR szName,
		unsigned *pNameLength,
		LPSTR szUnits,
		unsigned *pUnitsLength,
		LPSTR szDescription,
		unsigned *pDescriptionLength)
	{
		return E_NOTIMPL;
	}

	virtual void STDMETHODCALLTYPE CheckCounterInfo(
		D3D11_COUNTER_INFO *pCounterInfo)
	{
		/* none supported at the moment */
		pCounterInfo->LastDeviceDependentCounter = (D3D11_COUNTER)0;
		pCounterInfo->NumSimultaneousCounters = 0;
		pCounterInfo->NumDetectableParallelUnits = 1;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(
		D3D11_FEATURE Feature,
		void *pFeatureSupportData,
		unsigned FeatureSupportDataSize)
	{
		SYNCHRONIZED;

		switch(Feature)
		{
			case D3D11_FEATURE_THREADING:
			{
				D3D11_FEATURE_DATA_THREADING* data = (D3D11_FEATURE_DATA_THREADING*)pFeatureSupportData;
				if(FeatureSupportDataSize != sizeof(*data))
					return E_INVALIDARG;

				data->DriverCommandLists = FALSE;
				data->DriverConcurrentCreates = FALSE;
				return S_OK;
			}
			case D3D11_FEATURE_DOUBLES:
			{
				D3D11_FEATURE_DATA_DOUBLES* data = (D3D11_FEATURE_DATA_DOUBLES*)pFeatureSupportData;
				if(FeatureSupportDataSize != sizeof(*data))
					return E_INVALIDARG;

				data->DoublePrecisionFloatShaderOps = FALSE;
				return S_OK;
			}
			case D3D11_FEATURE_FORMAT_SUPPORT:
			{
				D3D11_FEATURE_DATA_FORMAT_SUPPORT* data = (D3D11_FEATURE_DATA_FORMAT_SUPPORT*)pFeatureSupportData;
				if(FeatureSupportDataSize != sizeof(*data))
					return E_INVALIDARG;

				return this->CheckFormatSupport(data->InFormat, &data->OutFormatSupport);
			}
			case D3D11_FEATURE_FORMAT_SUPPORT2:
			{
				D3D11_FEATURE_DATA_FORMAT_SUPPORT* data = (D3D11_FEATURE_DATA_FORMAT_SUPPORT*)pFeatureSupportData;
				if(FeatureSupportDataSize != sizeof(*data))
					return E_INVALIDARG;

				data->OutFormatSupport = 0;
				/* TODO: should this be S_OK? */
				return E_INVALIDARG;
			}
			case D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:
			{
				D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS* data = (D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS*)pFeatureSupportData;
				if(FeatureSupportDataSize != sizeof(*data))
					return E_INVALIDARG;

				data->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = FALSE;
				return S_OK;
			}
			default:
				return E_INVALIDARG;
		}
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(
		DXGI_FORMAT Format,
		unsigned *pFormatSupport
	)
	{
		SYNCHRONIZED;

		/* TODO: MSAA, advanced features */
		pipe_format format = dxgi_to_pipe_format[Format];
		if(!format)
			return E_INVALIDARG;

		int support = format_support[format];
		if(support < 0)
		{
			support = 0;
			unsigned buffer = D3D11_FORMAT_SUPPORT_BUFFER | D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER | D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER;
			unsigned sampler_view = D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_MIP | D3D11_FORMAT_SUPPORT_MIP_AUTOGEN;
			if(util_format_is_depth_or_stencil(format))
				sampler_view |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON;

			/* TODO: do this properly when Gallium drivers actually support index/vertex format queries */
			if(screen->is_format_supported(screen, format, PIPE_BUFFER, 0, PIPE_BIND_VERTEX_BUFFER, 0)
				|| (screen->is_format_supported(screen, format, PIPE_BUFFER, 0, PIPE_BIND_INDEX_BUFFER, 0)
				|| format == PIPE_FORMAT_R8_UNORM))
				support |= buffer;
			if(screen->is_format_supported(screen, format, PIPE_BUFFER, 0, PIPE_BIND_STREAM_OUTPUT, 0))
				support |= buffer | D3D11_FORMAT_SUPPORT_SO_BUFFER;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_1D, 0, PIPE_BIND_SAMPLER_VIEW, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE1D | sampler_view;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_SAMPLER_VIEW, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | sampler_view;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_CUBE, 0, PIPE_BIND_SAMPLER_VIEW, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | sampler_view;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_3D, 0, PIPE_BIND_SAMPLER_VIEW, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE3D | sampler_view;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET | D3D11_FORMAT_SUPPORT_BLENDABLE;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_DEPTH_STENCIL, 0))
				support |= D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_DEPTH_STENCIL;
			if(screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, PIPE_BIND_DISPLAY_TARGET, 0))
				support |= D3D11_FORMAT_SUPPORT_DISPLAY;
			format_support[format] = support;
		}
		*pFormatSupport = support;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(
		DXGI_FORMAT Format,
		unsigned SampleCount,
		unsigned *pNumQualityLevels
	)
	{
		SYNCHRONIZED;

		*pNumQualityLevels = 0;
		return S_OK;
	}

	template<typename T, typename U>
	bool convert_blend_state(T& to, const U& from, unsigned BlendEnable, unsigned RenderTargetWriteMask)
	{
		if(invalid(0
			|| from.SrcBlend >= D3D11_BLEND_COUNT
			|| from.SrcBlendAlpha >= D3D11_BLEND_COUNT
			|| from.DestBlend >= D3D11_BLEND_COUNT
			|| from.DestBlendAlpha >= D3D11_BLEND_COUNT
			|| from.BlendOp >= 6
			|| from.BlendOpAlpha >= 6
			|| !from.BlendOp
			|| !from.BlendOpAlpha
		))
			return false;

		to.blend_enable = BlendEnable;

		to.rgb_func = from.BlendOp - 1;
		to.alpha_func = from.BlendOpAlpha - 1;

		to.rgb_src_factor = d3d11_to_pipe_blend[from.SrcBlend];
		to.alpha_src_factor = d3d11_to_pipe_blend[from.SrcBlendAlpha];
		to.rgb_dst_factor = d3d11_to_pipe_blend[from.DestBlend];
		to.alpha_dst_factor = d3d11_to_pipe_blend[from.DestBlendAlpha];

		to.colormask = RenderTargetWriteMask & 0xf;
		return true;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
		const D3D11_BLEND_DESC *pBlendStateDesc,
		ID3D11BlendState **ppBlendState
	)
#else
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState1(
		const D3D10_BLEND_DESC1 *pBlendStateDesc,
		ID3D10BlendState1 **ppBlendState
	)
#endif
	{
		SYNCHRONIZED;

		pipe_blend_state state;
		memset(&state, 0, sizeof(state));
		state.alpha_to_coverage = !!pBlendStateDesc->AlphaToCoverageEnable;
		state.independent_blend_enable = !!pBlendStateDesc->IndependentBlendEnable;
		assert(PIPE_MAX_COLOR_BUFS >= 8);
		for(unsigned i = 0; i < 8; ++i)
		{
			 if(!convert_blend_state(
					 state.rt[i],
					 pBlendStateDesc->RenderTarget[i],
					 pBlendStateDesc->RenderTarget[i].BlendEnable,
					 pBlendStateDesc->RenderTarget[i].RenderTargetWriteMask))
				 return E_INVALIDARG;
		}

		if(!ppBlendState)
			return S_FALSE;

		void* object = immediate_pipe->create_blend_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*ppBlendState = new GalliumD3D11BlendState(this, object, *pBlendStateDesc);
		return S_OK;
	}

#if API < 11
	virtual HRESULT STDMETHODCALLTYPE CreateBlendState(
		const D3D10_BLEND_DESC *pBlendStateDesc,
		ID3D10BlendState **ppBlendState
	)
	{
		SYNCHRONIZED;

		pipe_blend_state state;
		memset(&state, 0, sizeof(state));
		state.alpha_to_coverage = !!pBlendStateDesc->AlphaToCoverageEnable;
		assert(PIPE_MAX_COLOR_BUFS >= 8);
		for(unsigned i = 0; i < 8; ++i)
		{
			if(!convert_blend_state(
				state.rt[i],
				*pBlendStateDesc,
				pBlendStateDesc->BlendEnable[i],
				pBlendStateDesc->RenderTargetWriteMask[i]))
				return E_INVALIDARG;
		}

		for(unsigned i = 1; i < 8; ++i)
		{
			if(memcmp(&state.rt[0], &state.rt[i], sizeof(state.rt[0])))
			{
				state.independent_blend_enable = TRUE;
				break;
			}
		}

		void* object = immediate_pipe->create_blend_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*ppBlendState = new GalliumD3D11BlendState(this, object, *pBlendStateDesc);
		return S_OK;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(
		const D3D11_DEPTH_STENCIL_DESC *pDepthStencilStateDesc,
		ID3D11DepthStencilState **ppDepthStencilState
	)
	{
		SYNCHRONIZED;

		pipe_depth_stencil_alpha_state state;
		memset(&state, 0, sizeof(state));
		state.depth.enabled = !!pDepthStencilStateDesc->DepthEnable;
		state.depth.writemask = pDepthStencilStateDesc->DepthWriteMask;
		state.depth.func = pDepthStencilStateDesc->DepthFunc - 1;
		state.stencil[0].enabled = !!pDepthStencilStateDesc->StencilEnable;
		state.stencil[0].writemask = pDepthStencilStateDesc->StencilWriteMask;
		state.stencil[0].valuemask = pDepthStencilStateDesc->StencilReadMask;
		state.stencil[0].zpass_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->FrontFace.StencilPassOp];
		state.stencil[0].fail_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->FrontFace.StencilFailOp];
		state.stencil[0].zfail_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->FrontFace.StencilDepthFailOp];
		state.stencil[0].func = pDepthStencilStateDesc->FrontFace.StencilFunc - 1;
		state.stencil[1].enabled = !!pDepthStencilStateDesc->StencilEnable;
		state.stencil[1].writemask = pDepthStencilStateDesc->StencilWriteMask;
		state.stencil[1].valuemask = pDepthStencilStateDesc->StencilReadMask;
		state.stencil[1].zpass_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->BackFace.StencilPassOp];
		state.stencil[1].fail_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->BackFace.StencilFailOp];
		state.stencil[1].zfail_op = d3d11_to_pipe_stencil_op[pDepthStencilStateDesc->BackFace.StencilDepthFailOp];
		state.stencil[1].func = pDepthStencilStateDesc->BackFace.StencilFunc - 1;

		if(!ppDepthStencilState)
			return S_FALSE;

		void* object = immediate_pipe->create_depth_stencil_alpha_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*ppDepthStencilState = new GalliumD3D11DepthStencilState(this, object, *pDepthStencilStateDesc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(
		const D3D11_RASTERIZER_DESC *pRasterizerDesc,
		ID3D11RasterizerState **ppRasterizerState)
	{
		SYNCHRONIZED;

		pipe_rasterizer_state state;
		memset(&state, 0, sizeof(state));
		state.gl_rasterization_rules = 1; /* D3D10/11 use GL rules */
		state.fill_front = state.fill_back = (pRasterizerDesc->FillMode == D3D11_FILL_WIREFRAME) ? PIPE_POLYGON_MODE_LINE : PIPE_POLYGON_MODE_FILL;
		if(pRasterizerDesc->CullMode == D3D11_CULL_FRONT)
			state.cull_face = PIPE_FACE_FRONT;
		else if(pRasterizerDesc->CullMode == D3D11_CULL_BACK)
			state.cull_face = PIPE_FACE_BACK;
		else
			state.cull_face = PIPE_FACE_NONE;
		state.front_ccw = !!pRasterizerDesc->FrontCounterClockwise;
		/* TODO: is this correct? */
		/* TODO: we are ignoring DepthBiasClamp! */
		state.offset_tri = state.offset_line = state.offset_point = pRasterizerDesc->SlopeScaledDepthBias || pRasterizerDesc->DepthBias;
		state.offset_scale = pRasterizerDesc->SlopeScaledDepthBias;
		state.offset_units = pRasterizerDesc->DepthBias;
		state.scissor = !!pRasterizerDesc->ScissorEnable;
		state.multisample = !!pRasterizerDesc->MultisampleEnable;
		state.line_smooth = !!pRasterizerDesc->AntialiasedLineEnable;

		/* TODO: is this correct? */
		state.point_quad_rasterization = 1;

		if(!ppRasterizerState)
			return S_FALSE;

		void* object = immediate_pipe->create_rasterizer_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*ppRasterizerState = new GalliumD3D11RasterizerState(this, object, *pRasterizerDesc, !pRasterizerDesc->DepthClipEnable);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(
		const D3D11_SAMPLER_DESC *pSamplerDesc,
		ID3D11SamplerState **ppSamplerState)
	{
		SYNCHRONIZED;

		pipe_sampler_state state;
		memset(&state, 0, sizeof(state));
		state.normalized_coords = 1;
		state.min_mip_filter = (pSamplerDesc->Filter & 1);
		state.mag_img_filter = ((pSamplerDesc->Filter >> 2) & 1);
		state.min_img_filter = ((pSamplerDesc->Filter >> 4) & 1);
		if(pSamplerDesc->Filter & 0x40)
			state.max_anisotropy = pSamplerDesc->MaxAnisotropy;
		if(pSamplerDesc->Filter & 0x80)
		{
			state.compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
			state.compare_func = pSamplerDesc->ComparisonFunc;
		}
		state.wrap_s = d3d11_to_pipe_wrap[pSamplerDesc->AddressU];
		state.wrap_t = d3d11_to_pipe_wrap[pSamplerDesc->AddressV];
		state.wrap_r = d3d11_to_pipe_wrap[pSamplerDesc->AddressW];
		state.lod_bias = pSamplerDesc->MipLODBias;
		memcpy(state.border_color, pSamplerDesc->BorderColor, sizeof(state.border_color));
		state.min_lod = pSamplerDesc->MinLOD;
		state.max_lod = pSamplerDesc->MaxLOD;

		if(!ppSamplerState)
			return S_FALSE;

		void* object = immediate_pipe->create_sampler_state(immediate_pipe, &state);
		if(!object)
			return E_FAIL;

		*ppSamplerState = new GalliumD3D11SamplerState(this, object, *pSamplerDesc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(
		const D3D11_INPUT_ELEMENT_DESC *pInputElementDescs,
		unsigned NumElements,
		const void *pShaderBytecodeWithInputSignature,
		SIZE_T BytecodeLength,
		ID3D11InputLayout **ppInputLayout)
	{
		SYNCHRONIZED;

		if(NumElements > D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT)
			return E_INVALIDARG;
		assert(D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT <= PIPE_MAX_ATTRIBS);

		// putting semantics matching in the core API seems to be a (minor) design mistake

		struct dxbc_chunk_signature* sig = dxbc_find_signature(pShaderBytecodeWithInputSignature, BytecodeLength, false);
		D3D11_SIGNATURE_PARAMETER_DESC* params;
		unsigned num_params = dxbc_parse_signature(sig, &params);

		typedef std::unordered_map<std::pair<c_string, unsigned>, unsigned> semantic_to_idx_map_t;
		semantic_to_idx_map_t semantic_to_idx_map;
		for(unsigned i = 0; i < NumElements; ++i)
			semantic_to_idx_map[std::make_pair(c_string(pInputElementDescs[i].SemanticName), pInputElementDescs[i].SemanticIndex)] = i;

		struct pipe_vertex_element elements[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];

		unsigned num_params_to_use = std::min(num_params, (unsigned)D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT);
		for(unsigned i = 0; i < num_params_to_use; ++i)
		{
			int idx = -1;
			semantic_to_idx_map_t::iterator iter = semantic_to_idx_map.find(std::make_pair(c_string(params[i].SemanticName), params[i].SemanticIndex));
			if(iter != semantic_to_idx_map.end())
				idx = iter->second;

			// TODO: I kind of doubt Gallium drivers will like null elements; should we do something about it, either here, in the interface, or in the drivers?
			// TODO: also, in which cases should we return errors? (i.e. duplicate semantics in vs, duplicate semantics in layout, unmatched semantic in vs, unmatched semantic in layout)
			memset(&elements[i], 0, sizeof(elements[i]));
			if(idx >= 0)
			{
				elements[i].src_format = dxgi_to_pipe_format[pInputElementDescs[idx].Format];
				elements[i].src_offset = pInputElementDescs[idx].AlignedByteOffset;
				elements[i].vertex_buffer_index = pInputElementDescs[idx].InputSlot;
				elements[i].instance_divisor = pInputElementDescs[idx].InstanceDataStepRate;
			}
		}

		free(params);

		if(!ppInputLayout)
			return S_FALSE;

		void* object = immediate_pipe->create_vertex_elements_state(immediate_pipe, num_params_to_use, elements);
		if(!object)
			return E_FAIL;

		*ppInputLayout = new GalliumD3D11InputLayout(this, object);
		return S_OK;
	}

	static unsigned d3d11_to_pipe_bind_flags(unsigned BindFlags)
	{
		unsigned bind = 0;
		if(BindFlags & D3D11_BIND_VERTEX_BUFFER)
			bind |= PIPE_BIND_VERTEX_BUFFER;
		if(BindFlags & D3D11_BIND_INDEX_BUFFER)
			bind |= PIPE_BIND_INDEX_BUFFER;
		if(BindFlags & D3D11_BIND_CONSTANT_BUFFER)
			bind |= PIPE_BIND_CONSTANT_BUFFER;
		if(BindFlags & D3D11_BIND_SHADER_RESOURCE)
			bind |= PIPE_BIND_SAMPLER_VIEW;
		if(BindFlags & D3D11_BIND_STREAM_OUTPUT)
			bind |= PIPE_BIND_STREAM_OUTPUT;
		if(BindFlags & D3D11_BIND_RENDER_TARGET)
			bind |= PIPE_BIND_RENDER_TARGET;
		if(BindFlags & D3D11_BIND_DEPTH_STENCIL)
			bind |= PIPE_BIND_DEPTH_STENCIL;
		return bind;
	}

	inline HRESULT create_resource(
		pipe_texture_target target,
		unsigned Width,
		unsigned Height,
		unsigned Depth,
		unsigned MipLevels,
		unsigned ArraySize,
		DXGI_FORMAT Format,
		const DXGI_SAMPLE_DESC* SampleDesc,
		D3D11_USAGE Usage,
		unsigned BindFlags,
		unsigned CPUAccessFlags,
		unsigned MiscFlags,
		const D3D11_SUBRESOURCE_DATA *pInitialData,
		DXGI_USAGE dxgi_usage,
		struct pipe_resource** ppresource
	)
	{
		if(invalid(Format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		if(MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
		{
			if(target != PIPE_TEXTURE_2D)
				return E_INVALIDARG;
			target = PIPE_TEXTURE_CUBE;

			if(ArraySize != 6)
				return E_NOTIMPL;
		}
		else
		{
			if(ArraySize > 1)
				return E_NOTIMPL;
			ArraySize = 1;
		}
		/* TODO: msaa */
		struct pipe_resource templat;
		memset(&templat, 0, sizeof(templat));
		templat.target = target;
		templat.width0 = Width;
		templat.height0 = Height;
		templat.depth0 = Depth;
		templat.last_level = MipLevels ? (MipLevels - 1) : 0;
		templat.format = dxgi_to_pipe_format[Format];
		templat.bind = d3d11_to_pipe_bind_flags(BindFlags);
		if(CPUAccessFlags & D3D11_CPU_ACCESS_READ)
			templat.bind |= PIPE_BIND_TRANSFER_READ;
		if(CPUAccessFlags & D3D11_CPU_ACCESS_WRITE)
			templat.bind |= PIPE_BIND_TRANSFER_WRITE;
		if(MiscFlags & D3D11_RESOURCE_MISC_SHARED)
			templat.bind |= PIPE_BIND_SHARED;
		if(MiscFlags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
			templat.bind |= PIPE_BIND_TRANSFER_READ | PIPE_BIND_TRANSFER_WRITE;
		if(dxgi_usage & DXGI_USAGE_BACK_BUFFER)
			templat.bind |= PIPE_BIND_DISPLAY_TARGET;
		templat.usage = d3d11_to_pipe_usage[Usage];
		if(invalid(!templat.format))
			return E_NOTIMPL;

		if(!ppresource)
			return S_FALSE;

		struct pipe_resource* resource = screen->resource_create(screen, &templat);
		if(!resource)
			return E_FAIL;
		if(pInitialData)
		{
			for(unsigned slice = 0; slice < ArraySize; ++slice)
			{
				for(unsigned level = 0; level <= templat.last_level; ++level)
				{
					struct pipe_subresource sr;
					sr.level = level;
					sr.face = slice;
					struct pipe_box box;
					box.x = box.y = box.z = 0;
					box.width = u_minify(Width, level);
					box.height = u_minify(Height, level);
					box.depth = u_minify(Depth, level);
					immediate_pipe->transfer_inline_write(immediate_pipe, resource, sr, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD | PIPE_TRANSFER_UNSYNCHRONIZED, &box, pInitialData->pSysMem, pInitialData->SysMemPitch, pInitialData->SysMemSlicePitch);
					++pInitialData;
				}
			}
		}
		*ppresource = resource;
		return S_OK;
	}

	static unsigned d3d_to_dxgi_usage(unsigned bind, unsigned misc)
	{
		unsigned dxgi_usage = 0;
		if(bind |= D3D11_BIND_RENDER_TARGET)
			dxgi_usage |= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		if(bind & D3D11_BIND_SHADER_RESOURCE)
			dxgi_usage |= DXGI_USAGE_SHADER_INPUT;
#if API >= 11
		if(bind & D3D11_BIND_UNORDERED_ACCESS)
			dxgi_usage |= DXGI_USAGE_UNORDERED_ACCESS;
#endif
		if(misc & D3D11_RESOURCE_MISC_SHARED)
			dxgi_usage |= DXGI_USAGE_SHARED;
		return dxgi_usage;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(
		const D3D11_TEXTURE1D_DESC *pDesc,
		const D3D11_SUBRESOURCE_DATA *pInitialData,
		ID3D11Texture1D **ppTexture1D)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(pDesc->BindFlags, pDesc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_1D, pDesc->Width, 1, 1, pDesc->MipLevels, pDesc->ArraySize, pDesc->Format, 0, pDesc->Usage, pDesc->BindFlags, pDesc->CPUAccessFlags, pDesc->MiscFlags, pInitialData, dxgi_usage, ppTexture1D ? &resource : 0);
		if(hr != S_OK)
			return hr;
		*ppTexture1D = new GalliumD3D11Texture1D(this, resource, *pDesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(
		const D3D11_TEXTURE2D_DESC *pDesc,
		const D3D11_SUBRESOURCE_DATA *pInitialData,
		ID3D11Texture2D **ppTexture2D)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(pDesc->BindFlags, pDesc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_2D, pDesc->Width, pDesc->Height, 1, pDesc->MipLevels, pDesc->ArraySize, pDesc->Format, &pDesc->SampleDesc, pDesc->Usage, pDesc->BindFlags, pDesc->CPUAccessFlags, pDesc->MiscFlags, pInitialData, dxgi_usage, ppTexture2D ? &resource : 0);
		if(hr != S_OK)
			return hr;
		if(pDesc->MipLevels == 1 && pDesc->ArraySize == 1)
			*ppTexture2D = new GalliumD3D11Surface(this, resource, *pDesc, dxgi_usage);
		else
			*ppTexture2D = new GalliumD3D11Texture2D(this, resource, *pDesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(
		const D3D11_TEXTURE3D_DESC *pDesc,
		const D3D11_SUBRESOURCE_DATA *pInitialData,
		ID3D11Texture3D **ppTexture3D)
	{
		SYNCHRONIZED;

		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(pDesc->BindFlags, pDesc->MiscFlags);
		HRESULT hr = create_resource(PIPE_TEXTURE_3D, pDesc->Width, pDesc->Height, pDesc->Depth, pDesc->MipLevels, 1, pDesc->Format, 0, pDesc->Usage, pDesc->BindFlags, pDesc->CPUAccessFlags, pDesc->MiscFlags, pInitialData, dxgi_usage, ppTexture3D ? &resource : 0);
		if(hr != S_OK)
			return hr;
		*ppTexture3D = new GalliumD3D11Texture3D(this, resource, *pDesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateBuffer(
		const D3D11_BUFFER_DESC *pDesc,
		const D3D11_SUBRESOURCE_DATA *pInitialData,
		ID3D11Buffer **ppBuffer)
	{
		SYNCHRONIZED;

#if API >= 11
		if(pDesc->StructureByteStride > 1)
			return E_NOTIMPL;
#endif
		struct pipe_resource* resource;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(pDesc->BindFlags, pDesc->MiscFlags);
		HRESULT hr = create_resource(PIPE_BUFFER, pDesc->ByteWidth, 1, 1, 1, 1, DXGI_FORMAT_R8_UNORM, 0, pDesc->Usage, pDesc->BindFlags, pDesc->CPUAccessFlags, pDesc->MiscFlags, pInitialData, dxgi_usage, ppBuffer ? &resource : 0);
		if(hr != S_OK)
			return hr;
		*ppBuffer = new GalliumD3D11Buffer(this, resource, *pDesc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE OpenGalliumResource(
		struct pipe_resource* resource,
		IUnknown** dxgi_resource)
	{
		SYNCHRONIZED;

		/* TODO: maybe support others */
		assert(resource->target == PIPE_TEXTURE_2D);
		*dxgi_resource = 0;
		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));
		desc.Width = resource->width0;
		desc.Height = resource->height0;
		init_pipe_to_dxgi_format();
		desc.Format = pipe_to_dxgi_format[resource->format];
		desc.SampleDesc.Count = resource->nr_samples;
		desc.SampleDesc.Quality = 0;
		desc.ArraySize = 1;
		desc.MipLevels = resource->last_level + 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		if(resource->bind & PIPE_BIND_RENDER_TARGET)
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if(resource->bind & PIPE_BIND_DEPTH_STENCIL)
			desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		if(resource->bind & PIPE_BIND_SAMPLER_VIEW)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if(resource->bind & PIPE_BIND_SHARED)
			desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		DXGI_USAGE dxgi_usage = d3d_to_dxgi_usage(desc.BindFlags, desc.MiscFlags);
		if(desc.MipLevels == 1 && desc.ArraySize == 1)
			*dxgi_resource = (ID3D11Texture2D*)new GalliumD3D11Surface(this, resource, desc, dxgi_usage);
		else
			*dxgi_resource = (ID3D11Texture2D*)new GalliumD3D11Texture2D(this, resource, desc, dxgi_usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateSurface(
		const DXGI_SURFACE_DESC *pDesc,
		unsigned NumSurfaces,
		DXGI_USAGE Usage,
		const DXGI_SHARED_RESOURCE *pSharedResource,
		IDXGISurface **ppSurface)
	{
		SYNCHRONIZED;

		D3D11_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(desc));

		struct pipe_resource* resource;
		desc.Width = pDesc->Width;
		desc.Height = pDesc->Height;
		desc.Format = pDesc->Format;
		desc.SampleDesc = pDesc->SampleDesc;
		desc.ArraySize = NumSurfaces;
		desc.MipLevels = 1;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
		if(Usage & DXGI_USAGE_RENDER_TARGET_OUTPUT)
			desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if(Usage & DXGI_USAGE_SHADER_INPUT)
			desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
#if API >= 11
		if(Usage & DXGI_USAGE_UNORDERED_ACCESS)
			desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
#endif
		if(Usage & DXGI_USAGE_SHARED)
			desc.MiscFlags |= D3D11_RESOURCE_MISC_SHARED;
		HRESULT hr = create_resource(PIPE_TEXTURE_2D, pDesc->Width, pDesc->Height, 1, 1, NumSurfaces, pDesc->Format, &pDesc->SampleDesc, D3D11_USAGE_DEFAULT, desc.BindFlags, D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE, desc.MiscFlags, 0, Usage, &resource);
		if(hr != S_OK)
			return hr;
		*ppSurface = new GalliumD3D11Surface(this, resource, desc, Usage);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(
		ID3D11Resource *pResource,
		const D3D11_SHADER_RESOURCE_VIEW_DESC *pDesc,
		ID3D11ShaderResourceView **ppSRView)
	{
#if API >= 11
		D3D11_SHADER_RESOURCE_VIEW_DESC def_desc;
#else
		if(pDesc->ViewDimension == D3D10_1_SRV_DIMENSION_TEXTURECUBEARRAY)
			return E_INVALIDARG;
		D3D10_SHADER_RESOURCE_VIEW_DESC1 desc1;
		memset(&desc1, 0, sizeof(desc1));
		memcpy(&desc1, pDesc, sizeof(*pDesc));
		return CreateShaderResourceView1(pResource, &desc1, (ID3D10ShaderResourceView1**)ppSRView);
	}

	virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView1(
			ID3D11Resource *pResource,
			const D3D10_SHADER_RESOURCE_VIEW_DESC1 *pDesc,
			ID3D10ShaderResourceView1 **ppSRView)
	{
		D3D10_SHADER_RESOURCE_VIEW_DESC1 def_desc;
#endif
		SYNCHRONIZED;

		if(!pDesc)
		{
			struct pipe_resource* resource = ((GalliumD3D11Resource<>*)pResource)->resource;
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_BUFFER:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
				def_desc.Buffer.ElementWidth = 1;
#if API >= 11
				def_desc.Buffer.NumElements = resource->width0;
#endif
				break;
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
				def_desc.Texture1D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				def_desc.Texture2D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_3D:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				def_desc.Texture3D.MipLevels = resource->last_level + 1;
				break;
			case PIPE_TEXTURE_CUBE:
				def_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
				def_desc.TextureCube.MipLevels = resource->last_level + 1;
				break;
			default:
				return E_INVALIDARG;
			}
			pDesc = &def_desc;
		}

		struct pipe_sampler_view templat;
		memset(&templat, 0, sizeof(templat));
		if(invalid(Format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		templat.format = dxgi_to_pipe_format[pDesc->Format];
		if(!templat.format)
			return E_NOTIMPL;
		templat.swizzle_r = PIPE_SWIZZLE_RED;
		templat.swizzle_g = PIPE_SWIZZLE_GREEN;
		templat.swizzle_b = PIPE_SWIZZLE_BLUE;
		templat.swizzle_a = PIPE_SWIZZLE_ALPHA;

		templat.texture = ((GalliumD3D11Resource<>*)pResource)->resource;
		switch(pDesc->ViewDimension)
		{
		case D3D11_SRV_DIMENSION_TEXTURE1D:
		case D3D11_SRV_DIMENSION_TEXTURE2D:
		case D3D11_SRV_DIMENSION_TEXTURE3D:
		case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
			/* yes, this works for all of these types (but TODO: texture arrays) */
			templat.first_level = pDesc->Texture1D.MostDetailedMip;
			templat.last_level = templat.first_level + pDesc->Texture1D.MipLevels - 1;
			break;
		case D3D11_SRV_DIMENSION_BUFFER:
		case D3D11_SRV_DIMENSION_TEXTURE2DMS:
		case D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!ppSRView)
			return S_FALSE;

		struct pipe_sampler_view* view = immediate_pipe->create_sampler_view(immediate_pipe, templat.texture, &templat);
		if(!view)
			return E_FAIL;
		*ppSRView = new GalliumD3D11ShaderResourceView(this, (GalliumD3D11Resource<>*)pResource, view, *pDesc);
		return S_OK;
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(
		ID3D11Resource *pResource,
		const D3D11_UNORDERED_ACCESS_VIEW_DESC *pDesc,
		ID3D11UnorderedAccessView **ppUAView)
	{
		SYNCHRONIZED;

		return E_NOTIMPL;

		// remember to return S_FALSE and not crash if ppUAView == 0 and parameters are valid
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(
		ID3D11Resource *pResource,
		const D3D11_RENDER_TARGET_VIEW_DESC *pDesc,
		ID3D11RenderTargetView **ppRTView)
	{
		SYNCHRONIZED;

		D3D11_RENDER_TARGET_VIEW_DESC def_desc;
		if(!pDesc)
		{
			struct pipe_resource* resource = ((GalliumD3D11Resource<>*)pResource)->resource;
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_BUFFER:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_BUFFER;
				def_desc.Buffer.ElementWidth = 1;
#if API >= 11
				def_desc.Buffer.NumElements = resource->width0;
#endif
				break;
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE1D;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				break;
			case PIPE_TEXTURE_3D:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE3D;
				def_desc.Texture3D.WSize = resource->depth0;
				break;
			case PIPE_TEXTURE_CUBE:
				def_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = 6;
				break;
			default:
				return E_INVALIDARG;
			}
			pDesc = &def_desc;
		}

		unsigned zslice = 0;
		unsigned face = 0;
		unsigned level;
		enum pipe_format format;
		if(invalid(pDesc->Format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		format = dxgi_to_pipe_format[pDesc->Format];
		if(!format)
			return E_NOTIMPL;

		switch(pDesc->ViewDimension)
		{
		case D3D11_RTV_DIMENSION_TEXTURE1D:
		case D3D11_RTV_DIMENSION_TEXTURE2D:
			level = pDesc->Texture1D.MipSlice;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE3D:
			level = pDesc->Texture3D.MipSlice;
			zslice = pDesc->Texture3D.FirstWSlice;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
			level = pDesc->Texture1DArray.MipSlice;
			face = pDesc->Texture1DArray.FirstArraySlice;
			break;
		case D3D11_RTV_DIMENSION_BUFFER:
		case D3D11_RTV_DIMENSION_TEXTURE2DMS:
		case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!ppRTView)
			return S_FALSE;

		struct pipe_surface* surface = screen->get_tex_surface(screen,
				((GalliumD3D11Resource<>*)pResource)->resource,
				face, level, zslice, PIPE_BIND_RENDER_TARGET);
		if(!surface)
			return E_FAIL;
		/* muhahahahaha, let's hope this actually works */
		surface->format = format;
		*ppRTView = new GalliumD3D11RenderTargetView(this, (GalliumD3D11Resource<>*)pResource, surface, *pDesc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(
		ID3D11Resource *pResource,
		const D3D11_DEPTH_STENCIL_VIEW_DESC *pDesc,
		ID3D11DepthStencilView **ppDepthStencilView)
	{
		SYNCHRONIZED;

		D3D11_DEPTH_STENCIL_VIEW_DESC def_desc;
		if(!pDesc)
		{
			struct pipe_resource* resource = ((GalliumD3D11Resource<>*)pResource)->resource;
			init_pipe_to_dxgi_format();
			memset(&def_desc, 0, sizeof(def_desc));
			def_desc.Format = pipe_to_dxgi_format[resource->format];
			switch(resource->target)
			{
			case PIPE_TEXTURE_1D:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE1D;
				break;
			case PIPE_TEXTURE_2D:
			case PIPE_TEXTURE_RECT:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				break;
			case PIPE_TEXTURE_CUBE:
				def_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				def_desc.Texture2DArray.ArraySize = 6;
				break;
			default:
				return E_INVALIDARG;
			}
			pDesc = &def_desc;
		}

		unsigned zslice = 0;
		unsigned face = 0;
		unsigned level;
		enum pipe_format format;
		if(invalid(pDesc->Format >= DXGI_FORMAT_COUNT))
			return E_INVALIDARG;
		format = dxgi_to_pipe_format[pDesc->Format];
		if(!format)
			return E_NOTIMPL;

		switch(pDesc->ViewDimension)
		{
		case D3D11_DSV_DIMENSION_TEXTURE1D:
		case D3D11_DSV_DIMENSION_TEXTURE2D:
			level = pDesc->Texture1D.MipSlice;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
		case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
			level = pDesc->Texture1DArray.MipSlice;
			face = pDesc->Texture1DArray.FirstArraySlice;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2DMS:
		case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
			return E_NOTIMPL;
		default:
			return E_INVALIDARG;
		}

		if(!ppDepthStencilView)
			return S_FALSE;

		struct pipe_surface* surface = screen->get_tex_surface(screen,
				((GalliumD3D11Resource<>*)pResource)->resource,
				face, level, zslice, PIPE_BIND_DEPTH_STENCIL);
		if(!surface)
			return E_FAIL;
		/* muhahahahaha, let's hope this actually works */
		surface->format = format;
		*ppDepthStencilView = new GalliumD3D11DepthStencilView(this, (GalliumD3D11Resource<>*)pResource, surface, *pDesc);
		return S_OK;
	}

	GalliumD3D11Shader<>* create_stage_shader(unsigned type, const void *pShaderBytecode, SIZE_T BytecodeLength
#if API >= 11
			, ID3D11ClassLinkage *pClassLinkage
#endif
			)
	{
		dxbc_chunk_header* sm4_chunk = dxbc_find_shader_bytecode(pShaderBytecode, BytecodeLength);
		if(!sm4_chunk)
			return 0;

		std::auto_ptr<sm4_program> sm4(sm4_parse(sm4_chunk + 1, bswap_le32(sm4_chunk->size)));
		if(!sm4.get())
			return 0;

		struct pipe_shader_state tgsi_shader;
		memset(&tgsi_shader, 0, sizeof(tgsi_shader));
		tgsi_shader.tokens = (const tgsi_token*)sm4_to_tgsi(*sm4);
		if(!tgsi_shader.tokens)
			return 0;

		void* shader_cso;
		GalliumD3D11Shader<>* shader;

		switch(type)
		{
		case PIPE_SHADER_VERTEX:
			shader_cso = immediate_pipe->create_vs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11VertexShader(this, shader_cso);
			break;
		case PIPE_SHADER_FRAGMENT:
			shader_cso = immediate_pipe->create_fs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11PixelShader(this, shader_cso);
			break;
		case PIPE_SHADER_GEOMETRY:
			shader_cso = immediate_pipe->create_gs_state(immediate_pipe, &tgsi_shader);
			shader = (GalliumD3D11Shader<>*)new GalliumD3D11GeometryShader(this, shader_cso);
			break;
		default:
			shader_cso = 0;
			shader = 0;
			break;
		}

		if(shader)
		{
			shader->slot_to_resource = sm4->slot_to_resource;
			shader->slot_to_sampler = sm4->slot_to_sampler;
		}

		free((void*)tgsi_shader.tokens);
		return shader;
	}

#if API >= 11
#define CREATE_SHADER_ARGS \
	const void *pShaderBytecode, \
	SIZE_T BytecodeLength, \
	ID3D11ClassLinkage *pClassLinkage
#define PASS_SHADER_ARGS pShaderBytecode, BytecodeLength, pClassLinkage
#else
#define CREATE_SHADER_ARGS \
	const void *pShaderBytecode, \
	SIZE_T BytecodeLength
#define PASS_SHADER_ARGS pShaderBytecode, BytecodeLength
#endif

#define IMPLEMENT_CREATE_SHADER(Stage, GALLIUM) \
	virtual HRESULT STDMETHODCALLTYPE Create##Stage##Shader( \
		CREATE_SHADER_ARGS, \
		ID3D11##Stage##Shader **pp##Stage##Shader) \
	{ \
		SYNCHRONIZED; \
		GalliumD3D11##Stage##Shader* shader = (GalliumD3D11##Stage##Shader*)create_stage_shader(PIPE_SHADER_##GALLIUM, PASS_SHADER_ARGS); \
		if(!shader) \
			return E_FAIL; \
		if(pp##Stage##Shader) \
		{ \
			*pp##Stage##Shader = shader; \
			return S_OK; \
		} \
		else \
		{ \
			shader->Release(); \
			return S_FALSE; \
		} \
	}

#define IMPLEMENT_NOTIMPL_CREATE_SHADER(Stage) \
	virtual HRESULT STDMETHODCALLTYPE Create##Stage##Shader( \
		CREATE_SHADER_ARGS, \
		ID3D11##Stage##Shader **pp##Stage##Shader) \
	{ \
		return E_NOTIMPL; \
	}

	IMPLEMENT_CREATE_SHADER(Vertex, VERTEX)
	IMPLEMENT_CREATE_SHADER(Pixel, FRAGMENT)
	IMPLEMENT_CREATE_SHADER(Geometry, GEOMETRY)
#if API >= 11
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Hull)
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Domain)
	IMPLEMENT_NOTIMPL_CREATE_SHADER(Compute)
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(
		const void *pShaderBytecode,
		SIZE_T BytecodeLength,
		const D3D11_SO_DECLARATION_ENTRY *pSODeclaration,
		unsigned NumEntries,
#if API >= 11
		const unsigned *pBufferStrides,
		unsigned NumStrides,
		unsigned RasterizedStream,
		ID3D11ClassLinkage *pClassLinkage,
#else
		UINT OutputStreamStride,
#endif
		ID3D11GeometryShader **ppGeometryShader)
	{
		SYNCHRONIZED;

		if(!ppGeometryShader)
			return S_FALSE;

		return E_NOTIMPL;

		// remember to return S_FALSE if ppGeometyShader == NULL and the shader is OK
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(
			ID3D11ClassLinkage **ppLinkage)
	{
		SYNCHRONIZED;

		if(!ppLinkage)
			return S_FALSE;

		return E_NOTIMPL;
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE CreateQuery(
		const D3D11_QUERY_DESC *pQueryDesc,
		ID3D11Query **ppQuery)
	{
		SYNCHRONIZED;

		if(invalid(pQueryDesc->Query >= D3D11_QUERY_COUNT))
			return E_INVALIDARG;
		unsigned query_type = d3d11_to_pipe_query[pQueryDesc->Query];
		if(!query_type)
			return E_NOTIMPL;

		if(ppQuery)
			return S_FALSE;

		struct pipe_query* query = immediate_pipe->create_query(immediate_pipe, query_type);
		if(!query)
			return E_FAIL;

		*ppQuery = new GalliumD3D11Query(this, query, d3d11_query_size[pQueryDesc->Query], *pQueryDesc);
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE CreatePredicate(
		const D3D11_QUERY_DESC *pPredicateDesc,
		ID3D11Predicate **ppPredicate)
	{
		SYNCHRONIZED;

		unsigned query_type;
		switch(pPredicateDesc->Query)
		{
		case D3D11_QUERY_SO_OVERFLOW_PREDICATE:
			return E_NOTIMPL;
		case D3D11_QUERY_OCCLUSION_PREDICATE:
			query_type = PIPE_QUERY_OCCLUSION_COUNTER;
			break;
		default:
			return E_INVALIDARG;
		}

		if(ppPredicate)
			return S_FALSE;

		struct pipe_query* query = immediate_pipe->create_query(immediate_pipe, query_type);
		if(!query)
			return E_FAIL;

		*ppPredicate = new GalliumD3D11Predicate(this, query, sizeof(BOOL), *pPredicateDesc);
		return S_OK;
	}


	virtual HRESULT STDMETHODCALLTYPE CreateCounter(
		const D3D11_COUNTER_DESC *pCounterDesc,
		ID3D11Counter **ppCounter)
	{
		SYNCHRONIZED;

		return E_NOTIMPL;

		// remember to return S_FALSE if ppCounter == NULL and everything is OK
	}

#if API >= 11
	virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(
		unsigned ContextFlags,
		ID3D11DeviceContext **ppDeferredContext)
	{
		SYNCHRONIZED;

		// TODO: this will have to be implemented using a new Gallium util module
		return E_NOTIMPL;

		// remember to return S_FALSE if ppCounter == NULL and everything is OK
	}
#endif

	virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(
			HANDLE hResource,
			REFIID ReturnedInterface,
			void **ppResource)
	{
		SYNCHRONIZED;

		// TODO: the problem here is that we need to communicate dimensions somehow
		return E_NOTIMPL;

		// remember to return S_FALSE if ppCounter == NULL and everything is OK
#if 0
		struct pipe_resou	rce templat;
		struct winsys_handle handle;
		handle.stride = 0;
		handle.handle = hResource;
		handle.type = DRM_API_HANDLE_TYPE_SHARED;
		screen->resource_from_handle(screen, &templat, &handle);
#endif
	}

#if API < 11
	/* these are documented as "Not implemented".
	 * According to the UMDDI documentation, they apparently turn on a
	 * (Width + 1) x (Height + 1) convolution filter for 1-bit textures.
	 * Probably nothing uses these, assuming it has ever been implemented anywhere.
	 */
	void STDMETHODCALLTYPE SetTextFilterSize(
		UINT Width,
		UINT Height
	)
	{}

	virtual void STDMETHODCALLTYPE GetTextFilterSize(
		UINT *Width,
		UINT *Height
	)
	{}
#endif

#if API >= 11
	virtual void STDMETHODCALLTYPE RestoreGalliumState()
	{
		GalliumD3D11ImmediateDeviceContext_RestoreGalliumState(immediate_context);
	}

	virtual void STDMETHODCALLTYPE RestoreGalliumStateBlitOnly()
	{
		GalliumD3D11ImmediateDeviceContext_RestoreGalliumStateBlitOnly(immediate_context);
	}
#endif

	virtual struct pipe_context* STDMETHODCALLTYPE GetGalliumContext(void)
	{
		return immediate_pipe;
	}

#undef SYNCHRONIZED
};
