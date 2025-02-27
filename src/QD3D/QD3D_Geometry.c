/****************************/
/*   	QD3D GEOMETRY.C	    */
/* (c)1997 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include <QD3D.h>
#include <QD3DGroup.h>
#include <QD3DTransform.h>
#include <QD3DMath.h>
#include <QD3DGeometry.h>
#include <QD3DView.h>

#include "globals.h"
#include "misc.h"
#include "qd3d_geometry.h"
#include "qd3d_support.h"
#include "objects.h"
#include "terrain.h"

extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	ObjNode				*gCurrentNode;
extern	float				gFramesPerSecondFrac;
extern	TQ3Point3D			gCoord;
extern	TQ3Object	gKeepBackfaceStyleObject;

/****************************/
/*    PROTOTYPES            */
/****************************/

static void CalcRadius_Recurse(TQ3Object obj);
static void ExplodeTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData);
static void ExplodeGeometry_Recurse(TQ3Object obj);
static void ScrollUVs_Recurse(TQ3Object obj);
static void ScrollUVs_TriMesh(TQ3Object theTriMesh);


/****************************/
/*    CONSTANTS             */
/****************************/


/*********************/
/*    VARIABLES      */
/*********************/

float				gMaxRadius;

TQ3Matrix4x4 		gWorkMatrix;

float		gBoomForce,gParticleDecaySpeed;
Byte		gParticleMode;
long		gParticleDensity;
ObjNode		*gParticleParentObj;

TQ3Matrix3x3		gUVTransformMatrix;

long				gNumParticles = 0;
ParticleType		gParticles[MAX_PARTICLES];



/*************** QD3D: CALC OBJECT BOUNDING BOX ************************/

void QD3D_CalcObjectBoundingBox(QD3DSetupOutputType *setupInfo, TQ3Object theObject, TQ3BoundingBox	*boundingBox)
{
	if (setupInfo == nil)
		DoFatalAlert("QD3D_CalcObjectBoundingBox: setupInfo = nil");
	if (theObject == nil)
		DoFatalAlert("QD3D_CalcObjectBoundingBox: theObject = nil");
	if (boundingBox == nil)
		DoFatalAlert("QD3D_CalcObjectBoundingBox: boundingBox = nil");

	Q3View_StartBoundingBox(setupInfo->viewObject, kQ3ComputeBoundsExact);
	do
	{
		Q3Object_Submit(theObject,setupInfo->viewObject);
	}while(Q3View_EndBoundingBox(setupInfo->viewObject, boundingBox) == kQ3ViewStatusRetraverse);
}



/****************** QD3D: CALC OBJECT RADIUS ***********************/
//
// Given any object as input, calculates the radius based on the farthest TriMesh vertex.
//

float QD3D_CalcObjectRadius(TQ3Object theObject)
{
	gMaxRadius = 0;	
	Q3Matrix4x4_SetIdentity(&gWorkMatrix);					// init to identity matrix
	CalcRadius_Recurse(theObject);
	return(gMaxRadius);
}


/****************** CALC RADIUS - RECURSE ***********************/

static void CalcRadius_Recurse(TQ3Object obj)
{
TQ3GroupPosition	position;
TQ3Object   		object;
TQ3ObjectType		oType;
TQ3TriMeshData		triMeshData;
long				v;
TQ3Point3D			tmPoint;
float				dist;
TQ3Matrix4x4		transform;
TQ3Matrix4x4  		stashMatrix;

				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		Q3Matrix4x4_Multiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
	else

				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		switch(oType)
		{
					/* MUST BE TRIMESH */
					
			case	kQ3GeometryTypeTriMesh:
					Q3TriMesh_GetData(obj,&triMeshData);							// get trimesh data	
					for (v = 0; v < triMeshData.numPoints; v++)						// scan thru all verts
					{
						tmPoint = triMeshData.points[v];							// get point
						Q3Point3D_Transform(&tmPoint, &gWorkMatrix, &tmPoint);		// transform it	
						
						dist = sqrt((tmPoint.x * tmPoint.x) +
									(tmPoint.y * tmPoint.y) +
									(tmPoint.z * tmPoint.z));
							
//						dist = Q3Point3D_Distance(&p000, &tmPoint);					// calc dist
						if (dist > gMaxRadius)
							gMaxRadius = dist;
					}
					Q3TriMesh_EmptyData(&triMeshData);
					break;
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
  		stashMatrix = gWorkMatrix;										// push matrix
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			CalcRadius_Recurse(object);								// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix  		
	}
}

