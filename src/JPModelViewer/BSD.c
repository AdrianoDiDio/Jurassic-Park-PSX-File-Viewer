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
#include "BSD.h"
#include "TSP.h"
#include "JPModelViewer.h" 
#include "../Common/ShaderManager.h"

void BSDRecusivelyFreeHierarchyBone(BSDHierarchyBone_t *Bone)
{
    if( !Bone ) {
        return;
    }
    BSDRecusivelyFreeHierarchyBone(Bone->Child2);
    BSDRecusivelyFreeHierarchyBone(Bone->Child1);
    free(Bone);
}
void BSDFreeRenderObject(BSDRenderObject_t *RenderObject)
{
    int i;
    int j;
    if( !RenderObject ) {
        return;
    }
    if( RenderObject->VertexTable ) {
        for( i = 0; i < RenderObject->NumVertexTables; i++ ) {
            if( RenderObject->VertexTable[i].VertexList ) {
                free(RenderObject->VertexTable[i].VertexList);
            }
            if( RenderObject->CurrentVertexTable[i].VertexList ) {
                free(RenderObject->CurrentVertexTable[i].VertexList);
            }
        }
        free(RenderObject->VertexTable);
        free(RenderObject->CurrentVertexTable);
    }
    if( RenderObject->Vertex ) {
        free(RenderObject->Vertex);
    }
    if( RenderObject->Color ) {
        free(RenderObject->Color);
    }
    if( RenderObject->TexturedFaceList ) {
        free(RenderObject->TexturedFaceList);
    }
    if( RenderObject->UntexturedFaceList ) {
        free(RenderObject->UntexturedFaceList);
    }
    if( RenderObject->FaceList ) {
        free(RenderObject->FaceList);
    }
    if( RenderObject->HierarchyDataRoot ) {
        BSDRecusivelyFreeHierarchyBone(RenderObject->HierarchyDataRoot);
    }
    if( RenderObject->TSP ) {
        TSPFree(RenderObject->TSP);
    }
    if( RenderObject->RenderObjectShader ) {
        free(RenderObject->RenderObjectShader);
    }
    if( RenderObject->AnimationList ) {
        for( i = 0; i < RenderObject->NumAnimations; i++ ) {
            for( j = 0; j < RenderObject->AnimationList[i].NumFrames; j++ ) {
                if( RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList ) {
                    free(RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList);
                }
                if( RenderObject->AnimationList[i].Frame[j].QuaternionList ) {
                    free(RenderObject->AnimationList[i].Frame[j].QuaternionList);
                }
                if( RenderObject->AnimationList[i].Frame[j].CurrentQuaternionList ) {
                    free(RenderObject->AnimationList[i].Frame[j].CurrentQuaternionList);
                }
            }
            free(RenderObject->AnimationList[i].Frame);
        }
        free(RenderObject->AnimationList);
    }
    VAOFree(RenderObject->VAO);
    free(RenderObject->FileName);
    free(RenderObject);
}

void BSDFreeRenderObjectList(BSDRenderObject_t *RenderObjectList)
{
    BSDRenderObject_t *Temp;
    while( RenderObjectList ) {
        Temp = RenderObjectList;
        RenderObjectList = RenderObjectList->Next;
        BSDFreeRenderObject(Temp);
    }
}
void BSDFree(BSD_t *BSD)
{
    if( !BSD ) {
        return;
    }
    if( BSD->RenderObjectTable.RenderObject ) {
        free(BSD->RenderObjectTable.RenderObject);
    }
    free(BSD);
}

char *BSDGetRenderObjectFileName(BSDRenderObject_t *RenderObject)
{
    if( !RenderObject ) {
        return "";
    }
    return RenderObject->Id == 0 ? RenderObject->TSP->FName : 
                        RenderObject->FileName;
}

