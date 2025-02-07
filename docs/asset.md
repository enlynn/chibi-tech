
# Thoughts on a asset system

Let's define a set of core asset types to start with
- Shaders
- Textures
- Materials
- Geometry

## Metadata

Each asset requires some additional meta data to identify itself:
- 128bit peristent GUID
- user identifiable name
- path to imported engine asset
- path to unimported asset
- list of GUIDs the asset is dependent on

Let's think more about dependencies; consider a material. It might have a:
- diffuse texture
- normal texture
- roughness texture

So the metadata would look like:
guid: some_guid
name: "This is an example material"
import_path: "local:materials/example.mat"
unimport_path: "local:blender_material/example.mat"
dependencies: [ guid0, guid1, guid2 ]

## Import Settings

An asset might need to know import settings. For example, a texture needs to know:
- desired format
- compression
- srgb import state

Import settings are based on the file type.

## Importing Assets

Each asset type needs it's own import. For example .obj files can be decomposed into
- n Geometries
- n Textures
- n Materials

A texture is converted from {png, jpeg, ...} -> DDS

A Material is an engine specific format and is likely dependent on the geometry being
imported.

Geometry is converted from a {obj, fbx, gltf} -> some internal binary format

A shader doesn't need to be "imported", only compiled.

## Dev vs Public assets
