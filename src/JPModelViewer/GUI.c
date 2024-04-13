// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/*
===========================================================================
    Copyright (C) 2024- Adriano Di Dio.
    
    JPModelViewer is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    JPModelViewer is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with JPModelViewer.  If not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

#include "GUI.h"
#include "../Common/VRAM.h"
#include "JPModelViewer.h"
#include "TSP.h"

void GUIFree(GUI_t *GUI)
{
    GUIReleaseContext(GUI->DefaultContext);
    ProgressBarDestroy(GUI->ProgressBar);
    FileDialogListFree();
    ErrorMessageDialogFree(GUI->ErrorMessageDialog);
    free(GUI->ConfigFilePath);
    free(GUI);
}

bool GUIIsMouseFree()
{
    ImGuiIO *IO;
    IO = igGetIO();
    return !IO->WantCaptureMouse;
}
bool GUIIsKeyboardFree()
{
    ImGuiIO *IO;
    IO = igGetIO();
    return !IO->WantCaptureKeyboard;
}
void GUIProcessEvent(GUI_t *GUI,SDL_Event *Event)
{
    ImGui_ImplSDL2_ProcessEvent(Event);
}

void GUIDrawDebugWindow(GUI_t *GUI,Camera_t *Camera,VideoSystem_t *VideoSystem)
{
    SDL_version LinkedVersion;
    SDL_version CompiledVersion;
    
    if( !GUI->DebugWindowHandle ) {
        return;
    }

    if( igBegin("Debug Settings",&GUI->DebugWindowHandle,ImGuiWindowFlags_AlwaysAutoResize) ) {
        if( igCollapsingHeader_TreeNodeFlags("Debug Statistics",ImGuiTreeNodeFlags_DefaultOpen) ) {
            igText("NumActiveWindows:%i",GUI->NumActiveWindows);
            igSeparator();
            igText("OpenGL Version: %s",glGetString(GL_VERSION));
            SDL_GetVersion(&LinkedVersion);
            SDL_VERSION(&CompiledVersion);
            igText("SDL Compiled Version: %u.%u.%u",CompiledVersion.major,CompiledVersion.minor,CompiledVersion.patch);
            igText("SDL Linked Version: %u.%u.%u",LinkedVersion.major,LinkedVersion.minor,LinkedVersion.patch);
            igSeparator();
            igText("Display Informations");
            igText("Resolution:%ix%i",VidConfigWidth->IValue,VidConfigHeight->IValue);
            igText("Refresh Rate:%i",VidConfigRefreshRate->IValue);
        }
    }
    igEnd();
}

void GUIDrawDebugOverlay(ComTimeInfo_t *TimeInfo)
{
    ImGuiViewport *Viewport;
    ImVec2 WorkPosition;
    ImVec2 WorkSize;
    ImVec2 WindowPosition;
    ImVec2 WindowPivot;
    ImGuiWindowFlags WindowFlags;
    
    WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize | 
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | 
                    ImGuiWindowFlags_NoMove;
    Viewport = igGetMainViewport();
    WorkPosition = Viewport->WorkPos;
    WorkSize = Viewport->WorkSize;
    WindowPosition.x = (WorkPosition.x + WorkSize.x - 10.f);
    WindowPosition.y = (WorkPosition.y + 10.f);
    WindowPivot.x = 1.f;
    WindowPivot.y = 0.f;

    
    if( GUIShowFPS->IValue ) {
        igSetNextWindowPos(WindowPosition, ImGuiCond_Always, WindowPivot);
        if( igBegin("FPS", NULL, WindowFlags) ) {
            igText(TimeInfo->FPSString);
        }
        igEnd(); 
    }
}