//===================================================================================================
//===================================================================================================
//===================================================================================================

#pragma mark =========== particle explosion ==============


/********************** QD3D: INIT PARTICLES **********************/

void QD3D_InitParticles(void)
{
long	i;

	gNumParticles = 0;

	for (i = 0; i < MAX_PARTICLES; i++)
	{
			/* INIT TRIMESH DATA */
			
		gParticles[i].isUsed = false;
		gParticles[i].triMesh.triMeshAttributeSet = nil;
		gParticles[i].triMesh.numTriangles = 1;
		gParticles[i].triMesh.triangles = &gParticles[i].triangle;
		gParticles[i].triMesh.numTriangleAttributeTypes = 0;
		gParticles[i].triMesh.triangleAttributeTypes = nil;
		gParticles[i].triMesh.numEdges = 0;
		gParticles[i].triMesh.edges = nil;
		gParticles[i].triMesh.numEdgeAttributeTypes = 0;
		gParticles[i].triMesh.edgeAttributeTypes = nil;
		gParticles[i].triMesh.numPoints = 3;
		gParticles[i].triMesh.points = &gParticles[i].points[0];
		gParticles[i].triMesh.numVertexAttributeTypes = 2;
		gParticles[i].triMesh.vertexAttributeTypes = &gParticles[i].vertAttribs[0];
		gParticles[i].triMesh.bBox.isEmpty = kQ3True;

			/* INIT TRIANGLE */

		gParticles[i].triangle.pointIndices[0] = 0;
		gParticles[i].triangle.pointIndices[1] = 1;
		gParticles[i].triangle.pointIndices[2] = 2;

			/* INIT VERTEX ATTRIB LIST */

		gParticles[i].vertAttribs[0].attributeType = kQ3AttributeTypeNormal;
		gParticles[i].vertAttribs[0].data = &gParticles[i].vertNormals[0];
		gParticles[i].vertAttribs[0].attributeUseArray = nil;

		gParticles[i].vertAttribs[1].attributeType = kQ3AttributeTypeShadingUV;
		gParticles[i].vertAttribs[1].data = &gParticles[i].uvs[0];
		gParticles[i].vertAttribs[1].attributeUseArray = nil;
	}
}


/********************* FIND FREE PARTICLE ***********************/
//
// OUTPUT: -1 == none free found
//

static inline long FindFreeParticle(void)
{
long	i;

	if (gNumParticles >= MAX_PARTICLES)
		return(-1);

	for (i = 0; i < MAX_PARTICLES; i++)
		if (gParticles[i].isUsed == false)
			return(i);

	return(-1);
}


/****************** QD3D: EXPLODE GEOMETRY ***********************/
//
// Given any object as input, breaks up all polys into separate objNodes &
// calculates velocity et.al.
//

void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed)
{
TQ3Object theObject;

	if (theNode->CType == INVALID_NODE_FLAG)				// make sure the node is valid
		return;


	gBoomForce = boomForce;
	gParticleMode = particleMode;
	gParticleDensity = particleDensity;
	gParticleDecaySpeed = particleDecaySpeed;
	gParticleParentObj = theNode;
	Q3Matrix4x4_SetIdentity(&gWorkMatrix);					// init to identity matrix


		/* SEE IF EXPLODING SKELETON OR PLAIN GEOMETRY */
		
	if (theNode->Genre == SKELETON_GENRE)
	{
		short	numTriMeshes,i;
		
		numTriMeshes = theNode->Skeleton->skeletonDefinition->numDecomposedTriMeshes;
		for (i = 0; i < numTriMeshes; i++)
		{
			ExplodeTriMesh(nil,&theNode->Skeleton->localTriMeshes[i]);				// explode each trimesh individually
		}
	}
	else
	{
		theObject = theNode->BaseGroup;						// get TQ3Object from ObjNode
		ExplodeGeometry_Recurse(theObject);	
	}
}


