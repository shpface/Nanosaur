//
// qd3d_geometry.h
//

#include "qd3d_support.h"


enum
{
	PARTICLE_MODE_BOUNCE = (1),
	PARTICLE_MODE_UPTHRUST = (1<<1),
	PARTICLE_MODE_HEAVYGRAVITY = (1<<2)
};


#define	MAX_PARTICLES		150

typedef struct
{
	Boolean					isUsed;
	TQ3Vector3D				rot,rotDelta;
	TQ3Point3D				coord,coordDelta;
	float					decaySpeed,scale;
	Byte					mode;
	TQ3Matrix4x4			matrix;
	
	TQ3TriMeshData			triMesh;
	TQ3Point3D				points[3];
	TQ3TriMeshTriangleData	triangle;
	TQ3Param2D				uvs[3];
	TQ3Vector3D				vertNormals[3],faceNormal;
	TQ3TriMeshAttributeData	vertAttribs[2];

}ParticleType;

extern	float QD3D_CalcObjectRadius(TQ3Object theObject);
extern	void QD3D_CalcObjectBoundingBox(QD3DSetupOutputType *setupInfo, TQ3Object theObject, TQ3BoundingBox	*boundingBox);
extern	void QD3D_ExplodeGeometry(ObjNode *theNode, float boomForce, Byte particleMode, long particleDensity, float particleDecaySpeed);
extern	void QD3D_ReplaceGeometryTexture(TQ3Object obj, TQ3SurfaceShaderObject theShader);
extern	void QD3D_ScrollUVs(TQ3Object theObject, float du, float dv);
extern	void QD3D_DuplicateTriMeshData(TQ3TriMeshData *inData, TQ3TriMeshData *outData);
extern	void QD3D_FreeDuplicateTriMeshData(TQ3TriMeshData *inData);
extern	void QD3D_InitParticles(void);
extern	void QD3D_MoveParticles(void);
extern	void QD3D_DrawParticles(QD3DSetupOutputType *setupInfo);


