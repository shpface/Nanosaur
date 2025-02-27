/****************************/
/* 	ENVIRONMENTMAP.C   		*/
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DCamera.h>
#include <QD3DErrors.h>
#include <QD3DGeometry.h>
#include <QD3DGroup.h>
#include <QD3DMath.h>
#include	<QD3DTransform.h>

#include "globals.h"
#include "qd3d_support.h"
#include "misc.h"
#include "environmentmap.h"
#include "objects.h"
#include "3dmath.h"

extern	TQ3Matrix4x4 		gWorkMatrix;
extern	ObjNode			*gFirstNodePtr;
extern	QD3DSetupOutputType		*gGameViewInfoPtr;
extern	long				gNodesDrawn;

/****************************/
/*    PROTOTYPES            */
/****************************/

static inline void ReflectVector(float viewX, float viewY, float viewZ, TQ3Vector3D *N, TQ3Vector3D *out);
static void CalcEnvMap_Recurse(TQ3Object obj);
static void EnvironmentMapTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData);

/****************************/
/*    CONSTANTS             */
/****************************/

#define	MAX_REFLECTIONMAP_QUEUESIZE	60



/*********************/
/*    VARIABLES      */
/*********************/


static short			gReflectionMapQueueSize = 0;
static TQ3TriMeshData	gReflectionMapQueue[MAX_REFLECTIONMAP_QUEUESIZE];
static TQ3TriMeshData	*gReflectionMapQueue2[MAX_REFLECTIONMAP_QUEUESIZE];		// ptr to skeleton's trimesh data
static TQ3Point3D	gCamCoord = {0,0,0};


/******************* INIT REFLECTION MAP QUEUE ************************/

void InitReflectionMapQueue(void)
{
short	i;

	for (i=0; i < gReflectionMapQueueSize; i++)								// free the data
	{
		if (gReflectionMapQueue2[i] == nil)									// if no skeleton stuff...
			Q3TriMesh_EmptyData(&gReflectionMapQueue[i]);					// ... then nuke non-skeleton tm data
	}

	gReflectionMapQueueSize = 0;
}


/******************* SUBMIT REFLECTION MAP QUEUE ************************/

void SubmitReflectionMapQueue(QD3DSetupOutputType *viewInfo)
{
short	i;
TQ3Status	status;

	QD3D_SetTextureFilter(kQATextureFilter_Mid);						// set nice textures

	for (i=0; i < gReflectionMapQueueSize; i++)
	{
		if (gReflectionMapQueue2[i])									// is skeleton?
			status = Q3TriMesh_Submit(gReflectionMapQueue2[i], viewInfo->viewObject);			
		else
			status = Q3TriMesh_Submit(&gReflectionMapQueue[i], viewInfo->viewObject);	
		if (status != kQ3Success)
			DoFatalAlert("SubmitReflectionMapQueue: Q3TriMesh_Submit failed!");
		gNodesDrawn++;
	}
	
	QD3D_SetTextureFilter(kQATextureFilter_Fast);						// set normal textures
	
}



/************ CALC ENVIRONMENT MAPPING COORDS ************/

void CalcEnvironmentMappingCoords(TQ3Point3D *camCoord)
{
ObjNode	*thisNodePtr;

	gCamCoord = *camCoord;

	InitReflectionMapQueue();	

				/****************************************/
				/* UPDATE OBJECTS WITH ENVIRONMENT MAPS */
				/****************************************/

	thisNodePtr = gFirstNodePtr;
	do
	{
		if (thisNodePtr->StatusBits & STATUS_BIT_REFLECTIONMAP)
		{
			short	n,i;
			
			switch(thisNodePtr->Genre)
			{
				case	SKELETON_GENRE:			
						n = thisNodePtr->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
						for (i = 0; i < n; i++)
							EnvironmentMapTriMesh(nil,&thisNodePtr->Skeleton->localTriMeshes[i]);
						break;
		
				default:					
						Q3Matrix4x4_SetIdentity(&gWorkMatrix);					// init to identity matrix
						CalcEnvMap_Recurse(thisNodePtr->BaseGroup);				// recurse this one
			}
		}
			
		thisNodePtr = thisNodePtr->NextNode;									// next node
	}
	while (thisNodePtr != nil);


}



/****************** CALC ENV MAP_RECURSE *********************/

