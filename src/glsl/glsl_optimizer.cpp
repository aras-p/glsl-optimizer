#include "glsl_optimizer.h"
#include "nir_optimize.h"
#include "nir_printer.h"
#include "buffer_layout.h"
#include "image_formats.h"
#include "main/mtypes.h"
#include "glsl_parser_extras.h"
#include "builtin_functions.h"
#include "standalone_scaffolding.h"
#include "pipe/p_screen.h"
#include "compiler/nir/nir.h"
#include "compiler/nir/nir_deref.h"
#include <string>
#include <vector>
#include <map>
#include <set>

// ---------------------------------------------------------------------------
// context

static void
initialize_mesa_context(struct gl_context* ctx, glslopt_target api)
{
   // ES input goes through a core context with ES 3.x compatibility.
   initialize_context_to_defaults(ctx, api == kGlslTargetOpenGL ? API_OPENGL_COMPAT : API_OPENGL_CORE);
   ctx->Extensions.ARB_shader_storage_buffer_object = true;
   ctx->Extensions.ARB_shader_image_load_store = true;
   ctx->Extensions.ARB_shader_image_size = true;
   ctx->Extensions.EXT_shader_framebuffer_fetch = true;
   ctx->Extensions.OES_EGL_image_external = true;
   ctx->Extensions.KHR_shader_subgroup = true;
   ctx->Const.ShaderSubgroupSize = 32;
   ctx->Const.ShaderSubgroupSupportedStages = ~0u;
   ctx->Const.ShaderSubgroupSupportedFeatures = ~0u;

   if (api != kGlslTargetOpenGLES30)
      ctx->Const.GLSLVersion = 450;
   if (api == kGlslTargetOpenGL) {
      ctx->Extensions.EXT_gpu_shader4 = true;
   } else {
      ctx->Extensions.ARB_ES3_compatibility = true;
      ctx->Extensions.ARB_ES3_1_compatibility = true;
      ctx->Extensions.ARB_ES3_2_compatibility = true;
   }

   // allow high amount of texcoords
   ctx->Const.MaxTextureCoordUnits = 16;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_COMPUTE].MaxTextureImageUnits = 16;

   ctx->Const.MaxDrawBuffers = 4;
   ctx->Const.MaxVertexStreams = 1;
   ctx->Const.MaxTransformFeedbackBuffers = 4;
   ctx->Const.MaxTransformFeedbackSeparateComponents = 64;
   ctx->Const.MaxTransformFeedbackInterleavedComponents = 64;
   // Compiler resource limits, independent of the eventual target device.
   ctx->Const.MaxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxUniformBufferBindings = 72;
   ctx->Const.MaxUniformBlockSize = 16384;
   ctx->Const.UniformBufferOffsetAlignment = 16;
   ctx->Const.MaxCombinedUniformBlocks = 72;
   ctx->Const.MaxShaderStorageBufferBindings = 16;
   ctx->Const.MaxShaderStorageBlockSize = 1 << 27;
   ctx->Const.ShaderStorageBufferOffsetAlignment = 16;
   ctx->Const.MaxCombinedShaderStorageBlocks = 48;
   ctx->Const.MaxUserAssignableUniformLocations = 1024;
   ctx->Const.MaxAtomicBufferBindings = 8;
   ctx->Const.MaxAtomicBufferSize = 16384;
   ctx->Const.MaxCombinedAtomicBuffers = 48;
   ctx->Const.MaxCombinedAtomicCounters = 24576;
   ctx->Const.MaxImageUnits = 8;
   ctx->Const.MaxImageSamples = 1;
   ctx->Const.MaxCombinedImageUniforms = 48;
   ctx->Const.MaxCombinedShaderOutputResources = 16;
   ctx->Const.MaxComputeSharedMemorySize = 32768;
   ctx->Const.MaxComputeWorkGroupCount[0] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[1] = 65535;
   ctx->Const.MaxComputeWorkGroupCount[2] = 65535;
   ctx->Const.MaxComputeWorkGroupSize[0] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[1] = 1024;
   ctx->Const.MaxComputeWorkGroupSize[2] = 64;
   ctx->Const.MaxComputeWorkGroupInvocations = 1024;
   ctx->Const.GenerateTemporaryNames = true;
   for (auto& program : ctx->Const.Program) {
      program.MaxUniformBlocks = 12;
      program.MaxCombinedUniformComponents = program.MaxUniformComponents +
                                             program.MaxUniformBlocks * (ctx->Const.MaxUniformBlockSize / 4);
      program.MaxShaderStorageBlocks = 8;
      program.MaxAtomicBuffers = 8;
      program.MaxAtomicCounters = 4096;
      program.MaxImageUniforms = 8;
   }
}

struct glslopt_ctx {
   glslopt_ctx(glslopt_target target) : target(target)
   {
      initialize_mesa_context(&mesa_ctx, target);
      _mesa_glsl_builtin_functions_init_or_ref();
      glslopt_init_nir_options(&nir_options, 8); // lower than Mesa's default
      for (int i = 0; i < MESA_SHADER_MESH_STAGES; i++)
         mesa_ctx.screen->nir_options[i] = &nir_options;
   }
   ~glslopt_ctx()
   {
      free(mesa_ctx.screen);
      _mesa_glsl_builtin_functions_decref();
   }
   struct gl_context mesa_ctx;
   nir_shader_compiler_options nir_options;
   glslopt_target target;
};

glslopt_ctx* glslopt_initialize(glslopt_target target)
{
   if (target != kGlslTargetOpenGL && target != kGlslTargetOpenGLES30 && target != kGlslTargetMetal)
      return NULL;
   return new glslopt_ctx(target);
}

