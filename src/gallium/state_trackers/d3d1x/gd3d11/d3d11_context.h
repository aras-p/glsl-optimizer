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

/* used to unbind things, we need 128 due to resources */
static const void* zero_data[128];

#define UPDATE_VIEWS_SHIFT (D3D11_STAGES * 0)
#define UPDATE_SAMPLERS_SHIFT (D3D11_STAGES * 1)
#define UPDATE_VERTEX_BUFFERS (1 << (D3D11_STAGES * 2))

#if API >= 11
template<typename PtrTraits>
struct GalliumD3D11DeviceContext :
	public GalliumD3D11DeviceChild<ID3D11DeviceContext>
{
#else
template<bool threadsafe>
struct GalliumD3D10Device : public GalliumD3D10ScreenImpl<threadsafe>
{
	typedef simple_ptr_traits PtrTraits;
	typedef GalliumD3D10Device GalliumD3D10DeviceContext;
#endif

	refcnt_ptr<GalliumD3D11Shader<>, PtrTraits> shaders[D3D11_STAGES];
	refcnt_ptr<GalliumD3D11InputLayout, PtrTraits> input_layout;
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> index_buffer;
	refcnt_ptr<GalliumD3D11RasterizerState, PtrTraits> rasterizer_state;
	refcnt_ptr<GalliumD3D11DepthStencilState, PtrTraits> depth_stencil_state;
	refcnt_ptr<GalliumD3D11BlendState, PtrTraits> blend_state;
	refcnt_ptr<GalliumD3D11DepthStencilView, PtrTraits> depth_stencil_view;
	refcnt_ptr<GalliumD3D11Predicate, PtrTraits> render_predicate;

	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> constant_buffers[D3D11_STAGES][D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11ShaderResourceView, PtrTraits> shader_resource_views[D3D11_STAGES][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11SamplerState, PtrTraits> samplers[D3D11_STAGES][D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> input_buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	refcnt_ptr<GalliumD3D11RenderTargetView, PtrTraits> render_target_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
	refcnt_ptr<GalliumD3D11Buffer, PtrTraits> so_targets[D3D11_SO_BUFFER_SLOT_COUNT];

#if API >= 11
	refcnt_ptr<ID3D11UnorderedAccessView, PtrTraits> cs_unordered_access_views[D3D11_PS_CS_UAV_REGISTER_COUNT];
	refcnt_ptr<ID3D11UnorderedAccessView, PtrTraits> om_unordered_access_views[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
#endif

	D3D11_VIEWPORT viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	D3D11_RECT scissor_rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
	unsigned so_offsets[D3D11_SO_BUFFER_SLOT_COUNT];
	D3D11_PRIMITIVE_TOPOLOGY primitive_topology;
	DXGI_FORMAT index_format;
	unsigned index_offset;
	BOOL render_predicate_value;
	float blend_color[4];
	unsigned sample_mask;
	unsigned stencil_ref;
	bool depth_clamp;

	void* default_input_layout;
	void* default_rasterizer;
	void* default_depth_stencil;
	void* default_blend;
	void* default_sampler;
	void* ld_sampler;
	void * default_shaders[D3D11_STAGES];

	// derived state
	int primitive_mode;
	struct pipe_vertex_buffer vertex_buffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
	struct pipe_resource* so_buffers[D3D11_SO_BUFFER_SLOT_COUNT];
	struct pipe_sampler_view* sampler_views[D3D11_STAGES][D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
	struct
	{
		void* ld; // accessed with a -1 index from v
		void* v[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
	} sampler_csos[D3D11_STAGES];
	struct pipe_resource * buffers[D3D11_SO_BUFFER_SLOT_COUNT];
	unsigned num_shader_resource_views[D3D11_STAGES];
	unsigned num_samplers[D3D11_STAGES];
	unsigned num_vertex_buffers;
	unsigned num_render_target_views;
	unsigned num_viewports;
	unsigned num_scissor_rects;
	unsigned num_so_targets;

	struct pipe_context* pipe;
	unsigned update_flags;

	bool owns_pipe;
	unsigned context_flags;

	GalliumD3D11Caps caps;

	cso_context* cso_ctx;
	gen_mipmap_state* gen_mipmap;

#if API >= 11
#define SYNCHRONIZED do {} while(0)

	GalliumD3D11DeviceContext(GalliumD3D11Screen* device, pipe_context* pipe, bool owns_pipe, unsigned context_flags = 0)
	: GalliumD3D11DeviceChild(device), pipe(pipe), owns_pipe(owns_pipe), context_flags(context_flags)
	{
		caps = device->screen_caps;
		init_context();
	}

	~GalliumD3D11DeviceContext()
	{
		destroy_context();
	}
#else
#define SYNCHRONIZED lock_t<maybe_mutex_t<threadsafe> > lock_(this->mutex)

	GalliumD3D10Device(pipe_screen* screen, pipe_context* pipe, bool owns_pipe, unsigned creation_flags, IDXGIAdapter* adapter)
	: GalliumD3D10ScreenImpl<threadsafe>(screen, pipe, owns_pipe, creation_flags, adapter), pipe(pipe), owns_pipe(owns_pipe), context_flags(0)
	{
		caps = this->screen_caps;
		init_context();
	}

	~GalliumD3D10Device()
	{
		destroy_context();
	}
#endif

	void init_context()
	{
		if(!pipe->begin_query)
			caps.queries = false;
		if(!pipe->render_condition)
			caps.render_condition = false;
		if(!pipe->bind_gs_state)
		{
			caps.gs = false;
			caps.stages = 2;
		}
		if(!pipe->set_stream_output_buffers)
			caps.so = false;

		update_flags = 0;

		// pipeline state
		memset(viewports, 0, sizeof(viewports));
		memset(scissor_rects, 0, sizeof(scissor_rects));
		memset(so_offsets, 0, sizeof(so_offsets));
		primitive_topology = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		index_format = DXGI_FORMAT_UNKNOWN;
		index_offset = 0;
		render_predicate_value = 0;
		memset(blend_color, 0, sizeof(blend_color));
		sample_mask = ~0;
		stencil_ref = 0;
		depth_clamp = 0;

		// derived state
		primitive_mode = 0;
		memset(vertex_buffers, 0, sizeof(vertex_buffers));
		memset(so_buffers, 0, sizeof(so_buffers));
		memset(sampler_views, 0, sizeof(sampler_views));
		memset(sampler_csos, 0, sizeof(sampler_csos));
		memset(num_shader_resource_views, 0, sizeof(num_shader_resource_views));
		memset(num_samplers, 0, sizeof(num_samplers));
		num_vertex_buffers = 0;
		num_render_target_views = 0;
		num_viewports = 0;
		num_scissor_rects = 0;
		num_so_targets = 0;

		default_input_layout = pipe->create_vertex_elements_state(pipe, 0, 0);

		struct pipe_rasterizer_state rasterizerd;
		memset(&rasterizerd, 0, sizeof(rasterizerd));
		rasterizerd.gl_rasterization_rules = 1;
		rasterizerd.cull_face = PIPE_FACE_BACK;
		default_rasterizer = pipe->create_rasterizer_state(pipe, &rasterizerd);

		struct pipe_depth_stencil_alpha_state depth_stencild;
		memset(&depth_stencild, 0, sizeof(depth_stencild));
		depth_stencild.depth.enabled = TRUE;
		depth_stencild.depth.writemask = 1;
		depth_stencild.depth.func = PIPE_FUNC_LESS;
		default_depth_stencil = pipe->create_depth_stencil_alpha_state(pipe, &depth_stencild);

		struct pipe_blend_state blendd;
		memset(&blendd, 0, sizeof(blendd));
		blendd.rt[0].colormask = 0xf;
		default_blend = pipe->create_blend_state(pipe, &blendd);

		struct pipe_sampler_state samplerd;
		memset(&samplerd, 0, sizeof(samplerd));
		samplerd.normalized_coords = 1;
		samplerd.min_img_filter = PIPE_TEX_FILTER_LINEAR;
		samplerd.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
		samplerd.min_mip_filter = PIPE_TEX_MIPFILTER_LINEAR;
		samplerd.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
		samplerd.border_color[0] = 1.0f;
		samplerd.border_color[1] = 1.0f;
		samplerd.border_color[2] = 1.0f;
		samplerd.border_color[3] = 1.0f;
		samplerd.min_lod = -FLT_MAX;
		samplerd.max_lod = FLT_MAX;
		samplerd.max_anisotropy = 1;
		default_sampler = pipe->create_sampler_state(pipe, &samplerd);

		memset(&samplerd, 0, sizeof(samplerd));
		samplerd.normalized_coords = 0;
		samplerd.min_img_filter = PIPE_TEX_FILTER_NEAREST;
		samplerd.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
		samplerd.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
		samplerd.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
		samplerd.min_lod = -FLT_MAX;
		samplerd.max_lod = FLT_MAX;
		samplerd.max_anisotropy = 1;
		ld_sampler = pipe->create_sampler_state(pipe, &samplerd);

		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			sampler_csos[s].ld = ld_sampler;
			for(unsigned i = 0; i < D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ++i)
				sampler_csos[s].v[i] = default_sampler;
		}

		// TODO: should this really be empty shaders, or should they be all-passthrough?
		memset(default_shaders, 0, sizeof(default_shaders));
		struct ureg_program *ureg;
		ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
		ureg_END(ureg);
		default_shaders[PIPE_SHADER_FRAGMENT] = ureg_create_shader_and_destroy(ureg, pipe);

		ureg = ureg_create(TGSI_PROCESSOR_VERTEX);
		ureg_END(ureg);
		default_shaders[PIPE_SHADER_VERTEX] = ureg_create_shader_and_destroy(ureg, pipe);

		cso_ctx = cso_create_context(pipe);
		gen_mipmap = util_create_gen_mipmap(pipe, cso_ctx);

		RestoreGalliumState();
	}

	void destroy_context()
	{
		util_destroy_gen_mipmap(gen_mipmap);
		cso_destroy_context(cso_ctx);
		pipe->delete_vertex_elements_state(pipe, default_input_layout);
		pipe->delete_rasterizer_state(pipe, default_rasterizer);
		pipe->delete_depth_stencil_alpha_state(pipe, default_depth_stencil);
		pipe->delete_blend_state(pipe, default_blend);
		pipe->delete_sampler_state(pipe, default_sampler);
		pipe->delete_sampler_state(pipe, ld_sampler);
		pipe->delete_fs_state(pipe, default_shaders[PIPE_SHADER_FRAGMENT]);
		pipe->delete_vs_state(pipe, default_shaders[PIPE_SHADER_VERTEX]);
		if(owns_pipe)
			pipe->destroy(pipe);
	}

	virtual unsigned STDMETHODCALLTYPE GetContextFlags(void)
	{
		return context_flags;
	}
#if API >= 11
#define SET_SHADER_EXTRA_ARGS , \
	__in_ecount_opt(NumClassInstances)  ID3D11ClassInstance *const *ppClassInstances, \
	unsigned NumClassInstances
#define GET_SHADER_EXTRA_ARGS , \
		__out_ecount_opt(*pNumClassInstances)  ID3D11ClassInstance **ppClassInstances, \
		__inout_opt  unsigned *pNumClassInstances
#else
#define SET_SHADER_EXTRA_ARGS
#define GET_SHADER_EXTRA_ARGS
#endif

/* On Windows D3D11, SetConstantBuffers and SetShaderResources crash if passed a null pointer.
 * Instead, you have to pass a pointer to nulls to unbind things.
 * We do the same.
 * TODO: is D3D10 the same?
 */
	template<unsigned s>
	void xs_set_shader(GalliumD3D11Shader<>* shader)
	{
		if(shader != shaders[s].p)
		{
			shaders[s] = shader;
			void* shader_cso = shader ? shader->object : default_shaders[s];
			switch(s)
			{
			case PIPE_SHADER_VERTEX:
				pipe->bind_vs_state(pipe, shader_cso);
				break;
			case PIPE_SHADER_FRAGMENT:
				pipe->bind_fs_state(pipe, shader_cso);
				break;
			case PIPE_SHADER_GEOMETRY:
				pipe->bind_gs_state(pipe, shader_cso);
				break;
			}
			update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + s)) | (1 << (UPDATE_VIEWS_SHIFT + s));
		}
	}

	template<unsigned s>
	void xs_set_constant_buffers(unsigned start, unsigned count, GalliumD3D11Buffer *const *constbufs)
	{
		for(unsigned i = 0; i < count; ++i)
		{
			if(constbufs[i] != constant_buffers[s][i].p)
			{
				constant_buffers[s][i] = constbufs[i];
				if(s < caps.stages && start + i < caps.constant_buffers[s])
					pipe->set_constant_buffer(pipe, s, start + i, constbufs[i] ? constbufs[i]->resource : NULL);
			}
		}
	}

	template<unsigned s>
	void xs_set_shader_resources(unsigned start, unsigned count, GalliumD3D11ShaderResourceView *const *srvs)
	{
		int last_different = -1;
		for(unsigned i = 0; i < count; ++i)
		{
			if(shader_resource_views[s][start + i].p != srvs[i])
			{
				shader_resource_views[s][start + i] = srvs[i];
				sampler_views[s][start + i] = srvs[i] ? srvs[i]->object : 0;
				last_different = i;
			}
		}
		if(last_different >= 0)
		{
			num_shader_resource_views[s] = std::max(num_shader_resource_views[s], start + last_different + 1);
			update_flags |= 1 << (UPDATE_VIEWS_SHIFT + s);
		}
	}

	template<unsigned s>
	void xs_set_samplers(unsigned start, unsigned count, GalliumD3D11SamplerState *const *samps)
	{
		int last_different = -1;
		for(unsigned i = 0; i < count; ++i)
		{
			if(samplers[s][start + i].p != samps[i])
			{
				samplers[s][start + i] = samps[i];
				sampler_csos[s].v[start + i] = samps[i] ? samps[i]->object : default_sampler;
			}
			if(last_different >= 0)
			{
				num_samplers[s] = std::max(num_samplers[s], start + last_different + 1);
				update_flags |= (UPDATE_SAMPLERS_SHIFT + s);
			}
		}
	}

#define IMPLEMENT_SHADER_STAGE(XS, Stage) \
	virtual void STDMETHODCALLTYPE XS##SetShader( \
		__in_opt  ID3D11##Stage##Shader *pShader \
		SET_SHADER_EXTRA_ARGS) \
	{ \
		SYNCHRONIZED; \
		xs_set_shader<D3D11_STAGE_##XS>((GalliumD3D11Shader<>*)pShader); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetShader(\
		__out  ID3D11##Stage##Shader **ppShader \
		GET_SHADER_EXTRA_ARGS) \
	{ \
		SYNCHRONIZED; \
		*ppShader = (ID3D11##Stage##Shader*)shaders[D3D11_STAGE_##XS].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetConstantBuffers(\
		__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  unsigned NumBuffers, \
		__in_ecount(NumBuffers)  ID3D11Buffer *const *ppConstantBuffers) \
	{ \
		SYNCHRONIZED; \
		xs_set_constant_buffers<D3D11_STAGE_##XS>(StartSlot, NumBuffers, (GalliumD3D11Buffer *const *)ppConstantBuffers); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetConstantBuffers(\
		__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot)  unsigned NumBuffers, \
		__out_ecount(NumBuffers)  ID3D11Buffer **ppConstantBuffers) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < NumBuffers; ++i) \
			ppConstantBuffers[i] = constant_buffers[D3D11_STAGE_##XS][StartSlot + i].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetShaderResources(\
		__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  unsigned NumViews, \
		__in_ecount(NumViews)  ID3D11ShaderResourceView *const *ppShaderResourceViews) \
	{ \
		SYNCHRONIZED; \
		xs_set_shader_resources<D3D11_STAGE_##XS>(StartSlot, NumViews, (GalliumD3D11ShaderResourceView *const *)ppShaderResourceViews); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetShaderResources(\
		__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  unsigned NumViews, \
		__out_ecount(NumViews)  ID3D11ShaderResourceView **ppShaderResourceViews) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < NumViews; ++i) \
			ppShaderResourceViews[i] = shader_resource_views[D3D11_STAGE_##XS][StartSlot + i].ref(); \
	} \
	virtual void STDMETHODCALLTYPE XS##SetSamplers(\
		__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  unsigned NumSamplers, \
		__in_ecount(NumSamplers)  ID3D11SamplerState *const *ppSamplers) \
	{ \
		SYNCHRONIZED; \
		xs_set_samplers<D3D11_STAGE_##XS>(StartSlot, NumSamplers, (GalliumD3D11SamplerState *const *)ppSamplers); \
	} \
	virtual void STDMETHODCALLTYPE XS##GetSamplers( \
		__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1)  unsigned StartSlot, \
		__in_range(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot)  unsigned NumSamplers, \
		__out_ecount(NumSamplers)  ID3D11SamplerState **ppSamplers) \
	{ \
		SYNCHRONIZED; \
		for(unsigned i = 0; i < NumSamplers; ++i) \
			ppSamplers[i] = samplers[D3D11_STAGE_##XS][StartSlot + i].ref(); \
	}

#define DO_VS(x) x
#define DO_GS(x) do {if(caps.gs) {x;}} while(0)
#define DO_PS(x) x
#define DO_HS(x)
#define DO_DS(x)
#define DO_CS(x)
	IMPLEMENT_SHADER_STAGE(VS, Vertex)
	IMPLEMENT_SHADER_STAGE(GS, Geometry)
	IMPLEMENT_SHADER_STAGE(PS, Pixel)

#if API >= 11
	IMPLEMENT_SHADER_STAGE(HS, Hull)
	IMPLEMENT_SHADER_STAGE(DS, Domain)
	IMPLEMENT_SHADER_STAGE(CS, Compute)

	virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  unsigned StartSlot,
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  unsigned NumUAVs,
		__in_ecount(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
		__in_ecount(NumUAVs)  const unsigned *pUAVInitialCounts)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < NumUAVs; ++i)
			cs_unordered_access_views[StartSlot + i] = ppUnorderedAccessViews[i];
	}

	virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  unsigned StartSlot,
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot)  unsigned NumUAVs,
		__out_ecount(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < NumUAVs; ++i)
			ppUnorderedAccessViews[i] = cs_unordered_access_views[StartSlot + i].ref();
	}
