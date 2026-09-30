// ============================================================================
// primitives.slang — procedural mesh-shader geometry for Horde's blockout shapes
// ============================================================================
//
// Every shape here (capsule / plane / box) is generated entirely on the GPU by
// its mesh shader — there is no vertex or index buffer anywhere. Each mesh
// shader reads a handful of scalar parameters (radius/height, extent, ...)
// from its push constant, and computes vertex positions/normals/UVs directly
// from those, in local (object) space, transforming into clip space with the
// per-draw `mvp` matrix (model * view * projection, already combined on the
// CPU side — see WorldComponents.h's ToMatrix() and whatever multiplies it by
// the camera's view*proj).
//
// A single shared fragment shader (FS_Main, at the bottom of this file) is
// used by all three shapes — it does the same simple, no-texture Phong
// shading Cube.slang's FS_Main does, just reading a per-draw `colour` instead
// of sampling a texture. UVs are still generated and threaded all the way
// through to the fragment shader even though nothing reads them yet — they're
// there so a texture can be dropped in later without touching the mesh
// shaders again.
//
// Matrix/vector convention matches the rest of the engine (see Cube.slang):
// row-major, row-vector — a position is transformed with `mul(position, M)`,
// not `mul(M, position)`.

// ----------------------------------------------------------------------------
// Shared vertex output
// ----------------------------------------------------------------------------
// What every mesh shader below hands to the rasterizer/fragment shader per
// vertex. Same fields as Cube.slang's VertexOutput, minus the reliance on a
// face-colour lookup table -- colour now comes from the push constant instead
// (see each shape's DrawData struct), since Horde entities can be the player,
// an enemy, or a generic prop, and coloured on a per-entity basis rather than
// a fixed per-face palette.
struct MeshVertexOutput
{
    float4 position : SV_Position;
    float3 normal   : NORMAL0;
    float3 colour   : COLOR0;
    float2 uv       : TEXCOORD0;   // unused by FS_Main today -- reserved for future texturing.
};

// ============================================================================
// Capsule
// ============================================================================
//
// A capsule is a cylinder capped with two hemispheres. To keep this cheap
// enough to build in a single mesh-shader workgroup with no amplification
// stage, the hemispheres are approximated with just one intermediate ring of
// latitude each (plus a single apex point) rather than a fully subdivided
// sphere -- more than enough detail for a blockout character silhouette.
//
// Layout (8 radial segments around the capsule's long axis, which is +Y):
//
//                    apex_top (1 vertex)
//                   /    |    \
//              top_lat[0..7]  (8 vertices, upper hemisphere's one extra ring)
//                   \    |    /
//              top_equator[0..7]  (8 vertices -- also the cylinder's top rim)
//                   |    |    |
//              bottom_equator[0..7]  (8 vertices -- cylinder's bottom rim)
//                   /    |    \
//              bottom_lat[0..7]  (8 vertices, lower hemisphere's one extra ring)
//                   \    |    /
//                    apex_bottom (1 vertex)
//
// Total vertices: 1 + 8 + 8 + 8 + 8 + 1 = 34
// Total triangles:
//   - bottom apex fan:     8
//   - bottom hemisphere band (apex-ring -> equator-ring): 8 quads * 2 = 16
//   - cylinder body (bottom equator -> top equator):       8 quads * 2 = 16
//   - top hemisphere band (equator-ring -> apex-ring):      8 quads * 2 = 16
//   - top apex fan:         8
//   Total: 8 + 16 + 16 + 16 + 8 = 64
//
// Ring math: for radial segment i (0..7), angle = 2*PI*i/8. A ring vertex at
// "latitude" phi (measured from the equator) sits at:
//     x = radius * cos(phi) * cos(angle)
//     y = <ring's fixed height offset>
//     z = radius * cos(phi) * sin(angle)
// with its outward normal (for the curved/rounded part of the capsule) being
// the same direction, just with y replaced by sin(phi):
//     normal = normalize(cos(phi)*cos(angle), sin(phi), cos(phi)*sin(angle))
//
// The two equator rings (top_equator / bottom_equator) are phi=0 -- note this
// means their normal has no Y component at all, which is exactly correct
// both for the sphere-latitude formula above AND for the cylinder side wall
// meeting them -- one ring, two adjoining surfaces, zero compromise on
// normals either way.
//
// The two "lat" rings (top_lat / bottom_lat) sit at a fixed intermediate
// latitude of 45 degrees (cos(45)==sin(45)==1/sqrt(2)) -- chosen purely
// because it visually reads as "a rounded cap" with a single extra ring; it's
// not a physically exact hemisphere, just a cheap, good-enough approximation.

