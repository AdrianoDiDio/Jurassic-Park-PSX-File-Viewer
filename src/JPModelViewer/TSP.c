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

#include "TSP.h"
#include "../Common/ShaderManager.h"
#include "JPModelViewer.h"

void TSPFree(TSP_t *TSP)
{
    TSPRenderingFace_t *Temp;
    int i;
    
    if( !TSP ) {
        return;
    }
    
    if( TSP->Node ) {
        for( i = 0; i < TSP->Header.NumNodes; i++ ) {
            VAOFree(TSP->Node[i].BBoxVAO);
            VAOFree(TSP->Node[i].OpaqueFacesVAO);
            VAOFree(TSP->Node[i].LeafCollisionFaceListVAO);
            while( TSP->Node[i].OpaqueFaceList ) {
                Temp = TSP->Node[i].OpaqueFaceList;
                TSP->Node[i].OpaqueFaceList = TSP->Node[i].OpaqueFaceList->Next;
                free(Temp);
            }
            if( TSP->Node[i].FaceList ) {
                free(TSP->Node[i].FaceList);
            }
            //Don't bother traversing the tree since we can walk the whole array and free all the pointers...
            TSP->Node[i].Child[0] = NULL;
            TSP->Node[i].Child[1] = NULL;
            TSP->Node[i].Child[2] = NULL;
        }
        free(TSP->Node);
    }
    if( TSP->Face ) {
        free(TSP->Face);
    }
    if( TSP->Vertex ) {
        free(TSP->Vertex);
    }
    if( TSP->Color ) {
        free(TSP->Color);
    }
    if( TSP->DynamicData ) {
        if( TSP->Header.NumDynamicDataBlock != 0 ) {
            for( i = 0; i < TSP->Header.NumDynamicDataBlock; i++ ) {
                free(TSP->DynamicData[i].FaceIndexList);
                if( TSP->DynamicData[i].FaceDataList ) {
                    free(TSP->DynamicData[i].FaceDataList);
                }
                if( TSP->DynamicData[i].FaceDataListV3 ) {
                    free(TSP->DynamicData[i].FaceDataListV3);
                }
            }
            free(TSP->DynamicData);
        }
    }
    
    if( TSP->CollisionData ) {
        if( TSP->CollisionData->KDTree ) {
            free(TSP->CollisionData->KDTree);
        }
        if( TSP->CollisionData->FaceIndexList ) {
            free(TSP->CollisionData->FaceIndexList);
        }
        if( TSP->CollisionData->Vertex ) {
            free(TSP->CollisionData->Vertex);
        }
        if( TSP->CollisionData->Normal ) {
            free(TSP->CollisionData->Normal);
        }
        if( TSP->CollisionData->Face ) {
            free(TSP->CollisionData->Face);
        }
        free(TSP->CollisionData);
    }
    
    while( TSP->TransparentFaceList ) {
        Temp = TSP->TransparentFaceList;
        TSP->TransparentFaceList = TSP->TransparentFaceList->Next;
        free(Temp);
    }
    VAOFree(TSP->VAOList);
    VAOFree(TSP->CollisionVAOList);
    VAOFree(TSP->TransparentVAO);
    free(TSP->FName);
    free(TSP);
}

void TSPFreeList(TSP_t *List)
{
    TSP_t *Temp;
    while( List ) {
        Temp = List;
        List = List->Next;
        TSPFree(Temp);
    }
}


TSPVec3_t TSPGLMVec3ToTSPVec3(vec3 In)
{
    TSPVec3_t Result;
    Result.x = In[0];
    Result.y = In[1];
    Result.z = In[2];
    return Result;
}


void TSPVec3ToVec2(TSPVec3_t In,vec2 Out)
{
    Out[0] = In.x;
    Out[1] = In.z;
}
void TSPVec3ToGLMVec3(TSPVec3_t In,vec3 Out)
{
    Out[0] = In.x;
    Out[1] = In.y;
    Out[2] = In.z;
}
float TSPFixedToFloat(int Input,int FixedShift)
{
    return ((float)Input / (float)(1 << FixedShift));
}