#endif

	template<unsigned s>
	void update_stage()
	{
		if(update_flags & (1 << (UPDATE_VIEWS_SHIFT + s)))
		{
			while(num_shader_resource_views[s] && !sampler_views[s][num_shader_resource_views[s] - 1]) \
				--num_shader_resource_views[s];
			if(s < caps.stages)
			{
				struct pipe_sampler_view* views_to_bind[PIPE_MAX_SAMPLERS];
				unsigned num_views_to_bind = shaders[s] ? shaders[s]->slot_to_resource.size() : 0;
				for(unsigned i = 0; i < num_views_to_bind; ++i)
				{
					views_to_bind[i] = sampler_views[s][shaders[s]->slot_to_resource[i]];
				}
				switch(s)
				{
				case PIPE_SHADER_VERTEX:
					pipe->set_vertex_sampler_views(pipe, num_views_to_bind, views_to_bind);
					break;
				case PIPE_SHADER_FRAGMENT:
					pipe->set_fragment_sampler_views(pipe, num_views_to_bind, views_to_bind);
					break;
				case PIPE_SHADER_GEOMETRY:
					pipe->set_geometry_sampler_views(pipe, num_views_to_bind, views_to_bind);
					break;
				}
			}
		}

		if(update_flags & (1 << (UPDATE_SAMPLERS_SHIFT + s)))
		{
			while(num_samplers[s] && !sampler_csos[s].v[num_samplers[s] - 1])
				--num_samplers[s];
			if(s < caps.stages)
			{
				void* samplers_to_bind[PIPE_MAX_SAMPLERS];
				unsigned num_samplers_to_bind =  shaders[s] ? shaders[s]->slot_to_sampler.size() : 0;
				for(unsigned i = 0; i < num_samplers_to_bind; ++i)
				{
					// index can be -1 to access sampler_csos[s].ld
					samplers_to_bind[i] = *(sampler_csos[s].v + shaders[s]->slot_to_sampler[i]);
				}
				switch(s)
				{
				case PIPE_SHADER_VERTEX:
					pipe->bind_vertex_sampler_states(pipe, num_samplers_to_bind, samplers_to_bind);
					break;
				case PIPE_SHADER_FRAGMENT:
					pipe->bind_fragment_sampler_states(pipe, num_samplers_to_bind, samplers_to_bind);
					break;
				case PIPE_SHADER_GEOMETRY:
					pipe->bind_geometry_sampler_states(pipe, num_samplers_to_bind, samplers_to_bind);
					break;
				}
			}
		}
	}

	void update_state()
	{
		update_stage<D3D11_STAGE_PS>();
		update_stage<D3D11_STAGE_VS>();
		update_stage<D3D11_STAGE_GS>();
#if API >= 11
		update_stage<D3D11_STAGE_HS>();
		update_stage<D3D11_STAGE_DS>();
		update_stage<D3D11_STAGE_CS>();
#endif

		if(update_flags & UPDATE_VERTEX_BUFFERS)
		{
			while(num_vertex_buffers && !vertex_buffers[num_vertex_buffers - 1].buffer)
				--num_vertex_buffers;
			pipe->set_vertex_buffers(pipe, num_vertex_buffers, vertex_buffers);
		}

		update_flags = 0;
	}

	virtual void STDMETHODCALLTYPE IASetInputLayout(
		__in_opt  ID3D11InputLayout *pInputLayout)
	{
		SYNCHRONIZED;
		if(pInputLayout != input_layout.p)
		{
			input_layout = pInputLayout;
			pipe->bind_vertex_elements_state(pipe, pInputLayout ? ((GalliumD3D11InputLayout*)pInputLayout)->object : default_input_layout);
		}
	}

	virtual void STDMETHODCALLTYPE IAGetInputLayout(
		__out  ID3D11InputLayout **ppInputLayout)
	{
		SYNCHRONIZED;
		*ppInputLayout = input_layout.ref();
	}

	virtual void STDMETHODCALLTYPE IASetVertexBuffers(
		__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  unsigned StartSlot,
		__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  unsigned NumBuffers,
		__in_ecount(NumBuffers)  ID3D11Buffer *const *ppVertexBuffers,
		__in_ecount(NumBuffers)  const unsigned *pStrides,
		__in_ecount(NumBuffers)  const unsigned *pOffsets)
	{
		SYNCHRONIZED;
		int last_different = -1;
		for(unsigned i = 0; i < NumBuffers; ++i)
		{
			ID3D11Buffer* buffer = ppVertexBuffers[i];
			if(buffer != input_buffers[StartSlot + i].p
				|| vertex_buffers[StartSlot + i].buffer_offset != pOffsets[i]
				|| vertex_buffers[StartSlot + i].stride != pOffsets[i]
			)
			{
				input_buffers[StartSlot + i] = buffer;
				vertex_buffers[StartSlot + i].buffer = buffer ? ((GalliumD3D11Buffer*)buffer)->resource : 0;
				vertex_buffers[StartSlot + i].buffer_offset = pOffsets[i];
				vertex_buffers[StartSlot + i].stride = pStrides[i];
				vertex_buffers[StartSlot + i].max_index = ~0;
				last_different = i;
			}
		}
		if(last_different >= 0)
		{
			num_vertex_buffers = std::max(num_vertex_buffers, StartSlot + NumBuffers);
			update_flags |= UPDATE_VERTEX_BUFFERS;
		}
	}

	virtual void STDMETHODCALLTYPE IAGetVertexBuffers(
		__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1)  unsigned StartSlot,
		__in_range(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot)  unsigned NumBuffers,
		__out_ecount_opt(NumBuffers)  ID3D11Buffer **ppVertexBuffers,
		__out_ecount_opt(NumBuffers)  unsigned *pStrides,
		__out_ecount_opt(NumBuffers)  unsigned *pOffsets)
	{
		SYNCHRONIZED;
		if(ppVertexBuffers)
		{
			for(unsigned i = 0; i < NumBuffers; ++i)
				ppVertexBuffers[i] = input_buffers[StartSlot + i].ref();
		}

		if(pOffsets)
		{
			for(unsigned i = 0; i < NumBuffers; ++i)
				pOffsets[i] = vertex_buffers[StartSlot + i].buffer_offset;
		}

		if(pStrides)
		{
			for(unsigned i = 0; i < NumBuffers; ++i)
				pStrides[i] = vertex_buffers[StartSlot + i].stride;
		}
	}

	void set_index_buffer()
	{
		pipe_index_buffer ib;
		if(!index_buffer)
		{
			memset(&ib, 0, sizeof(ib));
		}
		else
		{
			if(index_format == DXGI_FORMAT_R32_UINT)
				ib.index_size = 4;
			else if(index_format == DXGI_FORMAT_R16_UINT)
				ib.index_size = 2;
			else
				ib.index_size = 1;
			ib.offset = index_offset;
			ib.buffer = index_buffer ? ((GalliumD3D11Buffer*)index_buffer.p)->resource : 0;
		}
		pipe->set_index_buffer(pipe, &ib);
	}

	virtual void STDMETHODCALLTYPE IASetIndexBuffer(
		__in_opt  ID3D11Buffer *pIndexBuffer,
		__in  DXGI_FORMAT Format,
		__in  unsigned Offset)
	{
		SYNCHRONIZED;
		if(index_buffer.p != pIndexBuffer || index_format != Format || index_offset != Offset)
		{
			index_buffer = pIndexBuffer;
			index_format = Format;
			index_offset = Offset;

			set_index_buffer();
		}
	}

	virtual void STDMETHODCALLTYPE IAGetIndexBuffer(
		__out_opt  ID3D11Buffer **pIndexBuffer,
		__out_opt  DXGI_FORMAT *Format,
		__out_opt  unsigned *Offset)
	{
		SYNCHRONIZED;
		if(pIndexBuffer)
			*pIndexBuffer = index_buffer.ref();
		if(Format)
			*Format = index_format;
		if(Offset)
			*Offset = index_offset;
	}

	virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(
		__in  D3D11_PRIMITIVE_TOPOLOGY Topology)
	{
		SYNCHRONIZED;
		if(primitive_topology != Topology)
		{
			if(Topology < D3D_PRIMITIVE_TOPOLOGY_COUNT)
				primitive_mode = d3d_to_pipe_prim[Topology];
			else
				primitive_mode = 0;
			primitive_topology = Topology;
		}
	}

	virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(
		__out  D3D11_PRIMITIVE_TOPOLOGY *pTopology)
	{
		SYNCHRONIZED;
		*pTopology = primitive_topology;
	}

	virtual void STDMETHODCALLTYPE DrawIndexed(
		__in  unsigned IndexCount,
		__in  unsigned StartIndexLocation,
		__in  int BaseVertexLocation)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.count = IndexCount;
		info.start = StartIndexLocation;
		info.index_bias = BaseVertexLocation;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = 1;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE Draw(
		__in  unsigned VertexCount,
		__in  unsigned StartVertexLocation)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.count = VertexCount;
		info.start = StartVertexLocation;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = 1;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawIndexedInstanced(
		__in  unsigned IndexCountPerInstance,
		__in  unsigned InstanceCount,
		__in  unsigned StartIndexLocation,
		__in  int BaseVertexLocation,
		__in  unsigned StartInstanceLocation)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.count = IndexCountPerInstance;
		info.start = StartIndexLocation;
		info.index_bias = BaseVertexLocation;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = StartInstanceLocation;
		info.instance_count = InstanceCount;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawInstanced(
		__in  unsigned VertexCountPerInstance,
		__in  unsigned InstanceCount,
		__in  unsigned StartVertexLocation,
		__in  unsigned StartInstanceLocation)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.count = VertexCountPerInstance;
		info.start = StartVertexLocation;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = StartInstanceLocation;
		info.instance_count = InstanceCount;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawAuto(void)
	{
		if(!caps.so)
			return;

		SYNCHRONIZED;
		if(update_flags)
			update_state();

		pipe->draw_stream_output(pipe, primitive_mode);
	}

	virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(
		__in  ID3D11Buffer *pBufferForArgs,
		__in  unsigned AlignedByteOffsetForArgs)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		struct {
			unsigned count;
			unsigned instance_count;
			unsigned start;
			unsigned index_bias;
		} data;

		pipe_buffer_read(pipe, ((GalliumD3D11Buffer*)pBufferForArgs)->resource, AlignedByteOffsetForArgs, sizeof(data), &data);

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = TRUE;
		info.start = data.start;
		info.count = data.count;
		info.index_bias = data.index_bias;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = data.instance_count;

		pipe->draw_vbo(pipe, &info);
	}

	virtual void STDMETHODCALLTYPE DrawInstancedIndirect(
		__in  ID3D11Buffer *pBufferForArgs,
		__in  unsigned AlignedByteOffsetForArgs)
	{
		SYNCHRONIZED;
		if(update_flags)
			update_state();

		struct {
			unsigned count;
			unsigned instance_count;
			unsigned start;
		} data;

		pipe_buffer_read(pipe, ((GalliumD3D11Buffer*)pBufferForArgs)->resource, AlignedByteOffsetForArgs, sizeof(data), &data);

		pipe_draw_info info;
		info.mode = primitive_mode;
		info.indexed = FALSE;
		info.start = data.start;
		info.count = data.count;
		info.index_bias = 0;
		info.min_index = 0;
		info.max_index = ~0;
		info.start_instance = 0;
		info.instance_count = data.instance_count;

		pipe->draw_vbo(pipe, &info);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE Dispatch(
		__in  unsigned ThreadGroupCountX,
		__in  unsigned ThreadGroupCountY,
		__in  unsigned ThreadGroupCountZ)
	{
// uncomment this when this is implemented
//		SYNCHRONIZED;
//		if(update_flags)
//			update_state();
	}

	virtual void STDMETHODCALLTYPE DispatchIndirect(
		__in  ID3D11Buffer *pBufferForArgs,
		__in  unsigned AlignedByteOffsetForArgs)
	{
// uncomment this when this is implemented
//		SYNCHRONIZED;
//		if(update_flags)
//			update_state();
	}
#endif

	void set_clip()
	{
		SYNCHRONIZED;
		pipe_clip_state clip;
		clip.nr = 0;
		clip.depth_clamp = depth_clamp;
		pipe->set_clip_state(pipe, &clip);
	}

	virtual void STDMETHODCALLTYPE RSSetState(
		__in_opt  ID3D11RasterizerState *pRasterizerState)
	{
		SYNCHRONIZED;
		if(pRasterizerState != rasterizer_state.p)
		{
			rasterizer_state = pRasterizerState;
			pipe->bind_rasterizer_state(pipe, pRasterizerState ? ((GalliumD3D11RasterizerState*)pRasterizerState)->object : default_rasterizer);
			bool new_depth_clamp = pRasterizerState ? ((GalliumD3D11RasterizerState*)pRasterizerState)->depth_clamp : false;
			if(depth_clamp != new_depth_clamp)
			{
				depth_clamp = new_depth_clamp;
				set_clip();
			}
		}
	}

	virtual void STDMETHODCALLTYPE RSGetState(
		__out  ID3D11RasterizerState **ppRasterizerState)
	{
		SYNCHRONIZED;
		*ppRasterizerState = rasterizer_state.ref();
	}

	void set_viewport()
	{
		// TODO: is depth correct? it seems D3D10/11 uses a [-1,1]x[-1,1]x[0,1] cube
		pipe_viewport_state viewport;
		float half_width = viewports[0].Width * 0.5f;
		float half_height = viewports[0].Height * 0.5f;

		viewport.scale[0] = half_width;
		viewport.scale[1] = -half_height;
		viewport.scale[2] = (viewports[0].MaxDepth - viewports[0].MinDepth);
		viewport.scale[3] = 1.0f;
		viewport.translate[0] = half_width + viewports[0].TopLeftX;
		viewport.translate[1] = half_height + viewports[0].TopLeftY;
		viewport.translate[2] = viewports[0].MinDepth;
		viewport.translate[3] = 1.0f;
		pipe->set_viewport_state(pipe, &viewport);
	}

	virtual void STDMETHODCALLTYPE RSSetViewports(
		__in_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  unsigned NumViewports,
		__in_ecount_opt(NumViewports)  const D3D11_VIEWPORT *pViewports)
	{
		SYNCHRONIZED;
		if(NumViewports)
		{
			if(memcmp(&viewports[0], &pViewports[0], sizeof(viewports[0])))
			{
				viewports[0] = pViewports[0];
				set_viewport();
			}
			for(unsigned i = 1; i < NumViewports; ++i)
				viewports[i] = pViewports[i];
		}
		else if(num_viewports)
		{
			// TODO: what should we do here?
			memset(&viewports[0], 0, sizeof(viewports[0]));
			set_viewport();
		}
		num_viewports = NumViewports;
	}

	virtual void STDMETHODCALLTYPE RSGetViewports(
		__inout_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)   unsigned *pNumViewports,
		__out_ecount_opt(*pNumViewports)  D3D11_VIEWPORT *pViewports)
	{
		SYNCHRONIZED;
		if(pViewports)
		{
			unsigned i;
			for(i = 0; i < std::min(*pNumViewports, num_viewports); ++i)
				pViewports[i] = viewports[i];

			memset(pViewports + i, 0, (*pNumViewports - i) * sizeof(D3D11_VIEWPORT));
		}

		*pNumViewports = num_viewports;
	}

	void set_scissor()
	{
		pipe_scissor_state scissor;
		scissor.minx = scissor_rects[0].left;
		scissor.miny = scissor_rects[0].top;
		scissor.maxx = scissor_rects[0].right;
		scissor.maxy = scissor_rects[0].bottom;
		pipe->set_scissor_state(pipe, &scissor);
	}

	virtual void STDMETHODCALLTYPE RSSetScissorRects(
		__in_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)  unsigned NumRects,
		__in_ecount_opt(NumRects)  const D3D11_RECT *pRects)
	{
		SYNCHRONIZED;
		if(NumRects)
		{
			if(memcmp(&scissor_rects[0], &pRects[0], sizeof(scissor_rects[0])))
			{
				scissor_rects[0] = pRects[0];
				set_scissor();
			}
			for(unsigned i = 1; i < NumRects; ++i)
				scissor_rects[i] = pRects[i];
		}
		else if(num_scissor_rects)
		{
			// TODO: what should we do here?
			memset(&scissor_rects[0], 0, sizeof(scissor_rects[0]));
			set_scissor();
		}

		num_scissor_rects = NumRects;
	}

	virtual void STDMETHODCALLTYPE RSGetScissorRects(
		__inout_range(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE)   unsigned *pNumRects,
		__out_ecount_opt(*pNumRects)  D3D11_RECT *pRects)
	{
		SYNCHRONIZED;
		if(pRects)
		{
			unsigned i;
			for(i = 0; i < std::min(*pNumRects, num_scissor_rects); ++i)
				pRects[i] = scissor_rects[i];

			memset(pRects + i, 0, (*pNumRects - i) * sizeof(D3D11_RECT));
		}

		*pNumRects = num_scissor_rects;
	}

	virtual void STDMETHODCALLTYPE OMSetBlendState(
		__in_opt  ID3D11BlendState *pBlendState,
		__in_opt  const float BlendFactor[ 4 ],
		__in  unsigned SampleMask)
	{
		SYNCHRONIZED;
		float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};

		if(blend_state.p != pBlendState)
		{
			pipe->bind_blend_state(pipe, pBlendState ? ((GalliumD3D11BlendState*)pBlendState)->object : default_blend);
			blend_state = pBlendState;
		}

		// Windows D3D11 does this, even though it's apparently undocumented
		if(!BlendFactor)
			BlendFactor = white;

		if(memcmp(blend_color, BlendFactor, sizeof(blend_color)))
		{
			pipe->set_blend_color(pipe, (struct pipe_blend_color*)BlendFactor);
			memcpy(blend_color, BlendFactor, sizeof(blend_color));
		}

		if(sample_mask != SampleMask)
		{
			pipe->set_sample_mask(pipe, sample_mask);
			sample_mask = SampleMask;
		}
	}

	virtual void STDMETHODCALLTYPE OMGetBlendState(
		__out_opt  ID3D11BlendState **ppBlendState,
		__out_opt  float BlendFactor[ 4 ],
		__out_opt  unsigned *pSampleMask)
	{
		SYNCHRONIZED;
		if(ppBlendState)
			*ppBlendState = blend_state.ref();
		if(BlendFactor)
			memcpy(BlendFactor, blend_color, sizeof(blend_color));
		if(pSampleMask)
			*pSampleMask = sample_mask;
	}

	void set_stencil_ref()
	{
		struct pipe_stencil_ref sref;
		sref.ref_value[0] = stencil_ref;
		sref.ref_value[1] = stencil_ref;
		pipe->set_stencil_ref(pipe, &sref);
	}

	virtual void STDMETHODCALLTYPE OMSetDepthStencilState(
		__in_opt  ID3D11DepthStencilState *pDepthStencilState,
		__in  unsigned StencilRef)
	{
		SYNCHRONIZED;
		if(pDepthStencilState != depth_stencil_state.p)
		{
			pipe->bind_depth_stencil_alpha_state(pipe, pDepthStencilState ? ((GalliumD3D11DepthStencilState*)pDepthStencilState)->object : default_depth_stencil);
			depth_stencil_state = pDepthStencilState;
		}

		if(StencilRef != stencil_ref)
		{
			stencil_ref = StencilRef;
			set_stencil_ref();
		}
	}

	virtual void STDMETHODCALLTYPE OMGetDepthStencilState(
		__out_opt  ID3D11DepthStencilState **ppDepthStencilState,
		__out_opt  unsigned *pStencilRef)
	{
		SYNCHRONIZED;
		if(*ppDepthStencilState)
			*ppDepthStencilState = depth_stencil_state.ref();
		if(pStencilRef)
			*pStencilRef = stencil_ref;
	}

	void set_framebuffer()
	{
		struct pipe_framebuffer_state fb;
		memset(&fb, 0, sizeof(fb));
		if(depth_stencil_view)
		{
			struct pipe_surface* surf = ((GalliumD3D11DepthStencilView*)depth_stencil_view.p)->object;
			fb.zsbuf = surf;
			if(surf->width > fb.width)
				fb.width = surf->width;
			if(surf->height > fb.height)
				fb.height = surf->height;
		}
		fb.nr_cbufs = num_render_target_views;
		unsigned i;
		for(i = 0; i < num_render_target_views; ++i)
		{
			if(render_target_views[i])
			{
				struct pipe_surface* surf = ((GalliumD3D11RenderTargetView*)render_target_views[i].p)->object;
				fb.cbufs[i] = surf;
				if(surf->width > fb.width)
					fb.width = surf->width;
				if(surf->height > fb.height)
					fb.height = surf->height;
			}
		}

		pipe->set_framebuffer_state(pipe, &fb);
	}

	/* TODO: the docs say that we should unbind conflicting resources (e.g. those bound for read while we are binding them for write too), but we aren't.
	 * Hopefully nobody relies on this happening
	 */

	virtual void STDMETHODCALLTYPE OMSetRenderTargets(
		__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  unsigned NumViews,
		__in_ecount_opt(NumViews)  ID3D11RenderTargetView *const *ppRenderTargetViews,
		__in_opt  ID3D11DepthStencilView *pDepthStencilView)
	{
		SYNCHRONIZED;
		if(!ppRenderTargetViews)
			NumViews = 0;
		if(NumViews == num_render_target_views)
		{
			for(unsigned i = 0; i < NumViews; ++i)
			{
				if(ppRenderTargetViews[i] != render_target_views[i].p)
					goto changed;
			}
			return;
		}
changed:
		depth_stencil_view = pDepthStencilView;
		unsigned i;
		for(i = 0; i < NumViews; ++i)
		{
			render_target_views[i] = ppRenderTargetViews[i];
#if API >= 11
			om_unordered_access_views[i] = (ID3D11UnorderedAccessView*)NULL;
#endif
		}
		for(; i < num_render_target_views; ++i)
			render_target_views[i] = (ID3D11RenderTargetView*)NULL;
		num_render_target_views = NumViews;
		set_framebuffer();
	}

	virtual void STDMETHODCALLTYPE OMGetRenderTargets(
		__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  unsigned NumViews,
		__out_ecount_opt(NumViews)  ID3D11RenderTargetView **ppRenderTargetViews,
		__out_opt  ID3D11DepthStencilView **ppDepthStencilView)
	{
		SYNCHRONIZED;
		if(ppRenderTargetViews)
		{
			unsigned i;
			for(i = 0; i < std::min(num_render_target_views, NumViews); ++i)
				ppRenderTargetViews[i] = render_target_views[i].ref();

			for(; i < NumViews; ++i)
				ppRenderTargetViews[i] = 0;
		}

		if(ppDepthStencilView)
			*ppDepthStencilView = depth_stencil_view.ref();
	}