struct CapsuleDrawData
{
    float4x4 mvp;       // model * view * projection for this entity, already combined by the caller.
    float3   colour;    // flat per-entity colour (player/enemy/prop -- see HordeRenderer).
    float    radius;    // CapsuleRenderComponent::radius.
    float    height;    // CapsuleRenderComponent::height -- the CYLINDER's height, not including the rounded caps.
};

[[vk::push_constant]]
ConstantBuffer<CapsuleDrawData> g_capsule_data;

// Number of vertices/triangles/radial segments, kept as named constants so
// the loop math below and the numthreads/output-count declarations can't
// silently drift out of sync with each other.
static const uint kCapsuleSegments   = 8;
static const uint kCapsuleVertCount  = 2 + kCapsuleSegments * 4;   // 2 apexes + 4 rings of 8 = 34
static const uint kCapsuleTriCount   = kCapsuleSegments * 8;       // 5 bands worth of triangles = 64
static const float kPi               = 3.14159265358979f;
static const float kCapLatitude      = kPi * 0.25f;                // 45 degrees -- see comment above.

// Index of the first vertex in each ring, matching the layout diagram above.
static const uint kApexBottomIndex     = 0;
static const uint kBottomLatBase       = 1;                         // 1..8
static const uint kBottomEquatorBase   = 1 + kCapsuleSegments;      // 9..16
static const uint kTopEquatorBase      = 1 + kCapsuleSegments * 2;  // 17..24
static const uint kTopLatBase          = 1 + kCapsuleSegments * 3;  // 25..32
static const uint kApexTopIndex        = 1 + kCapsuleSegments * 4;  // 33

// Builds one ring vertex at radial segment `i`, height offset `y`, and
// latitude `phi` (0 for the two equator rings, kCapLatitude for the two cap
// rings). `phi` only affects the Y component of the position/normal -- the
// X/Z "radius" shrinks by cos(phi) so the cap rings pull inward as they
// curve up/down toward the apex, same as lines of latitude on a globe.
MeshVertexOutput CapsuleRingVertex(uint i, float y, float phi, float v_coord)
{
    const float angle = 2.0f * kPi * float(i) / float(kCapsuleSegments);
    const float ring_radius = cos(phi);
    const float3 local_normal = float3(ring_radius * cos(angle), sin(phi), ring_radius * sin(angle));

    MeshVertexOutput v;
    v.position = mul(float4(local_normal.x * g_capsule_data.radius,
                             y,
                             local_normal.z * g_capsule_data.radius, 1.0f), g_capsule_data.mvp);
    v.normal = local_normal;
    v.colour = g_capsule_data.colour;
    // v_coord is a fixed value per ring (0.0 at the bottom apex, 1.0 at the
    // top apex) rather than something derived from radius/height -- simpler,
    // and avoids a division that would be by zero for a zero-height capsule.
    v.uv = float2(float(i) / float(kCapsuleSegments), v_coord);
    return v;
}

// The two apex vertices are simple special cases of the ring formula with
// zero radius (a single point), so they get their own tiny helper rather
// than shoehorning them through CapsuleRingVertex.
MeshVertexOutput CapsuleApexVertex(float y, float normal_y)
{
    MeshVertexOutput v;
    v.position = mul(float4(0.0f, y, 0.0f, 1.0f), g_capsule_data.mvp);
    v.normal = float3(0.0f, normal_y, 0.0f);
    v.colour = g_capsule_data.colour;
    v.uv = float2(0.5f, normal_y < 0.0f ? 0.0f : 1.0f);
    return v;
}