/****************** EXPLODE GEOMETRY - RECURSE ***********************/

static void ExplodeGeometry_Recurse(TQ3Object obj)
{
TQ3GroupPosition	position;
TQ3Object   		object;
TQ3ObjectType		oType;
TQ3Matrix4x4		transform;
TQ3Matrix4x4  		stashMatrix;

				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		Q3Matrix4x4_Multiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
	else

				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		switch(oType)
		{
					/* MUST BE TRIMESH */
					
			case	kQ3GeometryTypeTriMesh:
					ExplodeTriMesh(obj,nil);
					break;
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
  		stashMatrix = gWorkMatrix;										// push matrix
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			ExplodeGeometry_Recurse(object);						// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix  		
	}
}


/********************** EXPLODE TRIMESH *******************************/
//
// INPUT: 	theTriMesh = trimesh object if input is object (nil if inData)
//			inData = trimesh data if input is raw data (nil if above)
//

static void ExplodeTriMesh(TQ3Object theTriMesh, TQ3TriMeshData *inData)
{
TQ3TriMeshData		triMeshData;
TQ3Point3D			centerPt = {0,0,0};
TQ3Vector3D			vertNormals[3],*normalPtr;
unsigned long		ind[3],t;
TQ3Param2D			*uvPtr;
long				i;

	if (inData)
		triMeshData = *inData;												// use input data
	else
		Q3TriMesh_GetData(theTriMesh,&triMeshData);							// get trimesh data	

			/*******************************/
			/* SCAN THRU ALL TRIMESH FACES */
			/*******************************/
					
	for (t = 0; t < triMeshData.numTriangles; t += gParticleDensity)	// scan thru all faces
	{
				/* GET FREE PARTICLE INDEX */
				
		i = FindFreeParticle();
		if (i == -1)													// see if all out
			break;
		
				/*********************/
				/* INIT TRIMESH DATA */
				/*********************/
 
		gParticles[i].triMesh.triMeshAttributeSet = triMeshData.triMeshAttributeSet;	// set illegal ref to the original attribute set			
		gParticles[i].triMesh.numVertexAttributeTypes = triMeshData.numVertexAttributeTypes;	// match attribute quantities
		
				/* DO POINTS */

		ind[0] = triMeshData.triangles[t].pointIndices[0];								// get indecies of 3 points
		ind[1] = triMeshData.triangles[t].pointIndices[1];			
		ind[2] = triMeshData.triangles[t].pointIndices[2];
				
		gParticles[i].points[0] = triMeshData.points[ind[0]];							// get coords of 3 points
		gParticles[i].points[1] = triMeshData.points[ind[1]];			
		gParticles[i].points[2] = triMeshData.points[ind[2]];
		

		Q3Point3D_Transform(&gParticles[i].points[0],&gWorkMatrix,&gParticles[i].points[0]);		// transform points
		Q3Point3D_Transform(&gParticles[i].points[1],&gWorkMatrix,&gParticles[i].points[1]);					
		Q3Point3D_Transform(&gParticles[i].points[2],&gWorkMatrix,&gParticles[i].points[2]);
	
		centerPt.x = (gParticles[i].points[0].x + gParticles[i].points[1].x + gParticles[i].points[2].x) * 0.3333f;		// calc center of polygon
		centerPt.y = (gParticles[i].points[0].y + gParticles[i].points[1].y + gParticles[i].points[2].y) * 0.3333f;				
		centerPt.z = (gParticles[i].points[0].z + gParticles[i].points[1].z + gParticles[i].points[2].z) * 0.3333f;				
		gParticles[i].points[0].x -= centerPt.x;											// offset coords to be around center
		gParticles[i].points[0].y -= centerPt.y;
		gParticles[i].points[0].z -= centerPt.z;
		gParticles[i].points[1].x -= centerPt.x;											// offset coords to be around center
		gParticles[i].points[1].y -= centerPt.y;
		gParticles[i].points[1].z -= centerPt.z;
		gParticles[i].points[2].x -= centerPt.x;											// offset coords to be around center
		gParticles[i].points[2].y -= centerPt.y;
		gParticles[i].points[2].z -= centerPt.z;
		
#if 1
		// Source port addition: Quesa won't render the mesh if its bounding box is empty
		Q3BoundingBox_SetFromPoints3D(&gParticles[i].triMesh.bBox, gParticles[i].points, 3, sizeof(TQ3Point3D));
#endif
		
				/* DO VERTEX NORMALS */
				
		if (triMeshData.vertexAttributeTypes[0].attributeType != kQ3AttributeTypeNormal)
			DoFatalAlert("Bleep2!");
				
		normalPtr = (TQ3Vector3D*)triMeshData.vertexAttributeTypes[0].data;			// assume vert attrib #0 == vertex normals
		vertNormals[0] = normalPtr[ind[0]];								// get vertex normals
		vertNormals[1] = normalPtr[ind[1]];
		vertNormals[2] = normalPtr[ind[2]];
		
		Q3Vector3D_Transform(&vertNormals[0],&gWorkMatrix,&vertNormals[0]);		// transform normals
		Q3Vector3D_Transform(&vertNormals[1],&gWorkMatrix,&vertNormals[1]);		// transform normals
		Q3Vector3D_Transform(&vertNormals[2],&gWorkMatrix,&vertNormals[2]);		// transform normals
		Q3Vector3D_Normalize(&vertNormals[0],&gParticles[i].vertNormals[0]);	// normalize normals & place in structure
		Q3Vector3D_Normalize(&vertNormals[1],&gParticles[i].vertNormals[1]);
		Q3Vector3D_Normalize(&vertNormals[2],&gParticles[i].vertNormals[2]);


				/* DO VERTEX UV'S */
					
		if (triMeshData.numVertexAttributeTypes > 1)					// see if also has UV (assumed to be attrib #1)
		{
			if (triMeshData.vertexAttributeTypes[1].attributeType != kQ3AttributeTypeShadingUV)
				DoFatalAlert("Bleep3!");
		
			uvPtr = (TQ3Param2D*)triMeshData.vertexAttributeTypes[1].data;	
			gParticles[i].uvs[0] = uvPtr[ind[0]];									// get vertex u/v's
			gParticles[i].uvs[1] = uvPtr[ind[1]];								
			gParticles[i].uvs[2] = uvPtr[ind[2]];								
		}


			/*********************/
			/* SET PHYSICS STUFF */
			/*********************/

		gParticles[i].coord = centerPt;
		gParticles[i].rot.x = gParticles[i].rot.y = gParticles[i].rot.z = 0;
		gParticles[i].scale = 1.0;
		
		gParticles[i].coordDelta.x = (RandomFloat() - 0.5) * gBoomForce;
		gParticles[i].coordDelta.y = (RandomFloat() - 0.5) * gBoomForce;
		gParticles[i].coordDelta.z = (RandomFloat() - 0.5) * gBoomForce;
		if (gParticleMode & PARTICLE_MODE_UPTHRUST)
			gParticles[i].coordDelta.y += 1.5 * gBoomForce;
		
		gParticles[i].rotDelta.x = (RandomFloat() - 0.5) * 4;			// random rotation deltas
		gParticles[i].rotDelta.y = (RandomFloat() - 0.5) * 4;
		gParticles[i].rotDelta.z = (RandomFloat() - 0.5) * 4;
		
		gParticles[i].decaySpeed = gParticleDecaySpeed;
		gParticles[i].mode = gParticleMode;

				/* SET VALID & INC COUNTER */
				
		gParticles[i].isUsed = true;
		gNumParticles++;
	}

	if (theTriMesh)
		Q3TriMesh_EmptyData(&triMeshData);
}


