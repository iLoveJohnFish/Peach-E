#include "../../include/Managers/PeachEngineRenderingManager.h"
/*
    This class is used to manage the render thread, and queue/unqueue objects safely
    Copyright(c) 2024-present Ranyodh Singh Mandur.

*/

namespace PeachEditor {


    PeachEngineRenderingManager::~PeachEngineRenderingManager()
    {
        delete pm_Camera2D;
        delete pm_CurrentWindow;
    }

    PeachEngineRenderingManager::PeachEngineRenderingManager()
    {

    }

    //creates a window and opengl context, enables sfml 2d graphics and such as well, returns the command queue for thread safe control
    shared_ptr<CommandQueue>
        PeachEngineRenderingManager::Initialize(const string& fp_Title, int fp_Width, int fp_Height)
    {
        if (pm_HasBeenInitialized)
        {
            InternalLogManager::InternalRenderingLogger().Warn("RenderingManager already initialized.", "RenderingManager");
            return nullptr;
        }

        // Create an SFML window and context settings
/*       sf::ContextSettings settings;
       settings.depthBits = 24;
       settings.stencilBits = 8; //THESE SETTINGS ARE BROKEN FOR SOME REASON, AND I DONT KNOW WHY
       settings.antialiasingLevel = 4;
       settings.majorVersion = 3;
       settings.minorVersion = 3;
       settings.attributeFlags = sf::ContextSettings::Core;*/

        pm_CurrentWindow = new sf::RenderWindow(sf::VideoMode(fp_Width, fp_Height), fp_Title, sf::Style::Default);

        // Camera Setup
        pm_Camera2D = new PeachCamera2D(*pm_CurrentWindow);
        pm_Camera2D->SetCenter(400, 300); // Set this dynamically as needed
        pm_Camera2D->SetSize(800, 600); // Set this to zoom in or out
        pm_Camera2D->Enable();

        if (!pm_CurrentWindow->isOpen())
        {
            throw runtime_error("Failed to create window.");
        }

        pm_CommandQueue = make_shared<CommandQueue>();
        pm_LoadedResourceQueue = ResourceLoadingManager::ResourceLoader().GetDrawableResourceLoadingQueue();

        InternalLogManager::InternalRenderingLogger().Debug("RenderingManager successfully initialized >w<", "RenderingManager");

        pm_HasBeenInitialized = true;

        CreatePeachEConsole();
        SetupRenderTexture(300, 300);

        return pm_CommandQueue; //returns one and only one ptr to whoever initializes RenderingManager, this is meant only for the main thread
    }

    void
        PeachEngineRenderingManager::ProcessCommands()
    {
        PeachCore::DrawCommand f_DrawCommand;
        while (pm_CommandQueue->PopSendersQueue(f_DrawCommand))
        {
            for (auto& drawable_data : f_DrawCommand.DrawableData)
            {
                visit(
                    PeachCore::overloaded
                    {
                    [&](const vector<PeachCore::CreateDrawableData>& fp_Data)
                    {
                        // Handle creation logic here
                    },
                    [&](const vector<PeachCore::UpdateActiveDrawableData>& fp_Data)
                    {
                        // Handle update logic here
                        // This could involve updating position based on deltaPosition
                        // Setting visibility, layer sorting, etc.
                    },
                    [&](const vector<PeachCore::DeleteDrawableData>& fp_Data)
                    {
                        // Handle deletion logic here
                        // Ensure resources are properly released and objects are cleaned up
                    }
                    }, drawable_data);
            }
        }
    }

    void
        PeachEngineRenderingManager::ProcessLoadedResourcePackages()
    {
        unique_ptr<PeachCore::LoadedResourcePackage> ResourcePackage;
        while (pm_LoadedResourceQueue->PopLoadedResourceQueue(ResourcePackage))
        {
            visit(
                PeachCore::overloaded
                {
                [&](unique_ptr<unsigned char>& fp_RawByteData)
                {
                    // Handle creation logic here
                },
                [&](unique_ptr<sf::Texture>& fp_TextureData)
                {
                    m_TestTexture = move(fp_TextureData);
                },
                [&](unique_ptr<string>& fp_JSONData)
                {
                    // Handle deletion logic here
                    // Ensure resources are properly released and objects are cleaned up
                }
                }, ResourcePackage.get()->ResourceData);
        }
    }