#if API >= 11
	/* TODO: what is this supposed to do _exactly_? are we doing the right thing? */
	virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(
		__in  unsigned NumRTVs,
		__in_ecount_opt(NumRTVs)  ID3D11RenderTargetView *const *ppRenderTargetViews,
		__in_opt  ID3D11DepthStencilView *pDepthStencilView,
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  unsigned UAVStartSlot,
		__in  unsigned NumUAVs,
		__in_ecount_opt(NumUAVs)  ID3D11UnorderedAccessView *const *ppUnorderedAccessViews,
		__in_ecount_opt(NumUAVs)  const unsigned *pUAVInitialCounts)
	{
		SYNCHRONIZED;
		if(NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
			OMSetRenderTargets(NumRTVs, ppRenderTargetViews, pDepthStencilView);

		if(NumUAVs != D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
		{
			for(unsigned i = 0; i < NumUAVs; ++i)
			{
				om_unordered_access_views[UAVStartSlot + i] = ppUnorderedAccessViews[i];
				render_target_views[UAVStartSlot + i] = (ID3D11RenderTargetView*)0;
			}
		}
	}

	virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(
		__in_range(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)  unsigned NumRTVs,
		__out_ecount_opt(NumRTVs)  ID3D11RenderTargetView **ppRenderTargetViews,
		__out_opt  ID3D11DepthStencilView **ppDepthStencilView,
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1)  unsigned UAVStartSlot,
		__in_range(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot)  unsigned NumUAVs,
		__out_ecount_opt(NumUAVs)  ID3D11UnorderedAccessView **ppUnorderedAccessViews)
	{
		SYNCHRONIZED;
		if(ppRenderTargetViews)
			OMGetRenderTargets(NumRTVs, ppRenderTargetViews, ppDepthStencilView);

		if(ppUnorderedAccessViews)
		{
			for(unsigned i = 0; i < NumUAVs; ++i)
				ppUnorderedAccessViews[i] = om_unordered_access_views[UAVStartSlot + i].ref();
		}
	}
#endif

	virtual void STDMETHODCALLTYPE SOSetTargets(
		__in_range(0, D3D11_SO_BUFFER_SLOT_COUNT)  unsigned NumBuffers,
		__in_ecount_opt(NumBuffers)  ID3D11Buffer *const *ppSOTargets,
		__in_ecount_opt(NumBuffers)  const unsigned *pOffsets)
	{
		SYNCHRONIZED;
		unsigned i;
		if(!ppSOTargets)
			NumBuffers = 0;
		bool changed = false;
		for(i = 0; i < NumBuffers; ++i)
		{
			ID3D11Buffer* buffer = ppSOTargets[i];
			if(buffer != so_targets[i].p || pOffsets[i] != so_offsets[i])
			{
				so_buffers[i] = buffer ? ((GalliumD3D11Buffer*)buffer)->resource : 0;
				so_targets[i] = buffer;
				so_offsets[i] = pOffsets[i];
				changed = true;
			}
		}
		for(; i < D3D11_SO_BUFFER_SLOT_COUNT; ++i)
		{
			if(so_targets[i].p || so_offsets[i])
			{
				changed = true;
				so_targets[i] = (ID3D11Buffer*)0;
				so_offsets[i] = 0;
			}
		}
		num_so_targets = NumBuffers;

		if(changed && caps.so)
			pipe->set_stream_output_buffers(pipe, so_buffers, (int*)so_offsets, num_so_targets);
	}

	virtual void STDMETHODCALLTYPE SOGetTargets(
		__in_range(0, D3D11_SO_BUFFER_SLOT_COUNT)  unsigned NumBuffers,
		__out_ecount(NumBuffers)  ID3D11Buffer **ppSOTargets
#if API < 11
		, __out_ecount(NumBuffers)  UINT *pOffsets
#endif
		)
	{
		SYNCHRONIZED;
		for(unsigned i = 0; i < NumBuffers; ++i)
		{
			ppSOTargets[i] = so_targets[i].ref();
#if API < 11
			pOffsets[i] = so_offsets[i];
#endif
		}
	}

	virtual void STDMETHODCALLTYPE Begin(
		__in  ID3D11Asynchronous *pAsync)
	{
		SYNCHRONIZED;
		if(caps.queries)
			pipe->begin_query(pipe, ((GalliumD3D11Asynchronous<>*)pAsync)->query);
	}

	virtual void STDMETHODCALLTYPE End(
		__in  ID3D11Asynchronous *pAsync)
	{
		SYNCHRONIZED;
		if(caps.queries)
			pipe->end_query(pipe, ((GalliumD3D11Asynchronous<>*)pAsync)->query);
	}

	virtual HRESULT STDMETHODCALLTYPE GetData(
		__in  ID3D11Asynchronous *pAsync,
		__out_bcount_opt(DataSize)  void *pData,
		__in  unsigned DataSize,
		__in  unsigned GetDataFlags)
	{
		SYNCHRONIZED;
		if(!caps.queries)
			return E_NOTIMPL;

		GalliumD3D11Asynchronous<>* async = (GalliumD3D11Asynchronous<>*)pAsync;
		void* data = alloca(async->data_size);
		boolean ret = pipe->get_query_result(pipe, ((GalliumD3D11Asynchronous<>*)pAsync)->query, !(GetDataFlags & D3D11_ASYNC_GETDATA_DONOTFLUSH), data);
		if(pData)
			memcpy(pData, data, std::min(async->data_size, DataSize));
		return ret ? S_OK : S_FALSE;
	}

	void set_render_condition()
	{
		if(caps.render_condition)
		{
			if(!render_predicate)
				pipe->render_condition(pipe, 0, 0);
			else
			{
				GalliumD3D11Predicate* predicate = (GalliumD3D11Predicate*)render_predicate.p;
				if(!render_predicate_value && predicate->desc.Query == D3D11_QUERY_OCCLUSION_PREDICATE)
				{
					unsigned mode = (predicate->desc.MiscFlags & D3D11_QUERY_MISC_PREDICATEHINT) ? PIPE_RENDER_COND_NO_WAIT : PIPE_RENDER_COND_WAIT;
					pipe->render_condition(pipe, predicate->query, mode);
				}
				else
				{
					/* TODO: add inverted predication to Gallium*/
					pipe->render_condition(pipe, 0, 0);
				}
			}
		}
	}

	virtual void STDMETHODCALLTYPE SetPredication(
		__in_opt  ID3D11Predicate *pPredicate,
		__in  BOOL PredicateValue)
	{
		SYNCHRONIZED;
		if(render_predicate.p != pPredicate || render_predicate_value != PredicateValue)
		{
			render_predicate = pPredicate;
			render_predicate_value = PredicateValue;
			set_render_condition();
		}
	}

	virtual void STDMETHODCALLTYPE GetPredication(
		__out_opt  ID3D11Predicate **ppPredicate,
		__out_opt  BOOL *pPredicateValue)
	{
		SYNCHRONIZED;
		if(ppPredicate)
			*ppPredicate = render_predicate.ref();
		if(pPredicateValue)
			*pPredicateValue = render_predicate_value;
	}

	static pipe_subresource d3d11_to_pipe_subresource(struct pipe_resource* resource, unsigned subresource)
	{
		pipe_subresource sr;
		if(subresource <= resource->last_level)
		{
			sr.level = subresource;
			sr.face = 0;
		}
		else
		{
			unsigned levels = resource->last_level + 1;
			sr.level = subresource % levels;
			sr.face = subresource / levels;
		}
		return sr;
	}

	virtual HRESULT STDMETHODCALLTYPE Map(
		__in  ID3D11Resource *pResource,
		__in  unsigned Subresource,
		__in  D3D11_MAP MapType,
		__in  unsigned MapFlags,
		__out  D3D11_MAPPED_SUBRESOURCE *pMappedResource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)pResource;
		if(resource->transfers.count(Subresource))
			return E_FAIL;
		pipe_subresource sr = d3d11_to_pipe_subresource(resource->resource, Subresource);
		pipe_box box;
		d3d11_to_pipe_box(resource->resource, sr.level, 0);
		unsigned usage = 0;
		if(MapType == D3D11_MAP_READ)
			usage = PIPE_TRANSFER_READ;
		else if(MapType == D3D11_MAP_WRITE)
			usage = PIPE_TRANSFER_WRITE;
		else if(MapType == D3D11_MAP_READ_WRITE)
			usage = PIPE_TRANSFER_READ_WRITE;
		else if(MapType == D3D11_MAP_WRITE_DISCARD)
			usage = PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD;
		else if(MapType == D3D11_MAP_WRITE_NO_OVERWRITE)
			usage = PIPE_TRANSFER_WRITE | PIPE_TRANSFER_NOOVERWRITE;
		else
			return E_INVALIDARG;
		if(MapType & D3D10_MAP_FLAG_DO_NOT_WAIT)
			usage |= PIPE_TRANSFER_DONTBLOCK;
		struct pipe_transfer* transfer = pipe->get_transfer(pipe, resource->resource, sr, usage, &box);
		if(!transfer) {
			if(MapType & D3D10_MAP_FLAG_DO_NOT_WAIT)
				return DXGI_ERROR_WAS_STILL_DRAWING;
			else
				return E_FAIL;
		}
		resource->transfers[Subresource] = transfer;
		pipe->transfer_map(pipe, transfer);
		pMappedResource->pData = transfer->data;
		pMappedResource->RowPitch = transfer->stride;
		pMappedResource->DepthPitch = transfer->slice_stride;
		return S_OK;
	}

	virtual void STDMETHODCALLTYPE Unmap(
		__in  ID3D11Resource *pResource,
		__in  unsigned Subresource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)pResource;
		std::unordered_map<unsigned, pipe_transfer*>::iterator i = resource->transfers.find(Subresource);
		if(i != resource->transfers.end())
		{
			pipe->transfer_unmap(pipe, i->second);
			pipe->transfer_destroy(pipe,  i->second);
			resource->transfers.erase(i);
		}
	}

	virtual void STDMETHODCALLTYPE CopySubresourceRegion(
		__in  ID3D11Resource *pDstResource,
		__in  unsigned DstSubresource,
		__in  unsigned DstX,
		__in  unsigned DstY,
		__in  unsigned DstZ,
		__in  ID3D11Resource *pSrcResource,
		__in  unsigned SrcSubresource,
		__in_opt  const D3D11_BOX *pSrcBox)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)pDstResource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)pSrcResource;
		pipe_subresource subdst = d3d11_to_pipe_subresource(dst->resource, DstSubresource);
		pipe_subresource subsrc = d3d11_to_pipe_subresource(src->resource, SrcSubresource);
		pipe_box box = d3d11_to_pipe_box(src->resource, subsrc.level, pSrcBox);
		for(unsigned i = 0; i < box.depth; ++i)
		{
			pipe->resource_copy_region(pipe,
				dst->resource, subdst, DstX, DstY, DstZ + i,
				src->resource, subsrc, box.x, box.y, box.z + i,
				box.width, box.height);
		}
	}

	virtual void STDMETHODCALLTYPE CopyResource(
		__in  ID3D11Resource *pDstResource,
		__in  ID3D11Resource *pSrcResource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)pDstResource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)pSrcResource;
		pipe_subresource sr;
		unsigned faces = dst->resource->target == PIPE_TEXTURE_CUBE ? 6 : 1;

		for(sr.face = 0; sr.face < faces; ++sr.face)
		{
			for(sr.level = 0; sr.level <= dst->resource->last_level; ++sr.level)
			{
				unsigned w = u_minify(dst->resource->width0, sr.level);
				unsigned h = u_minify(dst->resource->height0, sr.level);
				unsigned d = u_minify(dst->resource->depth0, sr.level);
				for(unsigned i = 0; i < d; ++i)
				{
					pipe->resource_copy_region(pipe,
							dst->resource, sr, 0, 0, i,
							src->resource, sr, 0, 0, i,
							w, h);
				}
			}
		}
	}

	virtual void STDMETHODCALLTYPE UpdateSubresource(
		__in  ID3D11Resource *pDstResource,
		__in  unsigned DstSubresource,
		__in_opt  const D3D11_BOX *pDstBox,
		__in  const void *pSrcData,
		__in  unsigned SrcRowPitch,
		__in  unsigned SrcDepthPitch)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)pDstResource;
		pipe_subresource subdst = d3d11_to_pipe_subresource(dst->resource, DstSubresource);
		pipe_box box = d3d11_to_pipe_box(dst->resource, subdst.level, pDstBox);
		pipe->transfer_inline_write(pipe, dst->resource, subdst, PIPE_TRANSFER_WRITE, &box, pSrcData, SrcRowPitch, SrcDepthPitch);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE CopyStructureCount(
		__in  ID3D11Buffer *pDstBuffer,
		__in  unsigned DstAlignedByteOffset,
		__in  ID3D11UnorderedAccessView *pSrcView)
	{
		SYNCHRONIZED;
	}