void TSPvec4FromXYZ(float x,float y,float z,vec4 Out)
{
    Out[0] = x;
    Out[1] = y;
    Out[2] = z;
    Out[3] = 1;
}
void TSPDumpFaceDataToObjFile(TSP_t *TSP,VRAM_t *VRAM,FILE *OutFile)
{
    char Buffer[256];
    float TextureWidth;
    float TextureHeight;
    int i;
    int j;
    
    if( !TSP || !OutFile ) {
        bool InvalidFile = (OutFile == NULL ? true : false);
        printf("TSPDumpFaceV3DataToObjFile: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return;
    }
    
    if( !VRAM ) {
        DPrintf("TSPDumpFaceV3DataToObjFile:Invalid VRAM\n");
        return;
    }
    TextureWidth = VRAM->Page.Width;
    TextureHeight = VRAM->Page.Height;

    
    for( i = 0; i < TSP->Header.NumNodes; i++ ) {
        if( TSP->Node[i].NumFaces == 0 ) {
            continue;
        }
        for( j = TSP->Node[i].NumFaces - 1; j >= 0 ; j-- ) {
            int ColorMode = (TSP->Node[i].FaceList[j].TSB >> 7) & 0x3;
            int VRAMPage = TSP->Node[i].FaceList[j].TSB & 0x1F;
            float U0 = (((float)TSP->Node[i].FaceList[j].UV0.u + VRAMGetTexturePageX(VRAMPage))/TextureWidth);
            float V0 = /*255 -*/1.f-(((float)TSP->Node[i].FaceList[j].UV0.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
            float U1 = (((float)TSP->Node[i].FaceList[j].UV1.u + VRAMGetTexturePageX(VRAMPage)) / TextureWidth);
            float V1 = /*255 -*/1.f-(((float)TSP->Node[i].FaceList[j].UV1.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) /TextureHeight);
            float U2 = (((float)TSP->Node[i].FaceList[j].UV2.u + VRAMGetTexturePageX(VRAMPage)) /TextureWidth);
            float V2 = /*255 -*/1.f-(((float)TSP->Node[i].FaceList[j].UV2.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
            sprintf(Buffer,"vt %f %f\nvt %f %f\nvt %f %f\n",U0,V0,U1,V1,U2,V2);
            fwrite(Buffer,strlen(Buffer),1,OutFile);
        }
        for( j = 0; j < TSP->Node[i].NumFaces; j++ ) {

            int Vert0 = TSP->Node[i].FaceList[j].V0;
            int Vert1 = TSP->Node[i].FaceList[j].V1;
            int Vert2 = TSP->Node[i].FaceList[j].V2;
            int BaseFaceUV = j * 3;
            sprintf(Buffer,"usemtl vram\n");
            fwrite(Buffer,strlen(Buffer),1,OutFile);
            sprintf(Buffer,"f %i/%i %i/%i %i/%i\n",-(Vert0+1),-(BaseFaceUV+3),-(Vert1+1),-(BaseFaceUV+2),-(Vert2+1),-(BaseFaceUV+1));
            fwrite(Buffer,strlen(Buffer),1,OutFile);
        }
    }
}
void TSPDumpDataToObjFile(TSP_t *TSPList,VRAM_t *VRAM,FILE* OutFile)
{
    TSP_t *Iterator;
    char Buffer[256];
    int i;
    int t;
    vec3 NewPos;
    
    if( !TSPList || !OutFile ) {
        bool InvalidFile = (OutFile == NULL ? true : false);
        printf("TSPDumpDataToFile: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return;
    }
    if( !VRAM ) {
        DPrintf("TSPDumpDataToFile:Invalid VRAM\n");
        return;
    }
    t = 1;
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next, t++ ) {
        sprintf(Buffer,"o TSP%i\n",t);
        fwrite(Buffer,strlen(Buffer),1,OutFile);
        for( i = Iterator->Header.NumVertices - 1; i >= 0 ; i-- ) {
            TSPVec3ToGLMVec3(Iterator->Vertex[i].Position,NewPos);
            glm_vec3_rotate(NewPos, DEGTORAD(180.f), GLM_XUP);
            sprintf(Buffer,"v %f %f %f %f %f %f\n",NewPos[0] / 4096.f,NewPos[1] / 4096.f,NewPos[2] / 4096.f,
                Iterator->Color[i].rgba[0] / 255.f,Iterator->Color[i].rgba[1] / 255.f,Iterator->Color[i].rgba[2] / 255.f
            );
            fwrite(Buffer,strlen(Buffer),1,OutFile);            
        }
        TSPDumpFaceDataToObjFile(Iterator,VRAM,OutFile);
    }
}

int TSPDumpFaceDataToPlyFile(TSP_t *TSP,int VertexOffset,FILE *OutFile)
{
    char Buffer[256];
    int NumRenderedNodes;
    int i;
    int j;
    int FaceOffset;
    if( !TSP || !OutFile ) {
        bool InvalidFile = (OutFile == NULL ? true : false);
        DPrintf("TSPDumpFaceDataToFile: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return -1;
    }
    
    NumRenderedNodes = 0;
    FaceOffset = 0;
    for( i = 0; i < TSP->Header.NumNodes; i++ ) {
        if( TSP->Node[i].NumFaces == 0 ) {
            continue;
        }
        for( j = 0; j < TSP->Node[i].NumFaces; j++ ) {
            int Vert0 = VertexOffset + FaceOffset + (j * 3) + 0;
            int Vert1 = VertexOffset + FaceOffset + (j * 3) + 1;
            int Vert2 = VertexOffset + FaceOffset + (j * 3) + 2;
            sprintf(Buffer,"3 %i %i %i\n",Vert0,Vert1,Vert2);
            fwrite(Buffer,strlen(Buffer),1,OutFile);
        }
        FaceOffset += TSP->Node[i].NumFaces * 3;
        NumRenderedNodes++;
    }
    return FaceOffset;
}

void TSPDumpDataToPlyFile(TSP_t *TSPList,VRAM_t *VRAM,FILE* OutFile)
{
    TSP_t *Iterator;
    char Buffer[256];
    int i;
    int j;
    vec3 NewPos;
    int FaceCount;
    int VertexCount;
    int VertexOffset;
    
    if( !TSPList || !OutFile ) {
        bool InvalidFile = (OutFile == NULL ? true : false);
        printf("TSPDumpDataToPlyFile: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return;
    }
    if( !VRAM ) {
        DPrintf("TSPDumpDataToPlyFile:Invalid VRAM\n");
        return;
    }
    sprintf(Buffer,"ply\nformat ascii 1.0\n");
    fwrite(Buffer,strlen(Buffer),1,OutFile);
    VertexCount = 0;
    FaceCount = 0;
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        VertexCount += Iterator->Header.NumVertices;
        FaceCount += Iterator->Header.NumFaces;
    }
    sprintf(Buffer,
        "element vertex %i\nproperty float x\nproperty float y\nproperty float z\nproperty float red\nproperty float green\nproperty float blue\nproperty float s\nproperty float t\n",FaceCount * 3);
    fwrite(Buffer,strlen(Buffer),1,OutFile);
    sprintf(Buffer,"element face %i\nproperty list uchar int vertex_indices\nend_header\n",FaceCount);
    fwrite(Buffer,strlen(Buffer),1,OutFile);
    VertexOffset = 0;
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        float TextureWidth;
        float TextureHeight;
        TextureWidth = VRAM->Page.Width;
        TextureHeight = VRAM->Page.Height;

        for( i = 0; i < Iterator->Header.NumNodes; i++ ) {
            if( Iterator->Node[i].NumFaces == 0 ) {
                continue;
            }
            for( j = 0; j < Iterator->Node[i].NumFaces; j++ ) {
                int ColorMode = (Iterator->Node[i].FaceList[j].TSB >> 7) & 0x3;
                int VRAMPage = Iterator->Node[i].FaceList[j].TSB & 0x1F;
                float U0 = (((float)Iterator->Node[i].FaceList[j].UV0.u + VRAMGetTexturePageX(VRAMPage))/TextureWidth);
                float V0 = /*255 -*/1.f-(((float)Iterator->Node[i].FaceList[j].UV0.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
                float U1 = (((float)Iterator->Node[i].FaceList[j].UV1.u + VRAMGetTexturePageX(VRAMPage)) / TextureWidth);
                float V1 = /*255 -*/1.f-(((float)Iterator->Node[i].FaceList[j].UV1.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) /TextureHeight);
                float U2 = (((float)Iterator->Node[i].FaceList[j].UV2.u + VRAMGetTexturePageX(VRAMPage)) /TextureWidth);
                float V2 = /*255 -*/1.f-(((float)Iterator->Node[i].FaceList[j].UV2.v + VRAMGetTexturePageY(VRAMPage,ColorMode)) / TextureHeight);
                int Vert0 = Iterator->Node[i].FaceList[j].V0;
                int Vert1 = Iterator->Node[i].FaceList[j].V1;
                int Vert2 = Iterator->Node[i].FaceList[j].V2;
                TSPVec3ToGLMVec3(Iterator->Vertex[Vert0].Position,NewPos);
                glm_vec3_rotate(NewPos, DEGTORAD(180.f), GLM_XUP);
                sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",NewPos[0] / 4096.f,NewPos[1] / 4096.f,NewPos[2] / 4096.f,
                        Iterator->Color[Vert0].rgba[0] / 255.f,Iterator->Color[Vert0].rgba[1] / 255.f,Iterator->Color[Vert0].rgba[2] / 255.f,
                        U0,V0
                );
                fwrite(Buffer,strlen(Buffer),1,OutFile);
                TSPVec3ToGLMVec3(Iterator->Vertex[Vert1].Position,NewPos);
                glm_vec3_rotate(NewPos, DEGTORAD(180.f), GLM_XUP);
                sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",NewPos[0] / 4096.f,NewPos[1] / 4096.f,NewPos[2] / 4096.f,
                        Iterator->Color[Vert1].rgba[0] / 255.f,Iterator->Color[Vert1].rgba[1] / 255.f,Iterator->Color[Vert1].rgba[2] / 255.f,
                        U1,V1
                );
                fwrite(Buffer,strlen(Buffer),1,OutFile);      
                TSPVec3ToGLMVec3(Iterator->Vertex[Vert2].Position,NewPos);
                glm_vec3_rotate(NewPos, DEGTORAD(180.f), GLM_XUP);
                sprintf(Buffer,"%f %f %f %f %f %f %f %f\n",NewPos[0] / 4096.f,NewPos[1] / 4096.f,NewPos[2] / 4096.f,
                        Iterator->Color[Vert2].rgba[0] / 255.f,Iterator->Color[Vert2].rgba[1] / 255.f,Iterator->Color[Vert2].rgba[2] / 255.f,
                        U2,V2
                );
                fwrite(Buffer,strlen(Buffer),1,OutFile); 
            }
        }
   }
    
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        VertexOffset += TSPDumpFaceDataToPlyFile(Iterator,VertexOffset,OutFile);
    }

}

bool TSPBoxInFrustum(TSPBBox_t BBox,mat4 MVPMatrix)
{
    vec4 BoxCornerList[8];
    vec4 FrustumPlaneList[6];
    bool IsBoxInsideFrustum;
    int i;
    int j;
    
    glm_frustum_planes(MVPMatrix,FrustumPlaneList);
    
    TSPvec4FromXYZ(BBox.Min.x,BBox.Min.y,BBox.Min.z,BoxCornerList[0]);
    TSPvec4FromXYZ(BBox.Max.x,BBox.Min.y,BBox.Min.z,BoxCornerList[1]);
    TSPvec4FromXYZ(BBox.Min.x,BBox.Max.y,BBox.Min.z,BoxCornerList[2]);
    TSPvec4FromXYZ(BBox.Max.x,BBox.Max.y,BBox.Min.z,BoxCornerList[3]);
    TSPvec4FromXYZ(BBox.Min.x,BBox.Min.y,BBox.Max.z,BoxCornerList[4]);
    TSPvec4FromXYZ(BBox.Max.x,BBox.Min.y,BBox.Max.z,BoxCornerList[5]);
    TSPvec4FromXYZ(BBox.Min.x,BBox.Max.y,BBox.Max.z,BoxCornerList[6]);
    TSPvec4FromXYZ(BBox.Max.x,BBox.Max.y,BBox.Max.z,BoxCornerList[7]);

    for( i = 0; i < 6; i++ ) {
        IsBoxInsideFrustum = 0;
        for( j = 0; j < 8; j++ ) {
            if( glm_vec4_dot(FrustumPlaneList[i],BoxCornerList[j]) > 0.f ) {
                IsBoxInsideFrustum = 1;
                break;
            }
        }
        if( !IsBoxInsideFrustum ) {
            return false;
        }
    }
    return true;
}

/*
 * Check if a given face is dynamic by looking into the dynamic array from the given TSP file.
 * Note that Face is either an index to the face array or an offset to the face data.
 * This is due to the difference that exists between MOH and MOH:Underground where in the
 * latter case the faces are stored differently and they cannot be referenced by index.
 */
int TSPIsFaceDynamic(TSP_t *TSP,int Face)
{
    int i;
    int j;

    if( TSP->Header.NumDynamicDataBlock == 0 ) {
        return false;
    }
    for( i = 0; i < TSP->Header.NumDynamicDataBlock; i++ ) {
        for( j = 0; j < TSP->DynamicData[i].Header.NumFacesIndex; j++ ) {
            if( TSP->DynamicData[i].FaceIndexList[j] == Face ) {
                return true;
            }
        }
    }
    return false;
}

void TSPNodeCountTransparentFaces(TSP_t *TSP,TSPNode_t *Node)
{
    int i;
    
    if( !Node ) {
        DPrintf("TSPNodeCountTransparentFaces:Invalid Node data\n");
        return;
    }
    Node->NumTransparentFaces = 0;
    if( Node->NumFaces <= 0 ) {
        DPrintf("TSPNodeCountTransparentFaces:Node is not a leaf...\n");
        return;
    }

    for( i = 0; i < Node->NumFaces; i++ ) {
        if( (Node->FaceList[i].TSB & 0x4000 ) != 0 ) {
            Node->NumTransparentFaces++;
        }
    }
    return;
}

int TSPGetColorIndex(int Color)
{
    return Color & 0xFF00FF;
}

void TSPFillFaceVertexBuffer(int *Buffer,int *BufferSize,TSPVert_t Vertex,Color1i_t Color,int U,int V,int CLUTX,int CLUTY,int ColorMode,bool Textured)
{
    if( !Buffer ) {
        DPrintf("TSPFillFaceVertexBuffer:Invalid Buffer\n");
        return;
    }
    if( !BufferSize ) {
        DPrintf("TSPFillFaceVertexBuffer:Invalid BufferSize\n");
        return;
    }
    Buffer[*BufferSize] =   Vertex.Position.x;
    Buffer[*BufferSize+1] = Vertex.Position.y;
    Buffer[*BufferSize+2] = Vertex.Position.z;
    Buffer[*BufferSize+3] = U;
    Buffer[*BufferSize+4] = V;
    Buffer[*BufferSize+5] = Color.rgba[0];
    Buffer[*BufferSize+6] = Color.rgba[1];
    Buffer[*BufferSize+7] = Color.rgba[2];
    Buffer[*BufferSize+8] = CLUTX;
    Buffer[*BufferSize+9] = CLUTY;
    Buffer[*BufferSize+10] = ColorMode;
    Buffer[*BufferSize+11] = Textured;
    *BufferSize += 12;
}

void TSPCreateFaceVAO(TSP_t *TSP,TSPNode_t *Node)
{
    int Base;
    int Target;
    int Stride;
    int CBA;
    int TSB;
    int Vert0;
    int Vert1;
    int Vert2;
    int U0,V0;
    int U1,V1;
    int U2,V2;
    int *VertexData;
    int *TransparentVertexData;
    int TotalVertexSize;
    int VertexSize;
    int TransparentVertexSize;
    int VertexPointer;
    int TransparentVertexPointer;
    int VertexOffset;
    int TextureOffset;
    int ColorOffset;
    int CLUTOffset;
    int ColorModeOffset;
    int TexturedOffset;
    int CLUTPosX;
    int CLUTPosY;
    int CLUTDestX;
    int CLUTDestY;
    int CLUTPage;
    int NumTransparentFaces;
    int ColorMode;
    int VRAMPage;
    int ABRRate;
    TSPRenderingFace_t *RenderingFace;
    VAO_t *VAO;
    int i;
    

    Base = 0;
    Target = Node->NumFaces;

//            XYZ UV RGB CLUT ColorMode Textured
    Stride = (3 + 2 + 3 + 2 + 1 + 1) * sizeof(int);
                
    VertexOffset = 0;
    TextureOffset = 3;
    ColorOffset = 5;
    CLUTOffset = 8;
    ColorModeOffset = 10;
    TexturedOffset = 11;

    NumTransparentFaces = Node->NumTransparentFaces;
    VertexSize = Stride * 3;
    TotalVertexSize = VertexSize * (Node->NumFaces - NumTransparentFaces);
    VertexData = malloc(VertexSize);
    VertexPointer = 0;
    TransparentVertexSize = Stride * 3;
    TransparentVertexData = malloc(TransparentVertexSize);
    TransparentVertexPointer = 0;
    VAO = VAOInitXYZUVRGBCLUTColorModeTexturedInteger(NULL,TotalVertexSize,Stride,VertexOffset,TextureOffset,ColorOffset,CLUTOffset,ColorModeOffset,
                                                      TexturedOffset,(Node->NumFaces - NumTransparentFaces) * 3);
    Node->OpaqueFacesVAO = VAO;
    //NOTE(Adriano):Some levels have duplicated triangles...we need to make sure that the order in which they are rendered
    //              is such that they do not get overwritten by a later triangle definition with an invalid texture coordinate.
    //              An example can be found in MOH MSN4:LVL2 where in some part of the level the geometry is specified twice.
    for( i = Target - 1; i >= Base; i-- ) {

        Vert0 = Node->FaceList[i].V0;
        Vert1 = Node->FaceList[i].V1;
        Vert2 = Node->FaceList[i].V2;
        U0 = Node->FaceList[i].UV0.u;
        V0 = Node->FaceList[i].UV0.v;
        U1 = Node->FaceList[i].UV1.u;
        V1 = Node->FaceList[i].UV1.v;
        U2 = Node->FaceList[i].UV2.u;
        V2 = Node->FaceList[i].UV2.v;
        TSB = Node->FaceList[i].TSB;
        CBA = Node->FaceList[i].CBA;
        

        ColorMode = (TSB >> 7) & 0x3;
        VRAMPage = TSB & 0x1F;
        ABRRate = (TSB & 0x60) >> 5;
        
        RenderingFace = malloc(sizeof(TSPRenderingFace_t));
        RenderingFace->Flags = 0;
        RenderingFace->Next = NULL;
        DPrintf("Vert0:%i Vert1:%i Vert2:%i\n",Vert0,Vert1,Vert2);
        RenderingFace->Vert0 = TSP->Vertex[Vert0];
        RenderingFace->Vert1 = TSP->Vertex[Vert1];
        RenderingFace->Vert2 = TSP->Vertex[Vert2];
        RenderingFace->Colors[0]= TSP->Color[Vert0];
        RenderingFace->Colors[1] = TSP->Color[Vert1];
        RenderingFace->Colors[2] = TSP->Color[Vert2];
        
        CLUTPosX = (CBA << 4) & 0x3F0;
        CLUTPosY = (CBA >> 6) & 0x1ff;
        CLUTPage = VRAMGetCLUTPage(CLUTPosX,CLUTPosY);
        CLUTDestX = VRAMGetCLUTPositionX(CLUTPosX,CLUTPosY,CLUTPage);
        CLUTDestY = CLUTPosY + VRAMGetCLUTOffsetY(ColorMode);
        CLUTDestX += VRAMGetTexturePageX(CLUTPage);

        DPrintf("TSB is %u\n",TSB);
        DPrintf("Expected VRam Page:%i\n",VRAMPage);
        DPrintf("Expected Color Mode:%i\n",ColorMode);
        DPrintf("Expected ABR rate:%i\n",ABRRate);
        DPrintf("Expected CLUT Position:%ix%i at page %i\n",CLUTPosX,CLUTPosY,CLUTPage);


        U0 += VRAMGetTexturePageX(VRAMPage);
        V0 += VRAMGetTexturePageY(VRAMPage,ColorMode);
        U1 += VRAMGetTexturePageX(VRAMPage);
        V1 += VRAMGetTexturePageY(VRAMPage,ColorMode);
        U2 += VRAMGetTexturePageX(VRAMPage);
        V2 += VRAMGetTexturePageY(VRAMPage,ColorMode);
     
        
        if( TSPGetColorIndex(TSP->Color[Vert0].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE || 
            TSPGetColorIndex(TSP->Color[Vert1].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE || 
            TSPGetColorIndex(TSP->Color[Vert2].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE ) {
            RenderingFace->Flags |= TSP_FX_ANIMATED_LIGHT_FACE;
        }
                    
        DPrintf("Tex Coords are %i;%i %i;%i %i;%i\n",
                    U0,V0,
                    U1,V1,
                    U2,V2);
        if( (TSB & 0x4000) != 0) {
            TSPFillFaceVertexBuffer(TransparentVertexData,&TransparentVertexPointer,TSP->Vertex[Vert0],
                                   TSP->Color[Vert0],U0,V0,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            TSPFillFaceVertexBuffer(TransparentVertexData,&TransparentVertexPointer,TSP->Vertex[Vert1],
                                   TSP->Color[Vert1],U1,V1,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            TSPFillFaceVertexBuffer(TransparentVertexData,&TransparentVertexPointer,TSP->Vertex[Vert2],
                                   TSP->Color[Vert2],U2,V2,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            RenderingFace->VAOBufferOffset = TSP->TransparentVAO->CurrentSize;
            RenderingFace->BlendingMode = (TSB >> 5 ) & 3;
            RenderingFace->Flags |= TSP_FX_TRANSPARENT_FACE;
            VAOUpdate(TSP->TransparentVAO,TransparentVertexData,TransparentVertexSize,3);
            RenderingFace->Next = TSP->TransparentFaceList;
            TSP->TransparentFaceList = RenderingFace;
            TransparentVertexPointer = 0;
            
        } else {
            TSPFillFaceVertexBuffer(VertexData,&VertexPointer,TSP->Vertex[Vert0],
                                    TSP->Color[Vert0],U0,V0,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            TSPFillFaceVertexBuffer(VertexData,&VertexPointer,TSP->Vertex[Vert1],
                                    TSP->Color[Vert1],U1,V1,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            TSPFillFaceVertexBuffer(VertexData,&VertexPointer,TSP->Vertex[Vert2],
                                    TSP->Color[Vert2],U2,V2,CLUTDestX,CLUTDestY,ColorMode,Node->FaceList[i].IsTextured);
            
            RenderingFace->VAOBufferOffset = Node->OpaqueFacesVAO->CurrentSize;
            RenderingFace->Flags |= TSP_FX_NONE;
            VAOUpdate(Node->OpaqueFacesVAO,VertexData,VertexSize,3);
            RenderingFace->Next = Node->OpaqueFaceList;
            Node->OpaqueFaceList = RenderingFace;
            VertexPointer = 0;
        }
        
        if( RenderingFace->Flags & TSP_FX_ANIMATED_LIGHT_FACE ) {
            RenderingFace->ColorIndex[0] = (TSPGetColorIndex(TSP->Color[Vert0].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE) 
                ? (TSP->Color[Vert0].c & 0xFF) : -1;
            RenderingFace->ColorIndex[1] = (TSPGetColorIndex(TSP->Color[Vert1].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE) 
                ? (TSP->Color[Vert1].c & 0xFF) : -1; 
            RenderingFace->ColorIndex[2] = (TSPGetColorIndex(TSP->Color[Vert2].c) < BSD_ANIMATED_LIGHTS_TABLE_SIZE) 
                ? (TSP->Color[Vert2].c & 0xFF) : -1;
        }
    }
    free(VertexData);
    free(TransparentVertexData);

}

void TSPCreateNodeBBoxVAO(TSP_t *TSPList)
{
    TSP_t *Iterator;
    float *VertexData;
    int VertexSize;
    int TransparentVertexSize;
    int VertexPointer;
    int Stride;
    int NumTransparentFaces;
    vec4 BoxColor;
    int i;
    
    unsigned short BBoxIndices[16] = {
        0, 1, 2, 3,
        4, 5, 6, 7,
        0, 4, 1, 5, 2, 6, 3, 7
    };
    
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        NumTransparentFaces = 0;
         for( i = 0; i < Iterator->Header.NumNodes; i++ ) {
             TSPNodeCountTransparentFaces(Iterator,&Iterator->Node[i]);
             NumTransparentFaces += Iterator->Node[i].NumTransparentFaces;
         }
        Stride = (3 + 2 + 3 + 2 + 1) * sizeof(int);
        TransparentVertexSize = Stride * 3 * NumTransparentFaces;
        Iterator->TransparentVAO = VAOInitXYZUVRGBCLUTColorModeTexturedInteger(NULL,TransparentVertexSize,Stride,0,3,5,8,10,11,NumTransparentFaces * 3);
        
        for( i = 0; i < Iterator->Header.NumNodes; i++ ) {
            if( Iterator->Node[i].NumFaces != 0 ) {
                TSPCreateFaceVAO(Iterator,&Iterator->Node[i]);
            }
            
          //       XYZ RGB
            Stride = (3 + 3) * sizeof(float);
            VertexSize = Stride;
            VertexData = malloc(VertexSize * 8/** sizeof(float)*/);
            VertexPointer = 0;
                    
            if( Iterator->Node[i].NumFaces != 0 ) {
                //Leaf -- Yellow
                BoxColor[0] = 1;
                BoxColor[1] = 1;
                BoxColor[2] = 0;
                BoxColor[3] = 1;
            } else {
                //Splitter -- Red
                BoxColor[0] = 1;
                BoxColor[1] = 0;
                BoxColor[2] = 0;
                BoxColor[3] = 1;
            }
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Min.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Min.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Min.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
                        
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Min.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Min.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Max.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Max.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Min.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Max.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Max.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Min.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Min.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Min.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Max.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Min.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Min.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Max.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Max.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
                        
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Max.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Max.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Max.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            VertexData[VertexPointer] =   Iterator->Node[i].BBox.Max.x;
            VertexData[VertexPointer+1] = Iterator->Node[i].BBox.Max.y;
            VertexData[VertexPointer+2] = Iterator->Node[i].BBox.Min.z;
            VertexData[VertexPointer+3] = BoxColor[0];
            VertexData[VertexPointer+4] = BoxColor[1];
            VertexData[VertexPointer+5] = BoxColor[2];

            VertexPointer += 6;
            
            Iterator->Node[i].BBoxVAO = VAOInitXYZRGBIBO(VertexData,VertexSize * 8,Stride,BBoxIndices,sizeof(BBoxIndices),0,3);            
            free(VertexData);
        }
    }
}

void TSPCreateCollisionVAO(TSP_t *TSPList)
{
    TSP_t *Iterator;
    int i;
    float *VertexData;
    int VertexSize;
    int VertexPointer;
    int Stride;

    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        VAO_t *VAO;
        //       XYZ
        Stride = (3) * sizeof(float);
                
        VertexSize = Stride;
        VertexData = malloc(VertexSize * 3 * Iterator->CollisionData->Header.NumFaces);
        VertexPointer = 0;
        for( i = 0; i < Iterator->CollisionData->Header.NumFaces; i++ ) {
            int Vert0 = Iterator->CollisionData->Face[i].V0;
            int Vert1 = Iterator->CollisionData->Face[i].V1;
            int Vert2 = Iterator->CollisionData->Face[i].V2;

                    
            VertexData[VertexPointer] =   Iterator->CollisionData->Vertex[Vert0].Position.x;
            VertexData[VertexPointer+1] = Iterator->CollisionData->Vertex[Vert0].Position.y;
            VertexData[VertexPointer+2] = Iterator->CollisionData->Vertex[Vert0].Position.z;
            VertexPointer += 3;
            
            VertexData[VertexPointer] =   Iterator->CollisionData->Vertex[Vert1].Position.x;
            VertexData[VertexPointer+1] = Iterator->CollisionData->Vertex[Vert1].Position.y;
            VertexData[VertexPointer+2] = Iterator->CollisionData->Vertex[Vert1].Position.z;
            VertexPointer += 3;
            
            VertexData[VertexPointer] =   Iterator->CollisionData->Vertex[Vert2].Position.x;
            VertexData[VertexPointer+1] = Iterator->CollisionData->Vertex[Vert2].Position.y;
            VertexData[VertexPointer+2] = Iterator->CollisionData->Vertex[Vert2].Position.z;
            VertexPointer += 3;
        }
        VAO = VAOInitXYZ(VertexData,VertexSize * 3 * Iterator->CollisionData->Header.NumFaces,Stride,0,Iterator->CollisionData->Header.NumFaces * 3);            
        VAO->Next = Iterator->CollisionVAOList;
        Iterator->CollisionVAOList = VAO;
        free(VertexData);
    }
}

void TSPCreateVAOs(TSP_t *TSPList)
{
    TSPCreateNodeBBoxVAO(TSPList);
    TSPList->VAOCreated = true;
//     TSPCreateCollisionVAO(TSPList);
}

void TSPPrintVec3(TSPVec3_t Vector)
{
    DPrintf("(%i;%i;%i)\n",Vector.x,Vector.y,Vector.z);
}

void TSPPrintColor(Color1i_t Color)
{
    DPrintf("RGBA:(%i;%i;%i;%i)\n",Color.rgba[0],Color.rgba[1],Color.rgba[2],Color.rgba[3]);
}

void TSPDrawNodeBBox(TSPNode_t *Node,mat4 MVPMatrix)
{
    Shader_t *Shader;
    int MVPMatrixId;
    
    Shader = ShaderCache("TSPBBoxShader","Shaders/TSPBBoxVertexShader.glsl","Shaders/TSPBBoxFragmentShader.glsl");
    
    if( !Shader ) {
        DPrintf("TSPDrawNodeBBox:Invalid Shader\n");
        return;
    }
    glUseProgram(Shader->ProgramId);
    
    MVPMatrixId = glGetUniformLocation(Shader->ProgramId,"MVPMatrix");
    glUniformMatrix4fv(MVPMatrixId,1,false,&MVPMatrix[0][0]);
    
    glBindVertexArray(Node->BBoxVAO->VAOId[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Node->BBoxVAO->IBOId[0]);
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, 0);
    glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_SHORT, (GLvoid*)(4*sizeof(unsigned short)));
    glDrawElements(GL_LINES, 8, GL_UNSIGNED_SHORT, (GLvoid*)(8*sizeof(unsigned short)));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
    glUseProgram(0);
}

void TSPDrawCollisionData(TSP_t *TSP,mat4 MVPMatrix)
{
    VAO_t *Iterator;
    Shader_t *Shader;
    int MVPMatrixId;
    
    if( !TSP ) {
        DPrintf("TSPDrawCollisionData:Invalid TSP...\n");
        return;
    }
    
    Shader = ShaderCache("TSPCollisionShader","Shaders/TSPCollisionVertexShader.glsl","Shaders/TSPCollisionFragmentShader.glsl");
    
    if( !Shader ) {
        DPrintf("TSPDrawCollisionData:Invalid Shader\n");
        return;
    }
    glUseProgram(Shader->ProgramId);

    MVPMatrixId = glGetUniformLocation(Shader->ProgramId,"MVPMatrix");
    glUniformMatrix4fv(MVPMatrixId,1,false,&MVPMatrix[0][0]);
    
    for( Iterator = TSP->CollisionVAOList; Iterator; Iterator = Iterator->Next ) {
        glBindVertexArray(Iterator->VAOId[0]);
        glDrawArrays(GL_TRIANGLES, 0, Iterator->Count);
        glBindVertexArray(0);
    }
    glUseProgram(0);

}

void TSPDrawNode(TSPNode_t *Node,RenderObjectShader_t *RenderObjectShader,VRAM_t *VRAM,mat4 MVPMatrix)
{    
    if( !Node ) {
        return;
    }
    
    if( 0/*LevelEnableFrustumCulling->IValue*/ && !TSPBoxInFrustum(Node->BBox,MVPMatrix) ) {
        return;
    }
    
    if( 0/*LevelDrawTSPTree->IValue*/ ) {
        TSPDrawNodeBBox(Node,MVPMatrix);
    }

    if( Node->NumFaces != 0 ) {
        if( 1/*LevelDrawSurfaces->IValue*/ ) {
            if( RenderObjectShader ) {
                if( EnableWireFrameMode->IValue ) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                } else {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
                glUseProgram(RenderObjectShader->Shader->ProgramId);
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_2D, VRAM->TextureIndexPage.TextureId);
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_2D, VRAM->PalettePage.TextureId);

                glDisable(GL_BLEND);
                glBindVertexArray(Node->OpaqueFacesVAO->VAOId[0]);
                glDrawArrays(GL_TRIANGLES, 0, Node->OpaqueFacesVAO->Count);
                glBindVertexArray(0);
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_2D,0);
                glDisable(GL_BLEND);
                glBlendColor(1.f, 1.f, 1.f, 1.f);
                if( EnableWireFrameMode->IValue ) {
                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                }
            }
        }
    } else {
        TSPDrawNode(Node->Child[1],RenderObjectShader,VRAM,MVPMatrix);
        TSPDrawNode(Node->Child[2],RenderObjectShader,VRAM,MVPMatrix);
        TSPDrawNode(Node->Child[0],RenderObjectShader,VRAM,MVPMatrix);

    }
}

void TSPUpdateAnimatedRenderingFace(TSPRenderingFace_t *Face,VAO_t *VAO,BSD_t *BSD,int Reset)
{
    Color1i_t OriginalColor;
    Color1i_t FinalColor;
    int ColorData[3];
    int Stride;
    int CurrentColor;
    int BaseOffset;
    if( !Face ) {
        DPrintf("TSPUpdateAnimatedRenderingFace:Invalid face data\n");
        return;
    }
    
    if( !(Face->Flags & TSP_FX_ANIMATED_LIGHT_FACE) ) {
        return;
    }
    
    Stride = (3 + 2 + 3 + 2 + 1 + 1) * sizeof(int);
    glBindBuffer(GL_ARRAY_BUFFER, VAO->VBOId[0]);
    BaseOffset = (Face->VAOBufferOffset * Stride );
    
    for( int i = 0; i < 3; i++ ) {
        if( Face->ColorIndex[i] == -1 ) {
            continue;
        }
        CurrentColor = 0/*BSDGetCurrentAnimatedLightColorByIndex(BSD,Face->ColorIndex[i])*/;
        OriginalColor.c = Face->Colors[i].c;
        FinalColor.c = (OriginalColor.c & 0xFF00) | (CurrentColor & 0xFFFFFF);
        if( Reset ) {
            ColorData[0] = OriginalColor.rgba[0];
            ColorData[1] = OriginalColor.rgba[1];
            ColorData[2] = OriginalColor.rgba[2];    
        } else {
            ColorData[0] = FinalColor.rgba[0];
            ColorData[1] = FinalColor.rgba[1];
            ColorData[2] = FinalColor.rgba[2];    
        }
        //The offset in which we write the color is based on the current VAO Offset to which
        //we add the stride times i which moves the pointer to one of the three vertices (each vertex takes Stride amount of bytes)
        //and finally we add 5 times sizeof(int) in order to grab the starting definition of the color data inside the vertex array.
        glBufferSubData(GL_ARRAY_BUFFER, BaseOffset + (Stride * i) + (5*sizeof(int)), 3 * sizeof(int), &ColorData);
    }
}

void TSPUpdateAnimatedFaceNodes(TSPNode_t *Node,BSD_t *BSD,mat4 MVPMatrix,int Reset)
{
    TSPRenderingFace_t *Iterator;

    if( !Node ) {
        return;
    }
    //NOTE(Adriano):Make sure we don't want to reset all the faces to the default state...
    if( !Reset ) {
        assert(MVPMatrix != NULL);
        //NOTE(Adriano):Always enable culling regardless of 'LevelEnableFrustumCulling' settings.
        if( !TSPBoxInFrustum(Node->BBox,MVPMatrix) ) {
            return;
        }
    }
    
    if( Node->NumFaces != 0 ) {
        for( Iterator = Node->OpaqueFaceList; Iterator; Iterator = Iterator->Next ) {
           TSPUpdateAnimatedRenderingFace(Iterator,Node->OpaqueFacesVAO,BSD,Reset);
        }
    } else {
        TSPUpdateAnimatedFaceNodes(Node->Child[1],BSD,MVPMatrix,Reset);
        TSPUpdateAnimatedFaceNodes(Node->Child[2],BSD,MVPMatrix,Reset);
        TSPUpdateAnimatedFaceNodes(Node->Child[0],BSD,MVPMatrix,Reset);
    }
}
void TSPUpdateTransparentAnimatedFaces(TSP_t *TSP,BSD_t *BSD,int Reset)
{
    TSPRenderingFace_t *Iterator;
    
    for( Iterator = TSP->TransparentFaceList; Iterator; Iterator = Iterator->Next ) {
        TSPUpdateAnimatedRenderingFace(Iterator,TSP->TransparentVAO,BSD,Reset);
    }
}
/*
 * Updates all the animated faces inside each loaded TSP file.
 * When ProjectionMatrix is not NULL, it attempts to cull invisible faces that lies outside the view frustum.
 * NOTE that frustum culling only applies to the faces belonging to the TSP tree and not to the transparent one.
 * NOTE that when Reset is 1 then the ProjectionMatrix parameter is ignored when performing frustum culling...
 * this means that all the faces will be reset.
*/
void TSPUpdateAnimatedFaces(TSP_t *TSPList,BSD_t *BSD,Camera_t *Camera,mat4 ProjectionMatrix,int Reset)
{
    TSP_t *Iterator;
    mat4 MVPMatrix;
    
    if( ProjectionMatrix ) {
        glm_mat4_mul(ProjectionMatrix,Camera->ViewMatrix,MVPMatrix);
        //Emulate PSX Coordinate system...
        glm_rotate_x(MVPMatrix,glm_rad(180.f), MVPMatrix);
    }
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        TSPUpdateAnimatedFaceNodes(&Iterator->Node[0],BSD,MVPMatrix,Reset);
        TSPUpdateTransparentAnimatedFaces(Iterator,BSD,Reset);
    }
}
void TSPDrawTransparentFaces(TSP_t *TSP,VRAM_t *VRAM)
{
    TSPRenderingFace_t *TransparentFaceIterator;

    if( 0/*!LevelDrawSurfaces->IValue*/ ) {
        return;
    }
    
    if( EnableWireFrameMode->IValue ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D, VRAM->TextureIndexPage.TextureId);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, VRAM->PalettePage.TextureId);
    glBindVertexArray(TSP->TransparentVAO->VAOId[0]);
    if( 0/*!LevelEnableSemiTransparency->IValue*/ ) {
        glDrawArrays(GL_TRIANGLES, 0, TSP->TransparentVAO->Count);
    } else {
        glDepthMask(0);
        glEnable(GL_BLEND);
        for( TransparentFaceIterator = TSP->TransparentFaceList; TransparentFaceIterator; TransparentFaceIterator = TransparentFaceIterator->Next) {
            switch( TransparentFaceIterator->BlendingMode ) {
                case TSP_BLENDING_MODE_HALF_BACKGROUND_PLUS_HALF_FOREGROUND:
                case TSP_BLENDING_MODE_BACKGROUND_PLUS_FOREGROUND:
                case TSP_BLENDING_MODE_BACKGROUND_PLUS_QUARTER_FOREGROUND:
                    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                    break;
                case TSP_BLENDING_MODE_BACKGROUND_MINUS_FOREGROUND:
                    glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_ADD);
                    break;
            }
            glBlendFunc(GL_ONE, GL_SRC_ALPHA);
            glDrawArrays(GL_TRIANGLES, TransparentFaceIterator->VAOBufferOffset, 3);
        }
    }
    glBindVertexArray(0);
    glDepthMask(1);
    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_2D,0);
    glDisable(GL_BLEND);
    if( EnableWireFrameMode->IValue ) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}
void TSPDrawList(TSP_t *TSPList,VRAM_t *VRAM,Camera_t *Camera,RenderObjectShader_t *RenderObjectShader,mat4 ProjectionMatrix)
{
    TSP_t *Iterator;
    mat4 MVPMatrix;
    
    if( !TSPList ) {
        DPrintf("TSPDrawList:Invalid TSP data\n");
        return;
    }
    
    if( !RenderObjectShader ) {
        DPrintf("TSPDrawList:Invalid Shader.\n");
        return;
    }
    glm_mat4_mul(ProjectionMatrix,Camera->ViewMatrix,MVPMatrix);
    
    //Emulate PSX Coordinate system...
    glm_rotate_x(MVPMatrix,glm_rad(180.f), MVPMatrix);
    glUseProgram(RenderObjectShader->Shader->ProgramId);
    glUniform1i(RenderObjectShader->EnableLightingId, EnableAmbientLight->IValue);
    glUniformMatrix4fv(RenderObjectShader->MVPMatrixId,1,false,&MVPMatrix[0][0]);

    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        if( !Iterator->VAOCreated ) {
            TSPCreateVAOs(Iterator);
        }
        TSPDrawNode(&Iterator->Node[0],RenderObjectShader,VRAM,MVPMatrix);
    }
    // Alpha pass.
    for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
        TSPDrawTransparentFaces(Iterator,VRAM);
        
    }
    glUseProgram(0);
//     if( 0/*LevelDrawCollisionData->IValue*/ ) {
//         for( Iterator = TSPList; Iterator; Iterator = Iterator->Next ) {
//             TSPDrawCollisionData(Iterator,MVPMatrix);
//         }
//     }
}

int TSPGetNodeByChildOffset(TSP_t *TSP,int Offset)
{
    int i;
    for( i = 0; i < TSP->Header.NumNodes; i++ ) {
        if( Offset == TSP->Node[i].FileOffset.Offset ) {
            DPrintf("Offset %i match node %i\n",Offset,i);
            return i;
        }
    }
    DPrintf("Offset %i doesn't match any node!\n",Offset);
    return -1;
}

void TSPLookUpChildNode(TSP_t *TSP,FILE *InFile)
{
    int i;

    for( i = 0; i < TSP->Header.NumNodes; i++ ) {
        if( TSP->Node[i].NumFaces != 0 ) {
            continue;
        }
        if( TSP->Node[i].Child1Index != -1 ) {
            TSP->Node[i].Child[0] = &TSP->Node[TSP->Node[i].Child1Index];
        } else {
            TSP->Node[i].Child[0] = NULL;
        }
        if( TSP->Node[i].Child2Index != -1 ) {
            TSP->Node[i].Child[1] = &TSP->Node[TSP->Node[i].Child2Index];
        } else {
            TSP->Node[i].Child[1] = NULL;
        }
        if( TSP->Node[i].Child3Index != -1 ) {
            TSP->Node[i].Child[2] = &TSP->Node[TSP->Node[i].Child3Index];
        } else {
            TSP->Node[i].Child[2] = NULL;
        }
    }
}

void TSPSkipFileChunk(FILE *InFile, int Bytes)
{
    fseek(InFile, Bytes, SEEK_CUR);
}
void TSPPrintFace(TSPFace_t *Face)
{
    DPrintf("Dumping Face data\n");
    DPrintf("V0:%i\n",Face->V0);
    DPrintf("V1:%i\n",Face->V1);
    DPrintf("V2:%i\n",Face->V2);
    DPrintf("UV0 u:%i v:%i\n",Face->UV0.u, Face->UV0.v);
    DPrintf("UV0 u:%i v:%i\n",Face->UV1.u, Face->UV1.v);
    DPrintf("UV0 u:%i v:%i\n",Face->UV2.u, Face->UV2.v);
    DPrintf("TSB:%i VRAM Page:%i ColorMode:%i\n",Face->TSB,Face->TSB & 0x1F,(Face->TSB >> 7) & 0x3);
    DPrintf("CBA:%i CLUTX:%i CLUTY:%i\n",Face->CBA,(Face->CBA << 4) & 0x3F0,(Face->CBA >> 6) & 0x1ff);


}
int TSPReadNodeChunk(TSP_t *TSP,FILE *InFile,int TSPOffset)
{
    int CurrentFaceIndex;
    int PrevFilePosition;
    unsigned short PrimitiveType;
    int i;
    
    if( !TSP || !InFile ) {
        bool InvalidFile = (InFile == NULL ? true : false);
        printf("TSPReadNodeChunk: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return 0;
    }
    if( TSP->Header.NumNodes == 0 ) {
        DPrintf("TSPReadNodeChunk:0 nodes found in file %s.\n",TSP->FName);
        return 0;
    }
    TSP->Node = malloc(TSP->Header.NumNodes * sizeof(TSPNode_t));
    if( !TSP->Node ) {
        DPrintf("TSPReadNodeChunk:Failed to allocate memory for node array.\n");
        return 0;
    }
    memset(TSP->Node,0,TSP->Header.NumNodes * sizeof(TSPNode_t));
    for( i = 0; i < TSP->Header.NumNodes; i++ ) {
        DPrintf(" -- NODE %i -- \n",i);
        TSP->Node[i].FileOffset.Offset = ftell(InFile);
        DPrintf("TSPReadNodeChunk:Reading node %i at %i\n",i,TSP->Node[i].FileOffset.Offset);

        fread(&TSP->Node[i].BBox,sizeof(TSPBBox_t),1,InFile);
        fread(&TSP->Node[i].Child1Index,sizeof(TSP->Node[i].Child1Index),1,InFile);
        fread(&TSP->Node[i].Child2Index,sizeof(TSP->Node[i].Child2Index),1,InFile);
        fread(&TSP->Node[i].Child3Index,sizeof(TSP->Node[i].Child3Index),1,InFile);
        fread(&TSP->Node[i].BaseData,sizeof(TSP->Node[i].BaseData),1,InFile);
        fread(&TSP->Node[i].NumFaces,sizeof(TSP->Node[i].NumFaces),1,InFile);
        fread(&TSP->Node[i].Type,sizeof(TSP->Node[i].Type),1,InFile);
        fread(&TSP->Node[i].U6,sizeof(TSP->Node[i].U6),1,InFile);

        TSP->Node[i].FaceList = NULL;
        DPrintf("Read %li bytes for node %i\n",ftell(InFile) - TSP->Node[i].FileOffset.Offset,i);
        DPrintf("TSPReadNodeChunk:Node BaseData %i (References offset %i)\n",TSP->Node[i].BaseData,
                TSP->Node[i].BaseData + TSP->Header.NodeOffset);
        DPrintf("TSPReadNodeChunk:Node Child1 %i\n",TSP->Node[i].Child1Index);
        DPrintf("TSPReadNodeChunk:Node Child2 %i\n",TSP->Node[i].Child2Index);
        DPrintf("TSPReadNodeChunk:Node Child3 %i\n",TSP->Node[i].Child3Index);
        DPrintf("TSPReadNodeChunk:Node NumFaces %i\n",TSP->Node[i].NumFaces);
        DPrintf("TSPReadNodeChunk:Node Type %i\n",TSP->Node[i].Type);
        DPrintf("TSPReadNodeChunk:Node U6 %i\n",TSP->Node[i].U6);
        if( TSP->Node[i].NumFaces != 0 ) {
            TSP->Node[i].OpaqueFaceList = NULL;
            TSP->Node[i].FaceList = malloc(TSP->Node[i].NumFaces * sizeof(TSPFace_t));
            if( !TSP->Node[i].FaceList ) {
                DPrintf("TSPReadNodeChunk:Failed to allocate memory for face list\n");
                return 0;
            }
            PrevFilePosition = ftell(InFile);
            fseek(InFile,TSP->Node[i].BaseData + TSP->Header.FaceOffset,SEEK_SET);
            CurrentFaceIndex = 0;
            DPrintf("Reading at %li (%li)\n",ftell(InFile),ftell(InFile) - 2048);
            while( CurrentFaceIndex < TSP->Node[i].NumFaces ) {
                fread(&PrimitiveType, sizeof(PrimitiveType), 1, InFile);
                DPrintf("Parsing primitive of type %i at %li\n",PrimitiveType,(ftell(InFile) - 2048) - 2);
                switch(PrimitiveType) {
                    case 0:
                    case 2:
                    case 4:
                    case 6:
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V0,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V0),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V1,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V1),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V2,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V2),1,InFile);
                        TSP->Node[i].FaceList[CurrentFaceIndex].IsTextured = false;
                        CurrentFaceIndex++;
                        break;
                    case 1:
                    case 3:
                    case 5:
                    case 7:
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V0,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V0),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V1,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V1),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].V2,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].V2),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].UV0,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].UV0),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].CBA,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].CBA),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].UV1,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].UV1),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].TSB,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].TSB),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].UV2,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].UV2),1,InFile);
                        fread(&TSP->Node[i].FaceList[CurrentFaceIndex].Pad,sizeof(TSP->Node[i].FaceList[CurrentFaceIndex].Pad),1,InFile);
                        TSP->Node[i].FaceList[CurrentFaceIndex].IsTextured = true;
                        TSPPrintFace(&TSP->Node[i].FaceList[CurrentFaceIndex]);
                        CurrentFaceIndex++;
                        break;
                    default:
                        break;
                }
            }
            DPrintf("Expected %i faces got %i\n",TSP->Node[i].NumFaces, CurrentFaceIndex);
            assert(CurrentFaceIndex == TSP->Node[i].NumFaces);
            fseek(InFile,PrevFilePosition,SEEK_SET);            
        }
    }
    TSPLookUpChildNode(TSP,InFile);
    DPrintf("Current file offset is %li\n",ftell(InFile));
    return 1;
}

