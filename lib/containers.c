#ifndef __CONTAINERS_C
#define __CONTAINERS_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define __CONTAINERS__hard_cast(type, value) \
    ((union { typeof(type) a; typeof(value) b; }) { .b = value }.a)
#define __CONTAINERS__pick_c(a, b, c, ...) c

typedef struct {
    char* func;
    int line;
} ErrorTrace;

#define ErrorTrace() ((ErrorTrace) { (void*) __func__, __LINE__ })

typedef struct {
    void (*print)(void* self, FILE* out);
    char* message;
    ErrorTrace trace;
} Error;

void __CONTAINERS__Error__print(void* self, FILE* out) {
    Error* error = (Error*) self;
    fprintf(out, "\33[31merror\33[0m at %s:%d: %s\n",
        error->trace.func, error->trace.line, error->message);
}

#define Error(message) ({ \
    /* indirection prevents bug `error: initializer element is not a compile-time constant` */ \
    Error __Error = { &__CONTAINERS__Error__print, message, ErrorTrace() }; __Error; })

void __CONTAINERS__panic_footer(ErrorTrace trace) {
    fprintf(stderr, "\33[31;2mpanicked at %s() [line %d]\33[0m\n",
        trace.func, trace.line);
    exit(1);
}

#define panic(error) ({ \
    typeof(error) __panic = (error); \
    __CONTAINERS__hard_cast(void (*)(void*, FILE*), __panic)(&__panic, stderr); \
    __CONTAINERS__panic_footer(ErrorTrace()); })
#define panicf(f...) (fprintf(stderr, "\33[31merror:\33[0m " f), __CONTAINERS__panic_footer(ErrorTrace()))

typedef struct {} __CONTAINERS__Option;
typedef struct {} __CONTAINERS__Result;
typedef void* Option;
typedef void* Result;
typedef int Void[0];

#define Option(T) typeof(struct { __CONTAINERS__Option s; T t; int e; }*)
#define __CONTAINERS__Result(T, E, ...) typeof(struct { __CONTAINERS__Result s; T t; E e; }*)
#define Result(T...) __CONTAINERS__Result(T, Error)

#define Some(x) ((Option) (typeof(x)[1]) { x })
#define None ((Option) 0)
#define Ok(x...) ((Result) __CONTAINERS__pick_c(x, (int[1]) { 1 }, (&(struct { int a; typeof(x) b[1]; }) { 1, x })))
#define Err(x) ((Result) &(struct { int a; typeof(x) b[1]; }) { 0, x })

#define Somet(x) ((Option(typeof(x))) Some(x))
#define Nonet(T) ((Option(T)) None)
#define Okt(E, x...) ((Result(__CONTAINERS__pick_c(x, Void, typeof(x)), E)) Ok(x))
#define Errt(T, x) ((Result(T, typeof(x))) Err(x))

#define is_some(x) (!!(x))
#define is_none(x) (!(x))
#define is_ok(x) (*(int*)(void*)(x))
#define is_err(x) (!*(int*)(void*)(x))

#define ounwrap_unchecked(x) (*(typeof((x)->t)*)(void*)(x))
#define runwrap_unchecked(x) (((struct { int a; typeof((x)->t) b; }*)(void*)(x))->b)
#define unwrap_unchecked(x) _Generic((x)->s, \
    __CONTAINERS__Option: ounwrap_unchecked(x), \
    __CONTAINERS__Result: runwrap_unchecked(x))

#define runwrap_err_unchecked(x) (((struct { int a; typeof((x)->e) b; }*)(void*)(x))->b)
#define unwrap_err_unchecked(x) runwrap_err_unchecked(x)

#define oexpectf(x, f...) ({ \
    typeof(x) __oexpect = (void*)(x); \
    if(is_none(__oexpect)) panicf(f); \
    ounwrap_unchecked(__oexpect); })
#define rexpectf(x, f...) ({ \
    typeof(x) __rexpect = (void*)(x); \
    if(is_err(__rexpect)) panicf(f); \
    runwrap_unchecked(__rexpect); })
#define expectf(x, f...) _Generic((x)->s, \
    __CONTAINERS__Option: oexpectf(x, f), \
    __CONTAINERS__Result: rexpectf(x, f))

#define ounwrap(x) ({ \
    typeof(x) __ounwrap = (void*)(x); \
    if(is_none(__ounwrap)) panicf("called unwrap() on None"); \
    ounwrap_unchecked(__ounwrap); })