// Computes vertex `idx` (0..33) by figuring out which ring it falls in.
MeshVertexOutput ComputeCapsuleVertex(uint idx)
{
    const float half_height = g_capsule_data.height * 0.5f;

    if (idx == kApexBottomIndex)
        return CapsuleApexVertex(-(half_height + g_capsule_data.radius), -1.0f);

    if (idx == kApexTopIndex)
        return CapsuleApexVertex(half_height + g_capsule_data.radius, 1.0f);

    if (idx >= kBottomLatBase && idx < kBottomLatBase + kCapsuleSegments)
        return CapsuleRingVertex(idx - kBottomLatBase, -half_height - g_capsule_data.radius * sin(kCapLatitude), -kCapLatitude, 0.2f);

    if (idx >= kBottomEquatorBase && idx < kBottomEquatorBase + kCapsuleSegments)
        return CapsuleRingVertex(idx - kBottomEquatorBase, -half_height, 0.0f, 0.4f);

    if (idx >= kTopEquatorBase && idx < kTopEquatorBase + kCapsuleSegments)
        return CapsuleRingVertex(idx - kTopEquatorBase, half_height, 0.0f, 0.6f);

    // idx is in the top-lat ring (kTopLatBase..kTopLatBase+7).
    return CapsuleRingVertex(idx - kTopLatBase, half_height + g_capsule_data.radius * sin(kCapLatitude), kCapLatitude, 0.8f);
}

// Computes triangle `idx` (0..63) as a uint3 of vertex indices. Winding for
// every band below was worked out (and checked against Cube's known-good
// winding) so that, with this engine's raster state (back-face culling on,
// front_counter_clockwise = !IsClipSpaceYDown()), the outward-facing side of
// the capsule is the one that survives culling.
//
// The four "band" regions (bottom cap, cylinder, top cap) all connect two
// adjacent rings of 8 vertices each with a quad per segment, split into two
// triangles: for ring A (inner/lower) and ring B (outer/upper), segment i and
// its neighbour j = (i+1)%8:
//     triangle 1: (A[i], B[j], A[j])
//     triangle 2: (A[i], B[i], B[j])
// The two apex fans are simpler -- one triangle per segment, fanning out from
// the single apex vertex to two adjacent ring vertices.
uint3 ComputeCapsuleTriangle(uint idx)
{
    const uint segs = kCapsuleSegments;

    // Band 0: bottom apex fan -- 8 triangles, one per segment.
    if (idx < segs)
    {
        const uint i = idx;
        const uint j = (i + 1) % segs;
        return uint3(kApexBottomIndex, kBottomLatBase + i, kBottomLatBase + j);
    }
    idx -= segs;

    // Band 1: bottom cap band, bottom_lat (A) -> bottom_equator (B) -- 16 triangles.
    if (idx < segs * 2)
    {
        const uint pair = idx / 2;
        const uint i = pair;
        const uint j = (pair + 1) % segs;
        const uint A = kBottomLatBase;
        const uint B = kBottomEquatorBase;
        return (idx % 2 == 0)
            ? uint3(A + i, B + j, A + j)
            : uint3(A + i, B + i, B + j);
    }
    idx -= segs * 2;

    // Band 2: cylinder body, bottom_equator (A) -> top_equator (B) -- 16 triangles.
    if (idx < segs * 2)
    {
        const uint pair = idx / 2;
        const uint i = pair;
        const uint j = (pair + 1) % segs;
        const uint A = kBottomEquatorBase;
        const uint B = kTopEquatorBase;
        return (idx % 2 == 0)
            ? uint3(A + i, B + j, A + j)
            : uint3(A + i, B + i, B + j);
    }
    idx -= segs * 2;

    // Band 3: top cap band, top_equator (A) -> top_lat (B) -- 16 triangles.
    if (idx < segs * 2)
    {
        const uint pair = idx / 2;
        const uint i = pair;
        const uint j = (pair + 1) % segs;
        const uint A = kTopEquatorBase;
        const uint B = kTopLatBase;
        return (idx % 2 == 0)
            ? uint3(A + i, B + j, A + j)
            : uint3(A + i, B + i, B + j);
    }
    idx -= segs * 2;

    // Band 4: top apex fan -- 8 triangles.
    {
        const uint i = idx;
        const uint j = (i + 1) % segs;
        return uint3(kTopLatBase + i, kApexTopIndex, kTopLatBase + j);
    }
}

// One thread per vertex/triangle -- 64 threads covers both the 34 vertices
// and the 64 triangles in a single dispatch, so every thread just guards
// itself against the count it doesn't apply to.
[shader("mesh")]
[outputtopology("triangle")]
[numthreads(64, 1, 1)]
void MS_Capsule(
    uint tid : SV_DispatchThreadID,
    out vertices MeshVertexOutput verts[kCapsuleVertCount],
    out indices uint3 tris[kCapsuleTriCount])
{
    SetMeshOutputCounts(kCapsuleVertCount, kCapsuleTriCount);

    if (tid < kCapsuleVertCount)
        verts[tid] = ComputeCapsuleVertex(tid);

    if (tid < kCapsuleTriCount)
        tris[tid] = ComputeCapsuleTriangle(tid);
}