void GUIDrawMainWindow(GUI_t *GUI,RenderObjectManager_t *RenderObjectManager,VideoSystem_t *VideoSystem,Camera_t *Camera)
{
    BSDRenderObjectPack_t *PackIterator;
    BSDRenderObject_t *RenderObjectIterator;
    BSDRenderObject_t *CurrentRenderObject;
    ImVec2 ZeroSize;
    int DisableNode;
    char SmallBuffer[256];
    ImGuiTreeNodeFlags TreeNodeFlags;

    
    if( !igBegin("Main Window", NULL, ImGuiWindowFlags_AlwaysAutoResize) ) {
        return;
    }
    
    ZeroSize.x = 0.f;
    ZeroSize.y = 0.f;
    
    if( igCollapsingHeader_TreeNodeFlags("Help",ImGuiTreeNodeFlags_DefaultOpen) ) {
        igText("Press and Hold The Left Mouse Button to Rotate the Camera");
        igText("Scroll the Mouse Wheel to Zoom the Camera In and Out");
        igText("Press A and S to strafe the Camera left-right,Spacebar and Z to move it up-down");
        igText("Press Escape to exit the program");
    }
    if( igCollapsingHeader_TreeNodeFlags("Camera",ImGuiTreeNodeFlags_DefaultOpen) ) {
        igText("Camera Spherical Position(Radius,Theta,Phi):%.3f;%.3f;%.3f",Camera->Position.Radius,Camera->Position.Theta,Camera->Position.Phi);
        igText("Camera View Point:%.3f;%.3f;%.3f",Camera->ViewPoint[0],Camera->ViewPoint[1],Camera->ViewPoint[2]);

        if( igButton("Reset Camera Position",ZeroSize) ) {
            CameraReset(Camera);
        }
        if( igSliderFloat("Camera Speed",&CameraSpeed->FValue,10.f,256.f,"%.2f",0) ) {
            ConfigSetNumber("CameraSpeed",CameraSpeed->FValue);
        }
        if( igSliderFloat("Camera Mouse Sensitivity",&CameraMouseSensitivity->FValue,1.f,20.f,"%.2f",0) ) {
            ConfigSetNumber("CameraMouseSensitivity",CameraMouseSensitivity->FValue);
        }
    }
    if( igCollapsingHeader_TreeNodeFlags("Settings",ImGuiTreeNodeFlags_DefaultOpen) ) {
        if( GUICheckBoxWithTooltip("WireFrame Mode",(bool *) &EnableWireFrameMode->IValue,EnableWireFrameMode->Description) ) {
            ConfigSetNumber("EnableWireFrameMode",EnableWireFrameMode->IValue);
        }
        if( GUICheckBoxWithTooltip("Ambient Light",(bool *) &EnableAmbientLight->IValue,EnableAmbientLight->Description) ) {
            ConfigSetNumber("EnableAmbientLight",EnableAmbientLight->IValue);
        }
        if( GUICheckBoxWithTooltip("Show FPS",(bool *) &GUIShowFPS->IValue,GUIShowFPS->Description) ) {
            ConfigSetNumber("GUIShowFPS",GUIShowFPS->IValue);
        }
    }
    TreeNodeFlags = RenderObjectManager->BSDList != NULL ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None;
    if( igCollapsingHeader_TreeNodeFlags("RenderObjects List",TreeNodeFlags) ) {
        for(PackIterator = RenderObjectManager->BSDList; PackIterator; PackIterator = PackIterator->Next) {
            TreeNodeFlags = ImGuiTreeNodeFlags_None;
            if(  PackIterator == RenderObjectManagerGetSelectedBSDPack(RenderObjectManager) ) {
                TreeNodeFlags |= ImGuiTreeNodeFlags_DefaultOpen;
            }
            if( igTreeNodeEx_Str(PackIterator->Name,TreeNodeFlags) ) {
                igSameLine(0,-1);
                if( igSmallButton("Remove") ) {
                    if( RenderObjectManagerDeleteBSDPack(RenderObjectManager,PackIterator->Name) ) {
                        igTreePop();
                        break;
                    }
                    ErrorMessageDialogSet(GUI->ErrorMessageDialog,"Failed to remove BSD pack from list");
                }
                for( RenderObjectIterator = PackIterator->RenderObjectList; RenderObjectIterator; 
                    RenderObjectIterator = RenderObjectIterator->Next ) {
                    TreeNodeFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;
                    DisableNode = 0;
                    sprintf(SmallBuffer,"%i (%s)",RenderObjectIterator->Id, BSDGetRenderObjectFileName(RenderObjectIterator));
                    if( RenderObjectIterator == RenderObjectManagerGetSelectedRenderObject(RenderObjectManager) ) {
                        TreeNodeFlags |= ImGuiTreeNodeFlags_Selected;
                        DisableNode = 1;
                    }
                    if( DisableNode ) {
                        igBeginDisabled(1);
                    }
                    if( igTreeNodeEx_Str(SmallBuffer,TreeNodeFlags) ) {
                        if (igIsMouseDoubleClicked_Nil(0) && igIsItemHovered(ImGuiHoveredFlags_None) ) {
                            RenderObjectManagerSetSelectedRenderObject(RenderObjectManager,PackIterator,RenderObjectIterator);
                        }
                    }
                    if( DisableNode ) {
                        igEndDisabled();
                    }
                }
                igTreePop();
            }
        }
    }
    if( igCollapsingHeader_TreeNodeFlags("Current RenderObject Informations",ImGuiTreeNodeFlags_DefaultOpen) ) {
        CurrentRenderObject = RenderObjectManagerGetSelectedRenderObject(RenderObjectManager);
        if(!CurrentRenderObject) {
            igText("No RenderObject selected.");
        } else {
            igText("Id:%u",CurrentRenderObject->Id, CurrentRenderObject->FileName);
            igText("FileName:%s",BSDGetRenderObjectFileName(CurrentRenderObject));
            igText("Scale:%f;%f;%f",CurrentRenderObject->Scale[0],CurrentRenderObject->Scale[1],CurrentRenderObject->Scale[2]);
            igSeparator();
            igText("Export selected model");
            if( igButton("Export to Ply",ZeroSize) ) {
                RenderObjectManagerExportSelectedModel(RenderObjectManager,GUI,VideoSystem,RENDER_OBJECT_MANAGER_EXPORT_FORMAT_PLY,false);
            }
        }
    }
    igEnd();
}
void GUIDrawMenuBar(Application_t *Application)
{
    if( !igBeginMainMenuBar() ) {
        return;
    }
    if (igBeginMenu("File",true)) {
        if( igMenuItem_Bool("Open",NULL,false,true) ) {
            RenderObjectManagerOpenFileDialog(Application->RenderObjectManager,Application->GUI,Application->Engine->VideoSystem);
        }
        if( igMenuItem_Bool("Exit",NULL,false,true) ) {
            Quit(Application);
        }
        igEndMenu();
    }
    if (igBeginMenu("Settings",true)) {
        if( igMenuItem_Bool("Video",NULL,Application->GUI->VideoSettingsWindowHandle,true) ) {
            Application->GUI->VideoSettingsWindowHandle = 1;
        }
        igEndMenu();
    }
    if (igBeginMenu("View",true)) {
        if( igMenuItem_Bool("Debug Window",NULL,Application->GUI->DebugWindowHandle,true) ) {
            Application->GUI->DebugWindowHandle = 1;
        }
        igEndMenu();
    }
    igEndMainMenuBar();
}

