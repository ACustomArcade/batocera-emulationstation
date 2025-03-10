#pragma once
#ifndef ES_CORE_RESOURCES_TEXTURE_DATA_MANAGER_H
#define ES_CORE_RESOURCES_TEXTURE_DATA_MANAGER_H

#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <set>
#include <unordered_map>

class TextureDataManager;
class TextureData;
class TextureResource;

class TextureLoader
{
public:
	TextureLoader(TextureDataManager* mgr);
	~TextureLoader();

	void load(std::shared_ptr<TextureData> textureData);
	bool remove(std::shared_ptr<TextureData> textureData);
	void clearQueue();

	size_t getQueueSize();

	static bool paused;

private:	
	void threadProc();

	std::set<std::shared_ptr<TextureData>> 											mProcessingTextureDataQ;
	std::list<std::shared_ptr<TextureData>> 										mTextureDataQ;
	std::set<std::shared_ptr<TextureData>> 											mTextureDataQSet;

	std::vector<std::thread>	mThreads;
	std::mutex					mLoaderLock;
	std::condition_variable		mEvent;
	bool 						mExit;

	TextureDataManager*			mManager;
};


//
// This class manages the loading and unloading of textures
//
// When textures are added, the texture data is just stored as-is. The texture
// data should only have been constructed and not loaded for this to work correctly.
// When the get() function is called it indicates that a texture wants to be used so
// at this point the texture data is loaded (via a call to load()).
//
// Once the load is complete (which may not be on the first call to get() if the
// data is loaded in a background thread) then the get() function call uploadAndBind()
// to upload to VRAM if necessary and bind the texture. This is followed by a call
// to releaseRAM() which frees the memory buffer if the texture can be reloaded from
// disk if needed again
//
class TextureDataManager
{
public:
	TextureDataManager();
	~TextureDataManager();

	enum TextureLoadMode : int
	{
		ENABLED = 0,
		DISABLED = 1,
		MOVETOTOPONLY = 2
	};

	std::shared_ptr<TextureData> add(const TextureResource* key, bool tiled, bool linear);

	// The texturedata being removed may be loading in a different thread. However it will
	// be referenced by a smart point so we only need to remove it from our array and it
	// will be deleted when the other thread has finished with it
	void remove(const TextureResource* key);

	void cancelAsync(const TextureResource* key);
	std::shared_ptr<TextureData> get(const TextureResource* key, TextureLoadMode enableLoading = TextureLoadMode::ENABLED);
	bool bind(const TextureResource* key);

	// Get the total size of all textures managed by this object, loaded and unloaded in bytes
	size_t	getTotalSize();
	// Get the total size of all committed textures (in VRAM) in bytes
	size_t	getCommittedSize();
	// Get the total size of all load-pending textures in the queue - these will
	// be committed to VRAM as the queue is processed
	size_t  getQueueSize();
	// Load a texture, freeing resources as necessary to make space
	void load(std::shared_ptr<TextureData> tex, bool block = false);

	void clearQueue();
	
	void cleanupVRAM(std::shared_ptr<TextureData> exclude = nullptr);

private:

	std::shared_ptr<TextureData> getBlankTexture();

	std::mutex					mMutex;

	std::list<std::shared_ptr<TextureData> >												mTextures;
	std::unordered_map<const TextureResource*, std::list<std::shared_ptr<TextureData> >::const_iterator > 	mTextureLookup;
	std::shared_ptr<TextureData>															mBlank;
	TextureLoader*																			mLoader;
};

#endif // ES_CORE_RESOURCES_TEXTURE_DATA_MANAGER_H
