#ifndef _ASSETSYS_H_
#define _ASSETSYS_H_

#include <std/types.h>
#include <std/str.h>

/*

development_mode
geometry_asset.ass
```
[import]
combine_geometry=true
import_materials=true

[data]
transform float4x4
material  mat_guid
```

distribution_mode
```
transform float4x4
material  mat_guid
```

*/

#define MAX_DEPENDENCIES 100

typedef enum {
	ASSET_TYPE_SHADER,
	ASSET_TYPE_GEOMETRY,
	ASSET_TYPE_MATERIAL,
	ASSET_TYPE_TEXTURE,
} asset_type_t;

typedef struct {
	asset_type_t type;
	str8_t       name;
	guid_t       guid;
	u8           version[4];

	str8_t       unimported_filename;
	str8_t       imported_filename;

	guid_t       dependencies[MAX_DEPENDENCIES];
} metadata_t ;

#endif //_ASSETSYS_H_