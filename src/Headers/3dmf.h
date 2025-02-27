//
// 3dmf.h
//

#include "qd3d_support.h"


#define	MAX_3DMF_GROUPS			25	
#define	MAX_OBJECTS_IN_GROUP	100


//====================================

extern	void Init3DMFManager(void);
extern	TQ3Object	Load3DMFModel(FSSpec *inFile);
extern	void LoadGrouped3DMF(FSSpec *spec, Byte groupNum);
extern	void Free3DMFGroup(Byte groupNum);
extern	void DeleteAll3DMFGroups(void);