void glslopt_cleanup(glslopt_ctx* ctx)
{
   delete ctx;
}

void glslopt_set_max_unroll_iterations(glslopt_ctx* ctx, unsigned iterations)
{
   ctx->nir_options.max_unroll_iterations = iterations;
}

// ---------------------------------------------------------------------------
// shader results

struct glslopt_shader_var {
   const char* name;
   glslopt_basic_type type;
   glslopt_precision prec;
   int vectorSize;
   int matrixSize;
   int arraySize;
   int location;
};

struct glslopt_buffer {
   glslopt_buffer_desc desc;
   std::vector<glslopt_buffer_member_desc> members;
};

struct glslopt_shader {
   glslopt_shader()
   {
      // Results own their arena and keep shared types/builtins alive independently
      // of the context, including when another context is destroyed.
      _mesa_glsl_builtin_functions_init_or_ref();
      mem_ctx = ralloc_context(NULL);
      shader = rzalloc(mem_ctx, gl_shader);
      shader->RefCount = 1;
   }
   ~glslopt_shader()
   {
      ralloc_free(shader->nir);
      ralloc_free(mem_ctx);
      _mesa_glsl_builtin_functions_decref();
   }

   void* mem_ctx;
   struct gl_shader* shader;

   static const int kMaxShaderUniforms = 1024;
   static const int kMaxShaderInputs = 128;
   static const int kMaxShaderTextures = 128;
   std::vector<glslopt_shader_var> uniforms, inputs, textures;
   std::vector<glslopt_image_desc> images;
   std::vector<bool> isImage;
   std::vector<glslopt_buffer> buffers;
   int uniformsSize = 0;
   int statsMath = 0, statsTex = 0, statsFlow = 0;
   int computeLocalSize[3] = {};

   char* optimizedOutput = nullptr;
   const char* infoLog = "Shader not compiled yet";
   bool status = false;

   const char* strdup(const char* s) { return s ? ralloc_strdup(mem_ctx, s) : nullptr; }
   void fail(const char* msg) { infoLog = ralloc_strdup(mem_ctx, msg); }
};

// ---------------------------------------------------------------------------
// parse state

struct parse_info {
   glslopt_source_info info;
   bool has_subroutines = false;
   bool variable_local_size = false;
};

static void
capture_parse_state(const _mesa_glsl_parse_state* state, void* data)
{
   parse_info& p = *(parse_info*)data;
   glslopt_source_info& info = p.info;
   info.version = state->language_version;
   info.es = state->es_shader;
   info.early_fragment_tests = state->fs_early_fragment_tests;
   info.local_size_specified = state->cs_ms_input_local_size_specified;
   for (unsigned i = 0; i < 3; i++)
      info.local_size[i] = state->cs_ms_input_local_size_specified ? state->cs_ms_input_local_size[i] : 0;
   p.has_subroutines = state->num_subroutines > 0 || state->num_subroutine_types > 0;
   p.variable_local_size = state->cs_input_local_size_variable_specified;

   const struct { bool on; const char* name; } exts[] = {
      {state->KHR_shader_subgroup_arithmetic_enable, "GL_KHR_shader_subgroup_arithmetic"},
      {state->KHR_shader_subgroup_ballot_enable, "GL_KHR_shader_subgroup_ballot"},
      {state->KHR_shader_subgroup_basic_enable, "GL_KHR_shader_subgroup_basic"},
      {state->KHR_shader_subgroup_vote_enable, "GL_KHR_shader_subgroup_vote"},
      {state->KHR_shader_subgroup_shuffle_enable, "GL_KHR_shader_subgroup_shuffle"},
      {state->KHR_shader_subgroup_shuffle_relative_enable, "GL_KHR_shader_subgroup_shuffle_relative"},
      {state->KHR_shader_subgroup_clustered_enable, "GL_KHR_shader_subgroup_clustered"},
      {state->KHR_shader_subgroup_quad_enable, "GL_KHR_shader_subgroup_quad"},
      {state->ARB_separate_shader_objects_enable, "GL_ARB_separate_shader_objects"},
      {state->EXT_shader_io_blocks_enable, "GL_EXT_shader_io_blocks"},
      {state->OES_shader_io_blocks_enable, "GL_OES_shader_io_blocks"},
      {state->ARB_shader_atomic_counters_enable, "GL_ARB_shader_atomic_counters"},
      {state->ARB_shader_image_load_store_enable, "GL_ARB_shader_image_load_store"},
      {state->ARB_shader_image_size_enable, "GL_ARB_shader_image_size"},
      {state->ARB_shader_texture_image_samples_enable, "GL_ARB_shader_texture_image_samples"},
      {state->ARB_shader_storage_buffer_object_enable, "GL_ARB_shader_storage_buffer_object"},
      {state->ARB_uniform_buffer_object_enable, "GL_ARB_uniform_buffer_object"},
      {state->ARB_shading_language_420pack_enable, "GL_ARB_shading_language_420pack"},
      {state->ARB_enhanced_layouts_enable, "GL_ARB_enhanced_layouts"},
      {state->ARB_compute_shader_enable, "GL_ARB_compute_shader"},
      {state->ARB_explicit_attrib_location_enable, "GL_ARB_explicit_attrib_location"},
      {state->ARB_shader_texture_lod_enable, "GL_ARB_shader_texture_lod"},
      {state->ARB_draw_instanced_enable, "GL_ARB_draw_instanced"},
      {state->ARB_gpu_shader5_enable, "GL_ARB_gpu_shader5"},
      {state->ARB_texture_gather_enable, "GL_ARB_texture_gather"},
      {state->EXT_gpu_shader4_enable, "GL_EXT_gpu_shader4"},
      {state->OES_EGL_image_external_essl3_enable, "GL_OES_EGL_image_external_essl3"},
      {state->EXT_shader_framebuffer_fetch_enable, "GL_EXT_shader_framebuffer_fetch"},
      {state->ARB_shader_bit_encoding_enable, "GL_ARB_shader_bit_encoding"},
      {state->EXT_texture_array_enable, "GL_EXT_texture_array"},
   };
   for (auto& e : exts)
      if (e.on)
         info.extensions.push_back(e.name);
}