static void CalcEnvMap_Recurse(TQ3Object obj)
{
TQ3Matrix4x4		transform;
TQ3GroupPosition	position;
TQ3Object   		object;
TQ3Matrix4x4  		stashMatrix;


				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		Q3Matrix4x4_Multiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	else
	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		if (Q3Geometry_GetType(obj) == kQ3GeometryTypeTriMesh)
			EnvironmentMapTriMesh(obj,nil);									// map this trimesh
	}
	
			/****************************/
			/* SEE IF RECURSE SUB-GROUP */
			/****************************/

	else
	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
  		stashMatrix = gWorkMatrix;										// push matrix
  		
  		
  		Q3Group_GetFirstPosition(obj, &position);						// get 1st object in group
  		while(position != nil)											// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != nil)
   			{
					/* RECURSE THIS OBJ */
					
				CalcEnvMap_Recurse(object);								// recurse this object    			
    			Q3Object_Dispose(object);								// dispose local ref
   			}
   			Q3Group_GetNextPosition(obj, &position);					// get next object in group
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix
	}
}



/****************************** ENVIRONMENT MAP TRI MESH *****************************************/
//
// NOTE:  This assumes no face normals, thus if there are face normals, they won't be transformed and weird things will happen.
//
// INPUT:	inData = if set, then input data is coming from already transformed skeleton.
//