    void PeachEngineRenderingManager::RenderFrame()
    {
        ProcessLoadedResourcePackages(); //move all loaded objects into memory here if necessary
        //ProcessCommands(); //process all updates
                // Main loop that continues until the window is closed

        //sf::Sprite sprite;
        //sprite.setTexture(*m_TestTexture);

        //// Get the size of the window
        //sf::Vector2u windowSize = pm_CurrentWindow->getSize();

        //// Get the size of the texture
        //sf::Vector2u textureSize = m_TestTexture->getSize();

        //// Calculate scale factors
        //float scaleX = float(windowSize.x) / textureSize.x;
        //float scaleY = float(windowSize.y) / textureSize.y;

        //// Set the scale of the sprite
        //sprite.setScale(scaleX, scaleY);

        //sprite.setOrigin(textureSize.x / 2.0f, textureSize.y / 2.0f);
        //sprite.setPosition(windowSize.x / 2.0f, windowSize.y / 2.0f);

        bool f_ShouldConsoleBeOpen = true;

        ImGui::SFML::Init(*pm_CurrentWindow); 	// sets up ImGui with ether a dark or light default theme

        sf::Clock deltaClock;

        PeachCore::RenderingManager::Renderer().Initialize();

        //////////////////////////////////////////////
        // MAIN RENDER LOOP FOR THE EDITOR
        //////////////////////////////////////////////

        while (pm_CurrentWindow->isOpen())
        {
            Clear(); //clears screen and beings drawing

            //////////////////////////////////////////////
            // Input Polling for Imgui/SFML
            //////////////////////////////////////////////

            sf::Event event;

            while (pm_CurrentWindow->pollEvent(event))
            {
                ImGui::SFML::ProcessEvent(event);

                if (event.type == sf::Event::Closed) 
                {
                    pm_CurrentWindow->close();
                }
            }

            //////////////////////////////////////////////
            // Render Texture Setup
            //////////////////////////////////////////////

            if (pm_ViewportRenderTexture)
            {
                pm_ViewportRenderTexture->clear(sf::Color(128, 128, 128)); // Clear with grey >w<, or any color you need

                //// Draw your game elements to the renderTexture instead of the window
                //// For example:
                //pm_ViewportRenderTexture->draw(sprite);

                pm_ViewportRenderTexture->display(); // Updates the texture with what has been drawn

                //// Now the renderTexture contains the updated scene
                //// You can convert it to a sprite and draw it on your main window or use it in the GUI
                sf::Sprite f_SceneSprite(pm_ViewportRenderTexture->getTexture());

                //// Set position, scale or any transformations needed for your GUI layout
                f_SceneSprite.setPosition(10, 10); // Example: position it within your GUI

                pm_CurrentWindow->draw(f_SceneSprite); // Draw the texture sprite to the main window
            }

            //////////////////////////////////////////////
            // Update Imgui UI
            //////////////////////////////////////////////

            ImGui::SFML::Update(*pm_CurrentWindow, deltaClock.restart()); // starts the ImGui content mode. Make all ImGui calls after this

            //////////////////////////////////////////////
            // Virtual File System Setup
            //////////////////////////////////////////////

            // Your ImGui code goes here
            ImGui::Begin("Editor");
            ImGui::Text("Hello, Raylib ImGui!");
            ImGui::End();

            //////////////////////////////////////////////
            // Menu Bar Setup
            //////////////////////////////////////////////

            if (ImGui::BeginMainMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("New"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("Open"))
                    {
                        // Open action
                    }
                    if (ImGui::MenuItem("Save"))
                    {
                        // Save action
                    }
                    if (ImGui::MenuItem("Exit")) //THIS CRASHES EVERYTHING BUT I GUESS THAT IS A WAY TO CLOSE STUFF
                    {
                        Clear(); // IMPORTANT: NEEDA EXPLICITLY ENDDRAWING() BEFORE CALLING CLOSEWINDOW!
                        break;
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Run"))
                {
                    if (ImGui::MenuItem("Run Peach-E Project") && !m_IsSceneCurrentlyRunning)
                    {
                        // Run the game in a new window
                        thread T_GameThread([]() 
                            {
                                PeachCore::RenderingManager::Renderer().CreateWindowAndCamera2D("Peach Game", 800, 600);
                                PeachCore::RenderingManager::Renderer().RenderFrame();
                                PeachCore::RenderingManager::Renderer().Shutdown();
                                PeachEngineRenderingManager::PeachEngineRenderer().m_IsSceneCurrentlyRunning = false;
                            });
                        T_GameThread.detach();
                        m_IsSceneCurrentlyRunning = true;
                    }
                    if (ImGui::MenuItem("Build Peach-E Project"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Project"))
                {
                    if (ImGui::MenuItem("Settings"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("Input Map"))
                    {
                        // Open action
                    }
                    if (ImGui::MenuItem("Refresh Project"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View"))
                {
                    if (ImGui::MenuItem("Delete"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Tools"))
                {
                    if (ImGui::MenuItem("Open Terminal"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("Editor Theme"))
                    {
                        // Open action
                    }
                    if (ImGui::MenuItem("Editor Settings"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Plugins"))
                {
                    if (ImGui::MenuItem("Load Plugin"))
                    {
                        // New action
                    }
                    if (ImGui::MenuItem("List of Active Plugins"))
                    {
                        // Open action
                    }
                    ImGui::EndMenu();
                }

                ImGui::EndMainMenuBar();
            }

            //////////////////////////////////////////////
            // Setting Up Scene View
            //////////////////////////////////////////////

            //ImGui::Begin("Viewport");

            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            ImVec2 viewportPos = ImGui::GetCursorScreenPos();

            // Get mouse position in viewport
            ImVec2 mousePos = ImGui::GetMousePos();
            bool isMouseInViewport = ImGui::IsMouseHoveringRect(viewportPos, ImVec2(viewportPos.x + viewportSize.x, viewportPos.y + viewportSize.y));

            if (isMouseInViewport)
            {
                //// Convert ImGui mouse coordinates to viewport coordinates
                //Vector2 mousePosInViewport = { mousePos.x - viewportPos.x, mousePos.y - viewportPos.y };

                //// Handle drag and drop or selection
                //if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                //    // Handle mouse click
                //}
                //if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                //    // Handle mouse drag
                //}
            }


            //////////////////////////////////////////////
            // Scene Tree View Panel
            //////////////////////////////////////////////

            // Panels
            ImGui::Begin("Hierarchy");
            ImGui::Text("Scene Hierarchy");
            ImGui::End();

            //////////////////////////////////////////////
            // Peach-E Console
            //////////////////////////////////////////////

            pm_EditorConsole->Draw("PEACH CONSOLE", &f_ShouldConsoleBeOpen);

            ImGui::Begin("Inspector");
            ImGui::Text("Inspector");
            ImGui::End();


            //pm_CurrentWindow->clear(sf::Color(0, 0, 139));

            ImGui::SFML::Render(*pm_CurrentWindow);			// ends the ImGui content mode. Make all ImGui calls before this

            // Draw the sprite
            //pm_CurrentWindow->draw(sprite);

            // Update the window
            pm_CurrentWindow->display();
        }
        //TO-DO: set flags that indicate the current state of closewindow and imguishutdown, because if we call them again in the destructor it realllllllly doesn't like that
        // after your game loop is over, before you close the window
        ImGui::SFML::Shutdown();		// cleans up ImGui
    }

    // Call this method to setup the render texture
    bool PeachEngineRenderingManager::SetupRenderTexture(unsigned int width, unsigned int height)
    {
        if (pm_ViewportRenderTexture)
        {
            InternalLogManager::InternalRenderingLogger().Warn("Attempted to setup render texture again when a valid instance of pm_ViewportRenderTexture is running", "PeachEngineRenderingManager");
            return false;
        }

        pm_ViewportRenderTexture = make_unique<sf::RenderTexture>();

        if (pm_ViewportRenderTexture->create(width, height))
        {
            pm_ViewportRenderTexture->setSmooth(true);
        }

        else
        {
            InternalLogManager::InternalRenderingLogger().Warn("Failed to create a valid instance of pm_ViewportRenderTexture, setting it back to nullptr", "PeachEngineRenderingManager");
            pm_ViewportRenderTexture = nullptr;
            return false;
        }

        return true;
    }

    void PeachEngineRenderingManager::Clear()
    {
        if (pm_CurrentWindow)
        {
            pm_CurrentWindow->clear();
        }
    }

    void PeachEngineRenderingManager::BeginFrame()
    {
        //BeginDrawing();
    }

    void PeachEngineRenderingManager::EndFrame()
    {
        if (pm_CurrentWindow)
        {
            pm_CurrentWindow->display();
        }
    }

    void PeachEngineRenderingManager::CreatePeachEConsole()
    {
        pm_EditorConsole = make_unique<PeachEConsole>();
    }

    void PeachEngineRenderingManager::CreateSceneTreeViewPanel()
    {


    }

    void PeachEngineRenderingManager::ResizeWindow()
    {

    }

    string PeachEngineRenderingManager::GetRendererType() const
    {
        return pm_RendererType;
    }
    void PeachEngineRenderingManager::GetCurrentViewPort()
    {

    }
    unsigned int PeachEngineRenderingManager::GetFrameRateLimit() const
    {
        return pm_FrameRateLimit;
    }
    bool PeachEngineRenderingManager::IsVSyncEnabled() const
    {
        return pm_IsVSyncEnabled;
    }
    void PeachEngineRenderingManager::SetVSync(const bool fp_IsEnabled)
    {
        if (fp_IsEnabled)
        {
            pm_IsVSyncEnabled = fp_IsEnabled;
        }
    }
    void PeachEngineRenderingManager::SetFrameRateLimit(unsigned int fp_Limit)
    {
        pm_FrameRateLimit = fp_Limit;
    }
}