// ---------------------------------------------------------------------------
// validation of what the printers can express

static bool
nir_uses_double(nir_shader* nir)
{
   nir_foreach_variable_in_shader(v, nir)
      if (glsl_contains_double(v->type)) return true;
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_function_temp_variable(v, impl)
         if (glsl_contains_double(v->type)) return true;
      nir_foreach_block(block, impl) {
         nir_foreach_instr(instr, block) {
            if (instr->type == nir_instr_type_alu) {
               nir_alu_instr* alu = nir_instr_as_alu(instr);
               if (alu->def.bit_size == 64 || (nir_op_infos[alu->op].num_inputs && alu->src[0].src.ssa->bit_size == 64))
                  return true;
            }
         }
      }
   }
   return false;
}

static bool
metal_storage_type_supported(const glsl_type* type)
{
   if (glsl_type_is_array(type))
      return metal_storage_type_supported(glsl_get_array_element(type));
   if (glsl_type_is_struct(type) || glsl_type_is_interface(type)) {
      for (unsigned i = 0; i < glsl_get_length(type); ++i)
         if (!metal_storage_type_supported(glsl_get_struct_field(type, i))) return false;
      return true;
   }
   switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_FLOAT: case GLSL_TYPE_INT: case GLSL_TYPE_UINT: case GLSL_TYPE_BOOL: return true;
   default: return false;
   }
}

static bool
metal_volatile_fields(const glsl_type* type)
{
   if (glsl_type_is_array(type))
      return metal_volatile_fields(glsl_get_array_element(type));
   if (glsl_type_is_struct(type) || glsl_type_is_interface(type)) {
      for (unsigned i = 0; i < glsl_get_length(type); ++i) {
         if (glsl_get_struct_field_data(type, i)->memory_volatile) return true;
         if (metal_volatile_fields(glsl_get_struct_field(type, i))) return true;
      }
   }
   return false;
}

static const char*
validate_metal(nir_shader* nir)
{
   std::set<const nir_variable*> referenced = glslopt_referenced_vars(nir);
   nir_foreach_variable_in_shader(v, nir) {
      const glsl_type* bare = glsl_without_array(v->type);
      if (v->name && !strcmp(v->name, "gl_ClipDistance") && referenced.count(v))
         return "gl_ClipDistance is not supported by the Metal backend.";
      if (glsl_type_is_sampler(bare)) {
         if (glsl_type_is_array(v->type))
            return "Sampler resource arrays are not supported by the Metal backend.";
         glsl_sampler_dim dim = glsl_get_sampler_dim(bare);
         glsl_base_type rt = (glsl_base_type)glsl_get_sampler_result_type(bare);
         bool ok = dim == GLSL_SAMPLER_DIM_2D || dim == GLSL_SAMPLER_DIM_CUBE || dim == GLSL_SAMPLER_DIM_EXTERNAL ||
                   (dim == GLSL_SAMPLER_DIM_3D && rt == GLSL_TYPE_FLOAT && !glsl_sampler_type_is_shadow(bare));
         if (!ok)
            return "This sampler type is not supported by the Metal backend.";
      }
      if (glsl_type_is_image(bare)) {
         if (!glslopt_metal_image_type_name(bare) ||
             (glsl_type_is_array(v->type) && (glsl_type_is_unsized_array(v->type) || glsl_type_is_array(glsl_get_array_element(v->type)))))
            return "Image dimension or resource-array output is not supported yet (Metal backend).";
         if (v->data.access & ACCESS_VOLATILE)
            return "Volatile image output is not supported yet (Metal backend).";
         if ((v->data.access & ACCESS_COHERENT) && glsl_get_sampler_dim(bare) == GLSL_SAMPLER_DIM_BUF)
            return "Coherent buffer image output is not supported yet (Metal backend).";
      }
      if ((v->data.mode == nir_var_shader_in || v->data.mode == nir_var_shader_out) && v->interface_type &&
          !metal_storage_type_supported(v->interface_type))
         return "Stage interface block member type output is not supported yet (Metal backend).";
      if (v->data.mode == nir_var_mem_shared && !metal_storage_type_supported(v->type))
         return "Shared-variable type output is not supported yet (Metal backend).";
   }

   auto blocks = glslopt_collect_buffer_blocks(nir);
   auto counters = glslopt_collect_atomic_counter_buffers(nir, blocks);
   if (!counters.empty() && counters.back().metal_index > 30)
      return "Metal atomic-counter buffers exceed the available buffer slots (1 through 30).";
   if (glslopt_buffer_size_table_index(blocks) > 30)
      return "Metal buffer size table exceeds the available buffer slots (1 through 30).";
   for (const auto& b : blocks) {
      if (glslopt_block_is_storage(b) && metal_volatile_fields(b.type))
         return "Metal volatile storage buffers are not supported.";
      if (!metal_storage_type_supported(b.type))
         return "Metal buffer blocks support only 32-bit float/integer and boolean storage.";
      if (b.metal_index + b.array_size > 31)
         return "Metal buffer blocks exceed the available buffer slots (1 through 30).";
   }

   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            continue;
         nir_intrinsic_instr* in = nir_instr_as_intrinsic(instr);
         switch (in->intrinsic) {
         case nir_intrinsic_image_deref_atomic:
         case nir_intrinsic_image_deref_atomic_swap:
         case nir_intrinsic_image_deref_store: {
            const glsl_type* t = glsl_without_array(nir_src_as_deref(in->src[0])->type);
            if (glsl_get_sampler_dim(t) == GLSL_SAMPLER_DIM_MS)
               return "Metal multisample images support reads and queries only.";
            if (in->intrinsic != nir_intrinsic_image_deref_store && glsl_get_sampler_result_type(t) != GLSL_TYPE_INT &&
                glsl_get_sampler_result_type(t) != GLSL_TYPE_UINT)
               return "Metal image atomics require 32-bit integer images (r32i or r32ui).";
            break;
         }
         case nir_intrinsic_atomic_counter_inc_deref:
         case nir_intrinsic_atomic_counter_pre_dec_deref:
         case nir_intrinsic_atomic_counter_post_dec_deref:
            if (nir->info.stage == MESA_SHADER_VERTEX)
               return "Metal vertex shaders can only read atomic counters.";
            break;
         case nir_intrinsic_deref_atomic:
         case nir_intrinsic_deref_atomic_swap: {
            nir_deref_instr* d = nir_src_as_deref(in->src[0]);
            if (!nir_deref_mode_is_one_of(d, (nir_variable_mode)(nir_var_mem_ssbo | nir_var_mem_shared)) ||
                !glsl_type_is_scalar(d->type) || !glsl_type_is_32bit(d->type) || glsl_type_is_float(d->type))
               return "Metal atomics require a 32-bit int or uint in an SSBO or shared variable.";
            if (nir_deref_mode_is(d, nir_var_mem_shared) &&
                glslopt_precision_info::deref_prec(d) == GLSL_PRECISION_MEDIUM)
               return "Metal shared atomics require highp int or uint storage.";
            break;
         }
         default:
            break;
         }
      }
   }
   return nullptr;
}