int TSPReadVertexChunk(TSP_t *TSP,FILE *InFile)
{
    int Ret;
    int i;
    
    if( !TSP || !InFile ) {
        bool InvalidFile = (InFile == NULL ? true : false);
        printf("TSPReadVertexChunk: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return 0;
    }
    
    if( TSP->Header.NumVertices == 0 ) {
        DPrintf("TSPReadVertexChunk:No vertices found in file %s.\n",TSP->FName);
        return 0;
    }
    
    TSP->Vertex = malloc(TSP->Header.NumVertices * sizeof(TSPVert_t));
    
    if( !TSP->Vertex ) {
        DPrintf("TSPReadVertexChunk:Failed to allocate memory for Verte array\n");
        return 0;
    }
    
    for( i = 0; i < TSP->Header.NumVertices; i++ ) {
        Ret = fread(&TSP->Vertex[i],sizeof(TSPVert_t),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadVertexChunk:Early failure when reading vertex %i\n",i);
            return 0;
        }
//         DPrintf(" -- Vertex %i --\n",i);
//         PrintTSPVec3(TSP->Vertex[i].Position);
//         assert(TSP->Vertex[i].Pad == 104 || TSP->Vertex[i].Pad == 105);
    }
    return 1;
}

int TSPReadColorChunk(TSP_t *TSP,FILE *InFile)
{
    int Ret;
    int i;
    
    if( !TSP || !InFile ) {
        bool InvalidFile = (InFile == NULL ? true : false);
        printf("TSPReadColorChunk: Invalid %s\n",InvalidFile ? "file" : "tsp struct");
        return 0;
    }
    if( TSP->Header.NumVertices == 0 ) {
        DPrintf("TSPReadColorChunk:0 colors found in file %s.\n",TSP->FName);
        return 0;
    }
    
    TSP->Color = malloc(TSP->Header.NumColors * sizeof(Color1i_t));
    
    if( !TSP->Color ) {
        DPrintf("TSPReadColorChunk:Failed to allocate memory for color array\n");
        return 0;
    }
    
    for( i = 0; i < TSP->Header.NumColors; i++ ) {
        Ret = fread(&TSP->Color[i],sizeof(Color1i_t),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadColorChunk:Early failure when reading color %i\n",i);
            return 0;
        }
    }
    return 1;
}

int TSPReadCollisionChunk(TSP_t *TSP,FILE *InFile)
{
    short Pad;
    int Ret;
    int i;
    
    if( !TSP || !InFile ) {
        bool InvalidFile = (InFile == NULL ? true : false);
        printf("TSPReadCollisionChunk: Invalid %s.\n",InvalidFile ? "file" : "tsp struct");
        return 0;
    }
    
    TSP->CollisionData = malloc(sizeof(TSPCollision_t));
    if( !TSP->CollisionData ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for CollisionData\n");
        return 0;
    }
    
    TSP->CollisionData->KDTree = NULL;
    TSP->CollisionData->FaceIndexList = NULL;
    TSP->CollisionData->Vertex = NULL;
    TSP->CollisionData->Normal = NULL;
    TSP->CollisionData->Face = NULL;
    
    Ret = fread(&TSP->CollisionData->Header,sizeof(TSPCollisionHeader_t),1,InFile);
    if( Ret != 1 ) {
        DPrintf("TSPReadCollisionChunk:Early failure when reading collision header.\n");
        return 0;
    }
    DPrintf("TSPReadCollisionChunk:Header\n");
//     DPrintf("U0|U1|U2|U3:%u %u %u %u\n",TSP->CollisionData->Header.U0,TSP->CollisionData->Header.U1,TSP->CollisionData->Header.U2,
//         TSP->CollisionData->Header.U3
//     );
    DPrintf("CollisionBoundMinX:%i\n",TSP->CollisionData->Header.CollisionBoundMinX);
    DPrintf("CollisionBoundMinZ:%i\n",TSP->CollisionData->Header.CollisionBoundMinZ);
    DPrintf("CollisionBoundMaxX:%i\n",TSP->CollisionData->Header.CollisionBoundMaxX);
    DPrintf("CollisionBoundMaxZ:%i\n",TSP->CollisionData->Header.CollisionBoundMaxZ);
    DPrintf("Num Collision KDTree Nodes:%u\n",TSP->CollisionData->Header.NumCollisionKDTreeNodes);
    DPrintf("Num Collision Face Index:%u\n",TSP->CollisionData->Header.NumCollisionFaceIndex);
    DPrintf("NumVertices:%u\n",TSP->CollisionData->Header.NumVertices);
    DPrintf("NumNormals:%u\n",TSP->CollisionData->Header.NumNormals);
    DPrintf("NumFaces:%u\n",TSP->CollisionData->Header.NumFaces);

    TSP->CollisionData->KDTree = malloc(TSP->CollisionData->Header.NumCollisionKDTreeNodes * sizeof(TSPCollisionKDTreeNode_t));
    if( !TSP->CollisionData->KDTree ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for KD-Tree\n");
        return 0;
    }
    for( i = 0; i < TSP->CollisionData->Header.NumCollisionKDTreeNodes; i++ ) {
        Ret = fread(&TSP->CollisionData->KDTree[i],sizeof(TSP->CollisionData->KDTree[i]),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadCollisionChunk:Early failure when reading KDTree nodes.\n");
            return 0;
        }
    }
    TSP->CollisionData->FaceIndexList = malloc(TSP->CollisionData->Header.NumCollisionFaceIndex * sizeof(short));
    if( !TSP->CollisionData->FaceIndexList ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for Face Index Array\n");
        return 0;
    }
    for( i = 0; i < TSP->CollisionData->Header.NumCollisionFaceIndex; i++ ) {
        Ret = fread(&TSP->CollisionData->FaceIndexList[i],sizeof(TSP->CollisionData->FaceIndexList[i]),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadCollisionChunk:Early failure when reading Collison Face Index data.\n");
            return 0;
        }
//         DPrintf("-- H %i at %i --\n",i,GetCurrentFilePosition(InFile));
//         DPrintf("%i\n",TSP->CollisionData->H[i]);
    }
    fread(&Pad,sizeof(Pad),1,InFile);
    if( Pad != 0 ) {
        //Undo the last read.
        fseek(InFile,-sizeof(Pad),SEEK_CUR);
        
    }
    DPrintf("TSPReadCollisionChunk:Vertex at %li\n",ftell(InFile));
    TSP->CollisionData->Vertex = malloc(TSP->CollisionData->Header.NumVertices * sizeof(TSPVert_t));
    if( !TSP->CollisionData->Vertex ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for Vertex Array\n");
        return 0;
    }
    for( i = 0; i < TSP->CollisionData->Header.NumVertices; i++ ) {
        Ret = fread(&TSP->CollisionData->Vertex[i],sizeof(TSP->CollisionData->Vertex[i]),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadCollisionChunk:Early failure when reading vertex data.\n");
            return 0;
        }
//         DPrintf("-- Vertex %i --\n",i);
//         PrintTSPVec3(TSP->CollisionData->Vertex[i].Position);
//         DPrintf("Pad is %i\n",TSP->CollisionData->Vertex[i].Pad);
//         assert(TSP->CollisionData->Vertex[i].Pad == 104 || TSP->CollisionData->Vertex[i].Pad == 105);
    }
    DPrintf("TSPReadCollisionChunk:Normals at %li\n",ftell(InFile));
    TSP->CollisionData->Normal = malloc(TSP->CollisionData->Header.NumNormals * sizeof(TSPVert_t));
    if( !TSP->CollisionData->Normal ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for Normal Array\n");
        return 0;
    }
    for( i = 0; i < TSP->CollisionData->Header.NumNormals; i++ ) {
        Ret = fread(&TSP->CollisionData->Normal[i],sizeof(TSP->CollisionData->Normal[i]),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadCollisionChunk:Early failure when reading normal data.\n");
            return 0;
        }
        DPrintf("-- Normal %i --\n",i);
        DPrintf("%i;%i;%i\n",TSP->CollisionData->Normal[i].Position.x,TSP->CollisionData->Normal[i].Position.y,
                TSP->CollisionData->Normal[i].Position.z);
//         DPrintf("Pad is %i\n",TSP->CollisionData->Normal[i].Pad);
//         assert(TSP->CollisionData->Normal[i].Pad == 0);
    }
    DPrintf("TSPReadCollisionChunk:Faces at %li\n",ftell(InFile));
    TSP->CollisionData->Face = malloc(TSP->CollisionData->Header.NumFaces * sizeof(TSPCollisionFace_t));
    if( !TSP->CollisionData->Face ) {
        DPrintf("TSPReadCollisionChunk:Failed to allocate memory for Face Array\n");
        return 0;
    }
    for( i = 0; i < TSP->CollisionData->Header.NumFaces; i++ ) {
        Ret = fread(&TSP->CollisionData->Face[i],sizeof(TSP->CollisionData->Face[i]),1,InFile);
        if( Ret != 1 ) {
            DPrintf("TSPReadCollisionChunk:Early failure when reading face data.\n");
            return 0;
        }
        DPrintf("-- Face %i --\n",i);
//         DPrintf("V0|V1|V2:%u %u %u\n",TSP->CollisionData->Face[i].V0,TSP->CollisionData->Face[i].V1,TSP->CollisionData->Face[i].V2);
        assert(TSP->CollisionData->Face[i].NormalIndex < TSP->CollisionData->Header.NumNormals);
        DPrintf("Normal Index:%u\n",TSP->CollisionData->Face[i].NormalIndex);

    }
    return 1;
}

TSPCollision_t *TSPGetCollisionDataFromPoint(TSP_t *TSPList,TSPVec3_t Point)
{
    TSP_t *TSP;

    for( TSP = TSPList; TSP; TSP = TSP->Next ) {
        if( Point.x >= TSP->CollisionData->Header.CollisionBoundMinX && Point.x <= TSP->CollisionData->Header.CollisionBoundMaxX &&
        Point.z >= TSP->CollisionData->Header.CollisionBoundMinZ && Point.z <= TSP->CollisionData->Header.CollisionBoundMaxZ ) {
            return TSP->CollisionData;
        }
    }
    return NULL;
}

int TSPGetYFromCollisionFace(TSPCollision_t *CollisionData,TSPVec3_t Point,TSPCollisionFace_t *Face)
{
    float OutY;
    float SolveFaceY;
    TSPVec3_t Normal;
    vec2 Point2D;
    
    TSPVec3ToVec2(Point,Point2D);

    
    Normal = CollisionData->Normal[Face->NormalIndex].Position;
    if( abs(Normal.y) < 257 ) {
        DPrintf("TSPGetYFromCollisionFace:Returning it normal...\n");
        OutY = TSPFixedToFloat(CollisionData->Vertex[Face->V0].Position.y,15);
    } else {
        DPrintf("Normal fixed is:%i;%i;%i\n",Normal.x,Normal.y,Normal.z);
        float NormalX;
        float NormalY;
        float NormalZ;
        float PointX;
        float PointZ;
        DPrintf("Normal as int is:%i;%i;%i\n",Normal.x,Normal.y,Normal.z);
        NormalX = TSPFixedToFloat(Normal.x,15);
        NormalY = TSPFixedToFloat(Normal.y,15);
        NormalZ = TSPFixedToFloat(Normal.z,15);
        PointX =  Point2D[0];
        PointZ =  Point2D[1];
        DPrintf("Normal as float is:%f;%f;%f\n",NormalX,NormalY,NormalZ);
        DPrintf("TSPGetYFromCollisionFace:DistanceOffset Fixed:%i Real:%i\n",Face->PlaneDistance << 0xf,Face->PlaneDistance );
        SolveFaceY = -(NormalX * PointX + NormalZ * PointZ + (Face->PlaneDistance /*<< 0xf*/));
        OutY = SolveFaceY / NormalY;
    }
    return (int) OutY;
}

//Cross product that returns the sign of the new vector.
float TSPSign (vec2 p1, vec2 p2, vec2 p3)
{
    return (p1[0] - p3[0]) * (p2[1] - p3[1]) - (p2[0] - p3[0]) * (p1[1] - p3[1]);
}

bool TSPPointInTriangle (TSPCollision_t *CollisionData,TSPVec3_t Point,TSPCollisionFace_t *Face)
{
    float d1, d2, d3;
    bool HasNegative, HasPositive;
    vec2 v1;
    vec2 v2;
    vec2 v3;
    vec2 pt;
 
    TSPVec3ToVec2(Point,pt);
    TSPVec3ToVec2(CollisionData->Vertex[Face->V0].Position,v1);
    TSPVec3ToVec2(CollisionData->Vertex[Face->V1].Position,v2);
    TSPVec3ToVec2(CollisionData->Vertex[Face->V2].Position,v3);
    
    if( CollisionData->Normal[Face->NormalIndex].Position.y > 0 ) {
        return false;
    }

    d1 = TSPSign(pt, v1, v2);
    d2 = TSPSign(pt, v2, v3);
    d3 = TSPSign(pt, v3, v1);

    HasNegative = (d1 < 0) || (d2 < 0) || (d3 < 0);
    HasPositive = (d1 > 0) || (d2 > 0) || (d3 > 0);

    return !(HasNegative && HasPositive);
}

int TSPCheckCollisionFaceIntersection(TSPCollision_t *CollisionData,TSPVec3_t Point,int StartingFaceListIndex,int NumFaces,int *OutY)
{
    int i;
    TSPCollisionFace_t *CurrentFace;
    int FaceListIndex;
    int FaceIndex;
    int Y;
    int MinY;
    
    MinY = 99999;
    DPrintf("TSPCheckCollisionFaceIntersection:Checking %i faces\n",NumFaces);
    FaceListIndex = StartingFaceListIndex;
    i = 0;

    while( i < NumFaces ) {
        //First fetch the face index from Face Index array...
        FaceIndex = CollisionData->FaceIndexList[FaceListIndex];
        //Then grab the corresponding face from the face array...
        CurrentFace = &CollisionData->Face[FaceIndex];
        DPrintf("Iteration %i\n",i);
        if( TSPPointInTriangle(CollisionData,Point,CurrentFace) == 1) {
            DPrintf("Point is in face %i...grabbing Y value\n",FaceIndex);
            Y = TSPGetYFromCollisionFace(CollisionData,Point,CurrentFace);
            DPrintf("Got %i as Y PointY was:%i\n",Y,Point.y);
            if( Y < MinY ) {
                MinY = Y;
            }
        } else {
            DPrintf("Missed face %i...\n",FaceIndex);
        }
        //Make sure to increment it in order to fetch the next face.
        FaceListIndex++;
        i++;
    }
    if( MinY != 99999 ) {
        *OutY = MinY;
        return 1;
    }
    return -1;
}

int TSPGetPointYComponentFromKDTree(vec3 Point,TSP_t *TSPList,int *PropertySetFileIndex,int *OutY)
{
    TSPCollision_t *CollisionData;
    TSPCollisionKDTreeNode_t *Node;
    TSPVec3_t Position;
    int WorldBoundMinX;
    int WorldBoundMinZ;
    int MinValue;
    int CurrentNode;
    
    Position = TSPGLMVec3ToTSPVec3(Point);
    CollisionData = TSPGetCollisionDataFromPoint(TSPList,Position);
    
    if( CollisionData == NULL ) {
        DPrintf("TSPGetPointYComponentFromKDTree:Point wasn't in any collision data...\n");
        return -1;
    }
    
    WorldBoundMinX = CollisionData->Header.CollisionBoundMinX;
    WorldBoundMinZ = CollisionData->Header.CollisionBoundMinZ;
    
    CurrentNode = 0;
    
    while( 1 ) {
//         CurrentPlaneIndex = (CurrentPlane - GOffset) / sizeof(TSPCollisionG_t);
        DPrintf("Node Index %i\n",CurrentNode);
        Node = &CollisionData->KDTree[CurrentNode];
        if( Node->Child0 < 0 ) {
            DPrintf("Node Child1:%i\n",Node->Child1);
            assert(Node->Child1 >= 0);
            DPrintf("Done...found a leaf...node %i FaceIndex:%i Child0:%i NumFaces:%i Child1:%i PropertySetFileIndex:%i\n",CurrentNode,
                   CollisionData->FaceIndexList[Node->Child1],
                   Node->Child0,~Node->Child0,Node->Child1,Node->PropertySetFileIndex);
            if( PropertySetFileIndex != NULL ) {
                *PropertySetFileIndex = Node->PropertySetFileIndex;
            }
            return TSPCheckCollisionFaceIntersection(CollisionData,Position,Node->Child1,~Node->Child0,OutY);
        }
        if( Node->Child1 < 0 ) {
            MinValue = WorldBoundMinZ + Node->SplitValue;
            if (Position.z < MinValue) {
                CurrentNode = Node->Child0;
            } else {
                CurrentNode = ~Node->Child1;
                WorldBoundMinZ = MinValue;
            }
        } else {
            MinValue = WorldBoundMinX + Node->SplitValue;
            if( Position.x < MinValue ) {
                CurrentNode = Node->Child0;
            } else {
                CurrentNode = Node->Child1;
                WorldBoundMinX = MinValue;
            }
        }
    }
    return -1;
}

TSP_t *TSPLoad(FILE *TSPFile,int TSPOffset)
{
    TSP_t *TSP;
    
    TSP = NULL;
    
    if( TSPFile == NULL ) {
        DPrintf("Invalid TSP File...\n");
        goto Failure;
    }
    TSP = malloc(sizeof(TSP_t));
    
    if( !TSP ) {
        DPrintf("TSPLoad:Failed to allocate memory for TSP struct\n");
        goto Failure;
    }
    
    TSP->Node = NULL;
    TSP->CollisionData = NULL;
    TSP->VAOList = NULL;
    TSP->CollisionVAOList = NULL;
    TSP->VAOCreated = false;
    TSP->Next = NULL;
    TSP->Number = 1;
    TSP->Face = NULL;
    TSP->Vertex = NULL;
    TSP->Color = NULL;
    TSP->TransparentFaceList = NULL;
    TSP->TransparentVAO = NULL;
    TSP->DynamicData = NULL;
    TSP->FName = StringCopy("World");
    
    fseek(TSPFile,TSPOffset,SEEK_SET);
    
    fread(&TSP->Header.Id,sizeof(TSP->Header.Id),1,TSPFile);
    fread(&TSP->Header.Version,sizeof(TSP->Header.Version),1,TSPFile);
    fread(&TSP->Header.NumNodes,sizeof(TSP->Header.NumNodes),1,TSPFile);
    fread(&TSP->Header.NodeOffset,sizeof(TSP->Header.NodeOffset),1,TSPFile);
    fread(&TSP->Header.NumFaces,sizeof(TSP->Header.NumFaces),1,TSPFile);
    fread(&TSP->Header.FaceOffset,sizeof(TSP->Header.FaceOffset),1,TSPFile);
    fread(&TSP->Header.NumVertices,sizeof(TSP->Header.NumVertices),1,TSPFile);
    fread(&TSP->Header.VertexOffset,sizeof(TSP->Header.VertexOffset),1,TSPFile);
    fread(&TSP->Header.NumB,sizeof(TSP->Header.NumB),1,TSPFile);
    fread(&TSP->Header.BOffset,sizeof(TSP->Header.BOffset),1,TSPFile);
    fread(&TSP->Header.NumColors,sizeof(TSP->Header.NumColors),1,TSPFile);
    fread(&TSP->Header.ColorOffset,sizeof(TSP->Header.ColorOffset),1,TSPFile);
    fread(&TSP->Header.NumC,sizeof(TSP->Header.NumC),1,TSPFile);
    fread(&TSP->Header.COffset,sizeof(TSP->Header.COffset),1,TSPFile);
        
    DPrintf("Sizeof TSPHeader is %li\n",sizeof(TSPHeader_t));
    DPrintf(" -- TSP HEADER --\n");
    DPrintf("TSP Number: %i\n",TSP->Number);
    DPrintf("TSP File: %s\n",TSP->FName);
    DPrintf("Id:%u\n",TSP->Header.Id);
    DPrintf("Version:%u\n",TSP->Header.Version);
    DPrintf("NumNodes:%i NodeOffset:%i\n",TSP->Header.NumNodes,TSP->Header.NodeOffset);
    DPrintf("NumFaces:%i FaceOffset:%i\n",TSP->Header.NumFaces,TSP->Header.FaceOffset);
    DPrintf("NumVertices:%i VertexOffset:%i\n",TSP->Header.NumVertices,TSP->Header.VertexOffset);
    DPrintf("NumB:%i BOffset:%i\n",TSP->Header.NumB,TSP->Header.BOffset);
    DPrintf("NumColors:%i ColorOffset:%i\n",TSP->Header.NumColors,TSP->Header.ColorOffset);
    DPrintf("NumC:%i COffset:%i\n",TSP->Header.NumC,TSP->Header.COffset);

    //NOTE(Adriano):We need to add the current TSP offset since it is hosted inside the BSD file
    TSP->Header.NodeOffset += TSPOffset;
    TSP->Header.FaceOffset += TSPOffset;
    TSP->Header.VertexOffset += TSPOffset;
    TSP->Header.ColorOffset += TSPOffset;
    
    assert(ftell(TSPFile) == TSP->Header.NodeOffset);
    if( !TSPReadNodeChunk(TSP,TSPFile,TSPOffset) ) {
        goto Failure;
    }
    fseek(TSPFile,TSP->Header.VertexOffset,SEEK_SET);
    assert(ftell(TSPFile) == TSP->Header.VertexOffset);
    if( !TSPReadVertexChunk(TSP,TSPFile) ) {
        goto Failure;
    }
    fseek(TSPFile,TSP->Header.ColorOffset,SEEK_SET);
    assert(ftell(TSPFile) == TSP->Header.ColorOffset);
    if( !TSPReadColorChunk(TSP,TSPFile) ) {
        goto Failure;
    }
    return TSP;
Failure:
    TSPFree(TSP);
    return NULL;
}