static void EnvironmentMapTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData)
{
TQ3Status			status;
long				numVertecies,vertNum,numFaceAttribTypes,faceNum,numFaces;
TQ3Point3D			*vertexList = nil;
TQ3TriMeshData		*triMeshDataPtr = nil;
TQ3TriMeshAttributeData	*attribList = nil, *faceAttribList = nil;
short				numVertAttribTypes,a;
TQ3Vector3D			*normals = nil, reflectedVector, *normals2 = nil;
TQ3Param2D			*uvList = nil;
Boolean				hasUVs = true;
TQ3Matrix4x4		tempm,invTranspose;

			/* GET POINTER TO TRIMESH DATA STRUCTURE */
			
	if (gReflectionMapQueueSize >= MAX_REFLECTIONMAP_QUEUESIZE)
		DoFatalAlert("EnvironmentMapTriMesh: gReflectionMapQueueSize >= MAX_REFLECTIONMAP_QUEUESIZE");
		
				/* GET TRIMESH INFO */
				
	if (inData)
	{
		gReflectionMapQueue2[gReflectionMapQueueSize] = inData;
		triMeshDataPtr = inData;
	}
	else
	{
		triMeshDataPtr = &gReflectionMapQueue[gReflectionMapQueueSize];
		gReflectionMapQueue2[gReflectionMapQueueSize] = nil;
		
		status = Q3TriMesh_GetData(theTriMesh, triMeshDataPtr);						// get trimesh data
		if (status != kQ3Success) 
			DoFatalAlert("EnvironmentMapTriMesh: Q3TriMesh_GetData failed!");
	}		
	
	numFaces 		= triMeshDataPtr->numTriangles;
	numVertecies 	= triMeshDataPtr->numPoints;										// get # verts
	vertexList 		= triMeshDataPtr->points;											// point to vert list
	attribList 		= triMeshDataPtr->vertexAttributeTypes;								// point to vert attib list
	numVertAttribTypes = triMeshDataPtr->numVertexAttributeTypes;
	numFaceAttribTypes = triMeshDataPtr->numTriangleAttributeTypes;
	faceAttribList 	= triMeshDataPtr->triangleAttributeTypes;

				/* FIND UV ATTRIBUTE LIST */
				
	for (a = 0; a < numVertAttribTypes; a++)									// scan for uv
	{
		if ((attribList[a].attributeType == kQ3AttributeTypeSurfaceUV) || 
			(attribList[a].attributeType == kQ3AttributeTypeShadingUV))
		{
			uvList = (TQ3Param2D*)attribList[a].data;										// point to list of normals
			goto got_uv;
		}
	}
	hasUVs = false;																// no uv's

got_uv:

#if 0
	// SOURCE PORT NOTE: This caused the background to disappear at specific angles in the menu.
	// Fixed below (after transforming vertex coords).

			/* TRANSFORM BOUNDING BOX */
			
	Q3Point3D_To3DTransformArray(&triMeshDataPtr->bBox.min, &gWorkMatrix, &triMeshDataPtr->bBox.min, 2,
							 sizeof(TQ3Point3D), sizeof(TQ3Point3D));				
	
#endif

				/* TRANSFORM VERTEX COORDS */
				
	Q3Point3D_To3DTransformArray(vertexList, &gWorkMatrix, vertexList, numVertecies,
								 sizeof(TQ3Point3D), sizeof(TQ3Point3D));				


				/* TRANSFORM BOUNDING BOX */

	// Source port fix
	Q3BoundingBox_SetFromPoints3D(&triMeshDataPtr->bBox, vertexList, numVertecies, sizeof(TQ3Point3D));

	
				/* TRANSFORM VERTEX NORMALS */

	Q3Matrix4x4_Invert(&gWorkMatrix, &tempm);							// calc inverse-transpose matrix
	Q3Matrix4x4_Transpose(&tempm,&invTranspose);			


	for (a = 0; a < numVertAttribTypes; a++)									// scan for normals
	{
		if (attribList[a].attributeType == kQ3AttributeTypeNormal)
		{
			normals = (TQ3Vector3D*)attribList[a].data;										// point to list of normals

			for (vertNum = 0; vertNum < numVertecies; vertNum++)				// transform all normals
			{				
				Q3Vector3D_Transform(&normals[vertNum], &invTranspose, &normals[vertNum]);
				Q3Vector3D_Normalize(&normals[vertNum], &normals[vertNum]);
			}
			break;
		}
	}


				/* TRANSFORM FACE NORMALS */

	for (a = 0; a < numFaceAttribTypes; a++)								// scan for normals
	{
		if (faceAttribList[a].attributeType == kQ3AttributeTypeNormal)
		{
			normals2 = (TQ3Vector3D*)faceAttribList[a].data;								// point to list of normals

			for (faceNum = 0; faceNum < numFaces; faceNum++)				// transform all normals
			{			
				Q3Vector3D_Transform(&normals2[faceNum], &invTranspose, &normals2[faceNum]);
				Q3Vector3D_Normalize(&normals2[faceNum], &normals2[faceNum]);
			}
			break;
		}
	}



		/****************************/
		/* CALC UVS FOR EACH VERTEX */
		/****************************/	

	if (hasUVs)																	// only if has UV's
	{
		float	camX,camY,camZ;
		
		camX = gCamCoord.x;
		camY = gCamCoord.y;
		camZ = gCamCoord.z;
		
		for (vertNum = 0; vertNum < numVertecies; vertNum++)
		{
			float	eyeVectorX,eyeVectorY,eyeVectorZ;
			
					/* CALC VECTOR TO VERTEX */
					
			eyeVectorX = vertexList[vertNum].x - camX;
			eyeVectorY = vertexList[vertNum].y - camY;
			eyeVectorZ = vertexList[vertNum].z - camZ;

		
					/* REFLECT VECTOR AROUND VERTEX NORMAL */
		
			ReflectVector(eyeVectorX, eyeVectorY, eyeVectorZ,
							&normals[vertNum],&reflectedVector);
		
		
						/* CALC UV */
				
			uvList[vertNum].u = (reflectedVector.x * .5f) + .5f;
			uvList[vertNum].v = (-reflectedVector.y * .5f) + .5f;	

		
		}
	}
	
	gReflectionMapQueueSize++;		
}



/*********************** REFLECT VECTOR *************************/
//
// compute reflection vector 
// which is N(2(N.V)) - V 
// N - Surface Normal 
// V - View Direction 
//
//

inline void ReflectVector(float viewX, float viewY, float viewZ, TQ3Vector3D *N, TQ3Vector3D *out)
{
float	normalX,normalY,normalZ;
float	dotProduct,reflectedX,reflectedZ,reflectedY;

	normalX = N->x;
	normalY = N->y;
	normalZ = N->z;
							
	/* compute NxV */
	dotProduct = normalX * viewX;
	dotProduct += normalY * viewY;
	dotProduct += normalZ * viewZ;
	
	/* compute 2(NxV) */
	dotProduct += dotProduct;
	
	/* compute final vector */
	reflectedX = normalX * dotProduct - viewX;
	reflectedY = normalY * dotProduct - viewY;
	reflectedZ = normalZ * dotProduct - viewZ;
	
	/* Normalize the result */
		
	FastNormalizeVector(reflectedX,reflectedY,reflectedZ,out);	
}