#endif

	virtual void STDMETHODCALLTYPE ClearRenderTargetView(
		__in  ID3D11RenderTargetView *pRenderTargetView,
		__in  const float ColorRGBA[4])
	{
		SYNCHRONIZED;
		GalliumD3D11RenderTargetView* view = ((GalliumD3D11RenderTargetView*)pRenderTargetView);
		pipe->clear_render_target(pipe, view->object, ColorRGBA, 0, 0, view->object->width, view->object->height);
	}

	virtual void STDMETHODCALLTYPE ClearDepthStencilView(
		__in  ID3D11DepthStencilView *pDepthStencilView,
		__in  unsigned ClearFlags,
		__in  float Depth,
		__in  UINT8 Stencil)
	{
		SYNCHRONIZED;
		GalliumD3D11DepthStencilView* view = ((GalliumD3D11DepthStencilView*)pDepthStencilView);
		unsigned flags = 0;
		if(ClearFlags & D3D11_CLEAR_DEPTH)
			flags |= PIPE_CLEAR_DEPTH;
		if(ClearFlags & D3D11_CLEAR_STENCIL)
			flags |= PIPE_CLEAR_STENCIL;
		pipe->clear_depth_stencil(pipe, view->object, flags, Depth, Stencil, 0, 0, view->object->width, view->object->height);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(
		__in  ID3D11UnorderedAccessView *pUnorderedAccessView,
		__in  const unsigned Values[ 4 ])
	{
		SYNCHRONIZED;
	}

	virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(
			__in  ID3D11UnorderedAccessView *pUnorderedAccessView,
			__in  const float Values[ 4 ])
	{
		SYNCHRONIZED;
	}
#endif

	virtual void STDMETHODCALLTYPE RestoreGalliumStateBlitOnly()
	{
		pipe->bind_blend_state(pipe, blend_state.p ? blend_state.p->object : default_blend);
		pipe->bind_depth_stencil_alpha_state(pipe, depth_stencil_state.p ? depth_stencil_state.p->object : default_depth_stencil);
		pipe->bind_rasterizer_state(pipe, rasterizer_state.p ? rasterizer_state.p->object : default_rasterizer);
		pipe->bind_vertex_elements_state(pipe, input_layout.p ? input_layout.p->object : default_input_layout);
		pipe->bind_fs_state(pipe, shaders[D3D11_STAGE_PS].p ? shaders[D3D11_STAGE_PS].p->object : default_shaders[PIPE_SHADER_FRAGMENT]);
		pipe->bind_vs_state(pipe, shaders[D3D11_STAGE_VS].p ? shaders[D3D11_STAGE_VS].p->object : default_shaders[PIPE_SHADER_VERTEX]);
		if(caps.gs)
			pipe->bind_gs_state(pipe, shaders[D3D11_STAGE_GS].p ? shaders[D3D11_STAGE_GS].p->object : default_shaders[PIPE_SHADER_GEOMETRY]);
		set_framebuffer();
		set_viewport();
		set_clip();
		set_render_condition();
		// TODO: restore stream output

		update_flags |= UPDATE_VERTEX_BUFFERS | (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_PS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_PS));
	}

	virtual void STDMETHODCALLTYPE GenerateMips(
			__in  ID3D11ShaderResourceView *pShaderResourceView)
	{
		SYNCHRONIZED;

		GalliumD3D11ShaderResourceView* view = (GalliumD3D11ShaderResourceView*)pShaderResourceView;
		if(caps.gs)
			pipe->bind_gs_state(pipe, 0);
		if(caps.so)
			pipe->bind_stream_output_state(pipe, 0);
		if(pipe->render_condition)
			pipe->render_condition(pipe, 0, 0);
		util_gen_mipmap(gen_mipmap, view->object, 0, 0, view->object->texture->last_level, PIPE_TEX_FILTER_LINEAR);
		RestoreGalliumStateBlitOnly();
	}

	virtual void STDMETHODCALLTYPE RestoreGalliumState()
	{
		SYNCHRONIZED;
		RestoreGalliumStateBlitOnly();

		set_index_buffer();
		set_stencil_ref();
		pipe->set_blend_color(pipe, (struct pipe_blend_color*)blend_color);
		pipe->set_sample_mask(pipe, sample_mask);

		for(unsigned s = 0; s < 3; ++s)
		{
			unsigned num = std::min(caps.constant_buffers[s], (unsigned)D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT);
			for(unsigned i = 0; i < num; ++i)
				pipe->set_constant_buffer(pipe, s, i, constant_buffers[s][i].p ? constant_buffers[s][i].p->resource : 0);
		}

		if(caps.so)
			pipe->set_stream_output_buffers(pipe, so_buffers, (int*)so_offsets, num_so_targets);

		update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_VS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_VS));
		update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + D3D11_STAGE_GS)) | (1 << (UPDATE_VIEWS_SHIFT + D3D11_STAGE_GS));

		set_scissor();
	}

