#ifndef VERTEX_FORMAT_HPP
#define VERTEX_FORMAT_HPP

/*

    Helpers for managing 3d mesh vertex and vertex attribute formats

*/

#include "../gl/glextutil.h"

#include <assert.h>

namespace VFMT {

template<int SIZE, GLenum GLTYPE>
struct ELEM_TYPE {
    static const int size = SIZE;
    static const GLenum gl_type = GLTYPE;
};

typedef ELEM_TYPE<1, GL_BYTE>           BYTE;
typedef ELEM_TYPE<1, GL_UNSIGNED_BYTE>  UBYTE;
typedef ELEM_TYPE<2, GL_SHORT>          SHORT;
typedef ELEM_TYPE<2, GL_UNSIGNED_SHORT> USHORT;
typedef ELEM_TYPE<2, GL_HALF_FLOAT>     HALF_FLOAT;
typedef ELEM_TYPE<4, GL_FLOAT>          FLOAT;
typedef ELEM_TYPE<4, GL_INT>            INT;
typedef ELEM_TYPE<4, GL_UNSIGNED_INT>   UINT;
typedef ELEM_TYPE<8, GL_DOUBLE>         DOUBLE;

struct ATTRIB_DESC {
    const char* name;
    const char* out_name;
    int         elem_size;
    int         count;
    GLenum      gl_type;
    bool        normalized;
};

template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_OUT_NAME>
class ATTRIB {
public:
    static const GLenum gl_type     = BASE_ELEM::gl_type;
    static const int elem_size      = BASE_ELEM::size;
    static const int count          = COUNT;
    static const bool normalized     = NORMALIZED;
    static const char* name;
    static const char* out_name;
};
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_OUT_NAME>
const char* ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_OUT_NAME>::name = STRING_NAME;
template<typename BASE_ELEM, int COUNT, bool NORMALIZED, const char* STRING_NAME, const char* STRING_OUT_NAME>
const char* ATTRIB<BASE_ELEM, COUNT, NORMALIZED, STRING_NAME, STRING_OUT_NAME>::out_name = STRING_OUT_NAME;

#define TYPEDEF_ATTRIB(ELEM, COUNT, NORMALIZED, NAME) \
    constexpr char NAME ## Name[] = #NAME; \
    constexpr char NAME ## OutName[] = "out_" #NAME; \
    typedef ATTRIB<ELEM, COUNT, NORMALIZED, NAME ## Name, NAME ## OutName> NAME; \
    inline ATTRIB_DESC createDesc ## NAME() { \
        ATTRIB_DESC desc; \
        desc.name = NAME ## Name; \
        desc.out_name = NAME ## OutName; \
        desc.elem_size = NAME::elem_size; \
        desc.count = NAME::count; \
        desc.gl_type = NAME::gl_type; \
        desc.normalized = NAME::normalized; \
        return desc; \
    } \
    static const ATTRIB_DESC NAME ## Desc = createDesc ## NAME();

TYPEDEF_ATTRIB(FLOAT, 3, false, Position);
TYPEDEF_ATTRIB(FLOAT, 2, false, UV);
TYPEDEF_ATTRIB(FLOAT, 2, false, UVLightmap);
TYPEDEF_ATTRIB(FLOAT, 3, false, Normal);
TYPEDEF_ATTRIB(FLOAT, 3, false, Tangent);
TYPEDEF_ATTRIB(FLOAT, 3, false, Bitangent);
TYPEDEF_ATTRIB(FLOAT, 4, false, BoneIndex4);
TYPEDEF_ATTRIB(FLOAT, 4, false, BoneWeight4);
TYPEDEF_ATTRIB(UBYTE, 4, true,  ColorRGBA);
TYPEDEF_ATTRIB(FLOAT, 3, false, Velocity);
TYPEDEF_ATTRIB(FLOAT, 1, false, TextUVLookup);

const int MAX_VERTEX_ATTRIBS = 32;

struct VERTEX_DESC {
    int attribCount;
    ATTRIB_DESC attribs[MAX_VERTEX_ATTRIBS];
};

template<int N, typename Arg, typename... Args>
class FORMAT;

template<int N, typename Arg>
class FORMAT<N, Arg> {
    static const ATTRIB_DESC desc;

protected:
    static VERTEX_DESC& generateVertexDesc(VERTEX_DESC& d) {
        d.attribs[N] = desc;
        d.attribCount = N + 1;
        return d;
    }
public:
    static int vertexSize() {
        return Arg::elem_size * Arg::count;
    }

    static int attribCount() {
        return 1;
    }

    static const ATTRIB_DESC& getAttribDesc(int idx) {
        if(idx == N) {
            return desc;
        } else {
            assert(false);
            return desc;
        }
    }

    static void makeAttribNameArray(const char** names) {
        names[N] = desc.name;
    }

    static void makeOutAttribNameArray(const char** names) {
        names[N] = desc.out_name;
    }

    static const VERTEX_DESC* getVertexDesc() {
      static VERTEX_DESC desc = generateVertexDesc(desc);
      return &desc;
    }
};
template<int N, typename Arg>
const ATTRIB_DESC FORMAT<N, Arg>::desc = { Arg::name, Arg::out_name, Arg::elem_size, Arg::count, Arg::gl_type, Arg::normalized };

template<int N, typename Arg, typename... Args>
class FORMAT : public FORMAT<N + 1, Args...> {
    static const ATTRIB_DESC desc;
    typedef FORMAT<N + 1, Args...> PARENT_T;

protected:
    static VERTEX_DESC& generateVertexDesc(VERTEX_DESC& d) {
        d.attribs[N] = desc;
        return PARENT_T::generateVertexDesc(d);
    }

public:
    static constexpr int attrib_count = sizeof...(Args) + 1;

    static int vertexSize() {
        return Arg::elem_size * Arg::count + PARENT_T::vertexSize();
    }

    static int attribCount() {
        return attrib_count;
    }

    static const ATTRIB_DESC& getAttribDesc(int idx) {
        if(idx == N) {
            return desc;
        } else {
            return PARENT_T::getAttribDesc(idx);
        }
    }

    static void makeAttribNameArray(const char** names) {
        names[N] = desc.name;
        PARENT_T::makeAttribNameArray(names);
    }

    static void makeOutAttribNameArray(const char** names) {
        names[N] = desc.out_name;
        PARENT_T::makeOutAttribNameArray(names);
    }

    static const VERTEX_DESC* getVertexDesc() {
        static VERTEX_DESC desc = generateVertexDesc(desc);
        return &desc;
    }
};
template<int N, typename Arg, typename... Args>
const ATTRIB_DESC FORMAT<N, Arg, Args...>::desc = { Arg::name, Arg::out_name, Arg::elem_size, Arg::count, Arg::gl_type, Arg::normalized };

#define DECL_VERTEX_FMT(NAME, ...) \
    struct ENUM_ ## NAME { enum { __VA_ARGS__ }; }; \
    typedef FORMAT<0, __VA_ARGS__> NAME;

DECL_VERTEX_FMT(GENERIC,
    Position, UV, UVLightmap,
    Normal, Tangent, Bitangent,
    BoneIndex4, BoneWeight4,
    ColorRGBA
);
DECL_VERTEX_FMT(PARTICLE,
    Position, Velocity, ColorRGBA
);
DECL_VERTEX_FMT(TEXT,
    Position, UV, TextUVLookup, ColorRGBA
);
DECL_VERTEX_FMT(QUAD_2D,
    Position, UV
);
DECL_VERTEX_FMT(LINE,
    Position, ColorRGBA
);

}

#endif

