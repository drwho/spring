#include "WeaponDef.h"

#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Rendering/UnitModels/IModelParser.h"

WeaponDef::~WeaponDef()
{
	delete explosionGenerator; explosionGenerator = 0;
}


S3DModel* WeaponDef::LoadModel()
{
	if ((visuals.model==NULL) && (!visuals.modelName.empty())) {
		std::string modelname = string("objects3d/") + visuals.modelName;
		if (modelname.find(".") == std::string::npos) {
			modelname += ".3do";
		}
		visuals.model = modelParser->Load3DModel(modelname);
	}
	return visuals.model;
}