void BSDRenderObjectExportFacesToPly(BSDRenderObject_t *RenderObject,BSDFace_t *FaceList,int NumFaces,VRAM_t *VRAM,FILE *OutFile)
{
    char Buffer[256];
    float TextureWidth;
    float TextureHeight;
    int i;
    int VRAMPage;
    int ColorMode;
    float U0;
    float V0;
    float U1;
    float V1;
    float U2;
    float V2;
    vec3 VertPos;
    vec3 OutVector;
    vec3 RotationAxis;
    mat4 RotationMatrix;
    mat4 ScaleMatrix;
    mat4 ModelMatrix;
    BSDFace_t *CurrentFace;
    
    if( !RenderObject || !OutFile ) {
        bool InvalidFile = (OutFile == NULL ? true : false);
        DPrintf("BSDRenderObjectExportFacesToPly: Invalid %s\n",InvalidFile ? "file" : "bsd struct");
        return;
    }
    
    if( !NumFaces || !FaceList ) {
        bool InvalidFaces = (FaceList == NULL ? true : false);
        DPrintf("BSDRenderObjectExportFacesToPly: Invalid %s\n",InvalidFaces ? "faces" : "number of faces");
        return;
    }
    
    if( !VRAM ) {
        DPrintf("BSDRenderObjectExportFacesToPly:Invalid VRAM data\n");
        return;
    }
    
    TextureWidth = VRAM->Page.Width;
    TextureHeight = VRAM->Page.Height;
    
    glm_mat4_identity(ModelMatrix);
    glm_mat4_identity(RotationMatrix);
    
    RotationAxis[0] = -1;
    RotationAxis[1] = 0;
    RotationAxis[2] = 0;
    glm_rotate(RotationMatrix,glm_rad(180.f), RotationAxis);    
    glm_scale_make(ScaleMatrix,RenderObject->Scale);
    
    glm_mat4_mul(RotationMatrix,ScaleMatrix,ModelMatrix);
    
    sprintf(Buffer,"ply\nformat ascii 1.0\n");
    fwrite(Buffer,strlen(Buffer),1,OutFile);
    
    sprintf(Buffer,
        "element vertex %i\nproperty float x\nproperty float y\nproperty float z\nproperty float red\nproperty float green\nproperty float "
        "blue\nproperty float s\nproperty float t\n",NumFaces * 3);
    fwrite(Buffer,strlen(Buffer),1,OutFile);
    sprintf(Buffer,"element face %i\nproperty list uchar int vertex_indices\nend_header\n",NumFaces);
    fwrite(Buffer,strlen(Buffer),1,OutFile);

    
    for( i = 0 ; i < NumFaces; i++ ) {
        CurrentFace = &FaceList[i];
        VRAMPage = CurrentFace->TexInfo;
        ColorMode = (CurrentFace->TexInfo & 0xC0) >> 7;
        U0 = (((float)CurrentFace->UV0.u + 
            VRAMGetTexturePageX(VRAMPage))/TextureWidth);
        V0 = 1.f - (((float)CurrentFace->UV0.v +
                    VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
        U1 = (((float)CurrentFace->UV1.u + 
            VRAMGetTexturePageX(VRAMPage)) / TextureWidth);
        V1 = 1.f - (((float)CurrentFace->UV1.v + 
                    VRAMGetTexturePageY(VRAMPage,ColorMode)) /TextureHeight);
        U2 = (((float)CurrentFace->UV2.u + 
            VRAMGetTexturePageX(VRAMPage)) / TextureWidth);
        V2 = 1.f - (((float)CurrentFace->UV2.v + 
                    VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
        
        VertPos[0] = RenderObject->Vertex[CurrentFace->Vert0].x;
        VertPos[1] = RenderObject->Vertex[CurrentFace->Vert0].y;
        VertPos[2] = RenderObject->Vertex[CurrentFace->Vert0].z;
        glm_mat4_mulv3(ModelMatrix,VertPos,1.f,OutVector);
        sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",OutVector[0] / 4096.f, 
                OutVector[1] / 4096.f, OutVector[2] / 4096.f,
                CurrentFace->RGB0.r / 255.f,CurrentFace->RGB0.g / 255.f,CurrentFace->RGB0.b / 255.f,U0,V0);
        fwrite(Buffer,strlen(Buffer),1,OutFile);
        
        VertPos[0] = RenderObject->Vertex[CurrentFace->Vert1].x;
        VertPos[1] = RenderObject->Vertex[CurrentFace->Vert1].y;
        VertPos[2] = RenderObject->Vertex[CurrentFace->Vert1].z;
        glm_mat4_mulv3(ModelMatrix,VertPos,1.f,OutVector);
        sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",OutVector[0] / 4096.f, 
                OutVector[1] / 4096.f, OutVector[2] / 4096.f,
                CurrentFace->RGB1.r / 255.f,CurrentFace->RGB1.g / 255.f,CurrentFace->RGB1.b / 255.f,U1,V1);
        fwrite(Buffer,strlen(Buffer),1,OutFile);

        VertPos[0] = RenderObject->Vertex[CurrentFace->Vert2].x;
        VertPos[1] = RenderObject->Vertex[CurrentFace->Vert2].y;
        VertPos[2] = RenderObject->Vertex[CurrentFace->Vert2].z;
        glm_mat4_mulv3(ModelMatrix,VertPos,1.f,OutVector);
        sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",OutVector[0] / 4096.f, 
                OutVector[1] / 4096.f, OutVector[2] / 4096.f,
                CurrentFace->RGB1.r / 255.f,CurrentFace->RGB1.g / 255.f,CurrentFace->RGB1.b / 255.f,U2,V2);
        fwrite(Buffer,strlen(Buffer),1,OutFile);
    }
    for( i = 0; i < NumFaces; i++ ) {
        int Vert0 = (i * 3) + 0;
        int Vert1 = (i * 3) + 1;
        int Vert2 = (i * 3) + 2;
        sprintf(Buffer,"3 %i %i %i\n",Vert0,Vert1,Vert2);
        fwrite(Buffer,strlen(Buffer),1,OutFile);
    }
}
void BSDRenderObjectExportToPly(BSDRenderObject_t *RenderObject,VRAM_t *VRAM,const char *Directory,char *BSDFileName)
{
    char *PlyFile;
    char *FileName;
    FILE *OutFile;
    
    if( !RenderObject ) {
        DPrintf("BSDRenderObjectExportToPly: Invalid bsd struct\n");
        return;
    }
    
    if( !VRAM ) {
        DPrintf("BSDRenderObjectExportToPly:Invalid VRAM data\n");
        return;
    }
    if( RenderObject->TSP ) {
        asprintf(&FileName,"RenderObject-%s-JP.ply",BSDFileName);
    } else {
        asprintf(&FileName,"RenderObject-%u-JP.ply",RenderObject->Id);
    }
    asprintf(&PlyFile,"%s%c%s",Directory,PATH_SEPARATOR,FileName);
    OutFile = fopen(PlyFile,"w");
    if( !OutFile ) {
        DPrintf("RenderObjectManagerExportSelectedModelToPly:Failed to open %s for writing\n",PlyFile);
        return;
    }
    if( RenderObject->TSP ) {
        TSPDumpDataToPlyFile(RenderObject->TSP,VRAM,OutFile);
    }
    if( RenderObject->NumTexturedFaces > 0 ) {
        BSDRenderObjectExportFacesToPly(RenderObject,RenderObject->TexturedFaceList,RenderObject->NumTexturedFaces,VRAM,OutFile);
    }
    if( RenderObject->NumUntexturedFaces > 0 ) {
        BSDRenderObjectExportFacesToPly(RenderObject,RenderObject->UntexturedFaceList,RenderObject->NumUntexturedFaces,VRAM,OutFile);
    }
    fclose(OutFile);
    free(FileName);
    free(PlyFile);
}

void BSDFillFaceVertexBuffer(int *Buffer,int *BufferSize,BSDVertex_t Vertex,int U0,int V0,BSDColor_t Color,int CLUTX,int CLUTY,int ColorMode,bool Textured)
{
    if( !Buffer ) {
        DPrintf("BSDFillFaceVertexBuffer:Invalid Buffer\n");
        return;
    }
    if( !BufferSize ) {
        DPrintf("BSDFillFaceVertexBuffer:Invalid BufferSize\n");
        return;
    }
    Buffer[*BufferSize] =   Vertex.x;
    Buffer[*BufferSize+1] = Vertex.y;
    Buffer[*BufferSize+2] = Vertex.z;
    Buffer[*BufferSize+3] = U0;
    Buffer[*BufferSize+4] = V0;
    Buffer[*BufferSize+5] = Color.r;
    Buffer[*BufferSize+6] = Color.g;
    Buffer[*BufferSize+7] = Color.b;
    Buffer[*BufferSize+8] = CLUTX;
    Buffer[*BufferSize+9] = CLUTY;
    Buffer[*BufferSize+10] = ColorMode;
    Buffer[*BufferSize+11] = Textured;
    *BufferSize += 12;
}
void BSDRenderObjectGenerateVAO(BSDRenderObject_t *RenderObject)
{
    BSDAnimatedModelFace_t *CurrentFace;
    int VertexOffset;
    int TextureOffset;
    int ColorOffset;
    int CLUTOffset;
    int ColorModeOffset;
    int TexturedOffset;
    int Stride;
    int *VertexData;
    int VertexSize;
    int VertexPointer;
    int VRAMPage;
    int ColorMode;
    int CLUTPage;
    int CLUTPosX;
    int CLUTPosY;
    int CLUTDestX;
    int CLUTDestY;
    int U0;
    int V0;
    int U1;
    int V1;
    int U2;
    int V2;
    int i;
    
    if( !RenderObject ) {
        DPrintf("BSDRenderObjectGenerateVAO:Invalid RenderObject\n");
        return;
    }
//            XYZ UV RGB CLUT ColorMode Textured
    Stride = (3 + 2 + 3 + 2 + 1 + 1) * sizeof(int);
                
    VertexOffset = 0;
    TextureOffset = 3;
    ColorOffset = 5;
    CLUTOffset = 8;
    ColorModeOffset = 10;
    TexturedOffset = 11;
    
    VertexSize = Stride * 3 * RenderObject->NumFaces;
    VertexData = malloc(VertexSize);
    VertexPointer = 0;
    DPrintf("BSDRenderObjectGenerateVAO:Generating for %i faces Id:%i\n",RenderObject->NumFaces,RenderObject->Id);
    for( i = 0; i < RenderObject->NumFaces; i++ ) {
        CurrentFace = &RenderObject->FaceList[i];
        VRAMPage = CurrentFace->TexInfo & 0x1F;
        ColorMode = (CurrentFace->TexInfo >> 7) & 0x3;
        CLUTPosX = (CurrentFace->CLUT << 4) & 0x3F0;
        CLUTPosY = (CurrentFace->CLUT >> 6) & 0x1ff;
        CLUTPage = VRAMGetCLUTPage(CLUTPosX,CLUTPosY);
        CLUTDestX = VRAMGetCLUTPositionX(CLUTPosX,CLUTPosY,CLUTPage);
        CLUTDestY = CLUTPosY + VRAMGetCLUTOffsetY(ColorMode);
        CLUTDestX += VRAMGetTexturePageX(CLUTPage);
 
        U0 = CurrentFace->UV0.u + VRAMGetTexturePageX(VRAMPage);
        V0 = CurrentFace->UV0.v + VRAMGetTexturePageY(VRAMPage,ColorMode);
        U1 = CurrentFace->UV1.u + VRAMGetTexturePageX(VRAMPage);
        V1 = CurrentFace->UV1.v + VRAMGetTexturePageY(VRAMPage,ColorMode);
        U2 = CurrentFace->UV2.u + VRAMGetTexturePageX(VRAMPage);
        V2 = CurrentFace->UV2.v + VRAMGetTexturePageY(VRAMPage,ColorMode);

        
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex0&0x1F].VertexList[CurrentFace->VertexTableDataIndex0],
                                U0,V0,CurrentFace->RGB0,CLUTDestX,CLUTDestY,ColorMode,true
                               );
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex1&0x1F].VertexList[CurrentFace->VertexTableDataIndex1],
                                U1,V1,CurrentFace->RGB1,CLUTDestX,CLUTDestY,ColorMode,true
                               );
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex2&0x1F].VertexList[CurrentFace->VertexTableDataIndex2],
                                U2,V2,CurrentFace->RGB2,CLUTDestX,CLUTDestY,ColorMode,true
                               );
    }
    RenderObject->VAO = VAOInitXYZUVRGBCLUTColorModeTexturedInteger(VertexData,VertexSize,Stride,VertexOffset,TextureOffset,
                                        ColorOffset,CLUTOffset,ColorModeOffset,TexturedOffset,RenderObject->NumFaces * 3);
    free(VertexData);
}
void BSDRenderObjectGenerateStaticTexturedVAO(BSDRenderObject_t *RenderObject)
{
    VAO_t *VAO;
    unsigned short Vert0;
    unsigned short Vert1;
    unsigned short Vert2;
    int *VertexData;
    int VertexSize;
    int VertexPointer;
    int Stride;
    int VertexOffset;
    int TextureOffset;
    int ColorOffset;
    int CLUTOffset;
    int ColorModeOffset;
    int TexturedOffset;
    int U0;
    int V0;
    int U1;
    int V1;
    int U2;
    int V2;
    int VRAMPage;
    int ColorMode;
    int CLUTPosX;
    int CLUTPosY;
    int CLUTPage;
    int CLUTDestX;
    int CLUTDestY;
    int i;
    
    if( !RenderObject->NumTexturedFaces ) {
        return;
    }
//            XYZ UV RGB CLUT ColorMode Textured
    Stride = (3 + 2 + 3 + 2 + 1 + 1) * sizeof(int);
    VertexSize = Stride * 3 * RenderObject->NumTexturedFaces;
    VertexData = malloc(VertexSize);
    VertexPointer = 0;
    VertexOffset = 0;
    TextureOffset = 3;
    ColorOffset = 5;
    CLUTOffset = 8;
    ColorModeOffset = 10;
    TexturedOffset = 11;

    for( i = 0; i < RenderObject->NumTexturedFaces; i++ ) {

        Vert0 = RenderObject->TexturedFaceList[i].Vert0;
        Vert1 = RenderObject->TexturedFaceList[i].Vert1;
        Vert2 = RenderObject->TexturedFaceList[i].Vert2;

        VRAMPage = RenderObject->TexturedFaceList[i].TexInfo & 0x1F;
        ColorMode = (RenderObject->TexturedFaceList[i].TexInfo & 0xC0) >> 7;
        U0 = RenderObject->TexturedFaceList[i].UV0.u + VRAMGetTexturePageX(VRAMPage);
        V0 = RenderObject->TexturedFaceList[i].UV0.v + VRAMGetTexturePageY(VRAMPage,ColorMode);
        U1 = RenderObject->TexturedFaceList[i].UV1.u + VRAMGetTexturePageX(VRAMPage);
        V1 = RenderObject->TexturedFaceList[i].UV1.v + VRAMGetTexturePageY(VRAMPage,ColorMode);
        U2 = RenderObject->TexturedFaceList[i].UV2.u + VRAMGetTexturePageX(VRAMPage);
        V2 = RenderObject->TexturedFaceList[i].UV2.v + VRAMGetTexturePageY(VRAMPage,ColorMode);
        CLUTPosX = (RenderObject->TexturedFaceList[i].CBA << 4) & 0x3F0;
        CLUTPosY = (RenderObject->TexturedFaceList[i].CBA >> 6) & 0x1ff;
        CLUTPage = VRAMGetCLUTPage(CLUTPosX,CLUTPosY);
        CLUTDestX = VRAMGetCLUTPositionX(CLUTPosX,CLUTPosY,CLUTPage);
        CLUTDestY = CLUTPosY + VRAMGetCLUTOffsetY(ColorMode);
        CLUTDestX += VRAMGetTexturePageX(CLUTPage);

        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert0],
                                U0,V0,RenderObject->TexturedFaceList[i].RGB0,CLUTDestX,CLUTDestY,ColorMode,true);
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert1],
                                U1,V1,RenderObject->TexturedFaceList[i].RGB1,CLUTDestX,CLUTDestY,ColorMode,true);
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert2],
                                U2,V2,RenderObject->TexturedFaceList[i].RGB2,CLUTDestX,CLUTDestY,ColorMode,true);
    }
    VAO = 
        VAOInitXYZUVRGBCLUTColorModeTexturedInteger(VertexData,VertexSize,Stride,VertexOffset,TextureOffset,ColorOffset,CLUTOffset,ColorModeOffset,
                                              TexturedOffset,RenderObject->NumTexturedFaces * 3);
    VAO->Next = RenderObject->VAO;
    RenderObject->VAO = VAO;
    free(VertexData);
}
void BSDRenderObjectGenerateStaticUntexturedVAO(BSDRenderObject_t *RenderObject)
{
    VAO_t *VAO;
    unsigned short Vert0;
    unsigned short Vert1;
    unsigned short Vert2;
    int *VertexData;
    int VertexSize;
    int VertexPointer;
    int Stride;
    int VertexOffset;
    int TextureOffset;
    int ColorOffset;
    int CLUTOffset;
    int ColorModeOffset;
    int TexturedOffset;
    int i;
    
    if( !RenderObject->NumUntexturedFaces ) {
        return;
    }
//            XYZ UV RGB CLUT ColorMode Textured
    Stride = (3 + 2 + 3 + 2 + 1 + 1) * sizeof(int);
    VertexSize = Stride * 3 * RenderObject->NumUntexturedFaces;
    VertexData = malloc(VertexSize);
    VertexPointer = 0;
    VertexOffset = 0;
    TextureOffset = 3;
    ColorOffset = 5;
    CLUTOffset = 8;
    ColorModeOffset = 10;
    TexturedOffset = 11;

    for( i = 0; i < RenderObject->NumUntexturedFaces; i++ ) {
        Vert0 = RenderObject->UntexturedFaceList[i].Vert0;
        Vert1 = RenderObject->UntexturedFaceList[i].Vert1;
        Vert2 = RenderObject->UntexturedFaceList[i].Vert2;

        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert0],
                                0,0,RenderObject->UntexturedFaceList[i].RGB0,0,0,0,false);
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert1],
                                0,0,RenderObject->UntexturedFaceList[i].RGB1,0,0,0,false);
        BSDFillFaceVertexBuffer(VertexData,&VertexPointer,
                                RenderObject->Vertex[Vert2],
                                0,0,RenderObject->UntexturedFaceList[i].RGB2,0,0,0,false);


    }
    VAO = 
        VAOInitXYZUVRGBCLUTColorModeTexturedInteger(VertexData,VertexSize,Stride,VertexOffset,TextureOffset,ColorOffset,CLUTOffset,ColorModeOffset,
                                              TexturedOffset,RenderObject->NumUntexturedFaces * 3);
    VAO->Next = RenderObject->VAO;
    RenderObject->VAO = VAO;
    free(VertexData);
}
void BSDRenderObjectUpdateVAO(BSDRenderObject_t *RenderObject)
{
    BSDAnimatedModelFace_t *CurrentFace;
    int Stride;
    int BaseOffset;
    int VertexData[3];
    int i;
    
    if( !RenderObject ) {
        DPrintf("BSDRenderObjectUpdateVAO:Invalid RenderObject\n");
        return;
    }
    if( !RenderObject->VAO ) {
        DPrintf("BSDRenderObjectUpdateVAO:Invalid VAO\n");
        return;
    }
    
    Stride = (3 + 2 + 3 + 2 + 1) * sizeof(int);
    glBindBuffer(GL_ARRAY_BUFFER, RenderObject->VAO->VBOId[0]);
    
    for( i = 0; i < RenderObject->NumFaces; i++ ) {
        BaseOffset = (i * Stride * 3);
        CurrentFace = &RenderObject->FaceList[i];
        VertexData[0] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex0&0x1F].VertexList[CurrentFace->VertexTableDataIndex0].x;
        VertexData[1] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex0&0x1F].VertexList[CurrentFace->VertexTableDataIndex0].y;
        VertexData[2] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex0&0x1F].VertexList[CurrentFace->VertexTableDataIndex0].z;

        glBufferSubData(GL_ARRAY_BUFFER, BaseOffset + (Stride * 0), 3 * sizeof(int), &VertexData);
        
        VertexData[0] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex1&0x1F].VertexList[CurrentFace->VertexTableDataIndex1].x;
        VertexData[1] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex1&0x1F].VertexList[CurrentFace->VertexTableDataIndex1].y;
        VertexData[2] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex1&0x1F].VertexList[CurrentFace->VertexTableDataIndex1].z;

        glBufferSubData(GL_ARRAY_BUFFER, BaseOffset + (Stride * 1), 3 * sizeof(int), &VertexData);
        
        VertexData[0] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex2&0x1F].VertexList[CurrentFace->VertexTableDataIndex2].x;
        VertexData[1] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex2&0x1F].VertexList[CurrentFace->VertexTableDataIndex2].y;
        VertexData[2] = RenderObject->CurrentVertexTable[CurrentFace->VertexTableIndex2&0x1F].VertexList[CurrentFace->VertexTableDataIndex2].z;

        glBufferSubData(GL_ARRAY_BUFFER, BaseOffset + (Stride * 2), 3 * sizeof(int), &VertexData);
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
void BSDRecursivelyApplyHierachyData(const BSDHierarchyBone_t *Bone,const BSDQuaternion_t *QuaternionList,BSDVertexTable_t *VertexTable,
                                     mat4 TransformMatrix)
{
    versor Quaternion;
    mat4 LocalRotationMatrix;
    mat4 LocalTranslationMatrix;
    mat4 LocalTransformMatrix;
    vec3 TransformedBonePosition;
    vec3 TransformedVertexPosition;
    vec3 Temp;
    int i;
    
    if( !Bone ) {
        DPrintf("BSDRecursivelyApplyHierachyData:NULL Bone.\n");
        return;
    }
    
    if( !QuaternionList ) {
        DPrintf("BSDRecursivelyApplyHierachyData:Invalid Quaternion List.\n");
        return;
    }
    if( !VertexTable ) {
        DPrintf("BSDRecursivelyApplyHierachyData:Invalid Vertex Table.\n");
        return;
    }

    glm_mat4_identity(LocalRotationMatrix);
    glm_mat4_identity(LocalTranslationMatrix);
    glm_mat4_identity(LocalTransformMatrix);
    
    Quaternion[0] = QuaternionList[Bone->VertexTableIndex].x / 4096.f;
    Quaternion[1] = QuaternionList[Bone->VertexTableIndex].y / 4096.f;
    Quaternion[2] = QuaternionList[Bone->VertexTableIndex].z / 4096.f;
    Quaternion[3] = QuaternionList[Bone->VertexTableIndex].w / 4096.f;
    
    glm_quat_mat4t(Quaternion,LocalRotationMatrix);
    
    Temp[0] = Bone->Position.x;
    Temp[1] = Bone->Position.y;
    Temp[2] = Bone->Position.z;
    
    glm_mat4_mulv3(TransformMatrix,Temp,1.f,TransformedBonePosition);
    glm_translate_make(LocalTranslationMatrix,TransformedBonePosition);
    glm_mat4_mul(LocalTranslationMatrix,LocalRotationMatrix,LocalTransformMatrix);

    if( VertexTable[Bone->VertexTableIndex].Offset != -1 && VertexTable[Bone->VertexTableIndex].NumVertex != 0 ) {
        for( i = 0; i < VertexTable[Bone->VertexTableIndex].NumVertex; i++ ) {
            Temp[0] = VertexTable[Bone->VertexTableIndex].VertexList[i].x;
            Temp[1] = VertexTable[Bone->VertexTableIndex].VertexList[i].y;
            Temp[2] = VertexTable[Bone->VertexTableIndex].VertexList[i].z;
            glm_mat4_mulv3(LocalTransformMatrix,Temp,1.f,TransformedVertexPosition);
            VertexTable[Bone->VertexTableIndex].VertexList[i].x = TransformedVertexPosition[0];
            VertexTable[Bone->VertexTableIndex].VertexList[i].y = TransformedVertexPosition[1];
            VertexTable[Bone->VertexTableIndex].VertexList[i].z = TransformedVertexPosition[2];
        }
    }

    if( Bone->Child2 ) {
        BSDRecursivelyApplyHierachyData(Bone->Child2,QuaternionList,VertexTable,TransformMatrix);
    }
    if( Bone->Child1 ) {
        BSDRecursivelyApplyHierachyData(Bone->Child1,QuaternionList,VertexTable,LocalTransformMatrix);
    }
}
void BSDRenderObjectResetVertexTable(BSDRenderObject_t *RenderObject)
{
    int i;

    if( !RenderObject ) {
        DPrintf("BSDRenderObjectResetVertexTable:Invalid RenderObject\n");
        return;
    }
    for( i = 0; i < RenderObject->NumVertexTables; i++ ) {
        RenderObject->CurrentVertexTable[i].Offset = RenderObject->VertexTable[i].Offset;
        RenderObject->CurrentVertexTable[i].NumVertex = RenderObject->VertexTable[i].NumVertex;
        memcpy(RenderObject->CurrentVertexTable[i].VertexList,
               RenderObject->VertexTable[i].VertexList,sizeof(BSDVertex_t) * RenderObject->VertexTable[i].NumVertex);
    }
}
void BSDRenderObjectResetFrameQuaternionList(BSDAnimationFrame_t *Frame)
{
    if( !Frame ) {
        return;
    }
    memcpy(Frame->CurrentQuaternionList,Frame->QuaternionList,Frame->NumQuaternions * sizeof(BSDQuaternion_t));
}
BSDAnimationFrame_t *BSDRenderObjectGetCurrentFrame(BSDRenderObject_t *RenderObject)
{
    if( !RenderObject ) {
        return NULL;
    }
    if( RenderObject->CurrentAnimationIndex == -1 || RenderObject->CurrentFrameIndex == -1 ) {
        return NULL;
    }
    return &RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].Frame[RenderObject->CurrentFrameIndex];
}
/*
 Set the RenderObject to a specific pose, given AnimationIndex and FrameIndex.
 Returns 0 if the pose was not valid ( pose was already set,pose didn't exists), 1 otherwise.
 NOTE that calling this function will modify the RenderObject's VAO.
 If the VAO is NULL a new one is created otherwise it will be updated to reflect the pose that was applied to the model.
 If Override is true then the pose will be set again in case the AnimationIndex and FrameIndex did not change.
 */
