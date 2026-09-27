PHX prototype capsule PBR textures
====================================

Each set contains:
- *_albedo.png : Base Color / Albedo
- *_normal.png : Tangent-space normal map
- *_orm.png    : packed material map

ORM packing:
- R = Ambient Occlusion
- G = Roughness
- B = Metallic

These are 1024x1024 RGB PNGs.

Player:
- blue painted surface
- cyan visual band included in albedo as a placeholder

Horde:
- worn red painted surface

For a physically-based shader, treat the cyan band as emissive only if you later
add a separate emissive mask/map; it is intentionally included in albedo here
so the prototype is immediately visible.

The maps are generic tiled material maps; your mesh shader still needs to provide
UVs (or generate capsule UVs procedurally).
