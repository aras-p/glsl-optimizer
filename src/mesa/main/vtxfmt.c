#include "glheader.h"
#include "api_loopback.h"
#include "mtypes.h"
#include "vtxfmt.h"



static void install_vtxfmt( struct _glapi_table *tab, GLvertexformat *vfmt )
{
   tab->ArrayElement = vfmt->ArrayElement;
   tab->Color3f = vfmt->Color3f;
   tab->Color3fv = vfmt->Color3fv;
   tab->Color3ub = vfmt->Color3ub;
   tab->Color3ubv = vfmt->Color3ubv;
   tab->Color4f = vfmt->Color4f;
   tab->Color4fv = vfmt->Color4fv;
   tab->Color4ub = vfmt->Color4ub;
   tab->Color4ubv = vfmt->Color4ubv;
   tab->EdgeFlag = vfmt->EdgeFlag;
   tab->EdgeFlagv = vfmt->EdgeFlagv;
   tab->EvalCoord1f = vfmt->EvalCoord1f;
   tab->EvalCoord1fv = vfmt->EvalCoord1fv;
   tab->EvalCoord2f = vfmt->EvalCoord2f;
   tab->EvalCoord2fv = vfmt->EvalCoord2fv;
   tab->EvalPoint1 = vfmt->EvalPoint1;
   tab->EvalPoint2 = vfmt->EvalPoint2;
   tab->FogCoordfEXT = vfmt->FogCoordfEXT;
   tab->FogCoordfvEXT = vfmt->FogCoordfvEXT;
   tab->Indexi = vfmt->Indexi;
   tab->Indexiv = vfmt->Indexiv;
   tab->Materialfv = vfmt->Materialfv;
   tab->MultiTexCoord1fARB = vfmt->MultiTexCoord1fARB;
   tab->MultiTexCoord1fvARB = vfmt->MultiTexCoord1fvARB;
   tab->MultiTexCoord2fARB = vfmt->MultiTexCoord2fARB;
   tab->MultiTexCoord2fvARB = vfmt->MultiTexCoord2fvARB;
   tab->MultiTexCoord3fARB = vfmt->MultiTexCoord3fARB;
   tab->MultiTexCoord3fvARB = vfmt->MultiTexCoord3fvARB;
   tab->MultiTexCoord4fARB = vfmt->MultiTexCoord4fARB;
   tab->MultiTexCoord4fvARB = vfmt->MultiTexCoord4fvARB;
   tab->Normal3f = vfmt->Normal3f;
   tab->Normal3fv = vfmt->Normal3fv;
   tab->SecondaryColor3fEXT = vfmt->SecondaryColor3fEXT;
   tab->SecondaryColor3fvEXT = vfmt->SecondaryColor3fvEXT;
   tab->SecondaryColor3ubEXT = vfmt->SecondaryColor3ubEXT;
   tab->SecondaryColor3ubvEXT = vfmt->SecondaryColor3ubvEXT;
   tab->TexCoord1f = vfmt->TexCoord1f;
   tab->TexCoord1fv = vfmt->TexCoord1fv;
   tab->TexCoord2f = vfmt->TexCoord2f;
   tab->TexCoord2fv = vfmt->TexCoord2fv;
   tab->TexCoord3f = vfmt->TexCoord3f;
   tab->TexCoord3fv = vfmt->TexCoord3fv;
   tab->TexCoord4f = vfmt->TexCoord4f;
   tab->TexCoord4fv = vfmt->TexCoord4fv;
   tab->Vertex2f = vfmt->Vertex2f;
   tab->Vertex2fv = vfmt->Vertex2fv;
   tab->Vertex3f = vfmt->Vertex3f;
   tab->Vertex3fv = vfmt->Vertex3fv;
   tab->Vertex4f = vfmt->Vertex4f;
   tab->Vertex4fv = vfmt->Vertex4fv;
   tab->Begin = vfmt->Begin;
   tab->End = vfmt->End;
   
/*     tab->NewList = vfmt->NewList; */
   tab->CallList = vfmt->CallList;

   tab->Rectf = vfmt->Rectf;
   tab->DrawArrays = vfmt->DrawArrays;
   tab->DrawElements = vfmt->DrawElements;
   tab->DrawRangeElements = vfmt->DrawRangeElements;
   tab->EvalMesh1 = vfmt->EvalMesh1;
   tab->EvalMesh2 = vfmt->EvalMesh2;
}


void _mesa_install_exec_vtxfmt( GLcontext *ctx, GLvertexformat *vfmt )
{
   install_vtxfmt( ctx->Exec, vfmt );
   if (ctx->ExecPrefersFloat != vfmt->prefer_float_colors)
      _mesa_loopback_prefer_float( ctx->Exec, vfmt->prefer_float_colors );
}


void _mesa_install_save_vtxfmt( GLcontext *ctx, GLvertexformat *vfmt )
{
   install_vtxfmt( ctx->Save, vfmt );
   if (ctx->SavePrefersFloat != vfmt->prefer_float_colors)
      _mesa_loopback_prefer_float( ctx->Save, vfmt->prefer_float_colors );
}