#if API >= 11
	/* TODO: hack SRVs or sampler states to handle this, or add to Gallium */
	virtual void STDMETHODCALLTYPE SetResourceMinLOD(
		__in  ID3D11Resource *pResource,
		float MinLOD)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)pResource;
		if(resource->min_lod != MinLOD)
		{
			// TODO: actually do anything?
			resource->min_lod = MinLOD;
		}
	}

	virtual float STDMETHODCALLTYPE GetResourceMinLOD(
		__in  ID3D11Resource *pResource)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* resource = (GalliumD3D11Resource<>*)pResource;
		return resource->min_lod;
	}
#endif

	virtual void STDMETHODCALLTYPE ResolveSubresource(
		__in  ID3D11Resource *pDstResource,
		__in  unsigned DstSubresource,
		__in  ID3D11Resource *pSrcResource,
		__in  unsigned SrcSubresource,
		__in  DXGI_FORMAT Format)
	{
		SYNCHRONIZED;
		GalliumD3D11Resource<>* dst = (GalliumD3D11Resource<>*)pDstResource;
		GalliumD3D11Resource<>* src = (GalliumD3D11Resource<>*)pSrcResource;
		pipe_subresource subdst = d3d11_to_pipe_subresource(dst->resource, DstSubresource);
		pipe_subresource subsrc = d3d11_to_pipe_subresource(src->resource, SrcSubresource);
		pipe->resource_resolve(pipe, dst->resource, subdst, src->resource, subsrc);
	}

