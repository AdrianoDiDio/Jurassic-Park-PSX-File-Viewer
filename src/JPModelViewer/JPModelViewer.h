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
#ifndef __JPMODELVIEWER_H_
#define __JPMODELVIEWER_H_ 

#include "../Common/Common.h"
#include "../Common/Config.h"
#include "../Common/Video.h"
#include "../Common/Engine.h"
#include "Camera.h"
#include "BSD.h"
#include "GUI.h"
#include "RenderObjectManager.h"

typedef struct Application_s {
    Engine_t                    *Engine;
    RenderObjectManager_t       *RenderObjectManager;
    Camera_t                    *Camera;
    GUI_t                       *GUI;
} Application_t;

void        Quit(Application_t *Application);

#endif //__JPMODELVIEWER_H_