// ---------------------------------------------------------------------------
// reflection

static void
glsl_type_to_optimizer_desc(const glsl_type* type, unsigned prec, glslopt_shader_var* out)
{
   out->arraySize = glsl_type_is_array(type) ? (int)glsl_get_aoa_size(type) : -1;
   if (glsl_type_is_array(type))
      type = glsl_without_array(type);

   const bool sampler = glsl_type_is_sampler(type), image = glsl_type_is_image(type);
   const glsl_base_type bt = glsl_get_base_type(type);
   if (bt == GLSL_TYPE_FLOAT || bt == GLSL_TYPE_FLOAT16)
      out->type = kGlslTypeFloat;
   else if (bt == GLSL_TYPE_INT || bt == GLSL_TYPE_UINT)
      out->type = kGlslTypeInt;
   else if (bt == GLSL_TYPE_BOOL)
      out->type = kGlslTypeBool;
   else if (image) {
      bool array = glsl_sampler_type_is_array(type);
      switch (glsl_get_sampler_dim(type)) {
      case GLSL_SAMPLER_DIM_1D: out->type = array ? kGlslTypeImage1DArray : kGlslTypeImage1D; break;
      case GLSL_SAMPLER_DIM_2D: out->type = array ? kGlslTypeImage2DArray : kGlslTypeImage2D; break;
      case GLSL_SAMPLER_DIM_3D: out->type = kGlslTypeImage3D; break;
      case GLSL_SAMPLER_DIM_CUBE: out->type = array ? kGlslTypeImageCubeArray : kGlslTypeImageCube; break;
      case GLSL_SAMPLER_DIM_BUF: out->type = kGlslTypeImageBuffer; break;
      case GLSL_SAMPLER_DIM_MS: out->type = array ? kGlslTypeImage2DMSArray : kGlslTypeImage2DMS; break;
      default: out->type = kGlslTypeOther; break;
      }
   } else if (sampler) {
      switch (glsl_get_sampler_dim(type)) {
      case GLSL_SAMPLER_DIM_2D:
         out->type = glsl_sampler_type_is_shadow(type) ? kGlslTypeTex2DShadow :
                     glsl_sampler_type_is_array(type) ? kGlslTypeTex2DArray : kGlslTypeTex2D;
         break;
      case GLSL_SAMPLER_DIM_3D: out->type = kGlslTypeTex3D; break;
      case GLSL_SAMPLER_DIM_CUBE: out->type = kGlslTypeTexCube; break;
      default: out->type = kGlslTypeOther; break;
      }
   } else
      out->type = kGlslTypeOther;

   out->vectorSize = sampler || image ? 0 : (int)glsl_get_vector_elements(type);
   out->matrixSize = sampler || image ? 0 : (int)glsl_get_matrix_columns(type);
   out->prec = prec == GLSL_PRECISION_MEDIUM || prec == GLSL_PRECISION_LOW ? kGlslPrecMedium : kGlslPrecHigh;
}

// Variables referenced by the final code, and the deepest deref each access reaches.
struct usage_info {
   std::set<const nir_variable*> vars;
   std::vector<nir_deref_instr*> accesses;
};

static usage_info
collect_usage(nir_shader* nir)
{
   usage_info u;
   u.vars = glslopt_referenced_vars(nir);
   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
         if (instr->type == nir_instr_type_deref) {
            nir_deref_instr* d = nir_instr_as_deref(instr);
            // deepest deref of a chain: none of its uses is another deref
            bool leaf = true;
            nir_foreach_use(use, &d->def)
               if (nir_src_use_instr(use)->type == nir_instr_type_deref &&
                   nir_src_as_deref(nir_instr_as_deref(nir_src_use_instr(use))->parent) == d)
                  leaf = false;
            if (leaf)
               u.accesses.push_back(d);
         }
      }
   }
   return u;
}

