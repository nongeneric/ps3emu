#define SHADER_DIR SYS_APP_HOME "/shader/"

#ifdef FRAMGNET_SHADER_NAME
#define VERTEX_SHADER(name)
#define FRAGMENT_SHADER(name) SHADER_DIR #name ".fpo",
#endif

#ifdef FRAMGNET_SHADER_ID
#define VERTEX_SHADER(name)
#define FRAGMENT_SHADER(name) FRAGMENT_SHADER_ ## name,
#endif

#ifdef VERTEX_SHADER_NAME
#define VERTEX_SHADER(name) SHADER_DIR #name ".vpo",
#define FRAGMENT_SHADER(name)
#endif

#ifdef VERTEX_SHADER_ID
#define VERTEX_SHADER(name) VERTEX_SHADER_ ## name,
#define FRAGMENT_SHADER(name)
#endif

// definition of shader
FRAGMENT_SHADER(fnop)
FRAGMENT_SHADER(ds2xaccuview_shifted)
FRAGMENT_SHADER(ds2xqc)
FRAGMENT_SHADER(ds2xqc_alt)
FRAGMENT_SHADER(fpshader)

VERTEX_SHADER(vpallquad)
VERTEX_SHADER(vpshader)

#undef SHADER_DIR
#undef VERTEX_SHADER
#undef FRAGMENT_SHADER