/* XXX put a copyright here */

/* I know my style's weird, get used to it */

static const char* r300_get_vendor(struct pipe_screen* pscreen) {
    return "X.Org R300 Project";
}

static const char* r300_get_name(struct pipe_screen* pscreen) {
    /* XXX lazy */
    return "unknown";
}

static int r300_get_param(struct pipe_screen* pscreen, int param) {
    switch (param) {
        /* Cases marked "IN THEORY" are possible on the hardware,
         * but haven't been implemented yet. */
        case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
            /* XXX I'm told this goes up to 16 */
            return 8;
        case PIPE_CAP_NPOT_TEXTURES:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_S3TC:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_TWO_SIDED_STENCIL:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_ANISOTROPIC_FILTER:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_POINT_SPRITE:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_OCCLUSION_QUERY:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_TEXTURE_SHADOW_MAP:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_GLSL:
            /* IN THEORY */
            return 0;
        case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
            /* 12 == 2048x2048
             * R500 can do 4096x4096 */
            return 12;
        case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
            /* XXX educated guess */
            return 8;
        case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            /* XXX educated guess */
            return 11;
        case PIPE_CAP_MAX_RENDER_TARGETS:
            /* XXX 4 eventually */
            return 1;
        default:
            return 0;
    }
}

static float r300_get_paramf(struct pipe_screen* pscreen, float param) {
    switch (param) {
        case PIPE_CAP_MAX_LINE_WIDTH:
        case PIPE_CAP_MAX_LINE_WIDTH_AA:
            /* XXX look this up, lazy ass! */
            return 0.0;
        case PIPE_CAP_MAX_POINT_WIDTH:
        case PIPE_CAP_MAX_POINT_WIDTH_AA:
            /* XXX see above */
            return 255.0;
        case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
            return 16.0;
        case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
            /* XXX again... */
            return 16.0;
        default:
            return 0.0;
    }
}

static boolean r300_is_format_supported(struct pipe_screen* pscreen,
                                        enum pipe_format format, uint type)
{
    return FALSE;
}

static r300_destroy_screen(struct pipe_screen* pscreen) {
    FREE(pscreen);
}

struct pipe_screen* r300_create_screen(struct pipe_winsys* winsys, uint pci_id) {
    struct r300_screen* r300screen = CALLOC_STRUCT(r300_screen);

    if (!r300screen)
        return NULL;

    /* XXX break this into its own function? */
    switch (pci_id) {
        default:
            debug_printf("%s: unknown PCI ID 0x%x, cannot create screen!\n",
                         __FUNCTION__, pci_id);
            return NULL;
    }

    r300screen->pci_id = pci_id;
    r300screen->screen.winsys = winsys;
    r300screen->screen.destroy = r300_destroy_screen;
    r300screen->screen.get_name = r300_get_name;
    r300screen->screen.get_vendor = r300_get_vendor;
    r300screen->screen.get_param = r300_get_param;
    r300screen->screen.get_paramf = r300_get_paramf;
    r300screen->screen.is_format_supported = r300_is_format_supported;
    r300screen->screen.surface_map = r300_surface_map;
    r300screen->screen.surface_unmap = r300_surface_unmap;

    return &r300screen->screen;
}
