#include "ResourceSystem.h"

#include <Platform/Assert.h>

namespace {
    void NormalizePath(std::string& Path) {
        ForRange(u64, i, Path.size()) {
            if (Path[i] == '\\')
                Path[i] = '/';
        }
        //Path.ShrinkToFit();
    }
}

ResourceSystem::ResourceSystem(const std::filesystem::path& tBasePath)
{
	mBasePath = tBasePath;
	//NormalizePath(mBasePath);
}

void ResourceSystem::registerLoader(ResourceLoader& Loader)
{
	ASSERT(Loader.mType != ResourceType::Unknown);
    UNIMPLEMENTED;

#if 0
	if (Loader.mType < ResourceType::custom)
	{
		u32 LoaderIndex = u32(Loader.mType);
		mLoaders[LoaderIndex].mLoader = Loader;

		u64 Length = mLoaders[LoaderIndex].mLoader.mRelativePath.Length();

		NormalizePath(mLoaders[LoaderIndex].mLoader.mRelativePath);

		Length = mLoaders[LoaderIndex].mLoader.mRelativePath.Length();

		// Convert the relative path to an absolute path.
		mLoaders[LoaderIndex].mAbsolutePath = mBasePath + "/" + mLoaders[LoaderIndex].mLoader.mRelativePath;
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}
#endif
}

bool ResourceSystem::load(ResourceType Type, std::string_view ResourceName, Resource* OutResource)
{
    ASSERT(Type != ResourceType::Unknown);
    UNIMPLEMENTED;

#if 0
	if (Type < ResourceType::custom)
	{
		ResourceLoader& Loader = mLoaders[u32(Type)].mLoader;
		return Loader.Load(&Loader, mLoaders[u32(Type)].mAbsolutePath, ResourceName, OutResource);
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}
#endif

	return false;
}

void ResourceSystem::unload(ResourceType Type, Resource* InResource)
{
    ASSERT(Type != ResourceType::Unknown);

	if (Type < ResourceType::Custom)
	{
		ResourceLoader& Loader = mLoaders[u32(Type)].mLoader;
		Loader.unload(&Loader, InResource);
	}
	else
	{
		// TODO(enlynn): Handle custom loaders
	}
}