int BSDRenderObjectSetAnimationPose(BSDRenderObject_t *RenderObject,int AnimationIndex,int FrameIndex,int Override)
{
    BSDQuaternion_t *QuaternionList;
    versor FromQuaternion;
    versor ToQuaternion;
    versor DestQuaternion;
    mat4 TransformMatrix;
    vec3 Translation;
    int NumVertices;
    int i;

    if( AnimationIndex < 0 || AnimationIndex > RenderObject->NumAnimations ) {
        DPrintf("BSDRenderObjectSetAnimationPose:Failed to set pose using index %i...Index is out of bounds\n",AnimationIndex);
        return 0;
    }
    if( (AnimationIndex == RenderObject->CurrentAnimationIndex && FrameIndex == RenderObject->CurrentFrameIndex ) && !Override) {
        DPrintf("BSDRenderObjectSetAnimationPose:Pose is already set\n");
        return 0;
    }
    if( !RenderObject->AnimationList[AnimationIndex].NumFrames ) {
        DPrintf("BSDRenderObjectSetAnimationPose:Failed to set pose using index %i...animation has no frames\n",AnimationIndex);
        return 0;
    }
    if( FrameIndex < 0 || FrameIndex > RenderObject->AnimationList[AnimationIndex].NumFrames ) {
        DPrintf("BSDRenderObjectSetAnimationPose:Failed to set pose using frame %i...Frame Index is out of bounds\n",FrameIndex);
        return 0;
    }
    BSDRenderObjectResetVertexTable(RenderObject);
    glm_vec3_zero(RenderObject->Center);
    glm_mat4_identity(TransformMatrix);
    Translation[0] = RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].Vector.x / 4096.f;
    Translation[1] = RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].Vector.y / 4096.f;
    Translation[2] = RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].Vector.z / 4096.f;
    glm_translate_make(TransformMatrix,Translation);
    //NOTE(Adriano):Interpolate only between frames of the same animation and not in-between two different one.
    //              Also do not interpolate if the frame is the same as the previous one.
    if( RenderObject->CurrentAnimationIndex == AnimationIndex && RenderObject->CurrentFrameIndex != -1 && 
        RenderObject->CurrentFrameIndex != FrameIndex
    ) {
        assert(RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].NumQuaternions == 
            RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].Frame[RenderObject->CurrentFrameIndex].NumQuaternions
        );
        QuaternionList = malloc(RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].NumQuaternions * sizeof(BSDQuaternion_t));
        for( i = 0; i < RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].NumQuaternions; i++ ) {
            FromQuaternion[0] = RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].
                Frame[RenderObject->CurrentFrameIndex].QuaternionList[i].x / 4096.f;
            FromQuaternion[1] = RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].
                Frame[RenderObject->CurrentFrameIndex].QuaternionList[i].y / 4096.f;
            FromQuaternion[2] = RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].
                Frame[RenderObject->CurrentFrameIndex].QuaternionList[i].z / 4096.f;
            FromQuaternion[3] = RenderObject->AnimationList[RenderObject->CurrentAnimationIndex].
                Frame[RenderObject->CurrentFrameIndex].QuaternionList[i].w / 4096.f;
            ToQuaternion[0] = RenderObject->AnimationList[AnimationIndex].
                Frame[FrameIndex].QuaternionList[i].x / 4096.f;
            ToQuaternion[1] = RenderObject->AnimationList[AnimationIndex].
                Frame[FrameIndex].QuaternionList[i].y / 4096.f;
            ToQuaternion[2] = RenderObject->AnimationList[AnimationIndex].
                Frame[FrameIndex].QuaternionList[i].z / 4096.f;
            ToQuaternion[3] = RenderObject->AnimationList[AnimationIndex].
                Frame[FrameIndex].QuaternionList[i].w / 4096.f;
            glm_quat_nlerp(FromQuaternion,
                ToQuaternion,
                0.5f,
                DestQuaternion
            );
            QuaternionList[i].x = DestQuaternion[0] * 4096.f;
            QuaternionList[i].y = DestQuaternion[1] * 4096.f;
            QuaternionList[i].z = DestQuaternion[2] * 4096.f;
            QuaternionList[i].w = DestQuaternion[3] * 4096.f;
        }
        BSDRecursivelyApplyHierachyData(RenderObject->HierarchyDataRoot,QuaternionList,
                                    RenderObject->CurrentVertexTable,TransformMatrix);
        free(QuaternionList);
    } else {
        BSDRecursivelyApplyHierachyData(RenderObject->HierarchyDataRoot,
                                    RenderObject->AnimationList[AnimationIndex].Frame[FrameIndex].CurrentQuaternionList,
                                    RenderObject->CurrentVertexTable,TransformMatrix);
    }
    RenderObject->CurrentAnimationIndex = AnimationIndex;
    RenderObject->CurrentFrameIndex = FrameIndex;
    
    NumVertices = 0;
    for( int i = 0; i < RenderObject->NumVertexTables; i++ ) {
        for( int j = 0; j < RenderObject->CurrentVertexTable[i].NumVertex; j++ ) {
            RenderObject->Center[0] += RenderObject->CurrentVertexTable[i].VertexList[j].x;
            RenderObject->Center[1] += RenderObject->CurrentVertexTable[i].VertexList[j].y;
            RenderObject->Center[2] += RenderObject->CurrentVertexTable[i].VertexList[j].z;
            NumVertices++;
        }
    }
    glm_vec3_scale(RenderObject->Center,1.f/NumVertices,RenderObject->Center);
    if( !RenderObject->VAO ) {
        BSDRenderObjectGenerateVAO(RenderObject);
    } else {
        BSDRenderObjectUpdateVAO(RenderObject);
    }
    return 1;
}

