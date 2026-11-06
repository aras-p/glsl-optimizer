/*
 * Copyright © 2014 Unity Technologies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ir_print_metal_visitor.h"
#include "ir_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "ir_hierarchical_visitor.h"
#include "ir_optimization.h"
#include <unordered_map>
#include <unordered_set>
#include "loop_analysis.h"
#include "ir_print_loop.h"

#include <math.h>
#include "ir_buffer_blocks.h"
#include "ir_image.h"
#include "string_to_uint_map.h"


static unsigned precision_from_ir(ir_instruction* ir)
{
   if (!ir) return GLSL_PRECISION_NONE;
   if (ir_variable* v = ir->as_variable()) return v->data.precision;
   if (ir_rvalue* r = ir->as_rvalue()) return r->get_precision();
   if (ir->ir_type == ir_type_function_signature) return ((ir_function_signature*)ir)->return_precision;
   return GLSL_PRECISION_NONE;
}
static unsigned higher_precision(unsigned a, unsigned b)
{
   if (!a) return b;
   if (!b) return a;
   return a < b ? a : b;
}
static void print_type_precision(string_buffer&, const glsl_type*, unsigned, bool);
static void print_metal_structs(string_buffer&, exec_list*);

static void print_type(string_buffer& buffer, ir_instruction* ir, const glsl_type *t, bool arraySize);
static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize);


struct ga_entry_metal : public exec_node
{
   ga_entry_metal(ir_instruction* ir)
   {
      assert(ir);
      this->ir = ir;
   }
   ir_instruction* ir;
};
struct gconst_entry_metal : public exec_node
{
   gconst_entry_metal(ir_constant* ir, unsigned id)
   {
      assert(ir);
      this->ir = ir;
      this->id = id;
   }
   ir_constant* ir;
   unsigned id;
};


struct global_print_tracker_metal
{
   global_print_tracker_metal ()
   {
      mem_ctx = ralloc_context(0);
      var_counter = 0;
      const_counter = 0;
      main_function_done = false;
   }

   ~global_print_tracker_metal()
   {
      ralloc_free(mem_ctx);
   }

   unsigned var_counter;
   std::unordered_map<const ir_variable*, long> var_hash;
   exec_list global_assignements;

   unsigned const_counter;
   std::unordered_map<const ir_constant*, long> const_hash;
   exec_list global_constants;

   void* mem_ctx;
   bool main_function_done;
};


struct metal_print_context
{
   metal_print_context(char* buffer)
      : str(buffer)
   , prefixStr(ralloc_strdup(buffer, ""))
   , inputStr(ralloc_strdup(buffer, ""))
   , outputStr(ralloc_strdup(buffer, ""))
   , inoutStr(ralloc_strdup(buffer, ""))
   , uniformStr(ralloc_strdup(buffer, ""))
   , paramsStr(ralloc_strdup(buffer, ""))
   , typedeclStr(ralloc_strdup(buffer, ""))
   , blockAliases(ralloc_strdup(buffer, ""))
   , stageLocals(ralloc_strdup(buffer, ""))
   , stageExports(ralloc_strdup(buffer, ""))
   , mem_ctx(ralloc_context(NULL))
   , writingParams(false)
   , matrixCastsDone(false)
   , matrixConstructorsDone(false)
   , shadowSamplerDone(false)
   , atomicCompSwapSpaces(0)
   , memoryBarrierDone(false)
   , imageAtomicDone(false)
   , cubeImageAtomicDone(false)
   , imageCompSwapDims(0)
   , textureCounter(0)
   , imageCounter(0)
   , attributeCounter(0)
   , uniformLocationCounter(0)
   , colorCounter(0)
   {
   }

   ~metal_print_context() { ralloc_free(mem_ctx); }

   string_buffer str;
   string_buffer prefixStr;
   string_buffer inputStr;
   string_buffer outputStr;
   string_buffer inoutStr;
   string_buffer uniformStr;
   string_buffer paramsStr;
   string_buffer typedeclStr;
   string_buffer blockAliases;
   string_buffer stageLocals;
   string_buffer stageExports;
   std::unordered_map<const ir_variable*, const char*> stageNames;
   unsigned stageFieldCount = 0;
   enum storage_kind {
      uniform_std140, storage_std140, storage_std430,
      coherent_std140, coherent_std430, storage_kind_count
   };
   static bool is_std430(unsigned kind) { return kind == storage_std430 || kind == coherent_std430; }
   static bool is_coherent(unsigned kind) { return kind == coherent_std140 || kind == coherent_std430; }
   static const char* buffer_address(unsigned kind, bool readOnly)
   {
      if (kind == uniform_std140) return "constant";
      if (is_coherent(kind)) return readOnly ? "const coherent(device) device" : "coherent(device) device";
      return readOnly ? "const device" : "device";
   }
   struct runtime_field { const char* storage; unsigned offset; unsigned stride; };
   struct block_reference {
      unsigned index; bool array; unsigned storage; bool row_major;
      const runtime_field* fields; unsigned member_index; const char* address;
   };
   std::unordered_map<const ir_variable*, block_reference> blockNames;
   std::unordered_map<const ir_variable*, unsigned> counterSlots;
   bool countersReadOnly = false;
   std::unordered_map<const glsl_type*, unsigned> uniformStorageTypes[storage_kind_count][2];
   std::vector<const char*> uniformStorageNames;
   std::unordered_map<const glsl_type*, const char*> sharedStructTypes;
   bool sharedMatrixTemplate = false;
   std::unordered_map<const glsl_type*, bool> nativeBufferTypes[storage_kind_count][2];
   void* mem_ctx;
   bool writingParams;
   bool matrixCastsDone;
   bool matrixConstructorsDone;
   bool shadowSamplerDone;
   unsigned atomicCompSwapSpaces;
   bool memoryBarrierDone;
   bool imageAtomicDone;
   bool cubeImageAtomicDone;
   unsigned imageCompSwapDims;
   int textureCounter;
   int imageCounter;
   int attributeCounter;
   std::unordered_set<int> explicitAttributeSlots;
   bool reflectAttributeLocations = false;
   int uniformLocationCounter;
   int colorCounter;
};


static void prepare_stage_blocks(metal_print_context&, exec_list*);
static void prepare_buffer_blocks(metal_print_context&, exec_list*);

class ir_print_metal_visitor : public ir_visitor {
public:
   ir_print_metal_visitor(metal_print_context& ctx_, string_buffer& buf, global_print_tracker_metal* globals_, PrintGlslMode mode_, const _mesa_glsl_parse_state* state_)
      : ctx(ctx_)
      , buffer(buf)
      , loopstate(NULL)
      , inside_lhs(false)
      , skipped_this_ir(false)
      , previous_skipped(false)
      , mode_whole(mode_)
   {
      indentation = 0;
      expression_depth = 0;
      globals = globals_;
      mode = mode_;
      state = state_;
   }

   virtual ~ir_print_metal_visitor()
   {
   }


   void indent(void);
   void newline_indent();
   void end_statement_line();
   void newline_deindent();
   void print_var_name (ir_variable* v);
   void print_array_initializer(ir_constant* value, unsigned precision);
   bool print_uniform_read(ir_rvalue* value);
   void print_uniform_storage(ir_rvalue* value, const metal_print_context::block_reference& block);
   void print_counter_address(ir_rvalue* value);
   void print_image_coordinates(ir_rvalue* coord, const glsl_type* image);
   void print_buffer_size_index(ir_rvalue* value);

   virtual void visit(ir_variable *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_if *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_demote *) { buffer.asprintf_append("discard_fragment()"); }
   virtual void visit(ir_barrier *) { buffer.asprintf_append("threadgroup_barrier(mem_flags::mem_threadgroup)"); }
   virtual void visit(ir_emit_vertex *);
   virtual void visit(ir_end_primitive *);

   bool print_shared_access(ir_rvalue* value);
   void print_shared_storage(ir_rvalue* value);
   bool emit_storage_matrix_write(ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex);
   void emit_buffer_aggregate(ir_dereference* lhs, ir_rvalue* rhs);
   void emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex);

   bool emit_canonical_for (ir_loop* ir);

   metal_print_context& ctx;
   int indentation;
   int expression_depth;
   bool inEntryPoint = false;
   string_buffer& buffer;
   global_print_tracker_metal* globals;
   const _mesa_glsl_parse_state* state;
   PrintGlslMode mode;
   const PrintGlslMode mode_whole;
   loop_state* loopstate;
   print_loop_initializers* loop_initializers = nullptr;
   bool inside_lhs;
   bool skipped_this_ir;
   bool previous_skipped;
};


char*
_mesa_print_ir_metal(exec_list *instructions,
                     struct _mesa_glsl_parse_state *state,
                     char* buffer, PrintGlslMode mode, int* outUniformsSize)
{
   if (mode == kPrintGlslFragment) optimize_split_arrays(instructions, false, true);
   metal_print_context ctx(buffer);
   ctx.countersReadOnly = state->stage == MESA_SHADER_VERTEX;
   // Raw printing leaves vertex locations in Mesa's slot numbering for linking.
   ctx.reflectAttributeLocations = outUniformsSize != nullptr;
   // Keep sampled textures paired with their sampler indices; images occupy
   // the remaining texture slots without pushing sampler indices upward.
   foreach_in_list(ir_instruction, instruction, instructions) {
      const auto* var = instruction->as_variable();
      if (var && var->data.mode == ir_var_uniform && var->type->without_array()->is_sampler())
         ++ctx.imageCounter;
      if (mode == kPrintGlslVertex && var && var->data.mode == ir_var_shader_in &&
          var->data.explicit_location && strncmp(var->name, "gl_", 3) != 0) {
         const int base = var->data.location - VERT_ATTRIB_GENERIC0;
         const int slots = var->type->count_attribute_slots(true);
         for (int i = 0; i < slots; ++i)
            ctx.explicitAttributeSlots.insert(base + i);
      }
   }

   // includes, prefix etc.
   ctx.prefixStr.asprintf_append ("#include <metal_stdlib>\n");
   ctx.prefixStr.asprintf_append ("#pragma clang diagnostic ignored \"-Wparentheses-equality\"\n");
   ctx.prefixStr.asprintf_append ("using namespace metal;\n");
   foreach_in_list(ir_instruction, instruction, instructions) {
      const auto* var = instruction->as_variable();
      if (var && var->type->without_array()->is_image() && var->data.memory_coherent) {
         ctx.prefixStr.asprintf_append(
            "#if __METAL_VERSION__ < 320\n"
            "#error Coherent images require Metal 3.2 or newer\n"
            "#endif\n");
         break;
      }
   }

   ctx.inputStr.asprintf_append("struct xlatMtlShaderInput {\n");
   ctx.outputStr.asprintf_append("struct xlatMtlShaderOutput {\n");
   ctx.uniformStr.asprintf_append("struct xlatMtlShaderUniform {\n");

   print_metal_structs(ctx.typedeclStr, instructions);
   prepare_buffer_blocks(ctx, instructions);
   prepare_stage_blocks(ctx, instructions);

   global_print_tracker_metal gtracker;

   loop_state* ls = analyze_loop_variables(instructions);
   print_loop_initializers initializers(instructions, ls);


   foreach_in_list(ir_instruction, ir, instructions)
   {
      string_buffer* strOut = &ctx.str;
      ctx.writingParams = false;
      if (ir->ir_type == ir_type_variable)
      {
         ir_variable *var = static_cast<ir_variable*>(ir);
         if (var->is_in_buffer_block() || var->type->contains_atomic() || ctx.stageNames.count(var)) continue;

         // skip gl_ variables if they aren't used/assigned
         if (strstr(var->name, "gl_") == var->name)
         {
            if (!var->data.used && !var->data.assigned)
               continue;
         }

         //
         if (var->data.mode == ir_var_uniform)
         {
            if (var->type->without_array()->is_sampler() || var->type->contains_image())
            {
               strOut = &ctx.paramsStr;
               ctx.writingParams = true;
               strOut->asprintf_append ("\n  , ");
            }
            else
               strOut = &ctx.uniformStr;
         }
         if (var->data.mode == ir_var_system_value)
         {
            strOut = &ctx.paramsStr;
            ctx.writingParams = true;
            strOut->asprintf_append ("\n  , ");
         }
         if (var->data.mode == ir_var_shader_in)
            strOut = &ctx.inputStr;
         if (var->data.mode == ir_var_shader_out)
            strOut = &ctx.outputStr;
         if (var->data.mode == ir_var_shader_out && var->data.fb_fetch_output)
            strOut = &ctx.inoutStr;
      }


      ir_print_metal_visitor v (ctx, *strOut, &gtracker, mode, state);
      v.loopstate = ls;
      v.loop_initializers = &initializers;

      if (strOut == &ctx.inputStr || strOut == &ctx.outputStr || strOut == &ctx.inoutStr || strOut == &ctx.uniformStr)
         strOut->asprintf_append("  ");
      ir->accept(&v);
      if (ir->ir_type != ir_type_function && !v.skipped_this_ir)
      {
         if (!ctx.writingParams)
            strOut->asprintf_append (";\n");
      }
   }

   delete ls;

   // append inout variables to both input & output structs
   if (!ctx.inoutStr.empty())
   {
      ctx.inputStr.asprintf_append("%s", ctx.inoutStr.c_str());
      ctx.outputStr.asprintf_append("%s", ctx.inoutStr.c_str());
   }
   ctx.inputStr.asprintf_append("};\n");
   ctx.outputStr.asprintf_append("};\n");
   ctx.uniformStr.asprintf_append("};\n");

   // emit global array/struct constants

   ctx.prefixStr.asprintf_append("%s", ctx.typedeclStr.c_str());
   foreach_in_list_safe(gconst_entry_metal, node, &gtracker.global_constants)
   {
      ir_constant* c = node->ir;

      ir_print_metal_visitor v (ctx, ctx.prefixStr, &gtracker, mode, state);

      v.buffer.asprintf_append ("constant ");
      print_type(v.buffer, c, c->type, false);
      v.buffer.asprintf_append (" _xlat_mtl_const%i", (int)((gconst_entry_metal*)node)->id);
      print_type_post(v.buffer, c->type, false);
      v.buffer.asprintf_append (" = {");

      if (c->type->is_array())
      {
         for (unsigned i = 0; i < c->type->length; i++)
         {
            if (i != 0)
               v.buffer.asprintf_append (", ");
            ir_constant* element = c->get_array_element(i);
            // Array precision applies to its elements, including propagated constants.
            if ((c->get_precision() == GLSL_PRECISION_MEDIUM) !=
                (element->get_precision() == GLSL_PRECISION_MEDIUM))
               v.print_array_initializer(element, c->get_precision());
            else
               element->accept(&v);
         }
      }
      else
      {
         assert(c->type->is_struct());
         bool first = true;
         for (unsigned component = 0; component < c->type->length; ++component)
         {
            if (!first)
               v.buffer.asprintf_append (", ");
            first = false;
            const glsl_struct_field& field = c->type->fields.structure[component];
            const bool cast = field.type->is_numeric() && field.precision == GLSL_PRECISION_MEDIUM;
            if (cast) { print_type_precision(v.buffer, field.type, field.precision, true); v.buffer.asprintf_append("("); }
            c->const_elements[component]->accept(&v);
            if (cast) v.buffer.asprintf_append(")");
         }
      }
      v.buffer.asprintf_append ("};\n");
   }


   if (mode != kPrintGlslCompute) {
      ctx.prefixStr.asprintf_append("%s", ctx.inputStr.c_str());
      ctx.prefixStr.asprintf_append("%s", ctx.outputStr.c_str());
   }
   ctx.prefixStr.asprintf_append("%s", ctx.uniformStr.c_str());
   ctx.prefixStr.asprintf_append("%s", ctx.str.c_str());

   if (outUniformsSize)
      *outUniformsSize = ctx.uniformLocationCounter;

   return ralloc_strdup(buffer, ctx.prefixStr.c_str());
}


void ir_print_metal_visitor::indent(void)
{
   if (previous_skipped)
      return;
   previous_skipped = false;
   for (int i = 0; i < indentation; i++)
      buffer.asprintf_append ("  ");
}

void ir_print_metal_visitor::end_statement_line()
{
   if (!skipped_this_ir)
      buffer.asprintf_append(";\n");
   previous_skipped = skipped_this_ir;
   skipped_this_ir = false;
}

void ir_print_metal_visitor::newline_indent()
{
   if (expression_depth % 4 == 0)
   {
      ++indentation;
      buffer.asprintf_append ("\n");
      indent();
   }
}
void ir_print_metal_visitor::newline_deindent()
{
   if (expression_depth % 4 == 0)
   {
      --indentation;
      buffer.asprintf_append ("\n");
      indent();
   }
}


void ir_print_metal_visitor::print_var_name (ir_variable* v)
{
   long id = globals->var_hash[v];
   if (!id && v->data.mode == ir_var_temporary)
   {
      id = ++globals->var_counter;
      globals->var_hash[v] = id;
   }
   if (id)
   {
      if (v->data.mode == ir_var_temporary)
         buffer.asprintf_append ("tmpvar_%d", (int)id);
      else
      {
         // reuse trailing underscore (avoid reserved double underscores)
         const size_t length = strlen(v->name);
         const char* separator = (length > 0 && v->name[length - 1] == '_') ? "" : "_";
         buffer.asprintf_append ("%s%s%d", v->name, separator, (int)id);
      }
   }
   else
   {
      buffer.asprintf_append ("%s", v->name);
   }
}



static void print_type_precision(string_buffer& buffer, const glsl_type *t, unsigned prec, bool arraySize)
{
   const bool halfPrec = (prec == GLSL_PRECISION_MEDIUM);

   const char* typeName = t->name;
   // scalars
   if (!strcmp(typeName, "float"))
      typeName = halfPrec ? "half" : "float";
   else if (!strcmp(typeName, "int"))
      typeName = halfPrec ? "short" : "int";
   else if (!strcmp(typeName, "uint")) typeName = halfPrec ? "ushort" : "uint";
   else if (!strcmp(typeName, "uvec2")) typeName = halfPrec ? "ushort2" : "uint2";
   else if (!strcmp(typeName, "uvec3")) typeName = halfPrec ? "ushort3" : "uint3";
   else if (!strcmp(typeName, "uvec4")) typeName = halfPrec ? "ushort4" : "uint4";
   // vectors
   else if (!strcmp(typeName, "vec2"))
      typeName = halfPrec ? "half2" : "float2";
   else if (!strcmp(typeName, "vec3"))
      typeName = halfPrec ? "half3" : "float3";
   else if (!strcmp(typeName, "vec4"))
      typeName = halfPrec ? "half4" : "float4";
   else if (!strcmp(typeName, "ivec2"))
      typeName = halfPrec ? "short2" : "int2";
   else if (!strcmp(typeName, "ivec3"))
      typeName = halfPrec ? "short3" : "int3";
   else if (!strcmp(typeName, "ivec4"))
      typeName = halfPrec ? "short4" : "int4";
   else if (!strcmp(typeName, "bvec2"))
      typeName = "bool2";
   else if (!strcmp(typeName, "bvec3"))
      typeName = "bool3";
   else if (!strcmp(typeName, "bvec4"))
      typeName = "bool4";
   // matrices
   else if (!strcmp(typeName, "mat2"))
      typeName = halfPrec ? "half2x2" : "float2x2";
   else if (!strcmp(typeName, "mat3"))
      typeName = halfPrec ? "half3x3" : "float3x3";
   else if (!strcmp(typeName, "mat4"))
      typeName = halfPrec ? "half4x4" : "float4x4";
   // non-square matrices
   else if (!strcmp(typeName, "mat2x2"))
      typeName = halfPrec ? "half2x2" : "float2x2";
   else if (!strcmp(typeName, "mat2x3"))
      typeName = halfPrec ? "half2x3" : "float2x3";
   else if (!strcmp(typeName, "mat2x4"))
      typeName = halfPrec ? "half2x4" : "float2x4";
   else if (!strcmp(typeName, "mat3x2"))
      typeName = halfPrec ? "half3x2" : "float3x2";
   else if (!strcmp(typeName, "mat3x3"))
      typeName = halfPrec ? "half3x3" : "float3x3";
   else if (!strcmp(typeName, "mat3x4"))
      typeName = halfPrec ? "half3x4" : "float3x4";
   else if (!strcmp(typeName, "mat4x2"))
      typeName = halfPrec ? "half4x2" : "float4x2";
   else if (!strcmp(typeName, "mat4x3"))
      typeName = halfPrec ? "half4x3" : "float4x3";
   else if (!strcmp(typeName, "mat4x4"))
      typeName = halfPrec ? "half4x4" : "float4x4";
   // samplers
   else if (!strcmp(typeName, "sampler2D") || !strcmp(typeName, "samplerExternalOES"))
      typeName = halfPrec ? "texture2d<half>" : "texture2d<float>";
   else if (!strcmp(typeName, "samplerCube"))
      typeName = halfPrec ? "texturecube<half>" : "texturecube<float>";
   else if (!strcmp(typeName, "sampler3D"))
      typeName = halfPrec ? "texture3d<half>" : "texture3d<float>";
   else if (!strcmp(typeName, "sampler2DShadow"))
      typeName = "depth2d<float>"; // depth type must always be float
   else if (!strcmp(typeName, "samplerCubeShadow"))
      typeName = "depthcube<float>"; // depth type must always be float
   else if (!strcmp(typeName, "sampler2DArray"))
      typeName = halfPrec ? "texture2d_array<half>" : "texture2d_array<float>";

   else if (!strcmp(typeName, "samplerCubeArray"))
      typeName = halfPrec ? "texturecube_array<half>" : "texturecube_array<float>";
   else if (!strcmp(typeName, "sampler2DArrayShadow"))
      typeName = "depth2d_array<float>";
   else if (!strcmp(typeName, "samplerCubeArrayShadow"))
      typeName = "depthcube_array<float>";
   else if (t->is_sampler() && (t->sampled_type == GLSL_TYPE_INT || t->sampled_type == GLSL_TYPE_UINT) &&
            (t->sampler_dimensionality == GLSL_SAMPLER_DIM_2D || t->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE)) {
      buffer.asprintf_append("texture%s%s<%s>", t->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE ? "cube" : "2d",
                             t->sampler_array ? "_array" : "", t->sampled_type == GLSL_TYPE_INT ? "int" : "uint");
      return;
   }

   if (t->base_type == GLSL_TYPE_ARRAY) {
      print_type_precision(buffer, t->without_array(), prec, false);
      if (arraySize)
         for (auto a = t; a->is_array(); a = a->fields.array) buffer.asprintf_append("[%u]", a->length);
   } else if ((t->base_type == GLSL_TYPE_STRUCT)
            && (strncmp("gl_", typeName, 3) != 0)) {
      buffer.asprintf_append ("%s", typeName);
   } else {
      buffer.asprintf_append ("%s", typeName);
   }
}


static void print_type(string_buffer& buffer, ir_instruction* ir, const glsl_type *t, bool arraySize)
{
   const glsl_type* image = t->without_array();
   if (image->is_image()) {
      const auto* var = ir->as_variable();
      bool multisample = image->sampler_dimensionality == GLSL_SAMPLER_DIM_MS;
      if (t->is_array()) buffer.asprintf_append("array<");
      buffer.asprintf_append("%s<%s, access::%s%s>",
                             metal_image_type_name(image),
                             image->sampled_type == GLSL_TYPE_UINT ? "uint" : image->sampled_type == GLSL_TYPE_INT ? "int" : "float",
                             multisample || (var && var->data.memory_read_only) ? "read" : var && var->data.memory_write_only ? "write" : "read_write",
                             var && var->data.memory_coherent ? ", memory_coherence_device" : "");
      if (t->is_array()) buffer.asprintf_append(", %u>", t->length);
      return;
   }
   unsigned prec = precision_from_ir(ir);
   print_type_precision(buffer, t, prec, arraySize);
}


static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize)
{
   if (t->is_array() && !t->without_array()->is_image()) {
      if (!arraySize)
         for (auto a = t; a->is_array(); a = a->fields.array) buffer.asprintf_append("[%u]", a->length);
   }
}

// Metal matrices have no threadgroup constructor. Store their columns in a
// plain aggregate and use native matrices for values in private storage.
static bool shared_storage_is_wrapped(const metal_print_context& ctx, const glsl_type* type)
{
   type = type->without_array();
   if (type->is_matrix()) return true;
   if (!type->is_struct()) return false;
   auto found = ctx.sharedStructTypes.find(type);
   return found != ctx.sharedStructTypes.end() && found->second;
}

static void copy_shared_field(metal_print_context& ctx, const glsl_type* type, const char* dst,
                              const char* src, bool toShared, unsigned depth)
{
   auto& out = ctx.typedeclStr;
   if (toShared && type->is_array() && type->fields.array->is_matrix() && type->length <= 4) {
      // Constant indices let Metal keep small private matrix arrays in registers.
      for (unsigned i = 0; i < type->length; ++i)
         out.asprintf_append("    %s[%u].write(%s[%u]);\n", dst, i, src, i);
   } else if (type->is_array()) {
      out.asprintf_append("    for (uint i%u = 0; i%u < %uu; ++i%u) {\n", depth, depth, type->length, depth);
      copy_shared_field(ctx, type->fields.array,
                        ralloc_asprintf(ctx.mem_ctx, "%s[i%u]", dst, depth),
                        ralloc_asprintf(ctx.mem_ctx, "%s[i%u]", src, depth), toShared, depth + 1);
      out.asprintf_append("    }\n");
   } else if (shared_storage_is_wrapped(ctx, type)) {
      out.asprintf_append(toShared ? "    %s.write(%s);\n" : "    %s = %s.read();\n", dst, src);
   } else {
      out.asprintf_append("    %s = %s;\n", dst, src);
   }
}

static void print_shared_type(metal_print_context& ctx, string_buffer& out, const glsl_type* type, unsigned precision);

static void ensure_shared_matrix_template(metal_print_context& ctx)
{
   if (!ctx.sharedMatrixTemplate) {
      ctx.sharedMatrixTemplate = true;
      ctx.typedeclStr.asprintf_append(
         "template<typename M, typename V, uint N>\n"
         "struct _xlat_shared_matrix {\n"
         "  V columns[N];\n"
         "  M read() const threadgroup {\n"
         "    M value;\n"
         "    for (uint i = 0; i < N; ++i) value[i] = columns[i];\n"
         "    return value;\n"
         "  }\n"
         "  template<typename T> void write(T value) threadgroup {\n"
         "    for (uint i = 0; i < N; ++i) columns[i] = V(value[i]);\n"
         "  }\n"
         "};\n");
   }
}

static const char* shared_struct_type(metal_print_context& ctx, const glsl_type* type)
{
   auto found = ctx.sharedStructTypes.find(type);
   if (found != ctx.sharedStructTypes.end()) return found->second;
   bool wrapped = false;
   for (unsigned i = 0; i < type->length; ++i) {
      const auto* field = type->fields.structure[i].type->without_array();
      if (field->is_matrix()) wrapped = true;
      else if (field->is_struct() && shared_struct_type(ctx, field)) wrapped = true;
   }
   const char* name = wrapped ? ralloc_asprintf(ctx.mem_ctx, "_xlat_shared_struct%zu", ctx.sharedStructTypes.size()) : nullptr;
   ctx.sharedStructTypes[type] = name;
   if (!wrapped) return nullptr;

   ensure_shared_matrix_template(ctx);
   auto& out = ctx.typedeclStr;
   out.asprintf_append("struct %s {\n", name);
   for (unsigned i = 0; i < type->length; ++i) {
      const auto& field = type->fields.structure[i];
      out.asprintf_append("  ");
      print_shared_type(ctx, out, field.type, field.precision);
      out.asprintf_append(" %s", field.name);
      print_type_post(out, field.type, false);
      out.asprintf_append(";\n");
   }
   out.asprintf_append("  %s read() const threadgroup {\n    %s _value;\n", type->name, type->name);
   for (unsigned i = 0; i < type->length; ++i) {
      const auto& field = type->fields.structure[i];
      copy_shared_field(ctx, field.type, ralloc_asprintf(ctx.mem_ctx, "_value.%s", field.name),
                        ralloc_asprintf(ctx.mem_ctx, "this->%s", field.name), false, 0);
   }
   out.asprintf_append("    return _value;\n  }\n  void write(%s _value) threadgroup {\n", type->name);
   for (unsigned i = 0; i < type->length; ++i) {
      const auto& field = type->fields.structure[i];
      copy_shared_field(ctx, field.type, ralloc_asprintf(ctx.mem_ctx, "this->%s", field.name),
                        ralloc_asprintf(ctx.mem_ctx, "_value.%s", field.name), true, 0);
   }
   out.asprintf_append("  }\n};\n");
   return name;
}

static void print_shared_type(metal_print_context& ctx, string_buffer& out, const glsl_type* type, unsigned precision)
{
   type = type->without_array();
   if (type->is_matrix()) {
      ensure_shared_matrix_template(ctx);
      out.asprintf_append("_xlat_shared_matrix<");
      print_type_precision(out, type, precision, false);
      out.asprintf_append(", ");
      print_type_precision(out, type->column_type(), precision, false);
      out.asprintf_append(", %u>", type->matrix_columns);
   } else if (const char* name = type->is_struct() ? shared_struct_type(ctx, type) : nullptr) {
      out.asprintf_append("%s", name);
   } else {
      print_type_precision(out, type, precision, false);
   }
}

class metal_struct_declarations : public ir_hierarchical_visitor {
public:
   string_buffer& out;
   std::unordered_set<const glsl_type*> emitted;
   metal_struct_declarations(string_buffer& out) : out(out) {}
   void emit(const glsl_type* type) {
      type = type->without_array();
      if ((!type->is_struct() && !type->is_interface()) || !strncmp(type->name, "gl_", 3) || !emitted.insert(type).second) return;
      for (unsigned i = 0; i < type->length; ++i) emit(type->fields.structure[i].type);
      if (type->is_interface()) return;
      out.asprintf_append("struct %s {\n", type->name);
      for (unsigned i = 0; i < type->length; ++i) {
         const glsl_struct_field& f = type->fields.structure[i];
         out.asprintf_append("  "); print_type_precision(out, f.type, f.precision, false);
         out.asprintf_append(" %s", f.name); print_type_post(out, f.type, false);
         out.asprintf_append(";\n");
      }
      out.asprintf_append("};\n");
   }
   ir_visitor_status visit(ir_variable* v) override { emit(v->type); if ((v->data.mode == ir_var_uniform || v->data.mode == ir_var_shader_storage) && v->get_interface_type()) emit(v->get_interface_type()); return visit_continue; }
   ir_visitor_status visit(ir_constant* c) override { emit(c->type); return visit_continue; }
   ir_visitor_status visit_enter(ir_function_signature* f) override { emit(f->return_type); return visit_continue; }
};
static void print_metal_structs(string_buffer& out, exec_list* ir)
{
   metal_struct_declarations visitor(out); visit_list_elements(&visitor, ir);
}

static void print_interpolation(string_buffer& out, unsigned mode, bool centroid, bool sample)
{
   if (mode == INTERP_MODE_FLAT)
      out.asprintf_append(" [[flat]]");
   else if (centroid || sample || mode != INTERP_MODE_NONE)
      out.asprintf_append(" [[%s_%s]]", sample ? "sample" : centroid ? "centroid" : "center",
                          mode == INTERP_MODE_NOPERSPECTIVE ? "no_perspective" : "perspective");
}

// Metal stage interfaces carry scalar/vector fields. Keep the GLSL aggregate
// in the entry point so array indexing and member accesses use its original type.
static void emit_stage_fields(metal_print_context& ctx, const glsl_type* type, bool input,
                              const char* local, const char* key, int location, unsigned precision,
                              unsigned interpolation, bool centroid, bool sample)
{
   if (type->is_struct() || type->is_interface()) {
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         bool block = type->is_interface();
         int fieldLocation = block ? (f.location < 0 ? -1 : f.location - VARYING_SLOT_VAR0 + location) : location;
         emit_stage_fields(ctx, f.type, input, ralloc_asprintf(ctx.mem_ctx, "%s.%s", local, f.name),
                           ralloc_asprintf(ctx.mem_ctx, "%s_%u", key, i), fieldLocation, f.precision,
                           block ? f.interpolation : interpolation, block ? f.centroid : centroid, block ? f.sample : sample);
         if (!block && location >= 0) location += f.type->count_attribute_slots(false);
      }
      return;
   }
   if (type->is_array() || type->is_matrix()) {
      const glsl_type* element = type->is_array() ? type->fields.array : type->column_type();
      unsigned count = type->is_array() ? type->length : type->matrix_columns;
      for (unsigned i = 0; i < count; ++i) {
         emit_stage_fields(ctx, element, input, ralloc_asprintf(ctx.mem_ctx, "%s[%u]", local, i),
                           ralloc_asprintf(ctx.mem_ctx, "%s_%u", key, i), location, precision, interpolation, centroid, sample);
         if (location >= 0) location += element->count_attribute_slots(false);
      }
      return;
   }

   unsigned index = ctx.stageFieldCount++;
   auto& fields = input ? ctx.inputStr : ctx.outputStr;
   fields.asprintf_append("  ");
   print_type_precision(fields, type, precision, false);
   fields.asprintf_append(" _xlat_varying%u [[user(", index);
   if (location >= 0) fields.asprintf_append("locn%d", location);
   else fields.asprintf_append("%s", key);
   fields.asprintf_append(")]]");
   print_interpolation(fields, type->is_integer_32() ? INTERP_MODE_FLAT : interpolation, centroid, sample);
   fields.asprintf_append(";\n");
   if (input) ctx.stageLocals.asprintf_append("  %s = _mtl_i._xlat_varying%u;\n", local, index);
   else ctx.stageExports.asprintf_append("  _mtl_o._xlat_varying%u = %s;\n", index, local);
}

static void prepare_stage_blocks(metal_print_context& ctx, exec_list* ir)
{
   foreach_in_list(ir_instruction, instruction, ir) {
      auto* var = instruction->as_variable();
      if (!var || (var->data.mode != ir_var_shader_in && var->data.mode != ir_var_shader_out)) continue;
      const auto* block = var->get_interface_type();
      if (!block || !strncmp(block->name, "gl_", 3)) continue;
      bool input = var->data.mode == ir_var_shader_in;
      unsigned index = (unsigned)ctx.stageNames.size();
      const char* local = ralloc_asprintf(ctx.mem_ctx, "_xlat_stage%u", index);
      const char* key = ralloc_asprintf(ctx.mem_ctx, "_xlat_block%zu_%s", strlen(block->name), block->name);
      ctx.stageNames[var] = local;
      ctx.stageLocals.asprintf_append("  ");
      if (var->is_interface_instance()) {
         const char* name = ralloc_asprintf(ctx.mem_ctx, "_xlat_stage_type%u", index);
         ctx.typedeclStr.asprintf_append("struct %s {\n", name);
         for (unsigned i = 0; i < block->length; ++i) {
            const auto& f = block->fields.structure[i];
            ctx.typedeclStr.asprintf_append("  ");
            print_type_precision(ctx.typedeclStr, f.type, f.precision, false);
            ctx.typedeclStr.asprintf_append(" %s", f.name);
            print_type_post(ctx.typedeclStr, f.type, false);
            ctx.typedeclStr.asprintf_append(";\n");
         }
         ctx.typedeclStr.asprintf_append("};\n");
         ctx.stageLocals.asprintf_append("%s", name);
      } else {
         print_type_precision(ctx.stageLocals, var->type, var->data.precision, false);
      }
      ctx.stageLocals.asprintf_append(" %s", local);
      print_type_post(ctx.stageLocals, var->type, false);
      ctx.stageLocals.asprintf_append(input ? ";\n" : " = {};\n");

      if (var->is_interface_instance()) {
         emit_stage_fields(ctx, var->type, input, local, key, 0, 0, 0, false, false);
      } else {
         for (unsigned i = 0; i < block->length; ++i) {
            const auto& f = block->fields.structure[i];
            if (strcmp(f.name, var->name)) continue;
            emit_stage_fields(ctx, var->type, input, local, ralloc_asprintf(ctx.mem_ctx, "%s_%u", key, i),
                              f.location < 0 ? -1 : f.location - VARYING_SLOT_VAR0,
                              var->data.precision, f.interpolation, f.centroid, f.sample);
            break;
         }
      }
   }
}

// Storage types follow the buffer layout independently of expression precision.
// Packed vectors allow a scalar to follow a vec3 at byte offset 12.
static const char* uniform_storage_type(metal_print_context& ctx, const glsl_type* type, bool row, unsigned storage);

// Reuse value types only when their representation and alignment match storage.
// Excluding padding also keeps aggregate stores from overwriting padding bytes.
static bool native_buffer_type(metal_print_context& ctx, const glsl_type* type, bool row, unsigned kind)
{
   // Coherent aggregate copies use explicit member accesses. Native matrix
   // operators do not accept coherent references.
   if (metal_print_context::is_coherent(kind)) return false;
   auto& cache = ctx.nativeBufferTypes[kind][row];
   auto found = cache.find(type);
   if (found != cache.end()) return found->second;
   bool std430 = metal_print_context::is_std430(kind);
   bool match = false;
   if (type->is_scalar() || type->is_vector()) {
      match = !type->is_boolean() && type->vector_elements != 3;
   } else if (type->is_matrix()) {
      match = !row && (type->vector_elements == 4 || (std430 && type->vector_elements == 2));
   } else if (type->is_struct()) {
      unsigned cursor = 0, alignment = 1;
      match = true;
      for (unsigned i = 0; i < type->length && match; ++i) {
         const auto& f = type->fields.structure[i];
         bool fieldRow = buffer_field_row_major(f, row);
         match = f.precision != GLSL_PRECISION_MEDIUM && native_buffer_type(ctx, f.type, fieldRow, kind) &&
                 buffer_field_offset(type, i, cursor, row, std430) == cursor;
         alignment = MAX2(alignment, buffer_alignment(f.type, fieldRow, std430));
         cursor += buffer_size(f.type, fieldRow, std430);
      }
      match = match && alignment == buffer_alignment(type, row, std430) &&
              cursor == buffer_size(type, row, std430);
   }
   cache[type] = match;
   return match;
}

static bool native_buffer_value(metal_print_context& ctx, const glsl_type* type, bool row, unsigned precision, unsigned kind)
{
   return (type->is_struct() || precision != GLSL_PRECISION_MEDIUM) && native_buffer_type(ctx, type, row, kind);
}

template<typename PrintStorage>
static void print_uniform_value(metal_print_context& ctx, string_buffer& out,
                                const glsl_type* type, bool row, unsigned precision, unsigned kind, const PrintStorage& storage)
{
   if ((type->is_struct() || type->is_matrix()) && native_buffer_value(ctx, type, row, precision, kind)) {
      storage();
   } else if (type->is_struct()) {
      out.asprintf_append("%s_read(", uniform_storage_type(ctx, type, row, kind));
      storage();
      out.asprintf_append(")");
   } else if (type->is_matrix()) {
      unsigned columns = row ? type->vector_elements : type->matrix_columns;
      unsigned rows = row ? type->matrix_columns : type->vector_elements;
      const char* scalar = precision == GLSL_PRECISION_MEDIUM ? "half" : "float";
      if (native_buffer_type(ctx, type, row, kind)) {
         out.asprintf_append("%s%ux%u(", scalar, columns, rows);
         storage(); out.asprintf_append(")");
         return;
      }
      if (row) out.asprintf_append("transpose(");
      out.asprintf_append("%s%ux%u(", scalar, columns, rows);
      for (unsigned c = 0; c < columns; ++c) {
         if (c) out.asprintf_append(", ");
         out.asprintf_append("%s%u(", scalar, rows);
         storage();
         out.asprintf_append(".vectors[%u].%.*s)", c, rows, "xyzw");
      }
      out.asprintf_append(row ? "))" : ")");
   } else {
      print_type_precision(out, type, precision, false);
      out.asprintf_append("(");
      storage();
      out.asprintf_append(")");
   }
}

// Structs with different storage and value representations require conversion.
// Field and array-element reads do not copy their parents.
static void copy_uniform_value(metal_print_context& ctx, string_buffer& out,
                               const glsl_type* type, bool row, unsigned precision,
                               const char* dst, const char* src, unsigned depth, unsigned kind)
{
   if (type->is_array()) {
      out.asprintf_append("%*sfor (uint _buf_i%u = 0; _buf_i%u < %u; ++_buf_i%u) {\n", 2 + depth * 2, "", depth, depth, type->length, depth);
      copy_uniform_value(ctx, out, type->fields.array, row, precision,
                         ralloc_asprintf(ctx.mem_ctx, "%s[_buf_i%u]", dst, depth),
                         ralloc_asprintf(ctx.mem_ctx, "%s.elements[_buf_i%u].value", src, depth), depth + 1, kind);
      out.asprintf_append("%*s}\n", 2 + depth * 2, "");
   } else {
      out.asprintf_append("%*s%s = ", 2 + depth * 2, "", dst);
      print_uniform_value(ctx, out, type, row, precision, kind, [&]() { out.asprintf_append("%s", src); });
      out.asprintf_append(";\n");
   }
}

enum class aggregate_source { value, buffer, shared };

// Write only value components, leaving layout padding untouched.
static void store_buffer_value(metal_print_context& ctx, string_buffer& out,
                               const glsl_type* type, bool row, const char* dst, const char* src, unsigned depth, unsigned kind,
                               aggregate_source source, bool sourceRow, unsigned sourceKind, unsigned sourcePrecision)
{
   bool native = native_buffer_type(ctx, type, row, kind);
   bool sourceStorage = source == aggregate_source::buffer;
   bool sourceShared = source == aggregate_source::shared;
   bool sourceNative = sourceShared ? !shared_storage_is_wrapped(ctx, type) :
      !sourceStorage || native_buffer_type(ctx, type, sourceRow, sourceKind);
   if ((type->is_struct() || type->is_matrix()) && native && sourceNative &&
       (!type->is_matrix() || !sourceStorage || sourcePrecision != GLSL_PRECISION_MEDIUM)) {
      out.asprintf_append("%*s%s = ", depth * 2, "", dst);
      if (type->is_matrix()) out.asprintf_append("float%ux%u(%s)", type->matrix_columns, type->vector_elements, src);
      else out.asprintf_append("%s", src);
      out.asprintf_append(";\n");
   } else if (type->is_struct()) {
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         store_buffer_value(ctx, out, f.type, buffer_field_row_major(f, row),
                            ralloc_asprintf(ctx.mem_ctx, "%s.%s", dst, f.name),
                            ralloc_asprintf(ctx.mem_ctx, "%s.%s", src, f.name), depth, kind,
                            source, buffer_field_row_major(f, sourceRow), sourceKind, f.precision);
      }
   } else if (type->is_array()) {
      out.asprintf_append("%*sfor (uint _buf_i%u = 0; _buf_i%u < %u; ++_buf_i%u) {\n", depth * 2, "", depth, depth, type->length, depth);
      store_buffer_value(ctx, out, type->fields.array, row,
                         ralloc_asprintf(ctx.mem_ctx, "%s.elements[_buf_i%u].value", dst, depth),
                         ralloc_asprintf(ctx.mem_ctx, sourceStorage ? "%s.elements[_buf_i%u].value" : "%s[_buf_i%u]", src, depth),
                         depth + 1, kind, source, sourceRow, sourceKind, sourcePrecision);
      out.asprintf_append("%*s}\n", depth * 2, "");
   } else if (type->is_matrix()) {
      unsigned count = row ? type->vector_elements : type->matrix_columns;
      unsigned width = row ? type->matrix_columns : type->vector_elements;
      for (unsigned i = 0; i < count; ++i) {
         out.asprintf_append("%*s%s", depth * 2, "", dst);
         if (native) out.asprintf_append("[%u]", i);
         else out.asprintf_append(".vectors[%u].%.*s", i, width, "xyzw");
         out.asprintf_append(" = float%u(", width);
         if (sourceShared && !row) {
            out.asprintf_append("%s.columns[%u]);\n", src, i);
            continue;
         }
         for (unsigned j = 0; j < width; ++j) {
            if (j) out.asprintf_append(", ");
            bool half = sourceStorage && sourcePrecision == GLSL_PRECISION_MEDIUM;
            if (half) out.asprintf_append("half(");
            unsigned c = row ? j : i, r = row ? i : j;
            if (sourceShared) out.asprintf_append("%s.columns[%u][%u]", src, c, r);
            else if (sourceNative) out.asprintf_append("%s[%u][%u]", src, c, r);
            else out.asprintf_append("%s.vectors[%u][%u]", src, sourceRow ? r : c, sourceRow ? c : r);
            if (half) out.asprintf_append(")");
         }
         out.asprintf_append(");\n");
      }
   } else {
      out.asprintf_append("%*s%s = ", depth * 2, "", dst);
      const glsl_type* valueType = type->is_boolean() ? glsl_type::get_instance(GLSL_TYPE_UINT, type->vector_elements, 1) : type;
      print_type_precision(out, valueType, GLSL_PRECISION_HIGH, false);
      out.asprintf_append("(");
      if (sourceStorage && (type->is_boolean() || sourcePrecision == GLSL_PRECISION_MEDIUM))
         print_uniform_value(ctx, out, type, sourceRow, sourcePrecision, sourceKind, [&]() { out.asprintf_append("%s", src); });
      else out.asprintf_append("%s", src);
      out.asprintf_append(");\n");
   }
}

static const char* uniform_storage_type(metal_print_context& ctx, const glsl_type* type, bool row, unsigned kind)
{
   bool std430 = metal_print_context::is_std430(kind);
   if (type->is_scalar() || type->is_vector()) {
      static const char* names[3][4] = {
         { "float", "float2", "packed_float3", "float4" },
         { "int", "int2", "packed_int3", "int4" },
         { "uint", "uint2", "packed_uint3", "uint4" }
      };
      unsigned scalar = type->is_float() ? 0 : type->base_type == GLSL_TYPE_INT ? 1 : 2;
      return names[scalar][type->vector_elements - 1];
   }
   auto& types = ctx.uniformStorageTypes[kind][row];
   auto found = types.find(type);
   if (found != types.end()) return ctx.uniformStorageNames[found->second];
   unsigned id = (unsigned)ctx.uniformStorageNames.size();
   bool native = native_buffer_type(ctx, type, row, kind);
   const char* name = native ? (type->is_struct() ? type->name :
      ralloc_asprintf(ctx.mem_ctx, "float%ux%u", type->matrix_columns, type->vector_elements)) :
      ralloc_asprintf(ctx.mem_ctx, "_xlat_buf_storage%u", id);
   types[type] = id;
   ctx.uniformStorageNames.push_back(name);
   auto& out = ctx.typedeclStr;
   unsigned size = type->is_interface() ? buffer_block_size(type, std430) : buffer_size(type, row, std430);
   if (native) {
      out.asprintf_append("static_assert(sizeof(%s) == %u, \"buffer size\");\n", name, size);
      return name;
   }
   if (type->is_array()) {
      auto element = type->fields.array;
      const char* storage = uniform_storage_type(ctx, element, row, kind);
      unsigned align = MAX2(std430 ? 1 : 16, buffer_alignment(element, row, std430));
      out.asprintf_append("struct alignas(%u) %s_element { %s value; };\n", align, name, storage);
      if (!type->length) {
         out.asprintf_append("static_assert(sizeof(%s_element) == %u, \"array stride\");\n", name, buffer_array_stride(element, row, std430));
         return name;
      }
      out.asprintf_append("struct %s { %s_element elements[%u]; };\n", name, name, type->length);
   } else if (type->is_matrix()) {
      unsigned width = row ? type->matrix_columns : type->vector_elements;
      out.asprintf_append("struct %s { float%u vectors[%u]; };\n", name, std430 && width == 2 ? 2 : 4, row ? type->vector_elements : type->matrix_columns);
   } else {
      string_to_uint_map memberNames;
      std::vector<const char*> fields;
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         memberNames.put(0, f.name);
         fields.push_back(uniform_storage_type(ctx, f.type, buffer_field_row_major(f, row), kind));
      }
      out.asprintf_append("struct alignas(%u) %s {\n", buffer_alignment(type, row, std430), name);
      unsigned cursor = 0, nextPadding = 0;
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         bool fieldRow = buffer_field_row_major(f, row);
         unsigned offset = buffer_field_offset(type, i, cursor, row, std430);
         if (offset > cursor) {
            unsigned suffix = MAX2(i, nextPadding), unused;
            const char* padding;
            do {
               padding = ralloc_asprintf(ctx.mem_ctx, "_buf_pad%u", suffix++);
            } while (memberNames.get(unused, padding));
            nextPadding = suffix;
            memberNames.put(0, padding);
            out.asprintf_append("  char %s[%u];\n", padding, offset - cursor);
         }
         out.asprintf_append("  alignas(%u) %s %s;\n", buffer_alignment(f.type, fieldRow, std430), fields[i], f.name);
         cursor = offset + buffer_size(f.type, fieldRow, std430);
      }
      out.asprintf_append("};\n");
   }
   out.asprintf_append("static_assert(sizeof(%s) == %u, \"%s size\");\n", name, size, std430 ? "std430" : "std140");
   if (type->is_struct()) {
      out.asprintf_append("inline __attribute__((always_inline)) %s %s_read(%s %s& storage)\n{\n  %s value;\n", type->name, name, metal_print_context::buffer_address(kind, true), name, type->name);
      for (unsigned i = 0; i < type->length; ++i) {
         const auto& f = type->fields.structure[i];
         copy_uniform_value(ctx, out, f.type, buffer_field_row_major(f, row), f.precision,
                            ralloc_asprintf(ctx.mem_ctx, "value.%s", f.name), ralloc_asprintf(ctx.mem_ctx, "storage.%s", f.name), 0, kind);
      }
      out.asprintf_append("  return value;\n}\n");
   }
   return name;
}

static void prepare_buffer_blocks(metal_print_context& ctx, exec_list* ir)
{
   auto blocks = collect_buffer_blocks(ir, true);
   bool coherentGuard = false;
   for (unsigned i = 0; i < blocks.size(); ++i) {
      const auto& b = blocks[i];
      unsigned kind = b.variable->data.mode == ir_var_shader_storage ?
         (buffer_uses_std430(b, true) ? metal_print_context::storage_std430 : metal_print_context::storage_std140) : metal_print_context::uniform_std140;
      bool readOnly = true;
      bool coherent = false;
      for (unsigned f = 0; f < b.type->length; ++f) {
         const auto& field = b.type->fields.structure[f];
         readOnly &= field.memory_read_only;
         coherent |= field.memory_coherent;
      }
      // Metal qualifies the buffer address. A block with a coherent member
      // uses device coherence for every access through that address.
      if (kind != metal_print_context::uniform_std140 && coherent) {
         kind = metal_print_context::is_std430(kind) ? metal_print_context::coherent_std430 : metal_print_context::coherent_std140;
         if (!coherentGuard) {
            coherentGuard = true;
            ctx.prefixStr.asprintf_append(
               "#if __METAL_VERSION__ < 320\n"
               "#error Coherent storage buffers require Metal 3.2 or newer\n"
               "#endif\n");
         }
      }
      const char* address = metal_print_context::buffer_address(kind, readOnly);
      bool runtime = buffer_has_runtime_array(b.type);
      bool row = b.type->get_interface_row_major();
      metal_print_context::runtime_field* fields = nullptr;
      const char* storage = "uchar";
      if (runtime) {
         fields = ralloc_array(ctx.mem_ctx, metal_print_context::runtime_field, b.type->length);
         unsigned cursor = 0;
         bool std430 = metal_print_context::is_std430(kind);
         for (unsigned f = 0; f < b.type->length; ++f) {
            const auto& field = b.type->fields.structure[f];
            bool fieldRow = buffer_field_row_major(field, row);
            cursor = buffer_field_offset(b.type, f, cursor, row, std430);
            fields[f] = {uniform_storage_type(ctx, field.type, fieldRow, kind), cursor,
                         field.type->is_unsized_array() ? buffer_array_stride(field.type->fields.array, fieldRow, std430) : 0};
            cursor += buffer_size(field.type, fieldRow, std430);
         }
      } else {
         storage = uniform_storage_type(ctx, b.type, row, kind);
      }
      bool array = b.variable->is_interface_instance() && b.variable->type->is_array();
      if (array) {
         ctx.blockAliases.asprintf_append("  %s %s* _mtl_b%u", address, storage, i);
         print_type_post(ctx.blockAliases, b.variable->type, false);
         ctx.blockAliases.asprintf_append(" = {");
      }
      for (unsigned element = 0; element < b.array_size; ++element) {
         unsigned slot = b.metal_index + element;
         ctx.paramsStr.asprintf_append("\n  , %s %s%s _mtl_buffer%u [[buffer(%u)]]", address, storage, runtime ? "*" : "&", slot, slot);
         if (array) ctx.blockAliases.asprintf_append("%s%s_mtl_buffer%u", element ? ", " : "", runtime ? "" : "&", slot);
      }
      if (array) ctx.blockAliases.asprintf_append("};\n");
      if (array && b.needs_size) {
         ctx.blockAliases.asprintf_append("  const uint _mtl_bs%u", i);
         print_type_post(ctx.blockAliases, b.variable->type, false);
         ctx.blockAliases.asprintf_append(" = {");
         for (unsigned element = 0; element < b.array_size; ++element)
            ctx.blockAliases.asprintf_append("%s%uu", element ? ", " : "", b.metal_index + element);
         ctx.blockAliases.asprintf_append("};\n");
      }
      string_to_uint_map memberRows;
      if (!b.variable->is_interface_instance()) {
         for (unsigned f = 0; f < b.type->length; ++f)
            memberRows.put(f, b.type->fields.structure[f].name);
      }
      foreach_in_list(ir_instruction, instruction, ir) {
         auto v = instruction->as_variable();
         if (v && (v->data.mode == ir_var_uniform || v->data.mode == ir_var_shader_storage) && v->get_interface_type() == b.type) {
            bool memberRow = row;
            unsigned member = 0;
            if (!v->is_interface_instance()) {
               bool found = memberRows.get(member, v->name);
               assert(found);
               if (!found) continue;
               memberRow = buffer_field_row_major(b.type->fields.structure[member], row);
            }
            ctx.blockNames[v] = {array ? i : b.metal_index, array, kind, memberRow, fields, member, address};
         }
      }
   }
   unsigned sizes = buffer_size_table_index(blocks);
   if (sizes) ctx.paramsStr.asprintf_append("\n  , constant uint* _mtl_buffer_sizes [[buffer(%u)]]", sizes);
   for (const auto& counter : collect_atomic_counter_buffers(ir)) {
      ctx.paramsStr.asprintf_append("\n  , %sdevice atomic_uint* _mtl_atomic%u [[buffer(%u)]]", ctx.countersReadOnly ? "const " : "", counter.metal_index, counter.metal_index);
      for (auto* v : counter.variables) ctx.counterSlots[v] = counter.metal_index;
   }
}

static void get_metal_type_size(const glsl_type* type, unsigned prec, int& size, int& alignment)
{
   if (type->is_array()) {
      get_metal_type_size(type->fields.array, prec, size, alignment);
      size = ((size + alignment - 1) & ~(alignment - 1)) * type->length;
      return;
   }
   if (type->is_struct()) {
      size = 0; alignment = 1;
      for (unsigned i = 0; i < type->length; ++i) {
         int field_size, field_align;
         get_metal_type_size(type->fields.structure[i].type, type->fields.structure[i].precision, field_size, field_align);
         size = ((size + field_align - 1) & ~(field_align - 1)) + field_size;
         alignment = MAX2(alignment, field_align);
      }
      size = (size + alignment - 1) & ~(alignment - 1);
      return;
   }
   int scalar = type->is_boolean() ? 1 : prec == GLSL_PRECISION_MEDIUM ? 2 : 4;
   int vector = type->vector_elements == 3 ? 4 : type->vector_elements;
   alignment = scalar * vector;
   size = alignment * type->matrix_columns;
}

void ir_print_metal_visitor::print_array_initializer(ir_constant* value, unsigned precision)
{
   if (value->type->is_array()) {
      buffer.asprintf_append("{");
      for (unsigned i = 0; i < value->type->length; ++i) {
         if (i) buffer.asprintf_append(", ");
         print_array_initializer(value->get_array_element(i), precision);
      }
      buffer.asprintf_append("}");
   } else {
      const bool cast = value->type->is_numeric();
      if (cast) { print_type_precision(buffer, value->type, precision, true); buffer.asprintf_append("("); }
      value->accept(this);
      if (cast) buffer.asprintf_append(")");
   }
}

void ir_print_metal_visitor::visit(ir_variable *ir)
{
   const char* storage = (ir->data.mode == ir_var_function_out || ir->data.mode == ir_var_function_inout) ? "thread " : "";
   if (ir->data.mode == ir_var_shader_shared) storage = "threadgroup ";

   // give an id to any variable defined in a function that is not an uniform
   if ((this->mode == kPrintGlslNone && ir->data.mode != ir_var_uniform))
   {
      long id = globals->var_hash[ir];
      if (id == 0)
      {
         id = ++globals->var_counter;
         globals->var_hash[ir] = id;
      }
   }

   if (loop_initializers && loop_initializers->omitted.count(ir)) {
      skipped_this_ir = true;
      return;
   }

   // Global temporaries and shared declarations are emitted inside main.
   if (this->mode != kPrintGlslNone && (ir->data.mode == ir_var_auto || ir->data.mode == ir_var_temporary || ir->data.mode == ir_var_shader_shared))
   {
      assert (!this->globals->main_function_done);
      this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
      skipped_this_ir = true;
      return;
   }

   buffer.asprintf_append ("%s", storage);
   if (!strcmp(ir->name, "gl_VertexID") || !strcmp(ir->name, "gl_InstanceID"))
      buffer.asprintf_append("uint");
   else if (!strcmp(ir->name, "gl_PointCoord"))
      buffer.asprintf_append("float2");
   else if (ir->data.mode == ir_var_shader_shared)
      print_shared_type(ctx, buffer, ir->type, ir->data.precision);
   else
      print_type(buffer, ir, ir->type, false);
   buffer.asprintf_append (" ");
   if (ir->data.mode == ir_var_function_out || ir->data.mode == ir_var_function_inout) buffer.asprintf_append("&");
   print_var_name (ir);
   print_type_post(buffer, ir->type, false);

   // special built-in variables
   if (!strcmp(ir->name, "gl_FragDepth"))
      buffer.asprintf_append (" [[depth(any)]]");
   else if (!strcmp(ir->name, "gl_FragCoord"))
      buffer.asprintf_append (" [[position]]");
   else if (!strcmp(ir->name, "gl_FrontFacing"))
      buffer.asprintf_append (" [[front_facing]]");
   else if (!strcmp(ir->name, "gl_PointCoord"))
      buffer.asprintf_append (" [[point_coord]]");
   else if (!strcmp(ir->name, "gl_PointSize"))
      buffer.asprintf_append (" [[point_size]]");
   else if (!strcmp(ir->name, "gl_Position"))
      buffer.asprintf_append (" [[position]]");
   else if (!strcmp(ir->name, "gl_VertexID"))
      buffer.asprintf_append (" [[vertex_id]]");
   else if (!strcmp(ir->name, "gl_InstanceID"))
      buffer.asprintf_append (" [[instance_id]]");
   else if (!strcmp(ir->name, "gl_GlobalInvocationID"))
      buffer.asprintf_append(" [[thread_position_in_grid]]");
   else if (!strcmp(ir->name, "gl_LocalInvocationID"))
      buffer.asprintf_append(" [[thread_position_in_threadgroup]]");
   else if (!strcmp(ir->name, "gl_LocalInvocationIndex"))
      buffer.asprintf_append(" [[thread_index_in_threadgroup]]");
   else if (!strcmp(ir->name, "gl_WorkGroupID"))
      buffer.asprintf_append(" [[threadgroup_position_in_grid]]");
   else if (!strcmp(ir->name, "gl_NumWorkGroups"))
      buffer.asprintf_append(" [[threadgroups_per_grid]]");

   // Interpolation belongs to fragment inputs / vertex outputs, not attributes
   // or render targets. Metal combines interpolation and sampling location.
   const bool varying = (mode_whole == kPrintGlslFragment && ir->data.mode == ir_var_shader_in) ||
                        (mode_whole == kPrintGlslVertex && ir->data.mode == ir_var_shader_out);
   if (varying && strncmp(ir->name, "gl_", 3) != 0) {
      if (ir->data.explicit_location)
         buffer.asprintf_append(" [[user(locn%d)]]", ir->data.location - VARYING_SLOT_VAR0);
      print_interpolation(buffer, ir->data.interpolation, ir->data.centroid, ir->data.sample);
   }
   // Metal supports invariance on the position output only.
   if (mode_whole == kPrintGlslVertex && !strcmp(ir->name, "gl_Position") && ir->data.invariant)
      buffer.asprintf_append(" [[invariant]]");

   // vertex shader input attribute?
   if (this->mode_whole == kPrintGlslVertex && ir->data.mode == ir_var_shader_in)
   {
      int location = ir->data.location - VERT_ATTRIB_GENERIC0;
      if (!ir->data.explicit_location) {
         const int slots = ir->type->count_attribute_slots(true);
         for (int slot = ctx.attributeCounter; slot < ctx.attributeCounter + slots; ++slot)
            if (ctx.explicitAttributeSlots.count(slot))
               ctx.attributeCounter = slot + 1;
         location = ctx.attributeCounter;
         ctx.attributeCounter += slots;
      }
      buffer.asprintf_append(" [[attribute(%d)]]", location);
      if (ctx.reflectAttributeLocations) {
         ir->data.explicit_location = true;
         ir->data.location = location;
      }
   }

   // fragment shader output?
   if (this->mode_whole == kPrintGlslFragment && ir->data.mode == ir_var_shader_out)
   {
      if (!ir->data.explicit_location)
      {
         ir->data.explicit_location = 1;
         ir->data.location = FRAG_RESULT_DATA0 + ctx.colorCounter;
         ++ctx.colorCounter;
      }

      if (ir->data.explicit_location)
      {
         const int binding_base = (int)FRAG_RESULT_DATA0;
         const int location = ir->data.location - binding_base;
         if (location >= 0 && !ir->type->is_array())
            buffer.asprintf_append (" [[color(%d)]]", location);
      }
   }

   // uniform texture?
   if (ir->data.mode == ir_var_uniform && ctx.writingParams)
   {
      bool image = ir->type->without_array()->is_image();
      int index = image ? ctx.imageCounter : ctx.textureCounter++;
      if (image) ctx.imageCounter += ir->type->is_array() ? ir->type->length : 1;
      buffer.asprintf_append (" [[texture(%i)]]", index);
      if (!image)
         buffer.asprintf_append (", sampler _mtlsmp_%s [[sampler(%i)]]", ir->name, index);
      ir->data.explicit_location = 1;
      ir->data.location = index;
   }
   // regular uniform?
   if (ir->data.mode == ir_var_uniform && !ctx.writingParams)
   {
      int size, align;
      get_metal_type_size(ir->type, (unsigned)ir->data.precision, size, align);

      int loc = ctx.uniformLocationCounter;
      loc = (loc + align-1) & ~(align-1); // align it

      ir->data.explicit_location = 1;
      ir->data.location = loc;

      loc += size;
      ctx.uniformLocationCounter = loc;
   }

   if (ir->constant_value &&
       ir->data.mode != ir_var_shader_in &&
       ir->data.mode != ir_var_shader_out &&
       ir->data.mode != ir_var_function_in &&
       ir->data.mode != ir_var_function_out &&
       ir->data.mode != ir_var_function_inout)
   {
      buffer.asprintf_append (" = ");
      if (ir->type->is_array()) print_array_initializer(ir->constant_value, ir->data.precision);
      else visit (ir->constant_value);
   }

   if (!ir->constant_value && (ir->data.mode == ir_var_auto || ir->data.mode == ir_var_temporary) && !ir->type->is_array() && (ir->type->matrix_columns == 1)) {
      switch (ir->type->base_type) {
      case GLSL_TYPE_INT:
      case GLSL_TYPE_FLOAT:
         buffer.asprintf_append (" = 0");
         break;
      case GLSL_TYPE_BOOL:
         buffer.asprintf_append (" = false");
         break;
      default:
         break;
      }
   }
}


void ir_print_metal_visitor::visit(ir_function_signature *ir)
{
   const bool isMain = (strcmp(ir->function()->name, "main") == 0);
   inEntryPoint = isMain;

   if (!isMain)
   {
      print_type(buffer, ir, ir->return_type, true);
      buffer.asprintf_append (" %s (", ir->function_name());

      if (!ir->parameters.is_empty())
      {
         buffer.asprintf_append ("\n");

         indentation++; previous_skipped = false;
         bool first = true;
         foreach_in_list(ir_variable, inst, &ir->parameters)
         {
            if (!first)
               buffer.asprintf_append (",\n");
            indent();
            inst->accept(this);
            first = false;
         }
         indentation--;

         buffer.asprintf_append ("\n");
         indent();
      }
   }
   else
   {
      if (this->mode_whole == kPrintGlslFragment) {
         if (state->fs_early_fragment_tests) buffer.asprintf_append("[[early_fragment_tests]] ");
         buffer.asprintf_append ("fragment ");
      }
      if (this->mode_whole == kPrintGlslVertex)
         buffer.asprintf_append ("vertex ");
      if (this->mode_whole == kPrintGlslCompute) {
         buffer.asprintf_append("// threadsPerThreadgroup: %u, %u, %u\n", state->cs_input_local_size[0], state->cs_input_local_size[1], state->cs_input_local_size[2]);
         buffer.asprintf_append("kernel void xlatMtlMain (constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]");
      } else {
         buffer.asprintf_append ("xlatMtlShaderOutput xlatMtlMain (xlatMtlShaderInput _mtl_i [[stage_in]], constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]");
      }
      if (!ctx.paramsStr.empty())
      {
         buffer.asprintf_append ("%s", ctx.paramsStr.c_str());
      }
   }

   if (!ir->is_defined)
   {
      buffer.asprintf_append (");\n");
      return;
   }

   buffer.asprintf_append (")\n");

   indent();
   buffer.asprintf_append ("{\n");
   indentation++; previous_skipped = false;

   if (isMain)
   {
      buffer.asprintf_append("%s", ctx.blockAliases.c_str());
      // output struct
      if (this->mode_whole != kPrintGlslCompute) {
         indent(); buffer.asprintf_append ("xlatMtlShaderOutput _mtl_o;\n");
      }

      buffer.asprintf_append("%s", ctx.stageLocals.c_str());

      // insert postponed global assigments and variable declarations
      assert (!globals->main_function_done);
      globals->main_function_done = true;
      foreach_in_list(ga_entry_metal, node, &globals->global_assignements)
      {
         ir_instruction* as = node->ir;
         as->accept(this);
         buffer.asprintf_append(";\n");
      }
   }

   foreach_in_list(ir_instruction, inst, &ir->body) {
      indent();
      inst->accept(this);
      end_statement_line();
   }

   if (isMain && this->mode_whole != kPrintGlslCompute)
   {
      buffer.asprintf_append("%s", ctx.stageExports.c_str());
      indent(); buffer.asprintf_append ("return _mtl_o;\n");
   }

   indentation--;
   indent();
   buffer.asprintf_append ("}\n");
}

void ir_print_metal_visitor::visit(ir_function *ir)
{
   bool found_non_builtin_proto = false;

   foreach_in_list(ir_function_signature, sig, &ir->signatures) {
      if (!sig->is_builtin())
         found_non_builtin_proto = true;
   }
   if (!found_non_builtin_proto)
      return;

   PrintGlslMode oldMode = this->mode;
   this->mode = kPrintGlslNone;

   foreach_in_list(ir_function_signature, sig, &ir->signatures) {
      indent();
      sig->accept(this);
      buffer.asprintf_append ("\n");
   }

   this->mode = oldMode;

   indent();
}


static const char* metal_operator(ir_expression_operation op)
{
   switch (op) {
   case ir_unop_bit_not: return "~";
   case ir_unop_logic_not: return "!";
   case ir_unop_neg: return "-";
   case ir_unop_abs: return "abs";
   case ir_unop_sign: return "sign";
   case ir_unop_rcp: return "1.0/";
   case ir_unop_rsq: return "rsqrt";
   case ir_unop_sqrt: return "sqrt";
   case ir_unop_normalize: return "normalize";
   case ir_unop_exp: return "exp";
   case ir_unop_log: return "log";
   case ir_unop_exp2: return "exp2";
   case ir_unop_log2: return "log2";
   case ir_unop_f2i: return "int";
   case ir_unop_f2u: return "int";
   case ir_unop_i2f: return "float";
   case ir_unop_f2b: return "bool";
   case ir_unop_b2f: return "float";
   case ir_unop_i2b: return "bool";
   case ir_unop_b2i: return "int";
   case ir_unop_u2f: return "float";
   case ir_unop_i2u: return "int";
   case ir_unop_u2i: return "int";
   case ir_unop_bitcast_i2f: return "as_type_";
   case ir_unop_bitcast_f2i: return "as_type_";
   case ir_unop_bitcast_u2f: return "as_type_";
   case ir_unop_bitcast_f2u: return "as_type_";
   case ir_unop_trunc: return "trunc";
   case ir_unop_ceil: return "ceil";
   case ir_unop_floor: return "floor";
   case ir_unop_fract: return "fract";
   case ir_unop_round_even: return "rint";
   case ir_unop_sin: return "sin";
   case ir_unop_cos: return "cos";
   case ir_unop_dFdx: return "dfdx";
   case ir_unop_dFdx_coarse: return "dfdx";
   case ir_unop_dFdx_fine: return "dfdx";
   case ir_unop_dFdy: return "dfdy";
   case ir_unop_dFdy_coarse: return "dfdy";
   case ir_unop_dFdy_fine: return "dfdy";
   case ir_unop_pack_snorm_2x16: return "packSnorm2x16";
   case ir_unop_pack_snorm_4x8: return "packSnorm4x8";
   case ir_unop_pack_unorm_2x16: return "packUnorm2x16";
   case ir_unop_pack_unorm_4x8: return "packUnorm4x8";
   case ir_unop_pack_half_2x16: return "packHalf2x16";
   case ir_unop_unpack_snorm_2x16: return "unpackSnorm2x16";
   case ir_unop_unpack_snorm_4x8: return "unpackSnorm4x8";
   case ir_unop_unpack_unorm_2x16: return "unpackUnorm2x16";
   case ir_unop_unpack_unorm_4x8: return "unpackUnorm4x8";
   case ir_unop_unpack_half_2x16: return "unpackHalf2x16";
   case ir_unop_bitfield_reverse: return "reverse_bits";
   case ir_unop_bit_count: return "popcount";
   case ir_unop_find_msb: return "findMSB";
   case ir_unop_find_lsb: return "findLSB";
   case ir_unop_saturate: return "saturate";
   case ir_binop_add: return "+";
   case ir_binop_sub: return "-";
   case ir_binop_mul: return "*";
   case ir_binop_div: return "/";
   case ir_binop_mod: return "fmod";
   case ir_binop_less: return "<";
   case ir_binop_gequal: return ">=";
   case ir_binop_equal: return "==";
   case ir_binop_nequal: return "!=";
   case ir_binop_all_equal: return "==";
   case ir_binop_any_nequal: return "!=";
   case ir_binop_lshift: return "<<";
   case ir_binop_rshift: return ">>";
   case ir_binop_bit_and: return "&";
   case ir_binop_bit_xor: return "^";
   case ir_binop_bit_or: return "|";
   case ir_binop_logic_and: return "&&";
   case ir_binop_logic_xor: return "!=";
   case ir_binop_logic_or: return "||";
   case ir_binop_dot: return "dot";
   case ir_binop_min: return "min";
   case ir_binop_max: return "max";
   case ir_binop_pow: return "pow";
   case ir_triop_fma: return "fma";
   case ir_triop_lrp: return "mix";
   default: return ir_expression_operation_strings[op];
   }
}

static bool is_binop_func_like(ir_expression_operation op, const glsl_type* type)
{
   if ((op == ir_binop_mod && type->is_float()) ||
       (op >= ir_binop_dot && op <= ir_binop_pow))
      return true;
   return false;
}


static bool is_different_precision(unsigned a, unsigned b)
{
   // Treat "undefined" as high precision
   if (a == GLSL_PRECISION_NONE)
      a = GLSL_PRECISION_HIGH;
   if (b == GLSL_PRECISION_NONE)
      b = GLSL_PRECISION_HIGH;

   return a != b;
}


static void print_cast(string_buffer& buffer, unsigned prec, ir_rvalue* ir)
{
   buffer.asprintf_append ("(");
   print_type_precision(buffer, ir->type, prec, false);
   buffer.asprintf_append (")");
}


void ir_print_metal_visitor::visit(ir_expression *ir)
{
   ++this->expression_depth;
   newline_indent();

   if (ir->operation == ir_unop_ssbo_unsized_array_length) {
      auto* value = ir->operands[0];
      const auto& block = ctx.blockNames.at(value->variable_referenced());
      auto* record = value->as_dereference_record();
      const auto& field = block.fields[record ? record->field_idx : block.member_index];
      buffer.asprintf_append("int((max(_mtl_buffer_sizes[");
      print_buffer_size_index(record ? record->record : value);
      buffer.asprintf_append("], %uu) - %uu) / %uu)", field.offset, field.offset, field.stride);
      --this->expression_depth;
      return;
   }

   unsigned arg_prec = GLSL_PRECISION_NONE;
   if (ir->operands[0])
      arg_prec = higher_precision(arg_prec, ir->operands[0]->get_precision());
   if (ir->operands[1])
      arg_prec = higher_precision(arg_prec, ir->operands[1]->get_precision());
   if (ir->operands[2])
      arg_prec = higher_precision(arg_prec, ir->operands[2]->get_precision());
   unsigned res_prec = ir->get_precision();

   bool op0cast = ir->operands[0] && is_different_precision(arg_prec, ir->operands[0]->get_precision());
   bool op1cast = ir->operands[1] && is_different_precision(arg_prec, ir->operands[1]->get_precision());
   bool op2cast = ir->operands[2] && is_different_precision(arg_prec, ir->operands[2]->get_precision());
   const bool op0matrix = ir->operands[0] && ir->operands[0]->type->is_matrix();
   const bool op1matrix = ir->operands[1] && ir->operands[1]->type->is_matrix();
   bool op0castTo1 = false;
   bool op1castTo0 = false;

   // Metal does not support matrix precision casts, so when any of the arguments is a matrix,
   // take precision from it. This isn't fully robust now, but oh well.
   if (op0cast && op0matrix && !op1cast)
   {
      op0cast = false;
      arg_prec = ir->operands[0]->get_precision();
      op1cast = ir->operands[1] && is_different_precision(arg_prec, ir->operands[1]->get_precision());
   }
   if (op1cast && op1matrix && !op0cast)
   {
      op1cast = false;
      arg_prec = ir->operands[1]->get_precision();
      op0cast = ir->operands[0] && is_different_precision(arg_prec, ir->operands[0]->get_precision());
   }

   // Metal does not have matrix+scalar and matrix-scalar operations; we need to create matrices
   // out of the non-matrix argument.
   if (ir->operation == ir_binop_add || ir->operation == ir_binop_sub)
   {
      if (op0matrix && !op1matrix)
      {
         op1cast = true;
         op1castTo0 = true;
      }
      if (op1matrix && !op0matrix)
      {
         op0cast = true;
         op0castTo1 = true;
      }
      if (op1castTo0 || op0castTo1)
      {
         if (!ctx.matrixConstructorsDone)
         {
            ctx.prefixStr.asprintf_append(
               "inline float4x4 _xlinit_float4x4(float v) { return float4x4(float4(v), float4(v), float4(v), float4(v)); }\n"
               "inline float3x3 _xlinit_float3x3(float v) { return float3x3(float3(v), float3(v), float3(v)); }\n"
               "inline float2x2 _xlinit_float2x2(float v) { return float2x2(float2(v), float2(v)); }\n"
               "inline half4x4 _xlinit_half4x4(half v) { return half4x4(half4(v), half4(v), half4(v), half4(v)); }\n"
               "inline half3x3 _xlinit_half3x3(half v) { return half3x3(half3(v), half3(v), half3(v)); }\n"
               "inline half2x2 _xlinit_half2x2(half v) { return half2x2(half2(v), half2(v)); }\n"
                                   );
            ctx.matrixConstructorsDone = true;
         }
      }
   }


   const bool conversion = ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2i;
   const bool rescast = !conversion && is_different_precision(arg_prec, res_prec) && !ir->type->is_boolean();
   if (rescast)
   {
      buffer.asprintf_append ("(");
      print_cast (buffer, res_prec, ir);
   }

   if (ir->num_operands == 1)
   {
      if (op0cast && !conversion)
         print_cast (buffer, arg_prec, ir->operands[0]);
      if (ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2i) {
         print_type(buffer, ir, ir->type, true);
         buffer.asprintf_append ("(");
      } else if (ir->operation >= ir_unop_bitcast_i2f && ir->operation <= ir_unop_bitcast_f2u) {
         buffer.asprintf_append("as_type<");
         print_type(buffer, ir, ir->type, true);
         buffer.asprintf_append(">(");
      } else if (ir->operation == ir_unop_rcp) {
         const bool halfCast = (arg_prec == GLSL_PRECISION_MEDIUM);
         buffer.asprintf_append (halfCast ? "((half)1.0/(" : "(1.0/(");
      } else {
         buffer.asprintf_append ("%s(", metal_operator(ir->operation));
      }
      if (ir->operands[0])
         ir->operands[0]->accept(this);
      buffer.asprintf_append (")");
      if (ir->operation == ir_unop_rcp) {
         buffer.asprintf_append (")");
      }
   }
   else if (ir->operation == ir_triop_csel) {
      buffer.asprintf_append("select(");
      if (op2cast) print_cast(buffer, arg_prec, ir->operands[2]);
      ir->operands[2]->accept(this);
      buffer.asprintf_append(", ");
      if (op1cast) print_cast(buffer, arg_prec, ir->operands[1]);
      ir->operands[1]->accept(this);
      buffer.asprintf_append(", "); ir->operands[0]->accept(this); buffer.asprintf_append(")");
   }
   else if (ir->operands[0]->type->is_vector() &&
            (ir->operation == ir_binop_all_equal || ir->operation == ir_binop_any_nequal)) {
      buffer.asprintf_append(ir->operation == ir_binop_all_equal ? "all(" : "any(");
      if (op0cast) print_cast(buffer, arg_prec, ir->operands[0]);
      ir->operands[0]->accept(this);
      buffer.asprintf_append(" %s ", metal_operator(ir->operation));
      if (op1cast) print_cast(buffer, arg_prec, ir->operands[1]);
      ir->operands[1]->accept(this); buffer.asprintf_append(")");
   }
   else if (ir->operation == ir_quadop_vector) {
      print_type(buffer, ir, ir->type, true); buffer.asprintf_append("(");
      for (unsigned i = 0; i < ir->type->vector_elements; ++i) {
         if (i) buffer.asprintf_append(", "); ir->operands[i]->accept(this);
      }
      buffer.asprintf_append(")");
   }
   else if (ir->operation == ir_binop_vector_extract)
   {
      // a[b]

      if (ir->operands[0])
         ir->operands[0]->accept(this);
      buffer.asprintf_append ("[");
      if (ir->operands[1])
         ir->operands[1]->accept(this);
      buffer.asprintf_append ("]");
   }
   else if (is_binop_func_like(ir->operation, ir->type))
   {
      // binary operation that must be printed like a function, "foo(a,b)"
      if (ir->operation == ir_binop_mod)
      {
         buffer.asprintf_append ("(");
         print_type(buffer, ir, ir->type, true);
         buffer.asprintf_append ("(");
      }
      buffer.asprintf_append ("%s (", metal_operator(ir->operation));

      if (ir->operands[0])
      {
         if (op0cast)
            print_cast (buffer, arg_prec, ir->operands[0]);
         ir->operands[0]->accept(this);
      }
      buffer.asprintf_append (", ");
      if (ir->operands[1])
      {
         if (op1cast)
            print_cast (buffer, arg_prec, ir->operands[1]);
         ir->operands[1]->accept(this);
      }
      buffer.asprintf_append (")");
      if (ir->operation == ir_binop_mod)
         buffer.asprintf_append ("))");
   }
   else if (ir->num_operands == 2 && ir->operation == ir_binop_div && op0matrix && !op1matrix)
   {
      // "matrix/scalar" - Metal does not have it, so print multiply by inverse instead
      buffer.asprintf_append ("(");
      ir->operands[0]->accept(this);
      const bool halfCast = (arg_prec == GLSL_PRECISION_MEDIUM);
      buffer.asprintf_append (halfCast ? " * (1.0h/half(" : " * (1.0/(");
      ir->operands[1]->accept(this);
      buffer.asprintf_append (")))");
   }
   else if (ir->num_operands == 2)
   {
      // regular binary operator
      buffer.asprintf_append ("(");
      if (ir->operands[0])
      {
         if (op0castTo1)
         {
            buffer.asprintf_append ("_xlinit_");
            print_type_precision(buffer, ir->operands[1]->type, arg_prec, false);
            buffer.asprintf_append ("(");
         }
         else if (op0cast)
         {
            print_cast (buffer, arg_prec, ir->operands[0]);
            buffer.asprintf_append ("(");
         }
         ir->operands[0]->accept(this);
         if (op0castTo1 || op0cast)
         {
            buffer.asprintf_append (")");
         }
      }

      buffer.asprintf_append (" %s ", ir->operation == ir_binop_mod ? "%" : metal_operator(ir->operation));

      if (ir->operands[1])
      {
         if (op1castTo0)
         {
            buffer.asprintf_append ("_xlinit_");
            print_type_precision(buffer, ir->operands[0]->type, arg_prec, false);
            buffer.asprintf_append ("(");
         }
         else if (op1cast)
         {
            print_cast (buffer, arg_prec, ir->operands[1]);
            buffer.asprintf_append ("(");
         }
         ir->operands[1]->accept(this);
         if (op1castTo0 || op1cast)
         {
            buffer.asprintf_append (")");
         }
      }
      buffer.asprintf_append (")");
   }
   else
   {
      // ternary op
      buffer.asprintf_append ("%s (", metal_operator(ir->operation));
      if (ir->operands[0])
      {
         if (op0cast)
            print_cast (buffer, arg_prec, ir->operands[0]);
         ir->operands[0]->accept(this);
      }
      buffer.asprintf_append (", ");
      if (ir->operands[1])
      {
         if (op1cast)
            print_cast (buffer, arg_prec, ir->operands[1]);
         ir->operands[1]->accept(this);
      }
      buffer.asprintf_append (", ");
      if (ir->operands[2])
      {
         if (op2cast)
            print_cast (buffer, arg_prec, ir->operands[2]);
         ir->operands[2]->accept(this);
      }
      buffer.asprintf_append (")");
   }

   if (rescast)
   {
      buffer.asprintf_append (")");
   }


   newline_deindent();
   --this->expression_depth;
}


static void print_texture_uv(ir_print_metal_visitor* vis, ir_texture* ir, bool is_shadow, bool is_proj, bool is_array)
{
   const unsigned dim = ir->sampler->type->sampler_dimensionality;
   const unsigned n = (dim == GLSL_SAMPLER_DIM_CUBE || dim == GLSL_SAMPLER_DIM_3D) ? 3 : 2;
   vis->buffer.asprintf_append("(float%u)((", n);
   ir->coordinate->accept(vis);
   vis->buffer.asprintf_append(")");
   if (is_array) vis->buffer.asprintf_append(n == 3 ? ".xyz" : ".xy");
   vis->buffer.asprintf_append(")");
   if (is_proj) { vis->buffer.asprintf_append(" / (float)("); ir->projector->accept(vis); vis->buffer.asprintf_append(")"); }
   if (is_array) {
      // GLSL gather rounds the layer to the nearest integer and clamps it to the array.
      if (ir->op == ir_tg4)
         vis->buffer.asprintf_append(", uint(clamp(floor(float((");
      else
         vis->buffer.asprintf_append(", uint((");
      ir->coordinate->accept(vis);
      if (ir->op == ir_tg4) {
         vis->buffer.asprintf_append(n == 3 ? ").w) + 0.5), 0.0, float(" : ").z) + 0.5), 0.0, float(");
         ir->sampler->accept(vis);
         vis->buffer.asprintf_append(".get_array_size() - 1u)))");
      } else {
         vis->buffer.asprintf_append(n == 3 ? ").w)" : ").z)");
      }
   }
   if (is_shadow && ir->op == ir_tg4) {
      vis->buffer.asprintf_append(", float(");
      ir->shadow_comparator->accept(vis);
      vis->buffer.asprintf_append(")");
   } else if (is_shadow) {
      vis->buffer.asprintf_append(", saturate((float)("); ir->shadow_comparator->accept(vis);
      vis->buffer.asprintf_append(")");
      if (is_proj) { vis->buffer.asprintf_append(" / (float)("); ir->projector->accept(vis); vis->buffer.asprintf_append(")"); }
      vis->buffer.asprintf_append(")");
   }
}

void ir_print_metal_visitor::visit(ir_texture *ir)
{
   if (ir->op == ir_txf)
   {
      ir->sampler->accept(this);
      buffer.asprintf_append (ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_3D ? ".read(uint3(" : ".read(uint2(");
      ir->coordinate->accept(this);
      if (ir->offset) { buffer.asprintf_append(" + "); ir->offset->accept(this); }
      buffer.asprintf_append ("), uint(");
      ir->lod_info.lod->accept(this);
      buffer.asprintf_append ("))");
      return;
   }
   if (ir->op == ir_txs)
   {
      buffer.asprintf_append (ir->type->vector_elements == 3 ? "int3(" : "int2(");
      ir->sampler->accept(this);
      buffer.asprintf_append (".get_width(");
      ir->lod_info.lod->accept(this);
      buffer.asprintf_append ("), ");
      ir->sampler->accept(this);
      buffer.asprintf_append (".get_height(");
      ir->lod_info.lod->accept(this);
      buffer.asprintf_append (")");
      if (ir->type->vector_elements == 3) {
         buffer.asprintf_append(", "); ir->sampler->accept(this);
         buffer.asprintf_append(ir->sampler->type->sampler_array ? ".get_array_size()" : ".get_depth()");
      }
      buffer.asprintf_append (")");
      return;
   }
   glsl_sampler_dim sampler_dim = (glsl_sampler_dim)ir->sampler->type->sampler_dimensionality;
   const bool is_shadow = ir->sampler->type->sampler_shadow;
   const bool is_array = ir->sampler->type->sampler_array;
   const bool is_proj = ir->projector != nullptr;

   const bool is_gather = ir->op == ir_tg4;
   if (is_gather && (sampler_dim == GLSL_SAMPLER_DIM_RECT || (ir->offset && ir->offset->type->is_array()))) {
      ctx.prefixStr.asprintf_append("#error Metal texture gather does not support rectangle textures or four independent offsets\n");
      buffer.asprintf_append("float4(0.0)");
      return;
   }
   const bool halfShadow = is_shadow && ir->get_precision() == GLSL_PRECISION_MEDIUM;
   if (halfShadow) buffer.asprintf_append(is_gather ? "half4(" : "half(");
   // texture name & call to sample
   ir->sampler->accept(this);
   if (is_shadow && !is_gather)
   {
      // For shadow sampling, Metal right now needs a hardcoded sampler state :|
      if (!ctx.shadowSamplerDone)
      {
         ctx.prefixStr.asprintf_append("constexpr sampler _mtl_xl_shadow_sampler(address::clamp_to_edge, filter::linear, compare_func::less_equal);\n");
         ctx.shadowSamplerDone = true;
      }
      buffer.asprintf_append (".sample_compare(_mtl_xl_shadow_sampler");
   }
   else
   {
      const char* method = is_gather ? (is_shadow ? "gather_compare" : "gather") : "sample";
      buffer.asprintf_append (".%s(_mtlsmp_", method);
      ir->sampler->accept(this);
   }
   buffer.asprintf_append (", ");

   // texture coordinate
   print_texture_uv (this, ir, is_shadow, is_proj, is_array);

   // lod bias
   if (ir->op == ir_txb)
   {
      buffer.asprintf_append (", bias(");
      ir->lod_info.bias->accept(this);
      buffer.asprintf_append (")");
   }

   // lod
   if (ir->op == ir_txl)
   {
      buffer.asprintf_append (", level(");
      ir->lod_info.lod->accept(this);
      buffer.asprintf_append (")");
   }

   // grad
   if (ir->op == ir_txd)
   {
      if (sampler_dim == GLSL_SAMPLER_DIM_CUBE)
         buffer.asprintf_append (", gradientcube((float3)(");
      else if (sampler_dim == GLSL_SAMPLER_DIM_3D)
         buffer.asprintf_append (", gradient3d((float3)(");
      else
         buffer.asprintf_append (", gradient2d((float2)(");

      ir->lod_info.grad.dPdx->accept(this);

      if (sampler_dim == GLSL_SAMPLER_DIM_CUBE || sampler_dim == GLSL_SAMPLER_DIM_3D)
         buffer.asprintf_append ("), (float3)(");
      else
         buffer.asprintf_append ("), (float2)(");

      ir->lod_info.grad.dPdy->accept(this);
      buffer.asprintf_append ("))");
   }

   if (ir->offset) { buffer.asprintf_append(", "); ir->offset->accept(this); }
   if (is_gather && !is_shadow) {
      const auto* component = ir->lod_info.component->as_constant();
      if (!component || component->value.i[0] < 0 || component->value.i[0] > 3) {
         ctx.prefixStr.asprintf_append("#error Metal texture gather requires a constant component from 0 through 3\n");
      } else if (component->value.i[0] != 0) {
         if (sampler_dim == GLSL_SAMPLER_DIM_2D && !ir->offset)
            buffer.asprintf_append(", int2(0)");
         buffer.asprintf_append(", component::%c", "xyzw"[component->value.i[0]]);
      }
   }

   buffer.asprintf_append (")");
   if (halfShadow) buffer.asprintf_append(")");
}


void ir_print_metal_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };

   if (ir->val->type->is_scalar()) {
      // A vector constructor broadcasts a scalar to every component.
      const bool broadcast = ir->mask.num_components > 1;
      if (broadcast) {
         print_type(buffer, ir, ir->type, true);
         buffer.asprintf_append("(");
      }
      ir->val->accept(this);
      if (broadcast)
         buffer.asprintf_append(")");
      return;
   }

   ir->val->accept(this);
   buffer.asprintf_append (".");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
      buffer.asprintf_append ("%c", "xyzw"[swiz[i]]);
   }
}

static void print_var_inout (string_buffer& buf, ir_variable* var, bool insideLHS)
{
   if (var->data.mode == ir_var_shader_in)
      buf.asprintf_append ("_mtl_i.");
   if (var->data.mode == ir_var_shader_out && !var->data.fb_fetch_output)
      buf.asprintf_append ("_mtl_o.");
   if (var->data.mode == ir_var_uniform && !var->type->without_array()->is_sampler() && !var->type->contains_image())
      buf.asprintf_append ("_mtl_u.");
   if (var->data.mode == ir_var_shader_out && var->data.fb_fetch_output)
      buf.asprintf_append (insideLHS ? "_mtl_o." : "_mtl_i.");
}

static bool uniform_read_row_major(ir_rvalue* value, bool rootRow)
{
   if (value->as_dereference_variable()) return rootRow;
   if (auto record = value->as_dereference_record())
      return buffer_field_row_major(record->record->type->fields.structure[record->field_idx], uniform_read_row_major(record->record, rootRow));
   return uniform_read_row_major(value->as_dereference_array()->array, rootRow);
}

void ir_print_metal_visitor::print_counter_address(ir_rvalue* value)
{
   if (auto* variable = value->as_dereference_variable()) {
      buffer.asprintf_append("_mtl_atomic%u", ctx.counterSlots.at(variable->var));
      if (variable->var->data.offset) buffer.asprintf_append(" + %uu", variable->var->data.offset / 4);
   } else {
      auto* array = value->as_dereference_array();
      print_counter_address(array->array);
      buffer.asprintf_append(" + ("); array->array_index->accept(this);
      buffer.asprintf_append(")");
      if (array->type->atomic_size() != 4) buffer.asprintf_append(" * %uu", array->type->atomic_size() / 4);
   }
}

void ir_print_metal_visitor::print_buffer_size_index(ir_rvalue* value)
{
   if (auto var = value->as_dereference_variable()) {
      const auto& block = ctx.blockNames.at(var->var);
      buffer.asprintf_append(block.array ? "_mtl_bs%u" : "%uu", block.index);
   } else {
      auto array = value->as_dereference_array();
      print_buffer_size_index(array->array);
      buffer.asprintf_append("[");
      array->array_index->accept(this);
      buffer.asprintf_append("]");
   }
}

void ir_print_metal_visitor::print_uniform_storage(ir_rvalue* value, const metal_print_context::block_reference& block)
{
   auto var = value->as_dereference_variable();
   auto record = value->as_dereference_record();
   if (block.fields && ((var && !var->var->is_interface_instance()) ||
                        (record && record->record->type->is_interface()))) {
      const auto& field = block.fields[record ? record->field_idx : block.member_index];
      bool runtime = value->type->is_unsized_array();
      buffer.asprintf_append(runtime ? "reinterpret_cast<%s %s_element*>(" : "(*reinterpret_cast<%s %s*>(", block.address, field.storage);
      if (record) print_uniform_storage(record->record, block);
      else buffer.asprintf_append("_mtl_buffer%u", block.index);
      buffer.asprintf_append(" + %u)", field.offset);
      if (!runtime) buffer.asprintf_append(")");
      return;
   }
   if (auto var = value->as_dereference_variable()) {
      buffer.asprintf_append(block.array ? "_mtl_b%u" : "_mtl_buffer%u", block.index);
      if (!var->var->is_interface_instance()) buffer.asprintf_append(".%s", var->var->name);
   } else if (auto record = value->as_dereference_record()) {
      print_uniform_storage(record->record, block);
      buffer.asprintf_append(".%s", record->record->type->fields.structure[record->field_idx].name);
   } else {
      auto array = value->as_dereference_array();
      bool blockArray = array->array->type->without_array()->is_interface();
      bool dereferenceBlock = blockArray && array->type->is_interface() && !block.fields;
      if (dereferenceBlock) buffer.asprintf_append("(*");
      print_uniform_storage(array->array, block);
      buffer.asprintf_append(blockArray || array->array->type->is_vector() || array->array->type->is_matrix() || array->array->type->is_unsized_array() ? "[" : ".elements[");
      bool savedLhs = inside_lhs;
      inside_lhs = false;
      array->array_index->accept(this);
      inside_lhs = savedLhs;
      buffer.asprintf_append("]");
      if (dereferenceBlock) buffer.asprintf_append(")");
      if (!blockArray && array->array->type->is_array()) buffer.asprintf_append(".value");
   }
}

bool ir_print_metal_visitor::print_uniform_read(ir_rvalue* value)
{
   if (ctx.blockNames.empty()) return false;
   auto var = value->variable_referenced();
   auto found = ctx.blockNames.find(var);
   if (found == ctx.blockNames.end()) return false;
   const auto& block = found->second;
   if (inside_lhs) { print_uniform_storage(value, block); return true; }
   unsigned kind = block.storage;
   bool row = uniform_read_row_major(value, block.row_major);
   if (auto array = value->as_dereference_array()) {
      if (array->array->type->is_matrix() && native_buffer_type(ctx, array->array->type, row, kind)) {
         print_type_precision(buffer, value->type, value->get_precision(), false);
         buffer.asprintf_append("("); print_uniform_storage(value, block); buffer.asprintf_append(")");
         return true;
      }
      if (array->array->type->is_matrix()) {
         // Reading a matrix column does not require constructing the full matrix.
         print_type_precision(buffer, value->type, value->get_precision(), false);
         buffer.asprintf_append("(");
         if (row) {
            for (unsigned r = 0; r < value->type->vector_elements; ++r) {
               if (r) buffer.asprintf_append(", ");
               print_uniform_storage(array->array, block);
               buffer.asprintf_append(".vectors[%u][", r);
               array->array_index->accept(this);
               buffer.asprintf_append("]");
            }
         } else {
            print_uniform_storage(array->array, block);
            buffer.asprintf_append(".vectors[");
            array->array_index->accept(this);
            buffer.asprintf_append("].%.*s", value->type->vector_elements, "xyzw");
         }
         buffer.asprintf_append(")");
         return true;
      }
      // A scalar matrix read selects its storage vector and component directly.
      if (auto column = array->array->as_dereference_array()) {
         if (column->array->type->is_matrix()) {
            print_type_precision(buffer, value->type, value->get_precision(), false);
            buffer.asprintf_append("(");
            print_uniform_storage(column->array, block);
            buffer.asprintf_append(native_buffer_type(ctx, column->array->type, row, kind) ? "[" : ".vectors[");
            (row ? array->array_index : column->array_index)->accept(this);
            buffer.asprintf_append("][");
            (row ? column->array_index : array->array_index)->accept(this);
            buffer.asprintf_append("])");
            return true;
         }
      }
   }
   print_uniform_value(ctx, buffer, value->type, row, value->get_precision(), kind, [&]() { print_uniform_storage(value, block); });
   return true;
}

void ir_print_metal_visitor::print_shared_storage(ir_rvalue* value)
{
   if (auto* variable = value->as_dereference_variable()) {
      print_var_name(variable->var);
   } else if (auto* array = value->as_dereference_array()) {
      print_shared_storage(array->array);
      buffer.asprintf_append(array->array->type->is_matrix() ? ".columns[" : "[");
      bool savedLhs = inside_lhs;
      inside_lhs = false;
      array->array_index->accept(this);
      inside_lhs = savedLhs;
      buffer.asprintf_append("]");
   } else if (auto* record = value->as_dereference_record()) {
      print_shared_storage(record->record);
      buffer.asprintf_append(".%s", record->record->type->fields.structure[record->field_idx].name);
   } else {
      value->accept(this);
   }
}

bool ir_print_metal_visitor::print_shared_access(ir_rvalue* value)
{
   if (!ctx.sharedMatrixTemplate) return false;
   auto* var = value->variable_referenced();
   if (!var || var->data.mode != ir_var_shader_shared || !shared_storage_is_wrapped(ctx, var->type)) return false;
   print_shared_storage(value);
   if (!inside_lhs && !value->type->is_array() && shared_storage_is_wrapped(ctx, value->type))
      buffer.asprintf_append(".read()");
   return true;
}

void ir_print_metal_visitor::visit(ir_dereference_variable *ir)
{
   if (print_shared_access(ir)) return;
   ir_variable *var = ir->variable_referenced();
   if (print_uniform_read(ir)) return;
   if (!ctx.stageNames.empty()) {
      auto stage = ctx.stageNames.find(var);
      if (stage != ctx.stageNames.end()) {
         buffer.asprintf_append("%s", stage->second);
         return;
      }
   }
   const bool signedID = !strcmp(var->name, "gl_VertexID") || !strcmp(var->name, "gl_InstanceID");
   const bool halfCoord = !strcmp(var->name, "gl_PointCoord") && var->data.precision == GLSL_PRECISION_MEDIUM;
   if (signedID) buffer.asprintf_append("int(");
   if (halfCoord) buffer.asprintf_append("half2(");
   print_var_inout(buffer, var, this->inside_lhs);
   print_var_name (var);
   if (signedID || halfCoord) buffer.asprintf_append(")");
}


void ir_print_metal_visitor::visit(ir_dereference_array *ir)
{
   if (print_shared_access(ir)) return;
   if (print_uniform_read(ir)) return;
   ir->array->accept(this);
   buffer.asprintf_append ("[");
   ir->array_index->accept(this);
   buffer.asprintf_append ("]");
}


void ir_print_metal_visitor::visit(ir_dereference_record *ir)
{
   if (print_shared_access(ir)) return;
   if (print_uniform_read(ir)) return;
   ir->record->accept(this);
   buffer.asprintf_append (".%s", ir->record->type->fields.structure[ir->field_idx].name);
}




void ir_print_metal_visitor::emit_buffer_aggregate(ir_dereference* lhs, ir_rvalue* rhs)
{
   const auto& block = ctx.blockNames.at(lhs->variable_referenced());
   bool row = uniform_read_row_major(lhs, block.row_major);
   const char* storage = uniform_storage_type(ctx, lhs->type, row, block.storage);
   buffer.asprintf_append("{\n"); ++indentation; indent();
   buffer.asprintf_append("%s %s& _buf_dst = ", block.address, storage);
   print_uniform_storage(lhs, block); buffer.asprintf_append(";\n"); indent();
   auto* variable = rhs->variable_referenced();
   auto source = ctx.blockNames.find(variable);
   bool sourceStorage = rhs->as_dereference() && source != ctx.blockNames.end();
   bool sourceShared = rhs->as_dereference() && variable && variable->data.mode == ir_var_shader_shared &&
                       shared_storage_is_wrapped(ctx, rhs->type);
   bool sourceRow = sourceStorage ? uniform_read_row_major(rhs, source->second.row_major) : false;
   unsigned sourceKind = sourceStorage ? source->second.storage : 0;
   if (!sourceShared && native_buffer_type(ctx, lhs->type, row, block.storage) &&
       (!sourceStorage || native_buffer_type(ctx, rhs->type, sourceRow, sourceKind))) {
      buffer.asprintf_append("_buf_dst = ");
      if (lhs->type->is_matrix()) buffer.asprintf_append("%s(", storage);
      rhs->accept(this);
      if (lhs->type->is_matrix()) buffer.asprintf_append(")");
      buffer.asprintf_append(";\n");
      --indentation; indent(); buffer.asprintf_append("}");
      return;
   }
   if (sourceStorage) {
      const auto& sourceBlock = source->second;
      buffer.asprintf_append("%s%s %s& _buf_src = ", strncmp(sourceBlock.address, "const ", 6) == 0 ? "" : "const ", sourceBlock.address,
                             uniform_storage_type(ctx, rhs->type, sourceRow, sourceKind));
      print_uniform_storage(rhs, sourceBlock);
   } else if (sourceShared) {
      buffer.asprintf_append("const threadgroup auto& _buf_src = ");
      print_shared_storage(rhs);
   } else {
      const char* address = variable && variable->data.mode == ir_var_uniform ? "constant" :
         variable && variable->data.mode == ir_var_shader_shared ? "threadgroup" : "thread";
      buffer.asprintf_append("const %s auto& _buf_src = ", address);
      rhs->accept(this);
   }
   buffer.asprintf_append(";\n");
   store_buffer_value(ctx, buffer, lhs->type, row, "_buf_dst", "_buf_src", indentation, block.storage,
                      sourceStorage ? aggregate_source::buffer : sourceShared ? aggregate_source::shared : aggregate_source::value,
                      sourceRow, sourceKind, rhs->get_precision());
   --indentation; indent(); buffer.asprintf_append("}");
}

// Matrix columns are contiguous only in column-major storage. Row-major
// column writes scatter into rows, without loading or rewriting other values.
bool ir_print_metal_visitor::emit_storage_matrix_write(ir_dereference* lhs, ir_rvalue* rhs,
                                                       unsigned write_mask, ir_rvalue* dstIndex)
{
   auto* column = lhs->as_dereference_array();
   if (!column) return false;
   ir_rvalue* component = dstIndex;
   unsigned components[4] = {0, 1, 2, 3}, componentCount = 0;
   if (column->array->type->is_vector()) {
      component = column->array_index;
      ir_rvalue* vector = column->array;
      if (vector->as_swizzle()) componentCount = vector->type->vector_elements;
      while (auto* swizzle = vector->as_swizzle()) {
         unsigned mask[4] = {swizzle->mask.x, swizzle->mask.y, swizzle->mask.z, swizzle->mask.w};
         for (unsigned i = 0; i < componentCount; ++i) components[i] = mask[components[i]];
         vector = swizzle->val;
      }
      column = vector->as_dereference_array();
   }
   if (!column || !column->array->type->is_matrix()) return false;
   auto* variable = lhs->variable_referenced();
   bool shared = variable->data.mode == ir_var_shader_shared;
   const auto* block = shared ? nullptr : &ctx.blockNames.at(variable);
   bool row = block && uniform_read_row_major(column->array, block->row_major);
   bool native = block && native_buffer_type(ctx, column->array->type, row, block->storage);
   const char* address = shared ? "threadgroup" : block->address;
   const char* columns = shared ? "columns" : "vectors";
   unsigned precision = shared ? column->array->as_dereference()->precision() : GLSL_PRECISION_HIGH;
   const char* scalar = precision == GLSL_PRECISION_MEDIUM ? "half" : "float";
   unsigned width = column->array->type->vector_elements;
   char mask[5]; unsigned rows[4], count = 0;
   for (unsigned i = 0; i < width; ++i)
      if (write_mask & (1u << i)) { rows[count] = i; mask[count++] = "xyzw"[i]; }
   mask[count] = 0;
   buffer.asprintf_append("{\n"); ++indentation; indent();
   buffer.asprintf_append("%s ", address);
   if (shared) print_shared_type(ctx, buffer, column->array->type, precision);
   else buffer.asprintf_append("%s", uniform_storage_type(ctx, column->array->type, row, block->storage));
   buffer.asprintf_append("& _buf_matrix = ");
   if (shared) print_shared_storage(column->array);
   else print_uniform_storage(column->array, *block);
   buffer.asprintf_append(";\n"); indent();
   buffer.asprintf_append("const auto _buf_column = "); column->array_index->accept(this);
   buffer.asprintf_append(";\n"); indent();
   if (component) {
      buffer.asprintf_append("const auto _buf_component = ");
      if (componentCount) {
         buffer.asprintf_append("uint%u(", componentCount);
         for (unsigned i = 0; i < componentCount; ++i)
            buffer.asprintf_append("%s%uu", i ? ", " : "", components[i]);
         buffer.asprintf_append(")[");
      }
      component->accept(this);
      if (componentCount) buffer.asprintf_append("]");
      buffer.asprintf_append(";\n"); indent();
   }
   auto print_rhs = [&]() {
      unsigned rhsWidth = rhs->type->vector_elements;
      buffer.asprintf_append(rhsWidth == 1 ? "%s(" : "%s%u(", scalar, rhsWidth);
      rhs->accept(this);
      if (!component && rhsWidth != count) buffer.asprintf_append(").%s", mask);
      else buffer.asprintf_append(")");
   };
   // Scalar pointers prevent component stores from becoming vector read-modify-
   // writes that could clobber another invocation's adjacent component.
   auto print_scalar = [&](const char* selectedRow) {
      buffer.asprintf_append("reinterpret_cast<%s %s*>(&_buf_matrix", address, scalar);
      if (native) buffer.asprintf_append("[_buf_column]");
      else buffer.asprintf_append(".%s[%s]", columns, row ? selectedRow : "_buf_column");
      buffer.asprintf_append(")[%s]", row ? "_buf_column" : selectedRow);
   };
   unsigned storageWidth = shared || native ? width : metal_print_context::is_std430(block->storage) && width == 2 ? 2 : 4;
   if (component) {
      print_scalar("_buf_component"); buffer.asprintf_append(" = ");
      print_rhs(); buffer.asprintf_append(";\n");
   } else if (!row && count == width && storageWidth == width) {
      buffer.asprintf_append(native ? "_buf_matrix[_buf_column] = " : "_buf_matrix.%s[_buf_column] = ", columns);
      print_rhs(); buffer.asprintf_append(";\n");
   } else {
      buffer.asprintf_append(count == 1 ? "const %s _buf_value = " : "const %s%u _buf_value = ", scalar, count);
      print_rhs(); buffer.asprintf_append(";\n");
      for (unsigned i = 0; i < count; ++i) {
         indent(); print_scalar(ralloc_asprintf(ctx.mem_ctx, "%u", rows[i]));
         buffer.asprintf_append(" = _buf_value");
         if (count > 1) buffer.asprintf_append(".%c", "xyzw"[i]);
         buffer.asprintf_append(";\n");
      }
   }
   --indentation; indent(); buffer.asprintf_append("}");
   return true;
}

void ir_print_metal_visitor::emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex)
{
   const auto* variable = lhs->variable_referenced();
   bool sharedMatrix = variable->data.mode == ir_var_shader_shared && shared_storage_is_wrapped(ctx, variable->type);
   if ((variable->data.mode == ir_var_shader_storage || sharedMatrix) &&
       emit_storage_matrix_write(lhs, rhs, write_mask, dstIndex)) return;
   if (sharedMatrix && !lhs->type->is_array() && shared_storage_is_wrapped(ctx, lhs->type)) {
      print_shared_storage(lhs);
      buffer.asprintf_append(".write(");
      rhs->accept(this);
      buffer.asprintf_append(")");
      return;
   }
   if (variable->data.mode == ir_var_shader_storage &&
       (lhs->type->is_struct() || lhs->type->is_array() || lhs->type->is_matrix())) {
      emit_buffer_aggregate(lhs, rhs);
      return;
   }
   // Metal arrays are not assignable; copy elements, retaining precision casts.
   if (lhs->type->is_array()) {
      void* mem_ctx = ralloc_parent(lhs);
      for (unsigned i = 0; i < lhs->type->length; ++i) {
         if (i) { buffer.asprintf_append(";\n"); indent(); }
         auto* dst = new(mem_ctx) ir_dereference_array(lhs, new(mem_ctx) ir_constant(i));
         auto* src = new(mem_ctx) ir_dereference_array(rhs, new(mem_ctx) ir_constant(i));
         emit_assignment_part(dst, src, (1u << dst->type->vector_elements) - 1, nullptr);
      }
      return;
   }
   const bool prev_lhs_flag = this->inside_lhs;
   this->inside_lhs = true;

   lhs->accept(this);

   this->inside_lhs = prev_lhs_flag;

   const glsl_type* lhsType = lhs->type;
   if (dstIndex)
   {
      // if dst index is a constant, then emit a swizzle
      ir_constant* dstConst = dstIndex->as_constant();
      if (dstConst)
      {
         const char* comps = "xyzw";
         char comp = comps[dstConst->get_int_component(0)];
         buffer.asprintf_append (".%c", comp);
      }
      else
      {
         buffer.asprintf_append ("[");
         dstIndex->accept(this);
         buffer.asprintf_append ("]");
      }

      if (lhsType->matrix_columns <= 1 && lhsType->vector_elements > 1)
         lhsType = glsl_type::get_instance(lhsType->base_type, 1, 1);
   }

   char mask[5];
   unsigned j = 0;
   const glsl_type* rhsType = rhs->type;
   if (!dstIndex && lhsType->matrix_columns <= 1 && lhsType->vector_elements > 1 && write_mask != (1<<lhsType->vector_elements)-1)
   {
      for (unsigned i = 0; i < 4; i++) {
         if ((write_mask & (1 << i)) != 0) {
            mask[j] = "xyzw"[i];
            j++;
         }
      }
      lhsType = glsl_type::get_instance(lhsType->base_type, j, 1);
   }
   mask[j] = '\0';
   bool hasWriteMask = false;
   if (mask[0])
   {
      buffer.asprintf_append (".%s", mask);
      hasWriteMask = true;
   }

   buffer.asprintf_append (" = ");

   const bool typeMismatch = !dstIndex && (lhsType != rhsType);

   const bool precMismatch = is_different_precision (lhs->get_precision(), rhs->get_precision());
   const bool addSwizzle = hasWriteMask && typeMismatch;
   if (lhs->variable_referenced()->data.mode == ir_var_shader_storage) {
      const glsl_type* storageType = typeMismatch ? rhsType : lhsType;
      if (storageType->is_boolean())
         storageType = glsl_type::get_instance(GLSL_TYPE_UINT, storageType->vector_elements, 1);
      print_type_precision(buffer, storageType, GLSL_PRECISION_HIGH, false);
      buffer.asprintf_append("(");
      rhs->accept(this);
      buffer.asprintf_append(")");
      if (addSwizzle) buffer.asprintf_append(".%s", mask);
      return;
   }
   if (typeMismatch || precMismatch)
   {
      if (!addSwizzle)
      {
         if (lhsType->is_matrix() && lhsType->matrix_columns == lhsType->vector_elements)
         {
            // Keep the legacy square matrix precision helpers.
            if (!ctx.matrixCastsDone)
            {
               ctx.prefixStr.asprintf_append(
                  "inline float4x4 _xlcast_float4x4(half4x4 v) { return float4x4(float4(v[0]), float4(v[1]), float4(v[2]), float4(v[3])); }\n"
                  "inline float3x3 _xlcast_float3x3(half3x3 v) { return float3x3(float3(v[0]), float3(v[1]), float3(v[2])); }\n"
                  "inline float2x2 _xlcast_float2x2(half2x2 v) { return float2x2(float2(v[0]), float2(v[1])); }\n"
                  "inline half4x4 _xlcast_half4x4(float4x4 v) { return half4x4(half4(v[0]), half4(v[1]), half4(v[2]), half4(v[3])); }\n"
                  "inline half3x3 _xlcast_half3x3(float3x3 v) { return half3x3(half3(v[0]), half3(v[1]), half3(v[2])); }\n"
                  "inline half2x2 _xlcast_half2x2(float2x2 v) { return half2x2(half2(v[0]), half2(v[1])); }\n"
                                      );
               ctx.matrixCastsDone = true;
            }
            buffer.asprintf_append ("_xlcast_");
         }
         print_type(buffer, lhs, lhsType, true);
      }
      buffer.asprintf_append ("(");
   }

   rhs->accept(this);

   if (typeMismatch || precMismatch)
   {
      buffer.asprintf_append (")");
      if (addSwizzle)
         buffer.asprintf_append (".%s", mask);
   }
}


// Try to print (X = X + const) as (X += const), mostly to satisfy
// OpenGL ES 2.0 loop syntax restrictions.
static bool try_print_increment (ir_print_metal_visitor* vis, ir_assignment* ir)
{
   if (ir->condition)
      return false;

   // Needs to be + on rhs
   ir_expression* rhsOp = ir->rhs->as_expression();
   if (!rhsOp || rhsOp->operation != ir_binop_add)
      return false;

   // Needs to write to whole variable
   ir_variable* lhsVar = ir->whole_variable_written();
   if (lhsVar == NULL)
      return false;

   // Types must match
   if (ir->lhs->type != ir->rhs->type)
      return false;

   // Type must be scalar
   if (!ir->lhs->type->is_scalar())
      return false;

   // rhs0 must be variable deref, same one as lhs
   ir_dereference_variable* rhsDeref = rhsOp->operands[0]->as_dereference_variable();
   if (rhsDeref == NULL)
      return false;
   if (lhsVar != rhsDeref->var)
      return false;

   // rhs1 must be a constant
   ir_constant* rhsConst = rhsOp->operands[1]->as_constant();
   if (!rhsConst)
      return false;

   // print variable name
   const bool prev_lhs_flag = vis->inside_lhs;
   vis->inside_lhs = true;

   ir->lhs->accept (vis);

   vis->inside_lhs = prev_lhs_flag;


   // print ++ or +=const
   if (ir->lhs->type->base_type <= GLSL_TYPE_INT && rhsConst->is_one())
   {
      vis->buffer.asprintf_append ("++");
   }
   else
   {
      vis->buffer.asprintf_append(" += ");
      rhsConst->accept (vis);
   }

   return true;
}


void ir_print_metal_visitor::visit(ir_assignment *ir)
{
   if (loop_initializers && loop_initializers->omitted.count(ir)) {
      skipped_this_ir = true;
      return;
   }
   // assignments in global scope are postponed to main function
   if (this->mode != kPrintGlslNone)
   {
      assert (!this->globals->main_function_done);
      this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
      buffer.asprintf_append ("//"); // for the ; that will follow (ugly, I know)
      return;
   }

   // if RHS is ir_triop_vector_insert, then we have to do some special dance. If source expression is:
   //   dst = vector_insert (a, b, idx)
   // then emit it like:
   //   dst = a;
   //   dst.idx = b;
   ir_expression* rhsOp = ir->rhs->as_expression();
   if (rhsOp && rhsOp->operation == ir_triop_vector_insert)
   {
      // skip assignment if lhs and rhs would be the same
      bool skip_assign = false;
      ir_dereference_variable* lhsDeref = ir->lhs->as_dereference_variable();
      ir_dereference_variable* rhsDeref = rhsOp->operands[0]->as_dereference_variable();
      if (lhsDeref && rhsDeref)
      {
         if (lhsDeref->var == rhsDeref->var)
            skip_assign = true;
      }

      // Indexed storage writes must not rewrite the vector's other components.
      const auto* variable = ir->lhs->variable_referenced();
      if ((variable->data.mode == ir_var_shader_storage ||
           (variable->data.mode == ir_var_shader_shared && shared_storage_is_wrapped(ctx, variable->type))) &&
          ir->lhs->equals(rhsOp->operands[0])) skip_assign = true;
      if (!skip_assign)
      {
         emit_assignment_part(ir->lhs, rhsOp->operands[0], ir->write_mask, NULL);
         buffer.asprintf_append ("; ");
      }
      emit_assignment_part(ir->lhs, rhsOp->operands[1], ir->write_mask, rhsOp->operands[2]);
      return;
   }

   if (try_print_increment (this, ir))
      return;

   if (ir->condition)
   {
      ir->condition->accept(this);
      buffer.asprintf_append (" ");
   }

   emit_assignment_part (ir->lhs, ir->rhs, ir->write_mask, NULL);
}


void ir_print_metal_visitor::visit(ir_constant *ir)
{
   const glsl_type* type = ir->type;

   // hoist array & struct constants into global scope
   if (type->is_array() || type->is_struct())
   {
      size_t id = globals->const_hash[ir];
      if (id == 0)
      {
         id = ++globals->const_counter;
         globals->const_hash[ir] = id;
         globals->global_constants.push_tail(new(globals->mem_ctx) gconst_entry_metal(ir,id));
      }
      buffer.asprintf_append("_xlat_mtl_const%i", (int)id);
      return;
   }

   if (type == glsl_type::float_type)
   {
      print_float (buffer, ir->value.f[0]);
      return;
   }
   else if (type == glsl_type::int_type)
   {
      buffer.asprintf_append ("%d", ir->value.i[0]);
      return;
   }
   else if (type == glsl_type::uint_type)
   {
      buffer.asprintf_append ("%u", ir->value.u[0]);
      return;
   }

   const glsl_type *const base_type = ir->type->get_base_type();

   print_type(buffer, ir, type, true);
   buffer.asprintf_append ("(");

   // should be dealt with above
   assert(!ir->type->is_array());
   assert(!ir->type->is_struct());
   bool first = true;

   // Metal needs matrices to be constructed from vectors, not from a bunch of scalars.
   // So instead of printing mat2(1,2,3,4) like in glsl, we have to print float2x2(float2(1,2), float2(3,4))
   // here.
   const bool mtx = ir->type->is_matrix();
   const glsl_type* vec_type = NULL; // matrix column type
   if (mtx)
      vec_type = glsl_type::get_instance (ir->type->base_type, ir->type->vector_elements, 1);

   for (unsigned i = 0; i < ir->type->components(); i++)
   {
      if (!first)
      {
         if (mtx && (i % ir->type->vector_elements == 0))
            buffer.asprintf_append (")");
         buffer.asprintf_append (", ");
      }
      first = false;

      if (mtx && (i % ir->type->vector_elements == 0))
      {
         print_type(buffer, ir, vec_type, true);
         buffer.asprintf_append ("(");
      }

      switch (base_type->base_type) {
      case GLSL_TYPE_UINT:  buffer.asprintf_append ("%u", ir->value.u[i]); break;
      case GLSL_TYPE_INT:   buffer.asprintf_append ("%d", ir->value.i[i]); break;
      case GLSL_TYPE_FLOAT: print_float(buffer, ir->value.f[i]); break;
      case GLSL_TYPE_BOOL:  buffer.asprintf_append ("%d", ir->value.b[i]); break;
      default: assert(0);
      }
   }
   if (mtx)
      buffer.asprintf_append (")");
   buffer.asprintf_append (")");
}


static unsigned image_spatial_rank(const glsl_type* image)
{
   if (image->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE) return 2;
   return glsl_get_sampler_dim_coordinate_components((glsl_sampler_dim)image->sampler_dimensionality);
}

static const char* uint_vector_type(unsigned rank)
{
   static const char* names[] = {"uint", "uint2", "uint3"};
   return names[rank - 1];
}

void ir_print_metal_visitor::print_image_coordinates(ir_rvalue* coord, const glsl_type* image)
{
   unsigned spatial = image_spatial_rank(image);
   bool split = spatial < (unsigned)image->coordinate_components();
   bool cubeArray = image->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE && image->sampler_array;
   buffer.asprintf_append("%s(", uint_vector_type(spatial));
   coord->accept(this);
   if (!split) {
      buffer.asprintf_append(")");
      return;
   }
   buffer.asprintf_append(spatial == 1 ? ".x), uint(" : ".xy), uint(");
   coord->accept(this);
   buffer.asprintf_append(spatial == 1 ? ".y)" : ".z)");
   // GLSL packs the cube index and face into the third coordinate.
   if (cubeArray) {
      buffer.asprintf_append(" %% 6u, uint(");
      coord->accept(this);
      buffer.asprintf_append(".z) / 6u");
   }
}

void
ir_print_metal_visitor::visit(ir_call *ir)
{
   // calls in global scope are postponed to main function
   if (this->mode != kPrintGlslNone)
   {
      assert (!this->globals->main_function_done);
      this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
      buffer.asprintf_append ("//"); // for the ; that will follow (ugly, I know)
      return;
   }

   if (ir->return_deref)
   {
      visit(ir->return_deref);
      buffer.asprintf_append (" = ");
   }

   if (const auto* counter = find_atomic_counter(ir->callee)) {
      bool decrement = counter->intrinsic == ir_intrinsic_atomic_counter_predecrement;
      if (decrement) buffer.asprintf_append("(");
      buffer.asprintf_append("%s(", counter->metal);
      print_counter_address(((ir_instruction*)ir->actual_parameters.get_head())->as_rvalue());
      if (counter->intrinsic != ir_intrinsic_atomic_counter_read) buffer.asprintf_append(", 1u");
      buffer.asprintf_append(", memory_order_relaxed)");
      if (decrement) buffer.asprintf_append(" - 1u)");
      return;
   }

   if (const auto* atomic = find_image_atomic(ir->callee)) {
      auto* image = ((ir_instruction*)ir->actual_parameters.get_head())->as_rvalue();
      const glsl_type* imageType = image->type;
      auto* coord = ((ir_instruction*)image->next)->as_rvalue();
      auto* operand = ((ir_instruction*)coord->next)->as_rvalue();
      bool array = imageType->sampler_array;
      bool volume = imageType->sampler_dimensionality == GLSL_SAMPLER_DIM_3D;
      bool cube = imageType->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE;
      unsigned spatial = image_spatial_rank(imageType);
      unsigned rank = imageType->coordinate_components();
      bool& guard = cube ? ctx.cubeImageAtomicDone : ctx.imageAtomicDone;
      if (!guard) {
         guard = true;
         ctx.prefixStr.asprintf_append(cube ?
            "#if __METAL_VERSION__ < 400\n#error Cube image atomics require Metal 4.0 or newer\n#endif\n" :
            "#if __METAL_VERSION__ < 310\n#error Image atomics require Metal 3.1 or newer\n#endif\n");
      }
      const char* type = imageType->sampled_type == GLSL_TYPE_UINT ? "uint" : "int";
      if (atomic->intrinsic == ir_intrinsic_image_atomic_comp_swap) {
         const char* suffix = metal_image_type_name(imageType) + strlen("texture");
         if (imageType->sampler_dimensionality == GLSL_SAMPLER_DIM_2D && array) suffix = "array";
         if (imageType->sampler_dimensionality == GLSL_SAMPLER_DIM_BUF) suffix = "1d";
         // Buffer and 1D images share the same address and bounds check.
         unsigned dim = imageType->sampler_dimensionality == GLSL_SAMPLER_DIM_BUF ?
            GLSL_SAMPLER_DIM_1D : imageType->sampler_dimensionality;
         unsigned bit = 1u << (dim * 2 + array);
         if (!(ctx.imageCompSwapDims & bit)) {
            ctx.imageCompSwapDims |= bit;
            const char* components = cube ? (array ? "p.xy, p.z % 6u, p.z / 6u" : "p.xy, p.z") :
               array ? (spatial == 1 ? "p.x, p.y" : "p.xy, p.z") : "p";
            auto& prefix = ctx.prefixStr;
            prefix.asprintf_append("template<typename I, typename T>\n"
                                   "inline T _xlat_image_comp_swap_%s(I image, %s p, T compare, T value) {\n", suffix, uint_vector_type(rank));
            // Bounds checks prevent endless retries for an out-of-range zero.
            if (rank == 1) prefix.asprintf_append("  if (p >= image.get_width()) return T(0);\n");
            else {
               prefix.asprintf_append("  if (any(p >= %s(image.get_width()", uint_vector_type(rank));
               if (spatial >= 2) prefix.asprintf_append(", image.get_height()");
               if (volume) prefix.asprintf_append(", image.get_depth()");
               else if (cube) prefix.asprintf_append(array ? ", 6u * image.get_array_size()" : ", 6u");
               else if (array) prefix.asprintf_append(", image.get_array_size()");
               prefix.asprintf_append("))) return T(0);\n");
            }
            prefix.asprintf_append(
               "  vec<T, 4> expected = vec<T, 4>(compare);\n"
               "  while (!image.atomic_compare_exchange_weak(%s, &expected, vec<T, 4>(value)) && expected.x == compare) {}\n"
               "  return expected.x;\n"
               "}\n", components);
         }
         buffer.asprintf_append("_xlat_image_comp_swap_%s(", suffix);
         image->accept(this);
         buffer.asprintf_append(", %s(", uint_vector_type(rank));
         coord->accept(this);
         buffer.asprintf_append("), %s(", type); operand->accept(this);
         buffer.asprintf_append("), %s(", type);
         ((ir_instruction*)operand->next)->accept(this);
         buffer.asprintf_append("))");
      } else {
         image->accept(this); buffer.asprintf_append(".%s(", atomic->metal);
         print_image_coordinates(coord, imageType);
         buffer.asprintf_append(", %s4(", type); operand->accept(this);
         buffer.asprintf_append(")).x");
      }
      return;
   }

   if (const char* imageFunction = image_function_name(ir->callee)) {
      auto* image = ((ir_instruction*)ir->actual_parameters.get_head())->as_rvalue();
      const glsl_type* type = image->type;
      if (!strcmp(imageFunction, "imageSamples")) {
         buffer.asprintf_append("int(");
         image->accept(this);
         buffer.asprintf_append(".get_num_samples())");
      } else if (!strcmp(imageFunction, "imageSize")) {
         unsigned spatial = image_spatial_rank(type);
         unsigned rank = spatial + type->sampler_array;
         buffer.asprintf_append(rank == 1 ? "int(" : rank == 2 ? "int2(" : "int3(");
         image->accept(this); buffer.asprintf_append(".get_width()");
         if (spatial >= 2) {
            buffer.asprintf_append(", "); image->accept(this);
            buffer.asprintf_append(".get_height()");
         }
         if (spatial == 3 || type->sampler_array) {
            buffer.asprintf_append(", "); image->accept(this);
            buffer.asprintf_append(type->sampler_array ? ".get_array_size()" : ".get_depth()");
         }
         buffer.asprintf_append(")");
      } else {
         auto* coord = ((ir_instruction*)image->next)->as_rvalue();
         bool store = !strcmp(imageFunction, "imageStore");
         image->accept(this);
         buffer.asprintf_append(store ? ".write(" : ".read(");
         if (store) {
            auto* value = ((ir_instruction*)coord->next)->as_rvalue();
            buffer.asprintf_append("%s4(", type->sampled_type == GLSL_TYPE_UINT ? "uint" : type->sampled_type == GLSL_TYPE_INT ? "int" : "float");
            value->accept(this); buffer.asprintf_append("), ");
         }
         print_image_coordinates(coord, type);
         if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_MS) {
            buffer.asprintf_append(", uint(");
            ((ir_instruction*)coord->next)->accept(this);
            buffer.asprintf_append(")");
         }
         buffer.asprintf_append(")");
      }
      return;
   }

   if (const auto* barrier = find_memory_barrier(ir->callee)) {
      // A following control barrier already synchronizes threadgroup memory.
      // Keep separate fences when the instructions are not adjacent.
      if (mode_whole == kPrintGlslCompute && barrier->intrinsic == ir_intrinsic_memory_barrier_shared) {
         const exec_node* next = ir->get_next();
         if (!next->is_tail_sentinel() && static_cast<const ir_instruction*>(next)->ir_type == ir_type_barrier) {
            skipped_this_ir = true;
            return;
         }
      }
      assert(barrier->metal_flags);
      if (!ctx.memoryBarrierDone) {
         ctx.memoryBarrierDone = true;
         ctx.prefixStr.asprintf_append(
            "#if __METAL_VERSION__ < 320\n"
            "#error Memory barriers require Metal 3.2 or newer\n"
            "#endif\n");
      }
      bool workgroup = barrier->intrinsic == ir_intrinsic_memory_barrier_shared || barrier->intrinsic == ir_intrinsic_group_memory_barrier;
      buffer.asprintf_append("atomic_thread_fence(%s", barrier->metal_flags);
      if (mode_whole == kPrintGlslCompute && barrier->intrinsic == ir_intrinsic_memory_barrier)
         buffer.asprintf_append(" | mem_flags::mem_threadgroup");
      buffer.asprintf_append(", memory_order_seq_cst, %s)", workgroup ? "thread_scope_threadgroup" : "thread_scope_device");
      return;
   }

   const auto* atomic = find_buffer_atomic(ir->callee);
   if (atomic) {
      auto* target = ((ir_instruction*)ir->actual_parameters.get_head())->as_rvalue();
      const auto* variable = target->variable_referenced();
      bool shared = variable->data.mode == ir_var_shader_shared;
      const auto* block = shared ? nullptr : &ctx.blockNames.at(variable);
      const char* address = shared ? "threadgroup" : block->address;
      unsigned spaceBit = shared ? 2 : metal_print_context::is_coherent(block->storage) ? 4 : 1;
      bool compareSwap = atomic->intrinsic == ir_intrinsic_generic_atomic_comp_swap;
      if (compareSwap && !(ctx.atomicCompSwapSpaces & spaceBit)) {
         ctx.atomicCompSwapSpaces |= spaceBit;
         // Retry spurious failures; GLSL compare-and-swap returns the old value.
         ctx.prefixStr.asprintf_append(
            "template<typename A, typename T>\n"
            "inline T _xlat_atomic_comp_swap(%s A* address, T compare, T value) {\n"
            "  T expected = compare;\n"
            "  while (!atomic_compare_exchange_weak_explicit(address, &expected, value, memory_order_relaxed, memory_order_relaxed) && expected == compare) {}\n"
            "  return expected;\n"
            "}\n", address);
      }
      const char* type = target->type->base_type == GLSL_TYPE_UINT ? "uint" : "int";
      buffer.asprintf_append("%s (reinterpret_cast<%s atomic_%s*>(&", atomic->metal, address, type);
      auto* component = target->as_dereference_array();
      auto* swizzle = target->as_swizzle();
      ir_rvalue* vector = swizzle ? target :
         component && component->array->type->is_vector() ? component->array : nullptr;
      unsigned components[4] = {0, 1, 2, 3};
      unsigned count = vector ? vector->type->vector_elements : 0;
      bool remapped = false;
      // Compose swizzles into component offsets without constructing a copy of
      // the storage vector. The atomic must address the original buffer member.
      while (vector && vector->as_swizzle()) {
         auto* s = vector->as_swizzle();
         const unsigned mask[] = {s->mask.x, s->mask.y, s->mask.z, s->mask.w};
         for (unsigned i = 0; i < count; ++i) components[i] = mask[components[i]];
         vector = s->val;
         remapped = true;
      }
      // Metal cannot take the address of a vector component. Offset from the
      // vector's address instead; each int/uint component occupies four bytes.
      if (shared) (vector ? vector : target)->accept(this);
      else print_uniform_storage(vector ? vector : target, *block);
      buffer.asprintf_append(")");
      if (swizzle) buffer.asprintf_append(" + %u", components[0]);
      else if (vector) {
         buffer.asprintf_append(" + ");
         auto* index = component->array_index;
         if (remapped && index->as_constant() && index->as_constant()->get_uint_component(0) < count) {
            buffer.asprintf_append("%u", components[index->as_constant()->get_uint_component(0)]);
         } else {
            if (remapped) {
               buffer.asprintf_append("uint%u(", count);
               for (unsigned i = 0; i < count; ++i)
                  buffer.asprintf_append("%s%u", i ? ", " : "", components[i]);
               buffer.asprintf_append(")[");
            }
            index->accept(this);
            if (remapped) buffer.asprintf_append("]");
         }
      }
      bool first = true;
      foreach_in_list(ir_rvalue, arg, &ir->actual_parameters) {
         if (first) { first = false; continue; }
         buffer.asprintf_append(", %s(", type);
         arg->accept(this);
         buffer.asprintf_append(")");
      }
      buffer.asprintf_append(compareSwap ? ")" : ", memory_order_relaxed)");
      return;
   }

   buffer.asprintf_append ("%s (", ir->callee_name());
   bool first = true;
   foreach_in_list(ir_instruction, inst, &ir->actual_parameters) {
      if (!first)
         buffer.asprintf_append (", ");
      inst->accept(this);
      first = false;
   }
   buffer.asprintf_append (")");
}


void
ir_print_metal_visitor::visit(ir_return *ir)
{
   if (inEntryPoint && mode_whole != kPrintGlslCompute && !ctx.stageNames.empty()) {
      buffer.asprintf_append("{\n%s  return _mtl_o;\n}", ctx.stageExports.c_str());
      return;
   }
   buffer.asprintf_append ("return");

   ir_rvalue *const value = ir->get_value();
   if (value) {
      buffer.asprintf_append (" ");
      value->accept(this);
   }
}


void
ir_print_metal_visitor::visit(ir_discard *ir)
{
   if (ir->condition) {
      buffer.asprintf_append("if (");
      ir->condition->accept(this);
      buffer.asprintf_append(") ");
   }
   buffer.asprintf_append("discard_fragment()");
}


void
ir_print_metal_visitor::visit(ir_if *ir)
{
   buffer.asprintf_append ("if (");
   ir->condition->accept(this);

   buffer.asprintf_append (") {\n");
   indentation++; previous_skipped = false;


   foreach_in_list(ir_instruction, inst, &ir->then_instructions) {
      indent();
      inst->accept(this);
      end_statement_line();
   }

   indentation--;
   indent();
   buffer.asprintf_append ("}");

   if (!ir->else_instructions.is_empty())
   {
      buffer.asprintf_append (" else {\n");
      indentation++; previous_skipped = false;

      foreach_in_list(ir_instruction, inst, &ir->else_instructions) {
         indent();
         inst->accept(this);
         end_statement_line();
      }
      indentation--;
      indent();
      buffer.asprintf_append ("}");
   }
}


bool ir_print_metal_visitor::emit_canonical_for (ir_loop* ir)
{
   loop_variable_state* const ls = this->loopstate->get(ir);

   if (!printable_for_loop(ir, ls)) return false;

   std::unordered_set<ir_instruction*> terminator_hash;
   std::unordered_set<ir_instruction*> induction_hash;

   buffer.asprintf_append("for (");

   const auto init = loop_initializers->initializers.find(ir);
   if (init != loop_initializers->initializers.end()) {
      ir_variable* var = init->second->whole_variable_written();
      print_type(buffer, var, var->type, false);
      buffer.asprintf_append(" ");
      emit_assignment_part(init->second->lhs, init->second->rhs, init->second->write_mask, nullptr);
   }
   buffer.asprintf_append("; ");

   // emit loop terminating conditions
   foreach_in_list(loop_terminator, term, &ls->terminators)
   {
      terminator_hash.insert(term->ir);

      // IR has conditions in the form of "if (x) break",
      // whereas for loop needs them negated, in the form
      // if "while (x) continue the loop".
      // See if we can print them using syntax that reads nice.
      bool handled = false;
      ir_expression* term_expr = term->ir->condition->as_expression();
      if (term_expr)
      {
         // Binary comparison conditions
         const char* termOp = NULL;
         switch (term_expr->operation)
         {
         case ir_binop_less: termOp = ">="; break;
         case ir_binop_gequal: termOp = "<"; break;
         case ir_binop_equal: termOp = "!="; break;
         case ir_binop_nequal: termOp = "=="; break;
         default: break;
         }
         if (termOp != NULL)
         {
            term_expr->operands[0]->accept(this);
            buffer.asprintf_append(" %s ", termOp);
            term_expr->operands[1]->accept(this);
            handled = true;
         }

         // Unary logic not
         if (!handled && term_expr->operation == ir_unop_logic_not)
         {
            term_expr->operands[0]->accept(this);
            handled = true;
         }
      }

      // More complex condition, print as "!(x)"
      if (!handled)
      {
         buffer.asprintf_append("!(");
         term->ir->condition->accept(this);
         buffer.asprintf_append(")");
      }
   }
   buffer.asprintf_append("; ");

   // emit loop induction variable updates
   bool first = true;
   foreach_in_list(loop_variable, indvar, &ls->induction_variables)
   {
      if (indvar != printed_for_induction(ir, ls)) continue;
      induction_hash.insert(indvar->first_assignment);
      if (!first)
         buffer.asprintf_append(", ");
      visit(indvar->first_assignment);
      first = false;
   }
   buffer.asprintf_append(") {\n");


   // emit loop body
   indentation++; previous_skipped = false;
   foreach_in_list(ir_instruction, inst, &ir->body_instructions) {

      // skip termination & induction statements,
      // they are part of "for" clause
      if (terminator_hash.count(inst))
         continue;
      if (induction_hash.count(inst))
         continue;

      indent();
      inst->accept(this);
      end_statement_line();
   }
   indentation--;

   indent();
   buffer.asprintf_append("}");




   return true;
}


void
ir_print_metal_visitor::visit(ir_loop *ir)
{
   if (emit_canonical_for(ir))
      return;

   buffer.asprintf_append ("while (true) {\n");
   indentation++; previous_skipped = false;
   foreach_in_list(ir_instruction, inst, &ir->body_instructions) {
      indent();
      inst->accept(this);
      end_statement_line();
   }
   indentation--;
   indent();
   buffer.asprintf_append ("}");
}


void
ir_print_metal_visitor::visit(ir_loop_jump *ir)
{
   buffer.asprintf_append ("%s", ir->is_break() ? "break" : "continue");
}

void
ir_print_metal_visitor::visit(ir_emit_vertex *ir)
{
   buffer.asprintf_append ("emit-vertex-TODO");
}

void
ir_print_metal_visitor::visit(ir_end_primitive *ir)
{
   buffer.asprintf_append ("end-primitive-TODO");
}