#if API >= 11
	virtual void STDMETHODCALLTYPE ExecuteCommandList(
		__in  ID3D11CommandList *pCommandList,
		BOOL RestoreContextState)
	{
		SYNCHRONIZED;
	}

	virtual HRESULT STDMETHODCALLTYPE FinishCommandList(
		BOOL RestoreDeferredContextState,
		__out_opt  ID3D11CommandList **ppCommandList)
	{
		SYNCHRONIZED;
		return E_NOTIMPL;
	}
#endif

	virtual void STDMETHODCALLTYPE ClearState(void)
	{
		SYNCHRONIZED;

		// we qualify all calls so that we avoid virtual dispatch and might get them inlined
		// TODO: make sure all this gets inlined, which might require more compiler flags
		// TODO: optimize this
#if API >= 11
		GalliumD3D11DeviceContext::PSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::GSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::VSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::HSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::DSSetShader(0, 0, 0);
		GalliumD3D11DeviceContext::CSSetShader(0, 0, 0);
#else
		GalliumD3D11DeviceContext::PSSetShader(0);
		GalliumD3D11DeviceContext::GSSetShader(0);
		GalliumD3D11DeviceContext::VSSetShader(0);
#endif

		GalliumD3D11DeviceContext::IASetInputLayout(0);
		GalliumD3D11DeviceContext::IASetIndexBuffer(0, DXGI_FORMAT_UNKNOWN, 0);
		GalliumD3D11DeviceContext::RSSetState(0);
		GalliumD3D11DeviceContext::OMSetDepthStencilState(0, 0);
		GalliumD3D11DeviceContext::OMSetBlendState(0, (float*)zero_data, ~0);
		GalliumD3D11DeviceContext::SetPredication(0, 0);
		GalliumD3D11DeviceContext::IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

		GalliumD3D11DeviceContext::PSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::GSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::VSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::DSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
		GalliumD3D11DeviceContext::CSSetConstantBuffers(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer**)zero_data);