void GUIDraw(Application_t *Application)
{
    
    GUIBeginFrame();
    GUIDrawDebugOverlay(Application->Engine->TimeInfo);
    GUIDrawMenuBar(Application);
    FileDialogRenderList();
    ErrorMessageDialogDraw(Application->GUI->ErrorMessageDialog);
    GUIDrawMainWindow(Application->GUI,Application->RenderObjectManager,Application->Engine->VideoSystem,Application->Camera);
    GUIDrawDebugWindow(Application->GUI,Application->Camera,Application->Engine->VideoSystem);
    GUIDrawVideoSettingsWindow(&Application->GUI->VideoSettingsWindowHandle,Application->Engine->VideoSystem);
//     igShowDemoWindow(NULL);
    GUIEndFrame();
}

GUI_t *GUIInit(VideoSystem_t *VideoSystem)
{
    GUI_t *GUI;
    char *ConfigPath;

    GUI = malloc(sizeof(GUI_t));
    
    if( !GUI ) {
        DPrintf("GUIInit:Failed to allocate memory for struct\n");
        return NULL;
    }
    
    memset(GUI,0,sizeof(GUI_t));
    GUI->ErrorMessageDialog = ErrorMessageDialogInit();
    if( !GUI->ErrorMessageDialog ) {
        DPrintf("GUIInit:Failed to initialize error message dialog\n");
        free(GUI);
        return NULL;
    }
    ConfigPath = AppGetConfigPath();
    asprintf(&GUI->ConfigFilePath,"%simgui.ini",ConfigPath);
    free(ConfigPath);

    GUILoadCommonSettings();
    
    GUI->DefaultContext = igCreateContext(NULL);
    GUI->ProgressBar = ProgressBarInitialize(VideoSystem);
    
    if( !GUI->ProgressBar ) {
        DPrintf("GUIInit:Failed to initialize ProgressBar\n");
        free(GUI);
        return NULL;
    }
    GUIContextInit(GUI->DefaultContext,VideoSystem,GUI->ConfigFilePath);
    GUI->NumActiveWindows = 0;

    return GUI;
}