int BSDCreateRenderObjectShader(BSDRenderObject_t *RenderObject)
{
    Shader_t *Shader;

    if( !RenderObject ) {
        DPrintf("BSDCreateRenderObjectShader:Invalid RenderObject\n");
        return 0;
    }
    Shader = ShaderCache("RenderObjectShader","Shaders/RenderObjectVertexShader.glsl","Shaders/RenderObjectFragmentShader.glsl");
    if( !Shader ) {
        DPrintf("BSDCreateRenderObjectShader:Couldn't cache Shader.\n");
        return 0;
    }
    RenderObject->RenderObjectShader = malloc(sizeof(RenderObjectShader_t));
    if( !RenderObject->RenderObjectShader ) {
        DPrintf("BSDCreateRenderObjectShader:Failed to allocate memory for shader\n");
        return 0;
    }
    RenderObject->RenderObjectShader->Shader = Shader;
    glUseProgram(RenderObject->RenderObjectShader->Shader->ProgramId);
    RenderObject->RenderObjectShader->MVPMatrixId = glGetUniformLocation(Shader->ProgramId,"MVPMatrix");
    RenderObject->RenderObjectShader->EnableLightingId = glGetUniformLocation(Shader->ProgramId,"enableLighting");
    RenderObject->RenderObjectShader->TextureIndexId = glGetUniformLocation(Shader->ProgramId,"indexTexture");
    RenderObject->RenderObjectShader->PaletteTextureId = glGetUniformLocation(Shader->ProgramId,"paletteTexture");
    glUniform1i(RenderObject->RenderObjectShader->TextureIndexId, 0);
    glUniform1i(RenderObject->RenderObjectShader->PaletteTextureId,  1);
    glUniform1i(RenderObject->RenderObjectShader->EnableLightingId, 1);
    glUseProgram(0);
    return 1;
}
void BSDRenderObjectGenerateVAOs(BSDRenderObject_t *RenderObject)
{
    BSDRenderObjectGenerateStaticTexturedVAO(RenderObject);
    BSDRenderObjectGenerateStaticUntexturedVAO(RenderObject);

}
void BSDDrawRenderObject(BSDRenderObject_t *RenderObject,VRAM_t *VRAM,Camera_t *Camera,mat4 ProjectionMatrix)
{
    vec3 Temp;
    mat4 ModelMatrix;
    mat4 ModelViewMatrix;
    mat4 MVPMatrix;
    VAO_t *Iterator;
    
    if( !RenderObject ) {
        return;
    }
    
    if( !RenderObject->RenderObjectShader ) {
        BSDCreateRenderObjectShader(RenderObject);
    }
    
    if( RenderObject->TSP ) {
        TSPDrawList(RenderObject->TSP,VRAM,Camera,RenderObject->RenderObjectShader,ProjectionMatrix);
        return;
    }
    if( !RenderObject->VAO ) {
        BSDRenderObjectGenerateVAOs(RenderObject);
    }
        
    if( EnableWireFrameMode->IValue ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    glm_mat4_identity(ModelMatrix);
    glm_mat4_identity(ModelViewMatrix);
    Temp[0] = -RenderObject->Center[0];
    Temp[1] = -RenderObject->Center[1];
    Temp[2] = -RenderObject->Center[2];
    glm_vec3_rotate(Temp, DEGTORAD(180.f), GLM_XUP);    
    glm_translate(ModelMatrix,Temp);
    Temp[0] = 0;
    Temp[1] = 1;
    Temp[2] = 0;
    glm_rotate(ModelMatrix,glm_rad(-90), Temp);
    glm_scale(ModelMatrix,RenderObject->Scale);
    glm_mat4_mul(Camera->ViewMatrix,ModelMatrix,ModelViewMatrix);
    glm_mat4_mul(ProjectionMatrix,ModelViewMatrix,MVPMatrix);
    //Emulate PSX Coordinate system...
    glm_rotate_x(MVPMatrix,glm_rad(180.f), MVPMatrix);
        
    glUseProgram(RenderObject->RenderObjectShader->Shader->ProgramId);
    glUniform1i(RenderObject->RenderObjectShader->EnableLightingId, EnableAmbientLight->IValue);
    glUniformMatrix4fv(RenderObject->RenderObjectShader->MVPMatrixId,1,false,&MVPMatrix[0][0]);
    
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, VRAM->TextureIndexPage.TextureId);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, VRAM->PalettePage.TextureId);

    glDisable(GL_BLEND);
    for( Iterator = RenderObject->VAO; Iterator; Iterator = Iterator->Next ) {
        glBindVertexArray(Iterator->VAOId[0]);
        glDrawArrays(GL_TRIANGLES, 0, Iterator->Count);
        glBindVertexArray(0);
    }
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_BLEND);
    glBlendColor(1.f, 1.f, 1.f, 1.f);
    glUseProgram(0);
    if( EnableWireFrameMode->IValue ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void BSDDrawRenderObjectList(BSDRenderObject_t *RenderObjectList,VRAM_t *VRAM,Camera_t *Camera,mat4 ProjectionMatrix)
{
    BSDRenderObject_t *Iterator;
    int i;
    if( !RenderObjectList ) {
        return;
    }

    i = 0;
    for( Iterator = RenderObjectList; Iterator; Iterator = Iterator->Next ) {
        BSDDrawRenderObject(Iterator,VRAM,Camera,ProjectionMatrix);
        i++;
    }
}

int BSDReadEntryTableChunk(BSD_t *BSD,FILE *BSDFile)
{    
    if( !BSD || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDReadEntryTableChunk: Invalid %s\n",InvalidFile ? "file" : "BSD struct");
        return 0;
    }

    fseek(BSDFile,BSD_ENTRY_TABLE_FILE_POSITION + BSD_HEADER_SIZE,SEEK_SET);
    DPrintf("Reading table at %li\n",ftell(BSDFile));
    assert(sizeof(BSD->EntryTable) == 80);
    fread(&BSD->EntryTable,sizeof(BSD->EntryTable),1,BSDFile);
    DPrintf("Node table is at %i (%i)\n",BSD->EntryTable.NodeTableOffset,BSD->EntryTable.NodeTableOffset + BSD_HEADER_SIZE);
    DPrintf("Unknown data is at %i (%i)\n",BSD->EntryTable.UnknownDataOffset,BSD->EntryTable.UnknownDataOffset + BSD_HEADER_SIZE);
    DPrintf("AnimationTableOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationTableOffset,
            BSD->EntryTable.AnimationTableOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationTableEntries);
    DPrintf("AnimationDataOffset is at %i (%i) contains %i elements.\n",BSD->EntryTable.AnimationDataOffset,
            BSD->EntryTable.AnimationDataOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationData);
    DPrintf("AnimationQuaternionDataOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationQuaternionDataOffset,
            BSD->EntryTable.AnimationQuaternionDataOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationQuaternionData);
    DPrintf("AnimationHierarchyDataOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationHierarchyDataOffset,
            BSD->EntryTable.AnimationHierarchyDataOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationHierarchyData);
    DPrintf("AnimationFaceTableOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationFaceTableOffset,
            BSD->EntryTable.AnimationFaceTableOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationFaceTables);
    DPrintf("AnimationFaceDataOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationFaceDataOffset,
            BSD->EntryTable.AnimationFaceDataOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationFaces);
    DPrintf("AnimationVertexTableIndexOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationVertexTableIndexOffset,
            BSD->EntryTable.AnimationVertexTableIndexOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationVertexTableIndex);
    DPrintf("AnimationVertexTableOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationVertexTableOffset,
            BSD->EntryTable.AnimationVertexTableOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationVertexTableEntry);
    DPrintf("AnimationVertexDataOffset is at %i (%i) and contains %i elements.\n",BSD->EntryTable.AnimationVertexDataOffset,
            BSD->EntryTable.AnimationVertexDataOffset + BSD_HEADER_SIZE,BSD->EntryTable.NumAnimationVertex);
    return 1;
}
BSDRenderObjectElement_t *BSDGetRenderObjectById(const BSD_t *BSD,unsigned int RenderObjectId)
{
    int i;
    for( i = 0; i < BSD->RenderObjectTable.NumRenderObject; i++ ) {
        if( BSD->RenderObjectTable.RenderObject[i].Id == RenderObjectId ) {
            return &BSD->RenderObjectTable.RenderObject[i];
        }
    }
    return NULL;
}
int BSDGetRenderObjectIndexById(const BSD_t *BSD,unsigned int RenderObjectId)
{
    int i;
    for( i = 0; i < BSD->RenderObjectTable.NumRenderObject; i++ ) {
        if( BSD->RenderObjectTable.RenderObject[i].Id == RenderObjectId ) {
            return i;
        }
    }
    return -1;
}
void BSDAppendRenderObjectToList(BSDRenderObject_t **List,BSDRenderObject_t *Node)
{
    BSDRenderObject_t *LastNode;
    if( !*List ) {
        *List = Node;
    } else {
        LastNode = *List;
        while( LastNode->Next ) {
            LastNode = LastNode->Next;
        }
        LastNode->Next = Node;
    }
}

int BSDReadRenderObjectChunk(BSD_t *BSD,FILE *BSDFile)
{
    int FirstRenderObjectPosition;
    int i;
    
    if( !BSD || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDReadRenderObjectChunk: Invalid %s\n",InvalidFile ? "file" : "BSD struct");
        return 0;
    }
    
    fseek(BSDFile,BSD_RENDER_OBJECT_STARTING_OFFSET + BSD_HEADER_SIZE,SEEK_SET);
    fread(&BSD->RenderObjectTable.NumRenderObject,sizeof(BSD->RenderObjectTable.NumRenderObject),1,BSDFile);
    FirstRenderObjectPosition = GetCurrentFilePosition(BSDFile);
    
    DPrintf("BSDReadRenderObjectChunk:Reading %i RenderObject Elements...size %lu\n",
            BSD->RenderObjectTable.NumRenderObject,sizeof(BSDRenderObjectElement_t));
    
    assert(sizeof(BSDRenderObjectElement_t) == JP_RENDER_OBJECT_SIZE);
    
    BSD->RenderObjectTable.RenderObject = malloc(BSD->RenderObjectTable.NumRenderObject * sizeof(BSDRenderObjectElement_t));
    if( !BSD->RenderObjectTable.RenderObject ) {
        DPrintf("BSDReadRenderObjectChunk:Failed to allocate memory for RenderObject Array\n");
        return 0;
    }
    for( i = 0; i < BSD->RenderObjectTable.NumRenderObject; i++ ) {
        assert(GetCurrentFilePosition(BSDFile) == FirstRenderObjectPosition + (i * JP_RENDER_OBJECT_SIZE));
        assert(sizeof(BSD->RenderObjectTable.RenderObject[i]) == JP_RENDER_OBJECT_SIZE);
        DPrintf("Reading RenderObject %i at %i\n",i,GetCurrentFilePosition(BSDFile));
        fread(&BSD->RenderObjectTable.RenderObject[i],sizeof(BSD->RenderObjectTable.RenderObject[i]),1,BSDFile);
        DPrintf("RenderObject Id:%i\n",BSD->RenderObjectTable.RenderObject[i].Id);
        DPrintf("RenderObject File:%s\n",BSD->RenderObjectTable.RenderObject[i].FileName);
//         DPrintf("RenderObject Type:%i\n",BSD->RenderObjectTable.RenderObject[i].Type);
//         if( BSD->RenderObjectTable.RenderObject[i].ReferencedRenderObjectId != -1 ) {
//             DPrintf("RenderObject References RenderObject Id:%i\n",BSD->RenderObjectTable.RenderObject[i].ReferencedRenderObjectId);
//         } else {
//             DPrintf("RenderObject No Reference set...\n");
//         }
        DPrintf("RenderObject Element TSP Offset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].TSPOffset,
                BSD->RenderObjectTable.RenderObject[i].TSPOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject Element Animation Offset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].AnimationDataOffset,
                BSD->RenderObjectTable.RenderObject[i].AnimationDataOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject Element Unknown Offset0: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].UnknownOffset0,
                BSD->RenderObjectTable.RenderObject[i].UnknownOffset0 + BSD_HEADER_SIZE);
        DPrintf("RenderObject Element Vertex Offset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].VertexOffset,
                BSD->RenderObjectTable.RenderObject[i].VertexOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject Element NumVertex: %i\n",BSD->RenderObjectTable.RenderObject[i].NumVertex);
        DPrintf("RenderObject AltAltFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].AltAltFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].AltAltFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject AltUntexturedFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].AltUntexturedFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].AltUntexturedFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject AltFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].AltFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].AltFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject AltTexturedFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].AltTexturedFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].AltTexturedFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject UntexturedFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].UntexturedFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].UntexturedFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject TexturedFaceOffset: %i (%i)\n",BSD->RenderObjectTable.RenderObject[i].TexturedFaceOffset,
                BSD->RenderObjectTable.RenderObject[i].TexturedFaceOffset + BSD_HEADER_SIZE);
        DPrintf("RenderObject Scale: %i;%i;%i (4096 is 1 meaning no scale)\n",
                 BSD->RenderObjectTable.RenderObject[i].ScaleX / 4,
                 BSD->RenderObjectTable.RenderObject[i].ScaleY / 4,
                 BSD->RenderObjectTable.RenderObject[i].ScaleZ / 4);
 
    }
    return 1;
}