/************************** QD3D: MOVE PARTICLES ****************************/

void QD3D_MoveParticles(void)
{
float	ty,y,fps,x,z;
long	i;
TQ3Matrix4x4	matrix,matrix2;

	if (gNumParticles == 0)												// quick check if any particles at all
		return;

	fps = gFramesPerSecondFrac;

	for (i=0; i < MAX_PARTICLES; i++)
	{
		if (!gParticles[i].isUsed)										// source port fix
			continue;

				/* ROTATE IT */

		gParticles[i].rot.x += gParticles[i].rotDelta.x * fps;
		gParticles[i].rot.y += gParticles[i].rotDelta.y * fps;
		gParticles[i].rot.z += gParticles[i].rotDelta.z * fps;
					
					/* MOVE IT */
					
		if (gParticles[i].mode & PARTICLE_MODE_HEAVYGRAVITY)
			gParticles[i].coordDelta.y -= fps * GRAVITY_CONSTANT/2;		// gravity
		else
			gParticles[i].coordDelta.y -= fps * GRAVITY_CONSTANT/3;		// gravity
			
		x = (gParticles[i].coord.x += gParticles[i].coordDelta.x * fps);	
		y = (gParticles[i].coord.y += gParticles[i].coordDelta.y * fps);	
		z = (gParticles[i].coord.z += gParticles[i].coordDelta.z * fps);	
		
		
					/* SEE IF BOUNCE */
					
		ty = GetTerrainHeightAtCoord_Quick(x,z);				// get terrain height here
		if (y <= ty)
		{
			if (gParticles[i].mode & PARTICLE_MODE_BOUNCE)
			{
				gParticles[i].coord.y  = ty;
				gParticles[i].coordDelta.y *= -0.5;
				gParticles[i].coordDelta.x *= 0.9;
				gParticles[i].coordDelta.z *= 0.9;
			}
			else
				goto del;
		}
		
					/* SCALE IT */
					
		gParticles[i].scale -= gParticles[i].decaySpeed * fps;
		if (gParticles[i].scale <= 0.0f)
		{
				/* DEACTIVATE THIS PARTICLE */
	del:	
			gParticles[i].isUsed = false;
			gNumParticles--;
			continue;
		}

			/***************************/
			/* UPDATE TRANSFORM MATRIX */
			/***************************/
			

				/* SET SCALE MATRIX */

		Q3Matrix4x4_SetScale(&gParticles[i].matrix, gParticles[i].scale,	gParticles[i].scale, gParticles[i].scale);
	
					/* NOW ROTATE IT */

		Q3Matrix4x4_SetRotate_XYZ(&matrix, gParticles[i].rot.x, gParticles[i].rot.y, gParticles[i].rot.z);
		Q3Matrix4x4_Multiply(&gParticles[i].matrix,&matrix, &matrix2);
	
					/* NOW TRANSLATE IT */

		Q3Matrix4x4_SetTranslate(&matrix, gParticles[i].coord.x, gParticles[i].coord.y, gParticles[i].coord.z);
		Q3Matrix4x4_Multiply(&matrix2,&matrix, &gParticles[i].matrix);
	}
}