#endif

		GalliumD3D11DeviceContext::IASetVertexBuffers(0, num_vertex_buffers, (ID3D11Buffer**)zero_data, (unsigned*)zero_data, (unsigned*)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(0, 0, 0 , 0, 0, 0, 0);
#else
		GalliumD3D11DeviceContext::OMSetRenderTargets(0, 0, 0 );
#endif
		GalliumD3D11DeviceContext::SOSetTargets(0, 0, 0);

		GalliumD3D11DeviceContext::PSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_PS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::GSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_GS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::VSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_VS], (ID3D11ShaderResourceView**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_HS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::DSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_DS], (ID3D11ShaderResourceView**)zero_data);
		GalliumD3D11DeviceContext::CSSetShaderResources(0, num_shader_resource_views[D3D11_STAGE_CS], (ID3D11ShaderResourceView**)zero_data);
#endif

		GalliumD3D11DeviceContext::PSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_PS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::GSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_GS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::VSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_VS], (ID3D11SamplerState**)zero_data);
#if API >= 11
		GalliumD3D11DeviceContext::HSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_HS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::DSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_DS], (ID3D11SamplerState**)zero_data);
		GalliumD3D11DeviceContext::CSSetSamplers(0, num_shader_resource_views[D3D11_STAGE_CS], (ID3D11SamplerState**)zero_data);