int BSDLoadAnimationVertexData(BSDRenderObject_t *RenderObject,int VertexTableIndexOffset,BSDEntryTable_t EntryTable,FILE *BSDFile)
{
    int VertexTableOffset;
    int i;
    int j;
    
    if( !RenderObject || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDLoadAnimationVertexData: Invalid %s\n",InvalidFile ? "file" : "RenderObject struct");
        return 0;
    }
    if( VertexTableIndexOffset == -1 ) {
        DPrintf("BSDLoadAnimationVertexData:Invalid Vertex Table Index Offset\n");
        return 0;
    }
  
    VertexTableIndexOffset += EntryTable.AnimationVertexTableIndexOffset + BSD_HEADER_SIZE;
    fseek(BSDFile,VertexTableIndexOffset,SEEK_SET);
    fread(&VertexTableOffset,sizeof(VertexTableOffset),1,BSDFile);
    fread(&RenderObject->NumVertexTables,sizeof(RenderObject->NumVertexTables),1,BSDFile);
    VertexTableOffset += EntryTable.AnimationVertexTableOffset + BSD_HEADER_SIZE;
    fseek(BSDFile,VertexTableOffset,SEEK_SET);
    
    RenderObject->VertexTable = malloc(RenderObject->NumVertexTables * sizeof(BSDVertexTable_t));

    if( !RenderObject->VertexTable ) {
        DPrintf("BSDLoadAnimationVertexData:Failed to allocate memory for VertexTable.\n");
        return 0;
    }
    RenderObject->CurrentVertexTable = malloc(RenderObject->NumVertexTables * sizeof(BSDVertexTable_t));
    if( !RenderObject->CurrentVertexTable ) {
        DPrintf("BSDLoadAnimationVertexData:Failed to allocate memory for VertexTable.\n");
        return 0;
    }
    for( i = 0; i < RenderObject->NumVertexTables; i++ ) {
        fread(&RenderObject->VertexTable[i].Offset,sizeof(RenderObject->VertexTable[i].Offset),1,BSDFile);
        fread(&RenderObject->VertexTable[i].NumVertex,sizeof(RenderObject->VertexTable[i].NumVertex),1,BSDFile);
        DPrintf("Table Index %i has %i vertices\n",i,RenderObject->VertexTable[i].NumVertex);
        RenderObject->VertexTable[i].VertexList = NULL;
        
        RenderObject->CurrentVertexTable[i].Offset = RenderObject->VertexTable[i].Offset;
        RenderObject->CurrentVertexTable[i].NumVertex = RenderObject->VertexTable[i].NumVertex;
        RenderObject->CurrentVertexTable[i].VertexList = NULL;
    }
    
    fseek(BSDFile,EntryTable.AnimationVertexDataOffset + BSD_HEADER_SIZE,SEEK_SET);
    for( i = 0; i < RenderObject->NumVertexTables; i++ ) {
        if( RenderObject->VertexTable[i].Offset == -1 ) {
            continue;
        }
        fseek(BSDFile,EntryTable.AnimationVertexDataOffset + RenderObject->VertexTable[i].Offset + BSD_HEADER_SIZE,SEEK_SET);
        RenderObject->VertexTable[i].VertexList = malloc(RenderObject->VertexTable[i].NumVertex * sizeof(BSDVertex_t));
        RenderObject->CurrentVertexTable[i].VertexList = malloc(RenderObject->VertexTable[i].NumVertex * sizeof(BSDVertex_t));
        for( j = 0; j < RenderObject->VertexTable[i].NumVertex; j++ ) {
            fread(&RenderObject->VertexTable[i].VertexList[j],sizeof(BSDVertex_t),1,BSDFile);
            RenderObject->CurrentVertexTable[i].VertexList[j] = RenderObject->VertexTable[i].VertexList[j];
        }
    }
    return 1;
}
void BSDCopyAnimatedModelFace(BSDAnimatedModelFace_t Src,BSDAnimatedModelFace_t *Dest)
{
    if( !Dest ) {
        DPrintf("BSDCopyAnimatedModelFace:Invalid Destination\n");
        return;
    }
    Dest->RGB0 = Src.RGB0;
    Dest->RGB1 = Src.RGB1;
    Dest->RGB2 = Src.RGB2;
    Dest->UV0 = Src.UV0;
    Dest->UV1 = Src.UV1;
    Dest->UV2 = Src.UV2;
    Dest->CLUT = Src.CLUT;
    Dest->TexInfo = Src.TexInfo;
    Dest->VertexTableIndex0 = Src.VertexTableIndex0;
    Dest->VertexTableDataIndex0 = Src.VertexTableDataIndex0;
    Dest->VertexTableIndex1 = Src.VertexTableIndex1;
    Dest->VertexTableDataIndex1 = Src.VertexTableDataIndex1;
    Dest->VertexTableIndex2 = Src.VertexTableIndex2;
    Dest->VertexTableDataIndex2 = Src.VertexTableDataIndex2;
}
void BSDPrintAnimatedModelFace(BSDAnimatedModelFace_t Face)
{
    int ColorMode;
    int VRAMPage;
    int ABRRate;
    int CLUTPosX;
    int CLUTPosY;
    
    ColorMode = (Face.TexInfo >> 7) & 0x3;
    VRAMPage = Face.TexInfo & 0x1F;
    ABRRate = (Face.TexInfo & 0x60) >> 5;
    CLUTPosX = (Face.CLUT << 4) & 0x3F0;
    CLUTPosY = (Face.CLUT >> 6) & 0x1ff;
    DPrintf("Tex info %i | Color mode %i | Texture Page %i | ABR Rate %i\n",Face.TexInfo,
            ColorMode,VRAMPage,ABRRate);
    DPrintf("CLUT:%i X:%i Y:%i\n",Face.CLUT,CLUTPosX,CLUTPosY);
    DPrintf("UV0:(%i;%i)\n",Face.UV0.u,Face.UV0.v);
    DPrintf("UV1:(%i;%i)\n",Face.UV1.u,Face.UV1.v);
    DPrintf("UV2:(%i;%i)\n",Face.UV2.u,Face.UV2.v);
    DPrintf("RGB0:(%i;%i;%i)\n",Face.RGB0.r,Face.RGB0.g,Face.RGB0.b);
    DPrintf("RGB1:(%i;%i;%i)\n",Face.RGB1.r,Face.RGB1.g,Face.RGB1.b);
    DPrintf("RGB2:(%i;%i;%i)\n",Face.RGB2.r,Face.RGB2.g,Face.RGB2.b);
    DPrintf("Table Index0 %i Data %i.\n",Face.VertexTableIndex0&0x1F,Face.VertexTableDataIndex0);
    DPrintf("Table Index1 %i Data %i.\n",Face.VertexTableIndex1&0x1F,Face.VertexTableDataIndex1);
    DPrintf("Table Index2 %i Data %i.\n",Face.VertexTableIndex2&0x1F,Face.VertexTableDataIndex2);
}

int BSDLoadAnimationFaceData(BSDRenderObject_t *RenderObject,int FaceTableOffset,int RenderObjectIndex,
                             BSDEntryTable_t EntryTable,FILE *BSDFile)
{
    int GlobalFaceTableOffset;
    int GlobalFaceDataOffset;
    int FaceDataOffset;
    int NumFaces;
    int i;
    
    if( !RenderObject || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDLoadAnimationFaceData: Invalid %s\n",InvalidFile ? "file" : "RenderObject struct");
        return 0;
    }
    if( FaceTableOffset == -1 ) {
        DPrintf("BSDLoadAnimationFaceData:Invalid Face Table Index Offset\n");
        return 0;
    }
    GlobalFaceTableOffset = EntryTable.AnimationFaceTableOffset + FaceTableOffset + BSD_HEADER_SIZE;
    fseek(BSDFile,GlobalFaceTableOffset,SEEK_SET);
    fread(&FaceDataOffset,sizeof(FaceDataOffset),1,BSDFile);
    fread(&NumFaces,sizeof(NumFaces),1,BSDFile);
    GlobalFaceDataOffset = EntryTable.AnimationFaceDataOffset + FaceDataOffset + BSD_HEADER_SIZE;
    fseek(BSDFile,GlobalFaceDataOffset,SEEK_SET);
    assert(sizeof(BSDAnimatedModelFace_t) == 28);
    RenderObject->FaceList = malloc(NumFaces * sizeof(BSDAnimatedModelFace_t));
    RenderObject->NumFaces = NumFaces;
    if( !RenderObject->FaceList ) {
        DPrintf("BSDLoadAnimationFaceData:Failed to allocate memory for face list.\n");
        return 0;
    }
    DPrintf("BSDLoadAnimationFaceData:Loading %i faces\n",NumFaces);
    for( i = 0; i < NumFaces; i++ ) {
        fread(&RenderObject->FaceList[i],sizeof(BSDAnimatedModelFace_t),1,BSDFile);
        DPrintf(" -- FACE %i --\n",i);
        BSDPrintAnimatedModelFace(RenderObject->FaceList[i]);
    }
    return 1;
}

BSDHierarchyBone_t *BSDRecursivelyLoadHierarchyData(int BoneDataStartingPosition,int BoneOffset,FILE *BSDFile)
{
    BSDHierarchyBone_t *Bone;
    int Child1Offset;
    int Child2Offset;
    
    if( !BSDFile ) {
        DPrintf("BSDRecursivelyLoadHierarchyData:Invalid Bone Table file\n");
        return NULL;
    }

    
    Bone = malloc(sizeof(BSDHierarchyBone_t));
    
    if( !Bone ) {
        DPrintf("BSDRecursivelyLoadHierarchyData:Failed to allocate bone data\n");
        return NULL;
    }

    Bone->Child1 = NULL;
    Bone->Child2 = NULL;

    fseek(BSDFile,BoneDataStartingPosition + BoneOffset + BSD_HEADER_SIZE,SEEK_SET);
    fread(&Bone->VertexTableIndex,sizeof(Bone->VertexTableIndex),1,BSDFile);
    fread(&Bone->Position,sizeof(Bone->Position),1,BSDFile);
    fread(&Bone->Pad,sizeof(Bone->Pad),1,BSDFile);
    fread(&Child1Offset,sizeof(Child1Offset),1,BSDFile);
    fread(&Child2Offset,sizeof(Child2Offset),1,BSDFile);
    
    DPrintf("Bone:VertexTableIndex:%i\n",Bone->VertexTableIndex);
    DPrintf("Bone:Position:%i;%i;%i\n",Bone->Position.x,Bone->Position.y,Bone->Position.z);

    assert(  Bone->Pad == -12851 );

    if( Child2Offset != -1 ) {
        Bone->Child2 = BSDRecursivelyLoadHierarchyData(BoneDataStartingPosition,Child2Offset,BSDFile);
    }
    if( Child1Offset != -1 ) {
        Bone->Child1 = BSDRecursivelyLoadHierarchyData(BoneDataStartingPosition,Child1Offset,BSDFile);
    }
    return Bone;
}