#define runwrap(x) ({ \
    typeof(x) __runwrap = (void*)(x); \
    if(is_err(__runwrap)) panic(runwrap_err_unchecked(__runwrap)); \
    runwrap_unchecked(__runwrap); })
#define unwrap(x) _Generic((x)->s, \
    __CONTAINERS__Option: ounwrap(x), \
    __CONTAINERS__Result: runwrap(x))

#define runwrap_err(x) ({ \
    typeof(x) __runwrap_err = (void*)(x); \
    if(is_ok(__runwrap_err)) panicf("called unwrap_err() on Ok"); \
    runwrap_err_unchecked(__runwrap_err); })
#define unwrap_err(x) runwrap_err(x)

#define ounwrap_or(x, default) ({ \
    typeof(x) __ounwrap_or = (void*)(x); \
    is_some(__ounwrap_or) ? ounwrap_unchecked(__ounwrap_or) : (default); })
#define runwrap_or(x, default) ({ \
    typeof(x) __runwrap_or = (void*)(x); \
    is_ok(__runwrap_or) ? runwrap_unchecked(__runwrap_or) : (default); })
#define unwrap_or(x, default) _Generic((x)->s, \
    __CONTAINERS__Option: ounwrap_or(x, default), \
    __CONTAINERS__Result: runwrap_or(x, default))

#define othrow(x) ({ \
    typeof(x) __othrow = (void*)(x); \
    if(is_none(__othrow)) return None; \
    ounwrap_unchecked(__othrow); })
#define rthrow(x) ({ \
    typeof(x) __rthrow = (void*)(x); \
    if(is_err(__rthrow)) return Err(runwrap_err_unchecked(__rthrow)); \
    runwrap_unchecked(__rthrow); })
#define throw(x) _Generic((x)->s, \
    __CONTAINERS__Option: othrow(x), \
    __CONTAINERS__Result: rthrow(x))

#define orescope(x) ({ \
    typeof(x) __orescope = (void*)(x); \
    is_some(__orescope) ? Some(ounwrap_unchecked(__orescope)) : None; })
#define rrescope(x) ({ \
    typeof(x) __rrescope = (void*)(x); \
    is_ok(__rrescope) ? Ok(runwrap_unchecked(__rrescope)) : Err(runwrap_err_unchecked(__rrescope)); })
#define rescope(x) _Generic((x)->s, \
    __CONTAINERS__Option: orescope(x), \
    __CONTAINERS__Result: rrescope(x))

#define ook_or(x, error) ({ \
    typeof(x) __ook_or = (void*)(x); \
    is_some(__ook_or) ? Ok(ounwrap_unchecked(__ook_or)) : Err(error); })
#define rok_or(x, error) ({ \
    typeof(x) __rok_or = (void*)(x); \
    is_ok(__rok_or) ? Ok(runwrap_unchecked(__rok_or)) : Err(error); })
#define ok_or(x, error) _Generic((x)->s, \
    __CONTAINERS__Option: ook_or(x, error), \
    __CONTAINERS__Result: rok_or(x, error))

#define rok(x) ({ \
    typeof(x) __rok = (void*)(x); \
    is_ok(__rok) ? Some(runwrap_unchecked(__rok)) : None; })
#define ok(x) rok(x)

#define rerr(x) ({ \
    typeof(x) __rerr = (void*)(x); \
    is_err(__rerr) ? Some(runwrap_err_unchecked(__rerr)) : None; })
#define err(x) rerr(x)

#define __CONTAINERS__if_let_Some(var, x) \
    for(typeof(x) __if_let = (void*)(x), *__cond = (void*) 1; __cond; __cond = (void*) 0) if(is_some(__if_let)) \
    for(typeof(__if_let->t) var = ounwrap_unchecked(__if_let); __cond; __cond = (void*) 0)
#define __CONTAINERS__choose_Some(...) __CONTAINERS__if_let_Some
#define __CONTAINERS__var_Some(var) var

#define __CONTAINERS__if_let_Ok(var, x) \
    for(typeof(x) __if_let = (void*)(x); __if_let; __if_let = (void*) 0) if(is_ok(__if_let)) \
    for(typeof(__if_let->t) var = runwrap_unchecked(__if_let); __if_let; __if_let = (void*) 0)
#define __CONTAINERS__choose_Ok(...) __CONTAINERS__if_let_Ok
#define __CONTAINERS__var_Ok(var) var

