#pragma once

#include <types.h>
#include <string>
#include <string_view>
#include <filesystem>

// TODO(enlynn):
// - Use std::fs::path instead of std::string
// - fix styling issues

enum class ResourceType : u8
{
	Shader,
	Custom,
	Count,
	Unknown = Count,
};

// Parent Struct
struct Resource
{
	ResourceType  mType{ResourceType::Unknown};      // Loader Id
	
	void*         mBaseData{nullptr};                // Unparsed file data. Owned by the ResourceLoader.
	u64           mBaseDataSize{0};                  // The Resource type is responsible for parsing the data

	Resource() = default;
	explicit Resource(ResourceType Type, void* Data, u64 DataSize) : mType(Type), mBaseData(Data), mBaseDataSize(DataSize) {}

	// implementations can choose to not use the inheritance approach. if so, don't override this function.
	virtual bool parse(struct ResourceLoader* Self, const char* Name, Resource* OutResource) { return true; }
};

struct ResourceLoader
{
	ResourceType          mType{ResourceType::Unknown};
	std::string           mCustomName{}; // Name for Custom Resource Types, TODO(enlynn): support
    std::filesystem::path mRelativePath{};

	bool (*load)(ResourceLoader* Self, const std::filesystem::path& AbsolutePath, const std::string_view ResourceName, Resource* OutResource);
	void (*unload)(ResourceLoader* self, Resource* resource);
};

class ResourceSystem
{
public:
    ResourceSystem() = default;
	explicit ResourceSystem(const std::filesystem::path& tBasePath);

	void registerLoader(ResourceLoader& Loader);

	// Loads a builtin Resource type
	bool load(ResourceType Type, std::string_view ResourceName, Resource* OutResource);
	void unload(ResourceType Type, Resource* InResource);

	// Loads a custom Resource type - TODO(enlynn)
	//bool LoadCustom  (const istr8 ResourceName, Resource* OutResource);
	//void UnloadCustom(const istr8 ResourceName, Resource* InResource);

private:
	struct resource_loader_entry 
	{
        std::filesystem::path mAbsolutePath{};
		ResourceLoader        mLoader{};
	};

	std::filesystem::path mBasePath{};
	resource_loader_entry mLoaders[u32(ResourceType::Count) - 1]{}; // Don't store custom loaders.
};

#if 0 // Usage code
ResourceSystem->RegisterLoader(ShaderResource::GetResourceLoader());

ShaderResource Resource = {};
ResourceSystem->Load(ResourceType::Shader, "SimpleVertex.Vtx", &ShaderResource);
// Do stuff...
ResourceSystem->unload(&Resource);
#endif