int BSDLoadAnimationHierarchyData(BSDRenderObject_t *RenderObject,int HierarchyDataRootOffset,BSDEntryTable_t EntryTable,FILE *BSDFile)
{
    if( !RenderObject || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDLoadAnimationHierarchyData: Invalid %s\n",InvalidFile ? "file" : "RenderObject struct");
        return 0;
    }
    if( HierarchyDataRootOffset == -1 ) {
        DPrintf("BSDLoadAnimationHierarchyData:Invalid Face Table Index Offset\n");
        return 0;
    }
    
    RenderObject->HierarchyDataRoot = BSDRecursivelyLoadHierarchyData(EntryTable.AnimationHierarchyDataOffset,HierarchyDataRootOffset,BSDFile);
    
    if( !RenderObject->HierarchyDataRoot ) {
        DPrintf("BSDLoadAnimationHierarchyData:Couldn't load hierarchy data\n");
        return 0;
    }
    return 1;
}
void BSDDecodeQuaternions(int QuatPart0,int QuatPart1,int QuatPart2,BSDQuaternion_t *OutQuaternion1,BSDQuaternion_t *OutQuaternion2)
{
    if( OutQuaternion1 ) {
        OutQuaternion1->x = ( (QuatPart0 << 0x10) >> 0x14) * 2;
        OutQuaternion1->y = (QuatPart1 << 0x14) >> 0x13;
        OutQuaternion1->z = ( ( ( (QuatPart1 >> 0xC) << 0x1C ) >> 0x14) | ( (QuatPart0 >> 0xC) & 0xF0) | (QuatPart0 & 0xF) ) * 2;
        OutQuaternion1->w = (QuatPart0 >> 0x14) * 2;
    }
    if( OutQuaternion2 ) {
        OutQuaternion2->x = (QuatPart1  >> 0x14) * 2;
        OutQuaternion2->y = ( (QuatPart2 << 0x4) >> 0x14 ) * 2;
        OutQuaternion2->w = ( (QuatPart2 << 0x10) >> 0x14) * 2;
        OutQuaternion2->z = ( (QuatPart2 >> 0x1C) << 0x8 | (QuatPart2 & 0xF ) << 0x4 | ( (QuatPart1 >> 0x10) & 0xF ) ) * 2;
    }
}
int BSDLoadAnimationData(BSDRenderObject_t *RenderObject,int AnimationDataOffset,BSDEntryTable_t EntryTable,FILE *BSDFile)
{
    short NumAnimationOffset;
    unsigned short Pad;
    BSDAnimationTableEntry_t *AnimationTableEntry;
    int *AnimationOffsetTable;
    int QuaternionListOffset;
    int i;
    int j;
    int w;
    int q;
    int Base;
    int QuatPart0;
    int QuatPart1;
    int QuatPart2;
    int NumEncodedQuaternions;
    int NumDecodedQuaternions;
    int NextFrame;
    int PrevFrame;
    int Jump;
    versor FromQuaternion;
    versor ToQuaternion;
    versor DestQuaternion;
    
    if( !RenderObject || !BSDFile ) {
        bool InvalidFile = (BSDFile == NULL ? true : false);
        printf("BSDLoadAnimationData: Invalid %s\n",InvalidFile ? "file" : "RenderObject struct");
        return 0;
    }
    if( AnimationDataOffset == -1 ) {
        DPrintf("BSDLoadAnimationData:Invalid Vertex Table Index Offset\n");
        return 0;
    }
    fseek(BSDFile,AnimationDataOffset + BSD_HEADER_SIZE,SEEK_SET);
    fread(&NumAnimationOffset,sizeof(NumAnimationOffset),1,BSDFile);
    fread(&Pad,sizeof(Pad),1,BSDFile);
    assert(Pad == 52685);
    
    AnimationOffsetTable = malloc(NumAnimationOffset * sizeof(int) );
    RenderObject->NumAnimations = NumAnimationOffset;
    for( i = 0; i < NumAnimationOffset; i++ ) {
        fread(&AnimationOffsetTable[i],sizeof(AnimationOffsetTable[i]),1,BSDFile);
        if( AnimationOffsetTable[i] == -1 ) {
            continue;
        }
    }

    AnimationTableEntry = malloc(RenderObject->NumAnimations * sizeof(BSDAnimationTableEntry_t) );
    for( i = 0; i < NumAnimationOffset; i++ ) {
        DPrintf("BSDLoadAnimationData:Animation Offset %i for entry %i\n",AnimationOffsetTable[i],i);
        if( AnimationOffsetTable[i] == -1 ) {
            continue;
        }
        DPrintf("BSDLoadAnimationData:Going to %i plus %i\n",EntryTable.AnimationTableOffset,AnimationOffsetTable[i]);
        fseek(BSDFile,EntryTable.AnimationTableOffset + AnimationOffsetTable[i] + BSD_HEADER_SIZE,SEEK_SET);
        fread(&AnimationTableEntry[i],sizeof(AnimationTableEntry[i]),1,BSDFile);
        DPrintf("BSDLoadAnimationData:AnimationEntry %i has pad %i\n",i,AnimationTableEntry[i].Pad);
        DPrintf("BSDLoadAnimationData:Loading %i vertices Size %i\n",AnimationTableEntry[i].NumAffectedVertex,
                AnimationTableEntry[i].NumAffectedVertex * 8);
        DPrintf("BSDLoadAnimationData: NumFrames %u NumAffectedVertex:%u || Offset %i\n",AnimationTableEntry[i].NumFrames,
                AnimationTableEntry[i].NumAffectedVertex,AnimationTableEntry[i].Offset);
        assert(AnimationTableEntry[i].Pad == 52480);
    }
    RenderObject->AnimationList = malloc(RenderObject->NumAnimations * sizeof(BSDAnimation_t));
    for( i = 0; i < NumAnimationOffset; i++ ) {
        RenderObject->AnimationList[i].Frame = NULL;
        RenderObject->AnimationList[i].NumFrames = 0;
        if( AnimationOffsetTable[i] == -1 ) {
            continue;
        }
        DPrintf(" -- ANIMATION ENTRY %i -- \n",i);
        DPrintf("Loading %i animations for entry %i\n",AnimationTableEntry[i].NumFrames,i);
        
        RenderObject->AnimationList[i].Frame = malloc(AnimationTableEntry[i].NumFrames * sizeof(BSDAnimationFrame_t));
        RenderObject->AnimationList[i].NumFrames = AnimationTableEntry[i].NumFrames;
        for( j = 0; j < AnimationTableEntry[i].NumFrames; j++ ) {
            DPrintf(" -- FRAME %i/%i -- \n",j,AnimationTableEntry[i].NumFrames);
            // 20 is the sizeof an animation
            fseek(BSDFile,EntryTable.AnimationDataOffset + AnimationTableEntry[i].Offset + BSD_HEADER_SIZE 
                + j * BSD_ANIMATION_FRAME_DATA_SIZE,SEEK_SET);
            DPrintf("Reading animation definition at %li each entry is %li bytes\n",ftell(BSDFile),sizeof(BSDAnimationFrame_t));

            fread(&RenderObject->AnimationList[i].Frame[j].U0,sizeof(RenderObject->AnimationList[i].Frame[j].U0),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].U4,sizeof(RenderObject->AnimationList[i].Frame[j].U4),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].EncodedVector,
                  sizeof(RenderObject->AnimationList[i].Frame[j].EncodedVector),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].U1,sizeof(RenderObject->AnimationList[i].Frame[j].U1),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].U2,sizeof(RenderObject->AnimationList[i].Frame[j].U2),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].U3,sizeof(RenderObject->AnimationList[i].Frame[j].U3),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].U5,sizeof(RenderObject->AnimationList[i].Frame[j].U5),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].FrameInterpolationIndex,
                  sizeof(RenderObject->AnimationList[i].Frame[j].FrameInterpolationIndex),1,BSDFile);
            fread(&RenderObject->AnimationList[i].Frame[j].NumQuaternions,
                  sizeof(RenderObject->AnimationList[i].Frame[j].NumQuaternions),1,BSDFile);
            fread(&QuaternionListOffset,sizeof(QuaternionListOffset),1,BSDFile);

            RenderObject->AnimationList[i].Frame[j].Vector.x = (RenderObject->AnimationList[i].Frame[j].EncodedVector << 0x16) >> 0x16;
            RenderObject->AnimationList[i].Frame[j].Vector.y = (RenderObject->AnimationList[i].Frame[j].EncodedVector << 0xb)  >> 0x15;
            RenderObject->AnimationList[i].Frame[j].Vector.z = (RenderObject->AnimationList[i].Frame[j].EncodedVector << 0x1)  >> 0x16;
            
            RenderObject->AnimationList[i].Frame[j].Vector.x = (RenderObject->AnimationList[i].Frame[j].EncodedVector << 6) >> 6;
            RenderObject->AnimationList[i].Frame[j].Vector.y = (RenderObject->AnimationList[i].Frame[j].EncodedVector << 5)  >> 5;
            RenderObject->AnimationList[i].Frame[j].Vector.z = (RenderObject->AnimationList[i].Frame[j].EncodedVector >> 15) >> 6;
            DPrintf("Entry %i => U0|U1|U2: %i;%i;%i QuaternionListOffset:%i\n",i,RenderObject->AnimationList[i].Frame[j].U0,
                    RenderObject->AnimationList[i].Frame[j].U1,
                    RenderObject->AnimationList[i].Frame[j].U2,QuaternionListOffset);
            DPrintf("U3: %i\n",RenderObject->AnimationList[i].Frame[j].U3);
            DPrintf("U4 is %i\n",RenderObject->AnimationList[i].Frame[j].U4);
            DPrintf("U5 is %i\n",RenderObject->AnimationList[i].Frame[j].U5);
            DPrintf("Frame Interpolation Index is %i -- Number of quaternions is %i\n",
                    RenderObject->AnimationList[i].Frame[j].FrameInterpolationIndex,
                RenderObject->AnimationList[i].Frame[j].NumQuaternions
            );
            DPrintf("Encoded Vector is %i\n",RenderObject->AnimationList[i].Frame[j].EncodedVector);
            DPrintf("We are at %li  AnimOffset:%i LocalOffset:%i Index %i times 20 (%i)\n",ftell(BSDFile),
                EntryTable.AnimationDataOffset,AnimationTableEntry[i].Offset,j,j*20
            );
            assert(ftell(BSDFile) - (EntryTable.AnimationDataOffset + AnimationTableEntry[i].Offset + BSD_HEADER_SIZE 
                + j * BSD_ANIMATION_FRAME_DATA_SIZE) == BSD_ANIMATION_FRAME_DATA_SIZE );
            RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList = NULL;
            RenderObject->AnimationList[i].Frame[j].QuaternionList = NULL;
            RenderObject->AnimationList[i].Frame[j].CurrentQuaternionList = NULL;
            if( QuaternionListOffset != -1 ) {
                fseek(BSDFile,EntryTable.AnimationQuaternionDataOffset + QuaternionListOffset + BSD_HEADER_SIZE,SEEK_SET);
                DPrintf("Reading Vector definition at %li\n",ftell(BSDFile));
                NumEncodedQuaternions = (RenderObject->AnimationList[i].Frame[j].NumQuaternions / 2) * 3;
                if( (RenderObject->AnimationList[i].Frame[j].NumQuaternions & 1 ) != 0 ) {
                    NumEncodedQuaternions += 2;
                }
                RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList = malloc( NumEncodedQuaternions * sizeof(int));
                for( w = 0; w < NumEncodedQuaternions; w++ ) {
                    DPrintf("Reading Encoded quaternion at %li\n",ftell(BSDFile));
                    fread(&RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[w],
                          sizeof(RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[w]),1,BSDFile);
                }
                DPrintf("Done...loaded a list of %i encoded quaternions\n",RenderObject->AnimationList[i].Frame[j].NumQuaternions * 2);
                RenderObject->AnimationList[i].Frame[j].QuaternionList = malloc( 
                    RenderObject->AnimationList[i].Frame[j].NumQuaternions * sizeof(BSDQuaternion_t));
                RenderObject->AnimationList[i].Frame[j].CurrentQuaternionList = malloc( 
                    RenderObject->AnimationList[i].Frame[j].NumQuaternions * sizeof(BSDQuaternion_t));
                NumDecodedQuaternions = 0;
                for( q = 0; q < RenderObject->AnimationList[i].Frame[j].NumQuaternions / 2; q++ ) {
                    Base = q * 3;
//                     DPrintf("Generating with base %i V0:%i V1:%i V2:%i\n",q,Base,Base+1,Base+2);
                    QuatPart0 = RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[Base];
                    QuatPart1 = RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[Base+1];
                    QuatPart2 = RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[Base+2];
                    
                    BSDDecodeQuaternions(QuatPart0,QuatPart1,QuatPart2,
                                         &RenderObject->AnimationList[i].Frame[j].QuaternionList[NumDecodedQuaternions],
                                         &RenderObject->AnimationList[i].Frame[j].QuaternionList[NumDecodedQuaternions+1]);
                    NumDecodedQuaternions += 2;
                }
//                 DPrintf("Decoded %i quaternions out of %i\n",NumDecodedQuaternions,RenderObject->AnimationList[i].Frame[j].NumQuaternions);
                if( NumDecodedQuaternions == (RenderObject->AnimationList[i].Frame[j].NumQuaternions - 1) ) {
                    QuatPart0 = RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[NumEncodedQuaternions-2];
                    QuatPart1 = RenderObject->AnimationList[i].Frame[j].EncodedQuaternionList[NumEncodedQuaternions-1];
//                     DPrintf("QuatPart0:%i QuatPart1:%i\n",QuatPart0,QuatPart1);
                    BSDDecodeQuaternions(QuatPart0,QuatPart1,-1,
                                         &RenderObject->AnimationList[i].Frame[j].QuaternionList[NumDecodedQuaternions],
                                         NULL);
//                     DPrintf("New quat is %i;%i;%i;%i\n",TempQuaternion.x,TempQuaternion.y,TempQuaternion.z,TempQuaternion.w);
                    NumDecodedQuaternions++;
                }
                DPrintf("Decoded %i out of %i\n",NumDecodedQuaternions,RenderObject->AnimationList[i].Frame[j].NumQuaternions);
                assert(NumDecodedQuaternions == RenderObject->AnimationList[i].Frame[j].NumQuaternions);
                BSDRenderObjectResetFrameQuaternionList(&RenderObject->AnimationList[i].Frame[j]);
            } else {
                DPrintf("QuaternionListOffset is not valid...\n");
            }
        }
    }
    for( i = 0; i < RenderObject->NumAnimations; i++ ) {
        if( AnimationOffsetTable[i] == -1 ) {
            continue;
        }
        for( j = 0; j < AnimationTableEntry[i].NumFrames; j++ ) {
            if( RenderObject->AnimationList[i].Frame[j].QuaternionList != NULL ) {
                continue;
            }
            RenderObject->AnimationList[i].Frame[j].QuaternionList = malloc(sizeof(BSDQuaternion_t) * 
                RenderObject->AnimationList[i].Frame[j].NumQuaternions);
            RenderObject->AnimationList[i].Frame[j].CurrentQuaternionList = malloc(sizeof(BSDQuaternion_t) * 
                RenderObject->AnimationList[i].Frame[j].NumQuaternions);
            NextFrame = j + (HighNibble(RenderObject->AnimationList[i].Frame[j].FrameInterpolationIndex));
            PrevFrame = j - (LowNibble(RenderObject->AnimationList[i].Frame[j].FrameInterpolationIndex));
            Jump = NextFrame-PrevFrame;
            DPrintf("Current FrameIndex:%i\n",j);
            DPrintf("Next FrameIndex:%i\n",NextFrame);
            DPrintf("Previous FrameIndex:%i\n",PrevFrame);
            DPrintf("Jump:%i\n",Jump);
            DPrintf("NumQuaternions:%i\n",RenderObject->AnimationList[i].Frame[j].NumQuaternions);
            for( q = 0; q < RenderObject->AnimationList[i].Frame[j].NumQuaternions; q++ ) {
                FromQuaternion[0] = RenderObject->AnimationList[i].Frame[PrevFrame].QuaternionList[q].x / 4096.f;
                FromQuaternion[1] = RenderObject->AnimationList[i].Frame[PrevFrame].QuaternionList[q].y / 4096.f;
                FromQuaternion[2] = RenderObject->AnimationList[i].Frame[PrevFrame].QuaternionList[q].z / 4096.f;
                FromQuaternion[3] = RenderObject->AnimationList[i].Frame[PrevFrame].QuaternionList[q].w / 4096.f;
                ToQuaternion[0] = RenderObject->AnimationList[i].Frame[NextFrame].QuaternionList[q].x / 4096.f;
                ToQuaternion[1] = RenderObject->AnimationList[i].Frame[NextFrame].QuaternionList[q].y / 4096.f;
                ToQuaternion[2] = RenderObject->AnimationList[i].Frame[NextFrame].QuaternionList[q].z / 4096.f;
                ToQuaternion[3] = RenderObject->AnimationList[i].Frame[NextFrame].QuaternionList[q].w / 4096.f;
                glm_quat_nlerp(FromQuaternion,
                    ToQuaternion,
                    1.f/Jump,
                    DestQuaternion
                );
                RenderObject->AnimationList[i].Frame[j].QuaternionList[q].x = DestQuaternion[0] * 4096.f;
                RenderObject->AnimationList[i].Frame[j].QuaternionList[q].y = DestQuaternion[1] * 4096.f;
                RenderObject->AnimationList[i].Frame[j].QuaternionList[q].z = DestQuaternion[2] * 4096.f;
                RenderObject->AnimationList[i].Frame[j].QuaternionList[q].w = DestQuaternion[3] * 4096.f;
            }
            BSDRenderObjectResetFrameQuaternionList(&RenderObject->AnimationList[i].Frame[j]);
        }
    }
    free(AnimationOffsetTable);
    free(AnimationTableEntry);
    return 1;
}
int BSDParseRenderObjectVertexAndColorData(BSDRenderObject_t *RenderObject,BSDRenderObjectElement_t *RenderObjectElement,FILE *BSDFile)
{
    int i;
    int Size;
    
    RenderObject->Vertex = NULL;
    RenderObject->Color = NULL;
    if( RenderObjectElement->VertexOffset != 0 ) {
        Size = RenderObjectElement->NumVertex * sizeof(BSDVertex_t);
        RenderObject->Vertex = malloc(Size);
        if( !RenderObject->Vertex ) {
            DPrintf("BSDParseRenderObjectVertexData:Failed to allocate memory for VertexData\n");
            return 0;
        } 
        memset(RenderObject->Vertex,0,Size);
        fseek(BSDFile,RenderObjectElement->VertexOffset + 2048,SEEK_SET);
        DPrintf("Reading Vertex definition at %i (Current:%i)\n",
                RenderObjectElement->VertexOffset + 2048,GetCurrentFilePosition(BSDFile)); 
        for( i = 0; i < RenderObjectElement->NumVertex; i++ ) {
            DPrintf("Reading Vertex at %i (%i)\n",GetCurrentFilePosition(BSDFile),GetCurrentFilePosition(BSDFile) - 2048);
            fread(&RenderObject->Vertex[i],sizeof(BSDVertex_t),1,BSDFile);
            DPrintf("Vertex %i;%i;%i %i\n",RenderObject->Vertex[i].x,
                    RenderObject->Vertex[i].y,RenderObject->Vertex[i].z,
                    RenderObject->Vertex[i].Pad
            );
        }
    }
    return 1;
}
void DecodeVertexData(int EncodedVertex,unsigned int *OutVertexIndex1,unsigned int *OutVertexIndex2,unsigned int *OutVertexIndex3)
{
    if( OutVertexIndex1 ) {
        *OutVertexIndex1 = EncodedVertex & 0xFF;
    }
    if( OutVertexIndex2 ) {
        *OutVertexIndex2 = (EncodedVertex & 0x3fc00) >> 10;
    }
    if( OutVertexIndex3 ) {
        *OutVertexIndex3 = (EncodedVertex & 0xFF00000 ) >> 20;
    }
}
void BSDMapColorFrom127To255(BSDColor_t *InColor,BSDColor_t *OutColor)
{
    OutColor->r = (InColor->r / 127) * 255;
    OutColor->g = (InColor->g / 127) * 255;
    OutColor->b = (InColor->b / 127) * 255;
}
void BSDFaceGT3PacketToBSDFace(BSDFace_t *Face,BSDFaceGT3Packet_t Packet)
{
        Face->TexInfo = Packet.TexInfo;
        Face->CBA = Packet.CBA;
        Face->UV0 = Packet.UV0;
        Face->UV1 = Packet.UV1;
        Face->UV2 = Packet.UV2;
        
        BSDMapColorFrom127To255(&Packet.RGB0,&Face->RGB0);
        BSDMapColorFrom127To255(&Packet.RGB1,&Face->RGB1);
        BSDMapColorFrom127To255(&Packet.RGB2,&Face->RGB2);
}
void BSDFaceFT3PacketToBSDFace(BSDFace_t *Face,BSDFaceFT3Packet_t Packet)
{
        Face->TexInfo = Packet.TexInfo;
        Face->CBA = Packet.CBA;
        Face->UV0 = Packet.UV0;
        Face->UV1 = Packet.UV1;
        Face->UV2 = Packet.UV2;
        
        BSDMapColorFrom127To255(&Packet.RGB0,&Face->RGB0);
        BSDMapColorFrom127To255(&Packet.RGB0,&Face->RGB1);
        BSDMapColorFrom127To255(&Packet.RGB0,&Face->RGB2);
}

