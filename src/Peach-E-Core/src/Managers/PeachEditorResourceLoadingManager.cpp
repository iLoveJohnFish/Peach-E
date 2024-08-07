#include "../../include/Managers/PeachEditorResourceLoadingManager.h"

namespace PeachEditor {

	PeachEngineResourceLoadingManager::~PeachEngineResourceLoadingManager()
	{

	}

	PeachEngineResourceLoadingManager::PeachEngineResourceLoadingManager()
	{

	}

	//THESE METHODS ONLY ALLOW A MAXIMUM OF TWO REFERENCES PASSED OUT TO ANYONE ASKING THIS IS MEANT FOR THE MAIN THREAD AND AUDIO/RENDER THREAD

	shared_ptr<PeachCore::LoadingQueue> PeachEngineResourceLoadingManager::GetAudioResourceLoadingQueue() //This method should be one of the first methods called on startup
	{
		assert(pm_AudioQueueReferenceCount <= 2);

		if (pm_AudioQueueReferenceCount == 2)  //stops unwanted extra references from being created accidentally
		{
			InternalLogManager::InternalResourceLoadingLogger().Warn("Attempted to get more than 2 references to PeachEngineResourceLoadingManager's AudioResourceLoadingQueue", "PeachEngineResourceLoadingManager");
			return nullptr;
		}
		else if (pm_AudioResourceLoadingQueue == 0) //lazy initialization for LoadingQueue cause why not
		{
			pm_AudioResourceLoadingQueue = make_shared<PeachCore::LoadingQueue>();
		}

		pm_AudioQueueReferenceCount++;
		return pm_AudioResourceLoadingQueue;
	}

	shared_ptr<PeachCore::LoadingQueue> PeachEngineResourceLoadingManager::GetDrawableResourceLoadingQueue() //This method should be one of the first methods called on startup
	{
		assert(pm_DrawableQueueReferenceCount <= 2);

		if (pm_DrawableQueueReferenceCount == 2) //stops unwanted extra references from being created accidentally
		{
			InternalLogManager::InternalResourceLoadingLogger().Warn("Attempted to get more than 2 references to PeachEngineResourceLoadingManager's DrawableResourceLoadingQueue", "PeachEngineResourceLoadingManager");
			return nullptr;
		}
		else if (pm_DrawableQueueReferenceCount == 0) //lazy initialization for LoadingQueue cause why not
		{
			pm_DrawableResourceLoadingQueue = make_shared<PeachCore::LoadingQueue>();
		}

		pm_DrawableQueueReferenceCount++;
		return pm_DrawableResourceLoadingQueue;
	}



	bool PeachEngineResourceLoadingManager::LoadTextureFromSpecifiedFilePath(const string& fp_FilePath)
	{
		auto f_LoadedTexture = make_unique<sf::Texture>();

		if (f_LoadedTexture->loadFromFile(fp_FilePath))
		{	//the extra move here might be breaking things since when we instantiate the smart ptr, it's already a ?-value i forget the letter but its not an r-value i think
			TryPushingLoadedResourcePackage(move(make_unique<PeachCore::LoadedResourcePackage>("someObjectID", move(f_LoadedTexture))));
			return true;
		}

		return false; // Handle failed load appropriately
	}

	// Called by the rendering thread  //TODO TRY PUSHING THE PACKAGES IN VECTORS ALWAYS AND ONLY UNPACK IN RENDERINGMANAGER,
	//WE SHOULD FIGURE OUT A WAY TO LOAD THINGS IN BUNCHES, OR GET ALL LOADED OBJECTS IN ONEGO AND ONLY PUSH THEM ONCE ITS ALL LOADED
	//THAT WOULD BE GOOD FOR LOADING SCENES, WE DONT WANT TO PUSH ANY RESOURCES EARLIER THAN NEEDED UNTIL THE ENTIRE SCENE IS LOADED
	//I'm kinda tired of working on the loading manager and i wanna do physics now so gl future ryan i hope things go well >w<
	bool PeachEngineResourceLoadingManager::TryPushingLoadedResourcePackage(unique_ptr<PeachCore::LoadedResourcePackage> fp_LoadedPackage)
	{
		pm_WaitingFullyLoadedResourcePackages.push_back(move(fp_LoadedPackage));

		if (!pm_DrawableResourceLoadingQueue->PushLoadedResourcePackage(pm_WaitingFullyLoadedResourcePackages))
		{
			InternalLogManager::InternalResourceLoadingLogger().Debug("Load deferred until later", "PeachEngineResourceLoadingManager");
			return false;
		}

		return true;
	}
}