/************************* QD3D: DRAW PARTICLES ****************************/

void QD3D_DrawParticles(QD3DSetupOutputType *setupInfo)
{
long	i;
TQ3ViewObject	view = setupInfo->viewObject;

	if (gNumParticles == 0)												// quick check if any particles at all
		return;

	Q3Push_Submit(view);												// save this state

	Q3Object_Submit(gKeepBackfaceStyleObject,view);						// draw particles both backfaces

	for (i=0; i < MAX_PARTICLES; i++)
	{
		if (gParticles[i].isUsed)
		{
			Q3MatrixTransform_Submit(&gParticles[i].matrix, view);		// submit matrix
			Q3TriMesh_Submit(&gParticles[i].triMesh, view);				// submit geometry
			Q3ResetTransform_Submit(view);								// reset matrix
		}
	}
	
	// Source port fix: this used to be Q3Push_Submit, which I think is a mistake, even though it seems to work either way (???)
	Q3Pop_Submit(view);													// restore state
	
}



//============================================================================================
//============================================================================================
//============================================================================================






/****************** QD3D: SCROLL UVs ***********************/
//
// Given any object as input this will scroll any u/v coordinates by the given amount
//

void QD3D_ScrollUVs(TQ3Object theObject, float du, float dv)
{
	Q3Matrix3x3_SetTranslate(&gUVTransformMatrix, du, dv);		// make the matrix

	Q3Matrix4x4_SetIdentity(&gWorkMatrix);						// init to identity matrix
	ScrollUVs_Recurse(theObject);	
}