// Each reflected field has one usage node; arrays combine all element accesses.
// Aggregate accesses mark descendants, while field accesses mark ancestors only.
struct buffer_usage {
   struct node {
      unsigned first_child = 0, child_count = 0;
      bool referenced = false, used = false;
   };
   std::vector<node> nodes;

   void build(unsigned index, const glsl_type* type)
   {
      unsigned count = glsl_type_is_array(type) ? 1 :
         (glsl_type_is_struct(type) || glsl_type_is_interface(type)) ? glsl_get_length(type) : 0;
      unsigned first = (unsigned)nodes.size();
      nodes[index].first_child = first;
      nodes[index].child_count = count;
      nodes.resize(first + count);
      for (unsigned i = 0; i < count; ++i)
         build(first + i, glsl_type_is_array(type) ? glsl_get_array_element(type) : glsl_get_struct_field(type, i));
   }

   explicit buffer_usage(const glsl_type* block)
   {
      nodes.emplace_back();
      build(0, block);
   }

   // walk a deref chain from the block variable down
   void mark(const glslopt_buffer_block& b, nir_deref_instr* leaf)
   {
      nir_deref_path path;
      nir_deref_path_init(&path, leaf, NULL);
      nir_variable* v = path.path[0]->var;
      unsigned cur = 0;
      unsigned i = 1;
      if (!b.named) {
         int field = glsl_get_field_index(b.type, v->name);
         if (field < 0) { nir_deref_path_finish(&path); return; }
         cur = nodes[0].first_child + field;
      } else if (glsl_type_is_array(v->type)) {
         // block array: skip the element indices
         while (path.path[i] && path.path[i]->deref_type == nir_deref_type_array &&
                glsl_type_is_array(nir_deref_instr_parent(path.path[i])->type) &&
                !glsl_type_is_interface(nir_deref_instr_parent(path.path[i])->type) &&
                glsl_without_array(nir_deref_instr_parent(path.path[i])->type) == b.type)
            i++;
      }
      for (; path.path[i]; i++) {
         nir_deref_instr* d = path.path[i];
         if (d->deref_type == nir_deref_type_struct)
            cur = nodes[cur].first_child + d->strct.index;
         else if (d->deref_type == nir_deref_type_array && glsl_type_is_array(nir_deref_instr_parent(d)->type))
            cur = nodes[cur].first_child;
         else
            break; // vector/matrix component
      }
      nodes[cur].referenced = true;
      nir_deref_path_finish(&path);
   }

   bool propagate(unsigned index, bool whole = false)
   {
      node& n = nodes[index];
      whole |= n.referenced;
      n.used = whole;
      for (unsigned i = 0; i < n.child_count; ++i)
         n.used |= propagate(n.first_child + i, whole);
      return n.used;
   }
};

static void reflect_fields(glslopt_shader* shader, glslopt_buffer& buffer, const glsl_type* type, int parent, unsigned offset,
                           bool row, bool known, bool std430, const buffer_usage& usage, unsigned first_usage,
                           bool readOnly, bool writeOnly);

static void
reflect_buffer_member(glslopt_shader* shader, glslopt_buffer& buffer, const glsl_type* type, const char* name, int parent,
                      unsigned offset, bool row, bool known, bool std430, const buffer_usage& usage, unsigned usageIndex,
                      bool readOnly, bool writeOnly)
{
   int index = (int)buffer.members.size();
   bool array = glsl_type_is_array(type), matrix = glsl_type_is_matrix(type);
   unsigned stride = array ? glslopt_buffer_array_stride(glsl_get_array_element(type), row, std430) : 0;
   unsigned matrixStride = matrix ? glslopt_buffer_array_stride(row ? glsl_get_row_type(type) : glsl_get_column_type(type), false, std430) : 0;
   buffer.members.push_back({shader->strdup(name), shader->strdup(glsl_get_type_name(type)), parent, known ? (int)offset : -1,
                             array ? (glsl_get_length(type) ? (int)glsl_get_length(type) : -1) : 0,
                             array ? (known ? (int)stride : -1) : 0,
                             matrix ? (known ? (int)matrixStride : -1) : 0, matrix && row,
                             usage.nodes[usageIndex].used, readOnly, writeOnly});
   if (array)
      reflect_buffer_member(shader, buffer, glsl_get_array_element(type), "[]", index, offset, row, known, std430,
                            usage, usage.nodes[usageIndex].first_child, readOnly, writeOnly);
   else if (glsl_type_is_struct(type))
      reflect_fields(shader, buffer, type, index, offset, row, known, std430, usage, usage.nodes[usageIndex].first_child,
                     readOnly, writeOnly);
}

// fields of a struct or block; block fields carry their own access qualifiers
static void
reflect_fields(glslopt_shader* shader, glslopt_buffer& buffer, const glsl_type* type, int parent, unsigned offset,
               bool row, bool known, bool std430, const buffer_usage& usage, unsigned first_usage,
               bool readOnly, bool writeOnly)
{
   std::vector<unsigned> offsets = glslopt_buffer_field_offsets(type, row, std430);
   for (unsigned i = 0; i < glsl_get_length(type); ++i) {
      const glsl_struct_field& f = *glsl_get_struct_field_data(type, i);
      reflect_buffer_member(shader, buffer, f.type, f.name, parent, offset + offsets[i], glslopt_field_row_major(f, row),
                            known, std430, usage, first_usage + i, readOnly || f.memory_read_only,
                            writeOnly || f.memory_write_only);
   }
}