#define __CONTAINERS__if_let_Err(var, x) \
    for(typeof(x) __if_let = (void*)(x); __if_let; __if_let = (void*) 0) if(is_err(__if_let)) \
    for(typeof(__if_let->e) var = runwrap_err_unchecked(__if_let); __if_let; __if_let = (void*) 0)
#define __CONTAINERS__choose_Err(...) __CONTAINERS__if_let_Err
#define __CONTAINERS__var_Err(var) var

#define if_let(var, x) __CONTAINERS__choose_ ## var (__CONTAINERS__var_ ## var, x)

#define owrap_if(x, decl, cond) ({ \
    typeof(x) owrap_if = (x); \
    decl = owrap_if; (cond) ? Some(owrap_if) : None; })
#define __CONTAINERS__owrap_if_voidptr(x) owrap_if(x, void* __owrap_with, __owrap_with != NULL)
#define __CONTAINERS__owrap_if_int(x) owrap_if(x, int __owrap_with, __owrap_with == 0)
#define owrap(x) _Generic((x), \
    void*: __CONTAINERS__owrap_if_voidptr(__CONTAINERS__hard_cast(void*, x)), \
    int: __CONTAINERS__owrap_if_int(__CONTAINERS__hard_cast(int, x)), \
    default: owrap_if(x, typeof(x) __owrap_with, __owrap_with))

#define rwrap_if_with(x, decl, cond, err) ({ \
    typeof(x) __rwrap_if_with = (x); \
    decl = __rwrap_if_with; (cond) ? Ok(__rwrap_if_with) : Err(err); })
#define rwrap_if(x, decl, cond) rwrap_if_with(x, decl, cond, Error(strerror(errno)))
#define __CONTAINERS__rwrap_if_with_voidptr(x, err) rwrap_if_with(x, void* __rwrap_with, __rwrap_with != NULL, err)
#define __CONTAINERS__rwrap_if_with_int(x, err) rwrap_if_with(x, int __rwrap_with, __rwrap_with == 0, err)
#define rwrap_with(x, err) _Generic((x), \
    void*: __CONTAINERS__rwrap_if_with_voidptr(__CONTAINERS__hard_cast(void*, x), err), \
    int: __CONTAINERS__rwrap_if_with_int(__CONTAINERS__hard_cast(int, x), err), \
    default: rwrap_if_with(x, typeof(x) __rwrap_with, __rwrap_with, err))
#define rwrap(x) rwrap_with(x, Error(strerror(errno)))

#define owrap_ift(x, decl, cond) ((Option(typeof(x))) owrap_if(x, decl, cond))
#define owrapt(x) ((Option(typeof(x))) owrap(x))

#define rwrap_if_witht(x, decl, cond, err) ((Result(typeof(x), typeof(err))) rwrap_if_with(x, decl, cond, err))
#define rwrap_ift(x, decl, cond) ((Result(typeof(x), Error)) rwrap_if(x, decl, cond))
#define rwrap_witht(x, err) ((Result(typeof(x), typeof(err))) rwrap_with(x, err))
#define rwrapt(x) ((Result(typeof(x), Error)) rwrap(x))

#define malloco(x...) __CONTAINERS__owrap_if_voidptr(malloc(x))
#define mallocou(x...) ounwrap((Option(void*)) malloco(x))
#define mallocot(x...) othrow((Option(void*)) malloco(x))
#define mallocr(x...) __CONTAINERS__rwrap_if_with_voidptr(malloc(x), Error(strerror(errno)))
#define mallocru(x...) runwrap((Result(void*, Error)) mallocr(x))
#define mallocrt(x...) rthrow((Result(void*, Error)) mallocr(x))
#define mallocu(x...) mallocru(x)
#define malloct(x...) mallocrt(x)

#define calloco(x...) __CONTAINERS__owrap_if_voidptr(calloc(x))
#define callocou(x...) ounwrap((Option(void*)) calloco(x))
#define callocot(x...) othrow((Option(void*)) calloco(x))
#define callocr(x...) __CONTAINERS__rwrap_if_with_voidptr(calloc(x), Error(strerror(errno)))
#define callocru(x...) runwrap((Result(void*, Error)) callocr(x))
#define callocrt(x...) rthrow((Result(void*, Error)) callocr(x))
#define callocu(x...) callocru(x)
#define calloct(x...) callocrt(x)