#endif

		GalliumD3D11DeviceContext::RSSetViewports(0, 0);
		GalliumD3D11DeviceContext::RSSetScissorRects(0, 0);
	}

	virtual void STDMETHODCALLTYPE Flush(void)
	{
		SYNCHRONIZED;
		pipe->flush(pipe, PIPE_FLUSH_FRAME, 0);
	}

	/* In Direct3D 10, if the reference count of an object drops to 0, it is automatically
	 * cleanly unbound from the pipeline.
	 * In Direct3D 11, the pipeline holds a reference.
	 *
	 * Note that instead of always scanning the pipeline on destruction, we could
	 * maintain the internal reference count on DirectX 10 and use it to check if an
	 * object is still bound.
	 * Presumably, on average, scanning is faster if the application is well written.
	 */
#if API < 11
#define IMPLEMENT_SIMPLE_UNBIND(name, member, gallium, def) \
	void Unbind##name(ID3D11##name* state) \
	{ \
		SYNCHRONIZED; \
		if((void*)state == (void*)member.p) \
		{ \
			member.p = 0; \
			pipe->bind_##gallium##_state(pipe, default_##def); \
		} \
	}
	IMPLEMENT_SIMPLE_UNBIND(BlendState, blend_state, blend, blend)
	IMPLEMENT_SIMPLE_UNBIND(RasterizerState, rasterizer_state, rasterizer, rasterizer)
	IMPLEMENT_SIMPLE_UNBIND(DepthStencilState, depth_stencil_state, depth_stencil_alpha, depth_stencil)
	IMPLEMENT_SIMPLE_UNBIND(InputLayout, input_layout, vertex_elements, input_layout)
	IMPLEMENT_SIMPLE_UNBIND(PixelShader, shaders[D3D11_STAGE_PS], fs, shaders[D3D11_STAGE_PS])
	IMPLEMENT_SIMPLE_UNBIND(VertexShader, shaders[D3D11_STAGE_VS], vs, shaders[D3D11_STAGE_VS])
	IMPLEMENT_SIMPLE_UNBIND(GeometryShader, shaders[D3D11_STAGE_GS], gs, shaders[D3D11_STAGE_GS])

	void UnbindPredicate(ID3D11Predicate* predicate)
	{
		SYNCHRONIZED;
		if(predicate == render_predicate)
		{
			render_predicate.p = NULL;
			render_predicate_value = 0;
			pipe->render_condition(pipe, 0, 0);
		}
	}

	void UnbindSamplerState(ID3D11SamplerState* state)
	{
		SYNCHRONIZED;
		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < num_samplers[s]; ++i)
			{
				if(samplers[s][i] == state)
				{
					samplers[s][i].p = NULL;
					sampler_csos[s].v[i] = NULL;
					update_flags |= (1 << (UPDATE_SAMPLERS_SHIFT + s));
				}
			}
		}
	}

	void UnbindBuffer(ID3D11Buffer* buffer)
	{
		SYNCHRONIZED;
		if(buffer == index_buffer)
		{
			index_buffer.p = 0;
			index_format = DXGI_FORMAT_UNKNOWN;
			index_offset = 0;
			struct pipe_index_buffer ib;
			memset(&ib, 0, sizeof(ib));
			pipe->set_index_buffer(pipe, &ib);
		}

		for(unsigned i = 0; i < num_vertex_buffers; ++i)
		{
			if(buffer == input_buffers[i])
			{
				input_buffers[i].p = 0;
				memset(&vertex_buffers[num_vertex_buffers], 0, sizeof(vertex_buffers[num_vertex_buffers]));
				update_flags |= UPDATE_VERTEX_BUFFERS;
			}
		}

		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < sizeof(constant_buffers) / sizeof(constant_buffers[0]); ++i)
			{
				if(constant_buffers[s][i] == buffer)
				{
					constant_buffers[s][i] = (ID3D10Buffer*)NULL;
					pipe->set_constant_buffer(pipe, s, i, NULL);
				}
			}
		}
	}

	void UnbindDepthStencilView(ID3D11DepthStencilView* view)
	{
		SYNCHRONIZED;
		if(view == depth_stencil_view)
		{
			depth_stencil_view.p = NULL;
			set_framebuffer();
		}
	}

	void UnbindRenderTargetView(ID3D11RenderTargetView* view)
	{
		SYNCHRONIZED;
		bool any_bound = false;
		for(unsigned i = 0; i < num_render_target_views; ++i)
		{
			if(render_target_views[i] == view)
			{
				render_target_views[i].p = NULL;
				any_bound = true;
			}
		}
		if(any_bound)
			set_framebuffer();
	}

	void UnbindShaderResourceView(ID3D11ShaderResourceView* view)
	{
		SYNCHRONIZED;
		for(unsigned s = 0; s < D3D11_STAGES; ++s)
		{
			for(unsigned i = 0; i < num_shader_resource_views[s]; ++i)
			{
				if(shader_resource_views[s][i] == view)
				{
					shader_resource_views[s][i].p = NULL;
					sampler_views[s][i] = NULL;
					update_flags |= (1 << (UPDATE_VIEWS_SHIFT + s));
				}
			}
		}
	}
#endif

#undef SYNCHRONIZED
};

#if API >= 11
/* This approach serves two purposes.
 * First, we don't want to do an atomic operation to manipulate the reference
 * count every time something is bound/unbound to the pipeline, since they are
 * expensive.
 * Fortunately, the immediate context can only be used by a single thread, so
 * we don't have to use them, as long as a separate reference count is used
 * (see dual_refcnt_t).
 *
 * Second, we want to avoid the Device -> DeviceContext -> bound DeviceChild -> Device
 * garbage cycle.
 * To avoid it, DeviceChild doesn't hold a reference to Device as usual, but adds
 * one for each external reference count, while internal nonatomic_add_ref doesn't
 * add any.
 *
 * Note that ideally we would to eliminate the non-atomic op too, but this is more
 * complicated, since we would either need to use garbage collection and give up
 * deterministic destruction (especially bad for large textures), or scan the whole
 * pipeline state every time the reference count of object drops to 0, which risks
 * pathological slowdowns.
 *
 * Since this microoptimization should matter relatively little, let's avoid it for now.
 *
 * Note that deferred contexts don't use this, since as a whole, they must thread-safe.
 * Eliminating the atomic ops for deferred contexts seems substantially harder.
 * This might be a problem if they are used in a one-shot multithreaded rendering
 * fashion, where SMP cacheline bouncing on the reference count may be visible.
 *
 * The idea would be to attach a structure of reference counts indexed by deferred
 * context id to each object. Ideally, this should be organized like ext2 block pointers.
 *
 * Every deferred context would get a reference count in its own cacheline.
 * The external count is protected by a lock bit, and there is also a "lock bit" in each
 * internal count.
 *
 * When the external count has to be dropped to 0, the lock bit is taken and all internal
 * reference counts are scanned, taking a count of them. A flag would also be set on them.
 * Deferred context manipulation would notice the flag, and update the count.
 * Once the count goes to zero, the object is freed.
 *
 * The problem of this is that if the external reference count ping-pongs between
 * zero and non-zero, the scans will take a lot of time.
 *
 * The idea to solve this is to compute the scans in a binary-tree like fashion, where
 * each binary tree node would have a "determined bit", which would be invalidated
 * by manipulations.
 *
 * However, all this complexity might actually be a loss in most cases, so let's just
 * stick to a single atomic refcnt for now.
 *
 * Also, we don't even support deferred contexts yet, so this can wait.
 */
struct nonatomic_device_child_ptr_traits
{
	static void add_ref(void* p)
	{
		if(p)
			((GalliumD3D11DeviceChild<>*)p)->nonatomic_add_ref();
	}

	static void release(void* p)
	{
		if(p)
			((GalliumD3D11DeviceChild<>*)p)->nonatomic_release();
	}
};

struct GalliumD3D11ImmediateDeviceContext
	: public GalliumD3D11DeviceContext<nonatomic_device_child_ptr_traits>
{
	GalliumD3D11ImmediateDeviceContext(GalliumD3D11Screen* device, pipe_context* pipe, unsigned context_flags = 0)
	: GalliumD3D11DeviceContext(device, pipe, context_flags)
	{
		// not necessary, but tests that the API at least basically works
		ClearState();
	}

	/* we do this since otherwise we would have a garbage cycle between this and the device */
	virtual ULONG STDMETHODCALLTYPE AddRef()
	{
		return this->device->AddRef();
	}

	virtual ULONG STDMETHODCALLTYPE Release()
	{
		return this->device->Release();
	}

	virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType()
	{
		return D3D11_DEVICE_CONTEXT_IMMEDIATE;
	}
};

static ID3D11DeviceContext* GalliumD3D11ImmediateDeviceContext_Create(GalliumD3D11Screen* device, struct pipe_context* pipe, bool owns_pipe)
{
	return new GalliumD3D11ImmediateDeviceContext(device, pipe, owns_pipe);
}

static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumState(ID3D11DeviceContext* context)
{
	((GalliumD3D11ImmediateDeviceContext*)context)->RestoreGalliumState();
}

static void GalliumD3D11ImmediateDeviceContext_RestoreGalliumStateBlitOnly(ID3D11DeviceContext* context)
{
	((GalliumD3D11ImmediateDeviceContext*)context)->RestoreGalliumStateBlitOnly();
}

static void GalliumD3D11ImmediateDeviceContext_Destroy(ID3D11DeviceContext* context)
{
	delete (GalliumD3D11ImmediateDeviceContext*)context;
}
#endif