void BSDFaceG3PacketToBSDFace(BSDFace_t *Face,BSDFaceG3Packet_t Packet)
{        
        BSDMapColorFrom127To255(&Packet.RGB0,&Face->RGB0);
        BSDMapColorFrom127To255(&Packet.RGB1,&Face->RGB1);
        BSDMapColorFrom127To255(&Packet.RGB2,&Face->RGB2);
}
/*
 * Parse the Textured face data at the given offset, UseFTPacket can be enabled to parse faces that do not requires a different color per vertex
 * Every time this function gets called the TexturedFaceList array gets created if NULL or expanded if not and NumTexturedFaces will reflect
 * the actual number of faces loaded (depending how many times this function was called).
*/
int BSDParseRenderObjectTexturedFaceData(BSDRenderObject_t *RenderObject,BSDRenderObjectElement_t *RenderObjectElement,FILE *BSDFile,int Offset,
                                         bool UseFTPacket)
{
    unsigned int   Vert0;
    unsigned int   Vert1;
    unsigned int   Vert2;
    unsigned int   PackedVertexData;
    int            FaceListSize;
    int            i;
    int            NumTexturedFaces;
    int            BaseIndex;
    BSDFace_t      *TempFaceList;
    BSDFaceGT3Packet_t FaceData[2];
    BSDFaceFT3Packet_t FlatFaceData[2];

    if( !RenderObject ) {
        DPrintf("BSDParseRenderObjectTexturedFaceData:Invalid RenderObject!\n");
        return 0;
    }
    
    if( !Offset ) {
        DPrintf("BSDParseRenderObjectTexturedFaceData:Invalid TexturedFaceOffset for RenderObject %i!\n",RenderObjectElement->Id);
        return 0;
    }
    fseek(BSDFile,Offset + 2048,SEEK_SET);
    fread(&NumTexturedFaces,sizeof(int),1,BSDFile);
    DPrintf("BSDParseRenderObjectTexturedFaceData:Reading %i faces\n",NumTexturedFaces);
    FaceListSize = (NumTexturedFaces + RenderObject->NumTexturedFaces) * sizeof(BSDFace_t);
    if( !RenderObject->TexturedFaceList ) {
        RenderObject->TexturedFaceList = malloc(FaceListSize);
        if( !RenderObject->TexturedFaceList ) {
            DPrintf("BSDParseRenderObjectUnTexturedFaceData:Failed to allocate memory for face array\n");
            return 0;
        }
        memset(RenderObject->TexturedFaceList,0,FaceListSize);
    } else {
        TempFaceList = realloc(RenderObject->TexturedFaceList,FaceListSize);
        if( !TempFaceList ) {
            DPrintf("BSDParseRenderObjectTexturedFaceData:Failed to re-allocate memory for face array\n");
            return 0;
        }
        RenderObject->TexturedFaceList = TempFaceList;
    }
    BaseIndex = RenderObject->NumTexturedFaces;
    RenderObject->NumTexturedFaces += NumTexturedFaces;
    int FirstFaceDef = GetCurrentFilePosition(BSDFile);
    for( i = BaseIndex; i < BaseIndex + NumTexturedFaces; i++ ) {
        DPrintf("BSDParseRenderObjectTexturedFaceData:Reading Face at %i (%i)\n",GetCurrentFilePosition(BSDFile),GetCurrentFilePosition(BSDFile) - 2048);
        DPrintf(" -- FACE %i --\n",i);
        if( UseFTPacket ) {
            DPrintf("Using FT packet (size %li)\n",sizeof(BSDFaceFT3Packet_t));
            fread(&FlatFaceData,sizeof(FlatFaceData),1,BSDFile);
            BSDFaceFT3PacketToBSDFace(&RenderObject->TexturedFaceList[i],FlatFaceData[0]);
        } else {
            fread(&FaceData,sizeof(FaceData),1,BSDFile);
            BSDFaceGT3PacketToBSDFace(&RenderObject->TexturedFaceList[i],FaceData[0]);
        }
        DPrintf("Tex info %i | Color mode %i | Texture Page %i\n",RenderObject->TexturedFaceList[i].TexInfo,
                (RenderObject->TexturedFaceList[i].TexInfo & 0xC0) >> 7,RenderObject->TexturedFaceList[i].TexInfo & 0x1f);
        DPrintf("CBA is %i %ix%i\n",RenderObject->TexturedFaceList[i].CBA,
                ((RenderObject->TexturedFaceList[i].CBA  & 0x3F ) << 4),((RenderObject->TexturedFaceList[i].CBA & 0x7FC0) >> 6));
        DPrintf("UV0:(%i;%i)\n",RenderObject->TexturedFaceList[i].UV0.u,RenderObject->TexturedFaceList[i].UV0.v);
        DPrintf("UV1:(%i;%i)\n",RenderObject->TexturedFaceList[i].UV1.u,RenderObject->TexturedFaceList[i].UV1.v);
        DPrintf("UV2:(%i;%i)\n",RenderObject->TexturedFaceList[i].UV2.u,RenderObject->TexturedFaceList[i].UV2.v);
        DPrintf("RGB0:(%i;%i;%i)\n",RenderObject->TexturedFaceList[i].RGB0.r,RenderObject->TexturedFaceList[i].RGB0.g,
                RenderObject->TexturedFaceList[i].RGB0.b);
        DPrintf("RGB1:(%i;%i;%i)\n",RenderObject->TexturedFaceList[i].RGB1.r,RenderObject->TexturedFaceList[i].RGB1.g,
                RenderObject->TexturedFaceList[i].RGB1.b);
        DPrintf("RGB2:(%i;%i;%i)\n",RenderObject->TexturedFaceList[i].RGB2.r,RenderObject->TexturedFaceList[i].RGB2.g,
                RenderObject->TexturedFaceList[i].RGB2.b);
        DPrintf("Reading vertex data at %i\n",GetCurrentFilePosition(BSDFile) - FirstFaceDef);
        fread(&PackedVertexData,sizeof(PackedVertexData),1,BSDFile);
        DPrintf("Packed Vertex data is %u\n",PackedVertexData);

        DecodeVertexData(PackedVertexData, &Vert0, &Vert1, &Vert2);        
        DPrintf("V0|V1|V2:%u;%u;%u (%i)\n",Vert0,Vert1,Vert2,RenderObject->NumVertex);
        RenderObject->TexturedFaceList[i].Vert0 = Vert0;
        RenderObject->TexturedFaceList[i].Vert1 = Vert1;
        RenderObject->TexturedFaceList[i].Vert2 = Vert2;
        DPrintf("V0|V1|V2:(%i;%i;%i)|(%i;%i;%i)|(%i;%i;%i)\n",
                RenderObject->Vertex[Vert0].x,RenderObject->Vertex[Vert0].y,RenderObject->Vertex[Vert0].z,
                RenderObject->Vertex[Vert1].x,RenderObject->Vertex[Vert1].y,RenderObject->Vertex[Vert1].z,
                RenderObject->Vertex[Vert2].x,RenderObject->Vertex[Vert2].y,RenderObject->Vertex[Vert2].z);
    }
    return 1;
}
/*
 * Parse the Non-Textured face data at the given offset.
 * Every time this function gets called the UntexturedFaceList array gets created if NULL or expanded if not and NumUntexturedFaces will reflect
 * the actual number of faces loaded (depending how many times this function was called).
*/
int BSDParseRenderObjectUntexturedFaceData(BSDRenderObject_t *RenderObject,BSDRenderObjectElement_t *RenderObjectElement,FILE *BSDFile,int Offset)
{
    unsigned int   Vert0;
    unsigned int   Vert1;
    unsigned int   Vert2;
    unsigned int   PackedVertexData;
    int            FaceListSize;
    int            i;
    int            NumUntexturedFaces;
    int            BaseIndex;
    BSDFace_t     *TempFaceList;
    BSDFaceG3Packet_t FaceData[2];
    if( !RenderObject ) {
        DPrintf("BSDParseRenderObjectUnTexturedFaceData:Invalid RenderObject!\n");
        return 0;
    }
    
    if( !Offset ) {
        DPrintf("BSDParseRenderObjectUnTexturedFaceData:Invalid Offset for RenderObject %i!\n",RenderObjectElement->Id);
        return 0;
    }
    fseek(BSDFile,Offset + 2048,SEEK_SET);
    fread(&NumUntexturedFaces,sizeof(int),1,BSDFile);
    DPrintf("BSDParseRenderObjectFaceData:Reading %i faces\n",NumUntexturedFaces);
    FaceListSize = (NumUntexturedFaces + RenderObject->NumUntexturedFaces) * sizeof(BSDFace_t);
    if( !RenderObject->UntexturedFaceList ) {
        RenderObject->UntexturedFaceList = malloc(FaceListSize);
        if( !RenderObject->UntexturedFaceList ) {
            DPrintf("BSDParseRenderObjectUnTexturedFaceData:Failed to allocate memory for face array\n");
            return 0;
        }
        memset(RenderObject->UntexturedFaceList,0,FaceListSize);
    } else {
        TempFaceList = realloc(RenderObject->UntexturedFaceList,FaceListSize);
        if( !TempFaceList ) {
            DPrintf("BSDParseRenderObjectUnTexturedFaceData:Failed to re-allocate memory for face array\n");
            return 0;
        }
        RenderObject->UntexturedFaceList = TempFaceList;
    }
    BaseIndex = RenderObject->NumUntexturedFaces;
    RenderObject->NumUntexturedFaces += NumUntexturedFaces;
    int FirstFaceDef = GetCurrentFilePosition(BSDFile);
    for( i = BaseIndex; i < BaseIndex + NumUntexturedFaces; i++ ) {
        DPrintf("BSDParseRenderObjectUnTexturedFaceData:Reading Face at %i (%i)\n",GetCurrentFilePosition(BSDFile),GetCurrentFilePosition(BSDFile) - 2048);
        DPrintf(" -- FACE %i --\n",i);
        DPrintf("Reading a packet of size %li\n",sizeof(FaceData));
        fread(&FaceData,sizeof(FaceData),1,BSDFile);
        BSDFaceG3PacketToBSDFace(&RenderObject->UntexturedFaceList[i],FaceData[0]);
        DPrintf("Reading vertex data at %i (%i)\n",GetCurrentFilePosition(BSDFile) - FirstFaceDef,GetCurrentFilePosition(BSDFile) - 2048);
        fread(&PackedVertexData,sizeof(PackedVertexData),1,BSDFile);
        DPrintf("Packed Vertex data is %u\n",PackedVertexData);
        DecodeVertexData(PackedVertexData, &Vert0, &Vert1, &Vert2);        
        DPrintf("V0|V1|V2:%u;%u;%u (%i)\n",Vert0,Vert1,Vert2,RenderObject->NumVertex);
        RenderObject->UntexturedFaceList[i].Vert0 = Vert0;
        RenderObject->UntexturedFaceList[i].Vert1 = Vert1;
        RenderObject->UntexturedFaceList[i].Vert2 = Vert2;
        DPrintf("V0|V1|V2:(%i;%i;%i)|(%i;%i;%i)|(%i;%i;%i)\n",
                RenderObject->Vertex[Vert0].x,RenderObject->Vertex[Vert0].y,RenderObject->Vertex[Vert0].z,
                RenderObject->Vertex[Vert1].x,RenderObject->Vertex[Vert1].y,RenderObject->Vertex[Vert1].z,
                RenderObject->Vertex[Vert2].x,RenderObject->Vertex[Vert2].y,RenderObject->Vertex[Vert2].z);
    }
    return 1;
}
int BSDParseRenderObjectFaceData(BSDRenderObject_t *RenderObject,BSDRenderObjectElement_t *RenderObjectElement,FILE *BSDFile)
{    
    if( !RenderObject ) {
        DPrintf("BSDParseRenderObjectFaceData:Invalid RenderObject!\n");
        return 0;
    }
    RenderObject->NumTexturedFaces = 0;
    RenderObject->NumUntexturedFaces = 0;
    if( RenderObjectElement->TexturedFaceOffset ) {
        if( !BSDParseRenderObjectTexturedFaceData(RenderObject,RenderObjectElement,BSDFile,RenderObjectElement->TexturedFaceOffset,0) ) {
            DPrintf("BSDParseRenderObjectFaceData:Failed to load textured face data for RenderObject %i\n",RenderObjectElement->Id);
            return 0;
        }
    }
    if( RenderObjectElement->AltTexturedFaceOffset ) {
        if( !BSDParseRenderObjectTexturedFaceData(RenderObject,RenderObjectElement,BSDFile,RenderObjectElement->AltTexturedFaceOffset,1) ) {
            DPrintf("BSDParseRenderObjectFaceData:Failed to load textured face data for RenderObject %i\n",RenderObjectElement->Id);
            return 0;
        }
    }
    if( RenderObjectElement->AltFaceOffset ) {
        if( !BSDParseRenderObjectTexturedFaceData(RenderObject,RenderObjectElement,BSDFile,RenderObjectElement->AltFaceOffset,0) ) {
            DPrintf("BSDParseRenderObjectFaceData:Failed to load textured face data for RenderObject %i\n",RenderObjectElement->Id);
            return 0;
        }
    }
    if( RenderObjectElement->UntexturedFaceOffset ) {
        if( !BSDParseRenderObjectUntexturedFaceData(RenderObject,RenderObjectElement,BSDFile,RenderObjectElement->UntexturedFaceOffset) ) {
            DPrintf("BSDParseRenderObjectFaceData:Failed to load untextured face data for RenderObject %i\n",RenderObjectElement->Id);
            return 0;
        }
    }
    if( RenderObjectElement->AltUntexturedFaceOffset ) {
        if( !BSDParseRenderObjectUntexturedFaceData(RenderObject,RenderObjectElement,BSDFile,RenderObjectElement->AltUntexturedFaceOffset) ) {
            DPrintf("BSDParseRenderObjectFaceData:Failed to load untextured face data for RenderObject %i\n",RenderObjectElement->Id);
            return 0;
        }
    }
    return 1;
}