// ============================================================================
// Plane
// ============================================================================
//
// The simplest of the three: a single flat quad on the XZ plane (facing
// +Y), sized by PlaneRenderComponent::extent (full width/depth, not
// half-extents). Good enough for a blockout ground/floor; if Horde ever
// needs a subdivided ground grid (e.g. for per-vertex lighting/foliage), this
// is the place to add more rings of vertices later.
//
// Vertex layout (matches Cube's own "Top (+Y)" face exactly, just at Y=0
// instead of Y=+0.5*scale and sized by `extent` instead of a fixed 1x1) --
// reusing already-verified winding rather than re-deriving it from scratch:
//
//     v3 (-x,z) ---- v2 (+x,z)
//       |               |
//     v0 (-x,-z) ---- v1 (+x,-z)
//
struct PlaneDrawData
{
    float4x4 mvp;
    float3   colour;
    float2   extent;   // PlaneRenderComponent::extent -- full width (x) and depth (y, i.e. world Z).
};

[[vk::push_constant]]
ConstantBuffer<PlaneDrawData> g_plane_data;

static const uint kPlaneVertCount = 4;
static const uint kPlaneTriCount  = 2;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(kPlaneVertCount, 1, 1)]
void MS_Plane(
    uint tid : SV_DispatchThreadID,
    out vertices MeshVertexOutput verts[kPlaneVertCount],
    out indices uint3 tris[kPlaneTriCount])
{
    SetMeshOutputCounts(kPlaneVertCount, kPlaneTriCount);

    // Local corner offsets and UVs, indexed the same way as the diagram above.
    static const float2 kCorners[4] = { float2(-1, -1), float2(1, -1), float2(1, 1), float2(-1, 1) };
    static const float2 kUVs[4]     = { float2(0, 0),   float2(1, 0),  float2(1, 1), float2(0, 1) };

    if (tid < kPlaneVertCount)
    {
        const float2 half_extent = g_plane_data.extent * 0.5f;
        const float2 corner = kCorners[tid] * half_extent;

        MeshVertexOutput v;
        v.position = mul(float4(corner.x, 0.0f, corner.y, 1.0f), g_plane_data.mvp);
        v.normal = float3(0.0f, 1.0f, 0.0f);
        v.colour = g_plane_data.colour;
        v.uv = kUVs[tid];
        verts[tid] = v;
    }

    // Same two-triangle winding as Cube's Top face (indices 2,1,0 and 0,3,2
    // relative to that face's own 4 vertices) -- verified correct for a
    // +Y-facing quad under this engine's back-face-culling raster state.
    if (tid < kPlaneTriCount)
    {
        tris[0] = uint3(2, 1, 0);
        tris[1] = uint3(0, 3, 2);
    }
}

// ============================================================================
// Box
// ============================================================================
//
// Reuses the exact 24-vertex/36-index cube layout from Samples/Cube/CubeApp.h
// (4 vertices per face so every face gets its own flat normal and its own
// full 0..1 UV square, rather than sharing/averaging normals at the
// corners), scaled by BoxRenderComponent::extent instead of hardcoded to a
// unit cube. The winding below is copied verbatim from CubeApp.h's
// cube_indices, which was already worked out and fixed for this engine's
// raster convention -- see the "winding order" fix earlier in this project.
struct BoxDrawData
{
    float4x4 mvp;
    float3   colour;
    float3   extent;   // BoxRenderComponent::extent -- full width/height/depth.
};

[[vk::push_constant]]
ConstantBuffer<BoxDrawData> g_box_data;