/****************** SCROLL UVs - RECURSE ***********************/

static void ScrollUVs_Recurse(TQ3Object obj)
{
TQ3GroupPosition	position;
TQ3Object   		object;
TQ3ObjectType		oType;
TQ3Matrix4x4		transform;
TQ3Matrix4x4  		stashMatrix;

				/*******************************/
				/* SEE IF ACCUMULATE TRANSFORM */
				/*******************************/
				
	if (Q3Object_IsType(obj,kQ3ShapeTypeTransform))
	{
  		Q3Transform_GetMatrix(obj,&transform);
  		Q3Matrix4x4_Multiply(&transform,&gWorkMatrix,&gWorkMatrix);
 	}
	else
				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		switch(oType)
		{
					/* MUST BE TRIMESH */
					
			case	kQ3GeometryTypeTriMesh:
					ScrollUVs_TriMesh(obj);
					break;
		}
	}
	else

				/***********************/
				/* SEE IF FOUND SHADER */
				/***********************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeShader))
	{	
		if (Q3Shader_GetType(obj) == kQ3ShaderTypeSurface)				// must be texture surface shader
		{
			Q3Shader_SetUVTransform(obj, &gUVTransformMatrix);
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
  		stashMatrix = gWorkMatrix;										// push matrix
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			ScrollUVs_Recurse(object);								// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
  		gWorkMatrix = stashMatrix;										// pop matrix  		
	}
}





/********************** SCROLL UVS: TRIMESH *******************************/

static void ScrollUVs_TriMesh(TQ3Object theTriMesh)
{
TQ3TriMeshData		triMeshData;
TQ3SurfaceShaderObject	shader;

	Q3TriMesh_GetData(theTriMesh,&triMeshData);							// get trimesh data	
	
	
			/* SEE IF HAS A TEXTURE */
			
	if (triMeshData.triMeshAttributeSet)
	{
		if (Q3AttributeSet_Contains(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader))
		{
			Q3AttributeSet_Get(triMeshData.triMeshAttributeSet, kQ3AttributeTypeSurfaceShader, &shader);
			Q3Shader_SetUVTransform(shader, &gUVTransformMatrix);
			Q3Object_Dispose(shader);
		}
	}
	Q3TriMesh_EmptyData(&triMeshData);
}



//============================================================================================
//============================================================================================
//============================================================================================


/****************** QD3D: REPLACE GEOMETRY TEXTURE ***********************/
//
// This is a self-recursive routine, so be careful.
//

void QD3D_ReplaceGeometryTexture(TQ3Object obj, TQ3SurfaceShaderObject theShader)
{
TQ3GroupPosition	position;
TQ3Object   		object;
TQ3ObjectType		oType;
TQ3TriMeshData		triMeshData;

				/*************************/
				/* SEE IF FOUND GEOMETRY */
				/*************************/

	if (Q3Object_IsType(obj,kQ3ShapeTypeGeometry))
	{
		oType = Q3Geometry_GetType(obj);									// get geometry type
		switch(oType)
		{
					/* MUST BE TRIMESH */
					
			case	kQ3GeometryTypeTriMesh:
					Q3TriMesh_GetData(obj,&triMeshData);					// get trimesh data	
					
					if (triMeshData.triMeshAttributeSet)					// see if has attribs
					{
						if (Q3AttributeSet_Contains(triMeshData.triMeshAttributeSet,
							 kQ3AttributeTypeSurfaceShader))
						{
							Q3AttributeSet_Add(triMeshData.triMeshAttributeSet,
											 kQ3AttributeTypeSurfaceShader, &theShader);					
							Q3TriMesh_SetData(obj,&triMeshData);
						}
					}
					Q3TriMesh_EmptyData(&triMeshData);
					break;
		}
	}
	else
	
			/* SEE IF RECURSE SUB-GROUP */

	if (Q3Object_IsType(obj,kQ3ShapeTypeGroup))
 	{
  		for (Q3Group_GetFirstPosition(obj, &position); position != nil;
  			 Q3Group_GetNextPosition(obj, &position))					// scan all objects in group
 		{
   			Q3Group_GetPositionObject (obj, position, &object);			// get object from group
			if (object != NULL)
   			{
    			QD3D_ReplaceGeometryTexture(object,theShader);			// sub-recurse this object
    			Q3Object_Dispose(object);								// dispose local ref
   			}
  		}
	}
}