BSDRenderObject_t *BSDLoadStaticRenderObject(BSDRenderObjectElement_t RenderObjectElement,BSDEntryTable_t BSDEntryTable,FILE *BSDFile,
                                               int RenderObjectIndex
)
{
    BSDRenderObject_t *RenderObject;
    
    RenderObject = NULL;
    if( !BSDFile ) {
        DPrintf("BSDLoadStaticRenderObject:Invalid BSD file\n");
        goto Failure;
    }
    RenderObject = malloc(sizeof(BSDRenderObject_t));
    if( !RenderObject ) {
        DPrintf("BSDLoadStaticRenderObject:Failed to allocate memory for RenderObject\n");
        goto Failure;
    }
    RenderObject->Id = RenderObjectElement.Id;
    RenderObject->ReferencedRenderObjectId = /*RenderObjectElement.ReferencedRenderObjectId*/0;
    RenderObject->FileName = StringCopy(RenderObjectElement.FileName);
    RenderObject->Type = /*RenderObjectElement.Type*/0;
    RenderObject->NumVertex = RenderObjectElement.NumVertex;
    RenderObject->VertexTable = NULL;
    RenderObject->CurrentVertexTable = NULL;
    RenderObject->Vertex = NULL;
    RenderObject->Color = NULL;
    RenderObject->TexturedFaceList = NULL;
    RenderObject->UntexturedFaceList = NULL;
    RenderObject->FaceList = NULL;
    RenderObject->HierarchyDataRoot = NULL;
    RenderObject->AnimationList = NULL;
    RenderObject->VAO = NULL;
    RenderObject->CurrentAnimationIndex = -1;
    RenderObject->CurrentFrameIndex = -1;
    RenderObject->Next = NULL;
    RenderObject->TSP = NULL;
    RenderObject->RenderObjectShader = NULL;

    RenderObject->Scale[0] = (float) (RenderObjectElement.ScaleX  / 16) / 4096.f;
    RenderObject->Scale[1] = (float) (RenderObjectElement.ScaleY  / 16) / 4096.f;
    RenderObject->Scale[2] = (float) (RenderObjectElement.ScaleZ  / 16) / 4096.f;

    glm_vec3_zero(RenderObject->Center);
    
    if( RenderObject->Id == 0 ) {
        assert(RenderObjectElement.TSPOffset > 0);
        RenderObject->TSP = TSPLoad(BSDFile, RenderObjectElement.TSPOffset + BSD_HEADER_SIZE);
        if( !RenderObject->TSP ) {
            DPrintf("BSDParseRenderObjectData:Failed to load TSP data for RenderObject %u\n",RenderObject->Id);
            goto Failure;
        }
    } else {
        if( !BSDParseRenderObjectVertexAndColorData(RenderObject,&RenderObjectElement,BSDFile) ) {
            DPrintf("BSDParseRenderObjectData:Failed to parse Vertex and Color data for RenderObject %u\n",RenderObject->Id);
            goto Failure;
        }
        DPrintf("Loading faces definition for Id:%u\n",RenderObjectElement.Id);
        if( !BSDParseRenderObjectFaceData(RenderObject,&RenderObjectElement,BSDFile) ) {
            DPrintf("BSDParseRenderObjectData:Failed to parse Face data for RenderObject %u\n",RenderObject->Id);
            goto Failure;
        }
    }
    return RenderObject;
Failure:
    BSDFreeRenderObject(RenderObject);
    return NULL;
}
BSD_t *BSDLoad(FILE *BSDFile)
{
    BSD_t *BSD;
    
    BSD = NULL;
    
    BSD = malloc(sizeof(BSD_t));
    if( !BSDReadRenderObjectChunk(BSD,BSDFile) ) {
        goto Failure;
    }
    return BSD;
Failure:
    BSDFree(BSD);
    return NULL;
}

BSDRenderObject_t *BSDLoadAllRenderObjects(const char *FName)
{
    FILE *BSDFile;
    BSD_t *BSD;
    BSDRenderObject_t *RenderObjectList;
    BSDRenderObject_t *RenderObject;
    int i;
    
    BSDFile = fopen(FName,"rb");
    if( BSDFile == NULL ) {
        DPrintf("Failed opening BSD File %s.\n",FName);
        return NULL;
    }
    BSD = BSDLoad(BSDFile);
    if( !BSD ) {
        fclose(BSDFile);
        return NULL;
    }

    RenderObjectList = NULL;
    
    for( i = 0; i < BSD->RenderObjectTable.NumRenderObject; i++ ) {
        DPrintf("BSDLoadAllAnimatedRenderObjects:Loading Animated RenderObject %s\n",BSD->RenderObjectTable.RenderObject[i].FileName);
        RenderObject = BSDLoadStaticRenderObject(BSD->RenderObjectTable.RenderObject[i],BSD->EntryTable,
                                                   BSDFile,i);
        if( !RenderObject ) {
            DPrintf("BSDLoadAllAnimatedRenderObjects:Failed to load animated RenderObject.\n");
            continue;
        }
        BSDAppendRenderObjectToList(&RenderObjectList,RenderObject);
    }
    BSDFree(BSD);
    fclose(BSDFile);
    return RenderObjectList;
}