static void
reflect_atomic_counter(glslopt_shader* shader, glslopt_buffer& buffer, const glsl_type* type, const char* name,
                       int parent, unsigned offset, bool used)
{
   int index = (int)buffer.members.size();
   bool array = glsl_type_is_array(type);
   buffer.members.push_back({shader->strdup(name), shader->strdup(glsl_get_type_name(type)), parent, (int)offset,
                             array ? (int)glsl_get_length(type) : 0,
                             array ? (int)glsl_atomic_size(glsl_get_array_element(type)) : 0, 0, false, used, false, false});
   if (array)
      reflect_atomic_counter(shader, buffer, glsl_get_array_element(type), "[]", index, offset, used);
}

static void
find_shader_buffers(glslopt_shader* shader, nir_shader* nir, const usage_info& usage, bool metal)
{
   auto blocks = glslopt_collect_buffer_blocks(nir);
   unsigned sizeTableIndex = metal ? glslopt_buffer_size_table_index(blocks) : 0;
   for (const auto& b : blocks) {
      buffer_usage bu(b.type);
      for (nir_deref_instr* d : usage.accesses) {
         nir_variable* v = nir_deref_instr_get_variable(d);
         if (v && v->interface_type == b.type && (v->data.mode & (nir_var_mem_ubo | nir_var_mem_ssbo)))
            bu.mark(b, d);
      }
      bu.propagate(0);

      glslopt_buffer buffer;
      auto layout = glsl_get_ifc_packing(b.type);
      bool std430 = glslopt_buffer_uses_std430(b, metal);
      bool known = glslopt_buffer_layout_known(b, metal);
      bool runtime = glslopt_buffer_has_runtime_array(b.type);
      buffer.desc = {shader->strdup(glsl_get_type_name(b.type)), b.named ? shader->strdup(b.variable->name) : nullptr,
                     glslopt_block_is_storage(b) ? kGlslBufferStorage : kGlslBufferUniform,
                     layout == GLSL_INTERFACE_PACKING_STD430 ? kGlslLayoutStd430 : layout == GLSL_INTERFACE_PACKING_STD140 ? kGlslLayoutStd140 :
                     layout == GLSL_INTERFACE_PACKING_PACKED ? kGlslLayoutPacked : kGlslLayoutShared,
                     b.variable->data.explicit_binding ? (int)b.variable->data.binding : -1,
                     (int)b.array_size, known && !runtime ? (int)glslopt_buffer_block_size(b.type, std430) : -1,
                     metal ? (int)b.metal_index : -1, metal && b.needs_size ? (int)sizeTableIndex : -1};
      reflect_fields(shader, buffer, b.type, -1, 0, glsl_matrix_type_is_row_major(b.type), known, std430, bu,
                     bu.nodes[0].first_child, false, false);
      shader->buffers.push_back(std::move(buffer));
   }

   for (const auto& counter : glslopt_collect_atomic_counter_buffers(nir, blocks)) {
      glslopt_buffer buffer;
      buffer.desc = {ralloc_asprintf(shader->mem_ctx, "atomic_counters_%u", counter.binding), nullptr,
                     kGlslBufferAtomicCounter, kGlslLayoutAtomicCounter, (int)counter.binding, 1, (int)counter.size,
                     metal ? (int)counter.metal_index : -1, -1};
      for (nir_variable* v : counter.variables)
         reflect_atomic_counter(shader, buffer, v->type, v->name, -1, v->data.offset, usage.vars.count(v) != 0);
      shader->buffers.push_back(std::move(buffer));
   }
}

static glslopt_shader_var
make_var(glslopt_shader* sh, const nir_variable* var)
{
   glslopt_shader_var v;
   v.name = sh->strdup(var->name);
   glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
   v.location = -1;
   return v;
}

static void
find_shader_variables(glslopt_shader* sh, nir_shader* nir, const glslopt_metal_layout* metal)
{
   nir_foreach_variable_in_shader(var, nir) {
      const glsl_type* bare = glsl_without_array(var->type);
      if (var->data.mode == nir_var_shader_in) {
         if ((int)sh->inputs.size() >= glslopt_shader::kMaxShaderInputs)
            continue;
         glslopt_shader_var v = make_var(sh, var);
         if (metal && metal->attribute_slot.count(var))
            v.location = metal->attribute_slot.at(var);
         else if (var->data.explicit_location)
            v.location = var->data.location - (var->interface_type ? VARYING_SLOT_VAR0 : 0);
         sh->inputs.push_back(v);
      }
      if (var->data.mode == nir_var_uniform && !glsl_type_is_sampler(bare) && !glsl_type_contains_image(var->type) &&
          !glsl_contains_atomic(var->type)) {
         if ((int)sh->uniforms.size() >= glslopt_shader::kMaxShaderUniforms)
            continue;
         glslopt_shader_var v = make_var(sh, var);
         v.location = var->data.explicit_location ? var->data.location : -1;
         if (metal && metal->uniform_offset.count(var))
            v.location = metal->uniform_offset.at(var);
         sh->uniforms.push_back(v);
      }
      if ((var->data.mode == nir_var_uniform || var->data.mode == nir_var_image) &&
          (glsl_type_is_sampler(bare) || glsl_type_is_image(bare))) {
         if ((int)sh->textures.size() >= glslopt_shader::kMaxShaderTextures)
            continue;
         glslopt_shader_var v = make_var(sh, var);
         if (metal)
            v.location = metal->texture_slot.count(var) ? metal->texture_slot.at(var) : -1;
         else
            v.location = var->data.explicit_binding ? (int)var->data.binding :
                         var->data.explicit_location ? var->data.location : -1;
         sh->textures.push_back(v);
         bool image = glsl_type_is_image(bare);
         sh->isImage.push_back(image);
         glslopt_image_desc desc = {};
         if (image) {
            switch (glsl_get_sampler_result_type(bare)) {
            case GLSL_TYPE_FLOAT: desc.componentType = kGlslImageComponentFloat; break;
            case GLSL_TYPE_INT: desc.componentType = kGlslImageComponentInt; break;
            case GLSL_TYPE_UINT: desc.componentType = kGlslImageComponentUInt; break;
            default: break;
            }
            desc.format = glslopt_image_format_name((enum pipe_format)var->data.image.format);
            desc.readOnly = (var->data.access & ACCESS_NON_WRITEABLE) != 0;
            desc.writeOnly = (var->data.access & ACCESS_NON_READABLE) != 0;
         }
         sh->images.push_back(desc);
      }
   }
}