/**************************** QD3D: DUPLICATE TRIMESH DATA *******************************/
//
// This is a specialized Copy routine only for use with the Skeleton TriMeshes.  Not all
// data is duplicated and many references are kept the same.  ** dont use for anything else!!
//
// ***NOTE:  Since this is not a legitimate TriMesh created via trimesh calls, DO NOT
//			 use the EmptyData call to free this memory.  Instead, call QD3D_FreeDuplicateTriMeshData!!
//

void QD3D_DuplicateTriMeshData(TQ3TriMeshData *inData, TQ3TriMeshData *outData)
{
UInt32	numPoints,numVertexAttributeTypes;
UInt32	i;

			/* COPY BASE STUFF */
			
	*outData = *inData;							// first do a carbon copy
	outData->numEdges = 0;						// don't copy edge info
	outData->edges = nil;
	outData->numEdgeAttributeTypes = 0;
	outData->edgeAttributeTypes = nil;


			/* GET VARS */
		
	numPoints = inData->numPoints;										// get # points
	numVertexAttributeTypes = inData->numVertexAttributeTypes;			// get # vert attrib types

			/********************/
			/* ALLOC NEW ARRAYS */
			/********************/
						 		
							/* ALLOC POINT ARRAY */
								
	outData->points = (TQ3Point3D *)AllocPtr(sizeof(TQ3Point3D) * numPoints);								// alloc new point array
	if (outData->points == nil)
		DoFatalAlert("QD3D_DuplicateTriMeshData: no mem for points");

					
				/* ALLOC NEW ATTIRBUTE DATA BASE STRUCTURE */
				
	outData->vertexAttributeTypes = (TQ3TriMeshAttributeData *)AllocPtr(sizeof(TQ3TriMeshAttributeData) *
									 numVertexAttributeTypes);												// alloc enough for attrib(s)
									 
	for (i=0; i < numVertexAttributeTypes; i++)
		outData->vertexAttributeTypes[i] = inData->vertexAttributeTypes[i];									// quick-copy contents	
	
	
				/* DO VERTEX NORMAL ATTRIB ARRAY */
		
	outData->vertexAttributeTypes[0].data = (void *)AllocPtr(sizeof(TQ3Vector3D) * numPoints);				// set new data array
	if (outData->vertexAttributeTypes[0].data == nil)
		DoFatalAlert("QD3D_DuplicateTriMeshData: no mem for vert normal attribs");


	if (numVertexAttributeTypes > 1)
	{
					/* DO VERTEX UV ATTRIB ARRAY */
		
		outData->vertexAttributeTypes[1].data = (void *)AllocPtr(sizeof(TQ3Param2D) * numPoints);			// set new data array
		if (outData->vertexAttributeTypes[1].data == nil)
			DoFatalAlert("QD3D_DuplicateTriMeshData: no mem for vert UV attribs");

		BlockMove(inData->vertexAttributeTypes[1].data, outData->vertexAttributeTypes[1].data,
				 sizeof(TQ3Param2D) * numPoints);															// copy uv values into new array
	}
}



/********************** QD3D: FREE DUPLICATE TRIMESH DATA *****************************/
//
// Called only to free memory allocated by above function.
//

void QD3D_FreeDuplicateTriMeshData(TQ3TriMeshData *inData)
{
	DisposePtr((Ptr)inData->points);
	inData->points = nil;

	DisposePtr((Ptr)inData->vertexAttributeTypes[0].data);
	inData->vertexAttributeTypes[0].data = nil;

	if (inData->numVertexAttributeTypes > 1)
	{
		DisposePtr((Ptr)inData->vertexAttributeTypes[1].data);
		inData->vertexAttributeTypes[1].data = nil;	
	}

	DisposePtr((Ptr)inData->vertexAttributeTypes);
	inData->vertexAttributeTypes = nil;
	
}

        







