

#ifndef __RYOS_MODULE_H__
#define __RYOS_MODULE_H__



#ifdef RY_USING_MODULE
struct ry_module_symtab
{
    void       *addr;
    const char *name;
};

#if defined(_MSC_VER)
#pragma section("RYMSymTab$f",read)
#define RYM_EXPORT(symbol)                                            \
__declspec(allocate("RYMSymTab$f"))const char __rymsym_##symbol##_name[] = "__vs_rym_"#symbol;
#pragma comment(linker, "/merge:RYMSymTab=mytext")

#elif defined(__MINGW32__)
#define RYM_EXPORT(symbol)

#else
#define RYM_EXPORT(symbol)                                            \
const char __rymsym_##symbol##_name[] SECTION(".rodata.name") = #symbol;     \
const struct ry_module_symtab __rymsym_##symbol SECTION("RYMSymTab")= \
{                                                                     \
    (void *)&symbol,                                                  \
    __rymsym_##symbol##_name                                          \
};
#endif

#else
#define RYM_EXPORT(symbol)
#endif

#endif