static void
calculate_stats(nir_shader* nir, int* math, int* tex, int* flow)
{
   *math = *tex = *flow = 0;
   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block(block, impl) {
      nir_cf_node* parent = block->cf_node.parent;
      // count each if / loop once, at the first block of its body
      if ((parent->type == nir_cf_node_if && nir_if_first_then_block(nir_cf_node_as_if(parent)) == block) ||
          (parent->type == nir_cf_node_loop && nir_loop_first_block(nir_cf_node_as_loop(parent)) == block))
         ++*flow;
      nir_foreach_instr(instr, block) {
         switch (instr->type) {
         case nir_instr_type_alu: {
            nir_op op = nir_instr_as_alu(instr)->op;
            if (op != nir_op_mov && op != nir_op_vec2 && op != nir_op_vec3 && op != nir_op_vec4)
               ++*math;
            break;
         }
         case nir_instr_type_tex:
            ++*tex;
            break;
         case nir_instr_type_intrinsic:
            switch (nir_instr_as_intrinsic(instr)->intrinsic) {
            case nir_intrinsic_image_deref_load: case nir_intrinsic_image_deref_store:
            case nir_intrinsic_image_deref_atomic: case nir_intrinsic_image_deref_atomic_swap:
            case nir_intrinsic_image_deref_size: case nir_intrinsic_image_deref_samples:
            case nir_intrinsic_terminate: case nir_intrinsic_demote:
               ++*tex;
               break;
            case nir_intrinsic_terminate_if: case nir_intrinsic_demote_if:
               ++*tex;
               ++*flow;
               break;
            default:
               break;
            }
            break;
         case nir_instr_type_jump:
            if (nir_instr_as_jump(instr)->type == nir_jump_return)
               ++*flow;
            break;
         default:
            break;
         }
      }
   }
}

// ---------------------------------------------------------------------------

glslopt_shader* glslopt_optimize(glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options)
{
   if (!ctx)
      return NULL;
   glslopt_shader* result = new glslopt_shader;
   gl_shader* shader = result->shader;
   switch (type) {
   case kGlslOptShaderVertex: shader->Stage = MESA_SHADER_VERTEX; break;
   case kGlslOptShaderFragment: shader->Stage = MESA_SHADER_FRAGMENT; break;
   case kGlslOptShaderCompute: shader->Stage = MESA_SHADER_COMPUTE; break;
   default:
      result->infoLog = ralloc_asprintf(result->mem_ctx, "Unknown shader type %d", (int)type);
      return result;
   }
   if (!shaderSource) {
      result->infoLog = "Shader source is null";
      return result;
   }
   const bool metal = ctx->target == kGlslTargetMetal;
   shader->Source = ralloc_strdup(shader, shaderSource);

   parse_info parsed;
   _mesa_glsl_compile_shader_glslopt(&ctx->mesa_ctx, shader, (options & kGlslOptionSkipPreprocessor) != 0,
                                     capture_parse_state, &parsed);
   result->infoLog = shader->InfoLog;
   if (shader->CompileStatus == COMPILE_FAILURE || !shader->nir)
      return result;
   glslopt_source_info& info = parsed.info;
   info.had_version = strstr(shaderSource, "#version") != nullptr;

   if (type == kGlslOptShaderCompute) {
      // (upstream checks this when linking)
      if (!info.local_size_specified && !parsed.variable_local_size) {
         result->fail("error: compute shader must contain a fixed or a variable local group size\n");
         return result;
      }
      if (!info.local_size_specified && metal) {
         result->fail("Metal compute output requires a fixed local group size.");
         return result;
      }
      for (unsigned i = 0; i < 3; ++i)
         result->computeLocalSize[i] = info.local_size[i];
   }
   if (!metal && info.es && info.version < 300) {
      result->fail("OpenGL ES 2.0 output is no longer supported; use #version 300 es or newer.");
      return result;
   }
   if (parsed.has_subroutines) {
      result->fail("Shader subroutines are not supported.");
      return result;
   }

   nir_shader* nir = shader->nir;
   if (!nir_shader_get_entrypoint(nir)) {
      result->fail("Shader has no main function.");
      return result;
   }
   if (nir_uses_double(nir)) {
      result->fail("Double-precision types are not supported.");
      return result;
   }
   glslopt_lower_nir(nir);
   if (metal) {
      if (const char* error = validate_metal(nir)) {
         result->fail(error);
         return result;
      }
   }

   glslopt_optimize_nir(nir);
   // aggregate copies print as one assignment where the dialect can express them
   glslopt_combine_copies(nir, [&](nir_deref_instr* dst, nir_deref_instr* src) {
      nir_variable* dv = nir_deref_instr_get_variable(dst);
      nir_variable* sv = nir_deref_instr_get_variable(src);
      if (!dv || !sv)
         return false;
      if (!metal)
         return !glsl_type_is_array(dst->type) || info.es || info.version >= 120;
      // Metal: C arrays do not assign, blocks have storage-layout structs, shared
      // matrices are column arrays
      if (glsl_type_is_array(dst->type))
         return false;
      bool dbuf = dv->data.mode & (nir_var_mem_ubo | nir_var_mem_ssbo), sbuf = sv->data.mode & (nir_var_mem_ubo | nir_var_mem_ssbo);
      if (dbuf || sbuf)
         return dbuf && sbuf && dv->interface_type == sv->interface_type;
      auto has_matrix = [](const glsl_type* t) {
         std::function<bool(const glsl_type*)> f = [&](const glsl_type* t) {
            if (glsl_type_is_matrix(t)) return true;
            if (glsl_type_is_array(t)) return f(glsl_get_array_element(t));
            for (unsigned i = 0; glsl_type_is_struct(t) && i < glsl_get_length(t); i++)
               if (f(glsl_get_struct_field(t, i))) return true;
            return false;
         };
         return f(t);
      };
      if ((dv->data.mode == nir_var_mem_shared || sv->data.mode == nir_var_mem_shared) && has_matrix(dst->type))
         return false;
      // half vs float aggregates do not assign
      auto p = [](nir_deref_instr* d) { return glslopt_precision_info::norm(glslopt_precision_info::deref_prec(d)) == GLSL_PRECISION_MEDIUM; };
      return p(dst) == p(src);
   });
   usage_info usage = collect_usage(nir);
   glslopt_metal_layout layout;
   std::string optText = metal ? glslopt_print_metal(nir, info, layout) : glslopt_print_glsl(nir, info);
   result->optimizedOutput = ralloc_strdup(result->mem_ctx, optText.c_str());
   result->uniformsSize = metal ? layout.uniforms_size : 0;

   find_shader_variables(result, nir, metal ? &layout : nullptr);
   find_shader_buffers(result, nir, usage, metal);
   calculate_stats(nir, &result->statsMath, &result->statsTex, &result->statsFlow);
   result->status = true;
   return result;
}