static const uint kBoxVertCount = 24;
static const uint kBoxTriCount  = 12;

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(kBoxVertCount, 1, 1)]
void MS_Box(
    uint tid : SV_DispatchThreadID,
    out vertices MeshVertexOutput verts[kBoxVertCount],
    out indices uint3 tris[kBoxTriCount])
{
    SetMeshOutputCounts(kBoxVertCount, kBoxTriCount);

    // Unit-cube corners (-0.5..0.5 per axis) and their per-face normals/UVs,
    // 4 vertices per face, in the same order as Samples/Cube/CubeApp.h:
    // Back(-Z), Front(+Z), Left(-X), Right(+X), Top(+Y), Bottom(-Y).
    static const float3 kPositions[24] = {
        // Back (-Z)
        float3(-0.5, -0.5, -0.5), float3(0.5, -0.5, -0.5), float3(0.5, 0.5, -0.5), float3(-0.5, 0.5, -0.5),
        // Front (+Z)
        float3(-0.5, -0.5, 0.5),  float3(0.5, -0.5, 0.5),  float3(0.5, 0.5, 0.5),  float3(-0.5, 0.5, 0.5),
        // Left (-X)
        float3(-0.5, -0.5, -0.5), float3(-0.5, -0.5, 0.5), float3(-0.5, 0.5, 0.5), float3(-0.5, 0.5, -0.5),
        // Right (+X)
        float3(0.5, -0.5, -0.5),  float3(0.5, -0.5, 0.5),  float3(0.5, 0.5, 0.5),  float3(0.5, 0.5, -0.5),
        // Top (+Y)
        float3(-0.5, 0.5, -0.5),  float3(0.5, 0.5, -0.5),  float3(0.5, 0.5, 0.5),  float3(-0.5, 0.5, 0.5),
        // Bottom (-Y)
        float3(-0.5, -0.5, -0.5), float3(0.5, -0.5, -0.5), float3(0.5, -0.5, 0.5), float3(-0.5, -0.5, 0.5),
    };

    static const float3 kNormals[24] = {
        float3(0,0,-1), float3(0,0,-1), float3(0,0,-1), float3(0,0,-1),
        float3(0,0,1),  float3(0,0,1),  float3(0,0,1),  float3(0,0,1),
        float3(-1,0,0), float3(-1,0,0), float3(-1,0,0), float3(-1,0,0),
        float3(1,0,0),  float3(1,0,0),  float3(1,0,0),  float3(1,0,0),
        float3(0,1,0),  float3(0,1,0),  float3(0,1,0),  float3(0,1,0),
        float3(0,-1,0), float3(0,-1,0), float3(0,-1,0), float3(0,-1,0),
    };

    static const float2 kUVs[24] = {
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
        float2(0,0), float2(1,0), float2(1,1), float2(0,1),
    };

    // Verbatim from CubeApp.h's cube_indices (already fixed/verified winding).
    static const uint3 kTriangles[12] = {
        uint3(2, 1, 0),   uint3(0, 3, 2),     // Back
        uint3(4, 5, 6),   uint3(6, 7, 4),     // Front
        uint3(8, 9, 10),  uint3(10, 11, 8),   // Left
        uint3(14, 13, 12),uint3(12, 15, 14),  // Right
        uint3(18, 17, 16),uint3(16, 19, 18),  // Top
        uint3(20, 21, 22),uint3(22, 23, 20),  // Bottom
    };

    if (tid < kBoxVertCount)
    {
        MeshVertexOutput v;
        v.position = mul(float4(kPositions[tid] * g_box_data.extent, 1.0f), g_box_data.mvp);
        v.normal = kNormals[tid];
        v.colour = g_box_data.colour;
        v.uv = kUVs[tid];
        verts[tid] = v;
    }

    if (tid < kBoxTriCount)
        tris[tid] = kTriangles[tid];
}

// ============================================================================
// Shared fragment shader
// ============================================================================
//
// Identical lighting to Cube.slang's FS_Main (a fixed directional light,
// ambient + diffuse + specular, gamma-corrected at the end) minus the
// texture-sampling branch -- there's no texture yet, so `input.colour` (the
// per-entity colour pushed by HordeRenderer) stands in for the albedo a
// texture sample would otherwise provide.
[shader("fragment")]
float4 FS_Main(MeshVertexOutput input) : SV_Target
{
    float3 light_dir    = normalize(float3(0.5, 1.0, -0.5));
    float3 light_colour = float3(1.0, 1.0, 1.0);
    float3 view_dir     = normalize(float3(0.0, -1.0, 0.0));

    // Ambient
    float3 ambient = 0.1 * input.colour;

    // Diffuse
    float3 N = normalize(input.normal);
    float diff = max(dot(N, light_dir), 0.0);
    float3 diffuse = diff * light_colour * input.colour;

    // Specular
    float3 reflect_dir = reflect(-light_dir, N);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 32.0);
    float3 specular = float3(0.5) * spec * light_colour;

    float3 final_colour = pow(ambient + diffuse + specular, float3(1.0 / 2.2));
    return float4(final_colour, 1.0);
}