#define realloco(x...) __CONTAINERS__owrap_if_voidptr(realloc(x))
#define reallocou(x...) ounwrap((Option(void*)) realloco(x))
#define reallocot(x...) othrow((Option(void*)) realloco(x))
#define reallocr(x...) __CONTAINERS__rwrap_if_with_voidptr(realloc(x), Error(strerror(errno)))
#define reallocru(x...) runwrap((Result(void*, Error)) reallocr(x))
#define reallocrt(x...) rthrow((Result(void*, Error)) reallocr(x))
#define reallocu(x...) reallocru(x)
#define realloct(x...) reallocrt(x)

#define fopeno(x...) __CONTAINERS__owrap_if_voidptr(fopen(x))
#define fopenou(x...) ounwrap((Option(FILE*)) fopeno(x))
#define fopenot(x...) othrow((Option(FILE*)) fopeno(x))
#define fopenr(x...) __CONTAINERS__rwrap_if_with_voidptr(fopen(x), Error(strerror(errno)))
#define fopenru(x...) runwrap((Result(FILE*, Error)) fopenr(x))
#define fopenrt(x...) rthrow((Result(FILE*, Error)) fopenr(x))
#define fopenu(x...) fopenru(x)
#define fopent(x...) fopenrt(x)

#define fcloseo(x...) owrap_if(fclose(x), int __fclose, __fclose != EOF)
#define fcloseou(x...) ounwrap((Option(int)) fcloseo(x))
#define fcloseot(x...) othrow((Option(int)) fcloseo(x))
#define fcloser(x...) rwrap_if(fclose(x), int __fclose, __fclose != EOF)
#define fcloseru(x...) runwrap((Result(int, Error)) fcloser(x))
#define fclosert(x...) rthrow((Result(int, Error)) fcloser(x))
#define fcloseu(x...) fcloseru(x)
#define fcloset(x...) fclosert(x)

#define freado(x...) owrap_if(fread(x), size_t __fread, __fread != 0)
#define freadou(x...) ounwrap((Option(size_t)) freado(x))
#define freadot(x...) othrow((Option(size_t)) freado(x))
#define freadr(x...) rwrap_if(fread(x), size_t __fread, __fread != 0)
#define freadru(x...) runwrap((Result(size_t, Error)) freadr(x))
#define freadrt(x...) rthrow((Result(size_t, Error)) freadr(x))
#define freadu(x...) freadru(x)
#define freadt(x...) freadrt(x)

#define fwriteo(ptr, size, nitems, stream) ({ \
    size_t __nitems = (nitems); \
    owrap_if(fwrite((ptr), (size), __nitems, (stream)), size_t __fwrite, __fwrite == __nitems); })
#define fwriteou(x...) ounwrap((Option(size_t)) fwriteo(x))
#define fwriteot(x...) othrow((Option(size_t)) fwriteo(x))
#define fwriter(ptr, size, nitems, stream) ({ \
    size_t __nitems = (nitems); \
    rwrap_if(fwrite((ptr), (size), __nitems, (stream)), size_t __fwrite, __fwrite == __nitems); })
#define fwriteru(x...) runwrap((Result(size_t, Error)) fwriter(x))
#define fwritert(x...) rthrow((Result(size_t, Error)) fwriter(x))
#define fwriteu(x...) fwriteru(x)
#define fwritet(x...) fwritert(x)

#define fseek_o(x...) __CONTAINERS__owrap_if_int(fseek(x))
#define fseek_ou(x...) ounwrap((Option(int)) fseek_o(x))
#define fseek_ot(x...) othrow((Option(int)) fseek_o(x))
#define fseekr(x...) __CONTAINERS__rwrap_if_with_int(fseek(x), Error(strerror(errno)))
#define fseekru(x...) runwrap((Result(int, Error)) fseekr(x))
#define fseekrt(x...) rthrow((Result(int, Error)) fseekr(x))
#define fseeku(x...) fseekru(x)
#define fseekt(x...) fseekrt(x)

#define ftello(x...) owrap_if(ftell(x), long __ftell, __ftell != -1)
#define ftellou(x...) ounwrap((Option(long)) ftello(x))
#define ftellot(x...) othrow((Option(long)) ftello(x))
#define ftellr(x...) rwrap_if(ftell(x), long __ftell, __ftell != -1)
#define ftellru(x...) runwrap((Result(long, Error)) ftellr(x))
#define ftellrt(x...) rthrow((Result(long, Error)) ftellr(x))
#define ftellu(x...) ftellru(x)
#define ftellt(x...) ftellrt(x)

#endif