void glslopt_shader_delete(glslopt_shader* shader)
{
   delete shader;
}

bool glslopt_get_status(glslopt_shader* shader)
{
   return shader->status;
}

const char* glslopt_get_output(glslopt_shader* shader)
{
   return shader->optimizedOutput;
}

const char* glslopt_get_log(glslopt_shader* shader)
{
   return shader->infoLog;
}

void glslopt_shader_get_compute_local_size(glslopt_shader* shader, int* x, int* y, int* z)
{
   *x = shader->computeLocalSize[0];
   *y = shader->computeLocalSize[1];
   *z = shader->computeLocalSize[2];
}

int glslopt_shader_get_buffer_count(glslopt_shader* shader) { return (int)shader->buffers.size(); }
const glslopt_buffer_desc* glslopt_shader_get_buffer_desc(glslopt_shader* shader, int index)
{
   return index >= 0 && index < (int)shader->buffers.size() ? &shader->buffers[index].desc : nullptr;
}
int glslopt_shader_get_buffer_member_count(glslopt_shader* shader, int index)
{
   return index >= 0 && index < (int)shader->buffers.size() ? (int)shader->buffers[index].members.size() : 0;
}
const glslopt_buffer_member_desc* glslopt_shader_get_buffer_member_desc(glslopt_shader* shader, int blockIndex, int memberIndex)
{
   if (blockIndex < 0 || blockIndex >= (int)shader->buffers.size()) return nullptr;
   const auto& members = shader->buffers[blockIndex].members;
   return memberIndex >= 0 && memberIndex < (int)members.size() ? &members[memberIndex] : nullptr;
}

int glslopt_shader_get_input_count(glslopt_shader* shader) { return (int)shader->inputs.size(); }
int glslopt_shader_get_uniform_count(glslopt_shader* shader) { return (int)shader->uniforms.size(); }
int glslopt_shader_get_uniform_total_size(glslopt_shader* shader) { return shader->uniformsSize; }
int glslopt_shader_get_texture_count(glslopt_shader* shader) { return (int)shader->textures.size(); }

static void
get_var_desc(const glslopt_shader_var& v, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec,
             int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
   *outName = v.name;
   *outType = v.type;
   *outPrec = v.prec;
   *outVecSize = v.vectorSize;
   *outMatSize = v.matrixSize;
   *outArraySize = v.arraySize;
   *outLocation = v.location;
}

void glslopt_shader_get_input_desc(glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
   get_var_desc(shader->inputs[index], outName, outType, outPrec, outVecSize, outMatSize, outArraySize, outLocation);
}

void glslopt_shader_get_uniform_desc(glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
   get_var_desc(shader->uniforms[index], outName, outType, outPrec, outVecSize, outMatSize, outArraySize, outLocation);
}

void glslopt_shader_get_texture_desc(glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
   get_var_desc(shader->textures[index], outName, outType, outPrec, outVecSize, outMatSize, outArraySize, outLocation);
}

const glslopt_image_desc* glslopt_shader_get_image_desc(glslopt_shader* shader, int textureIndex)
{
   if (textureIndex < 0 || textureIndex >= (int)shader->textures.size() || !shader->isImage[textureIndex])
      return nullptr;
   return &shader->images[textureIndex];
}

void glslopt_shader_get_stats(glslopt_shader* shader, int* approxMath, int* approxTex, int* approxFlow)
{
   *approxMath = shader->statsMath;
   *approxTex = shader->statsTex;
   *approxFlow = shader->statsFlow;
}
