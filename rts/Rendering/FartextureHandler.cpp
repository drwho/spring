#include "StdAfx.h"
#include "mmgr.h"

#include "FartextureHandler.h"
#include "GlobalUnsynced.h"
#include "UnitModels/UnitDrawer.h"
#include "Textures/Bitmap.h"
#include "Rendering/UnitModels/3DModel.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include "Map/MapInfo.h"

CFartextureHandler* fartextureHandler = NULL;


CFartextureHandler::CFartextureHandler()
{
	usedFarTextures = 0;

	// 1 unit: 8 orientations, each image is 16x16 pixels, each pixel has RGBA (4) values
	static const size_t farTextureMem_size = 8*16*16*4;
	farTextureMem = new unsigned char[farTextureMem_size];

	for (size_t a=0; a < farTextureMem_size; ++a) {
		farTextureMem[a] = 0;
	}

	farTexture = 0;
}


CFartextureHandler::~CFartextureHandler()
{
	delete[] farTextureMem;
	glDeleteTextures (1, &farTexture);
}


/**
 * @brief Add the model to the queue of units waiting for their fartexture.
 * On the next CreateFarTextures() call the fartexture for this model will be
 * created.
 */
void CFartextureHandler::CreateFarTexture(S3DModel* model)
{
	GML_STDMUTEX_LOCK(tex); // CreateFarTexture

	pending.push_back(model);
}


/**
 * @brief Process the queue of pending fartexture creation requests.
 * This loops through the queue calling ReallyCreateFarTexture() on each entry,
 * and empties the queue afterwards.
 */
void CFartextureHandler::CreateFarTextures()
{
	GML_STDMUTEX_LOCK(tex); // CreateFarTextures

	for(std::vector<S3DModel*>::const_iterator it = pending.begin(); it != pending.end(); ++it) {
		ReallyCreateFarTexture(*it);
	}
	pending.clear();
}


/**
 * @brief Really create the far texture for the given model.
 */
void CFartextureHandler::ReallyCreateFarTexture(S3DModel* model)
{
	//UnitModelGeometry& geometry=*model.geometry;

	model->farTextureNum = usedFarTextures;

	if (usedFarTextures >= 511) {
//		logOutput.Print("Out of fartextures");
		return;
	}

	// setup lights
	const GLfloat LightDiffuseLand2[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	GLfloat LightAmbientLand2[]       = { 0.6f, 0.6f, 0.6f, 1.0f };
	for (size_t a=0; a < 3; ++a) {
		LightAmbientLand2[a] = std::min(1.0f, unitDrawer->unitAmbientColor[a]+unitDrawer->unitSunColor[a]*0.2f);
	}
	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbientLand2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuseLand2);
	glLightfv(GL_LIGHT1, GL_SPECULAR, LightAmbientLand2);

	// setup viewport
	glViewport(0, 0, 16, 16);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-model->radius, model->radius, -model->radius, model->radius, -model->radius*1.5f, model->radius*1.5f);
	glMatrixMode(GL_MODELVIEW);
	glClearColor(1.0f, 0.0f, 1.0f, 0.0f); // image transparency key
	glDisable(GL_BLEND);

	glEnable(GL_LIGHTING);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	glAlphaFunc(GL_GREATER, 0.05f);
	glEnable(GL_ALPHA_TEST);
	const float cols[]  = {1, 1, 1, 1};
	const float cols2[] = {1, 1, 1, 1};
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, cols);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, cols2);
	glColor3f(1, 1, 1);
	glRotatef(10, 1, 0, 0);
	glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);
	glEnable(GL_LIGHT1);

	// 1 orientation: each image is 16x16 pixels, each pixel has RGBA (4) values
	unsigned char imgBuf[16*16*4];
	// draw the model in 8 different orientations
	for (size_t o=0; o < 8; ++o) {
		// draw the model to a temporary buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		texturehandler3DO->Set3doAtlases();
		glPushMatrix();
		glTranslatef(0, -model->height*0.5f, 0);
		model->DrawStatic();
		glPopMatrix();
		// fetch pixels
		glReadPixels(0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, imgBuf);

		// store the pixels
		const size_t baseX = o * 16;
		for (size_t y=0; y < 16; ++y) {
			for (size_t x=0; x < 16; ++x) {
				const size_t pixel_ftx = (baseX + x + y*128) * 4;
				const size_t pixel_img = (x + y*16) * 4;
				farTextureMem[pixel_ftx]     = imgBuf[pixel_img];     // red
				farTextureMem[pixel_ftx + 1] = imgBuf[pixel_img + 1]; // green
				farTextureMem[pixel_ftx + 2] = imgBuf[pixel_img + 2]; // blue
				if ((imgBuf[pixel_img]     == 255) &&
				    (imgBuf[pixel_img + 1] == 0) &&
				    (imgBuf[pixel_img + 2] == 255)) { // #FF00FF -> pink
					farTextureMem[pixel_ftx + 3] = 0;                 // alpha
				} else {
					farTextureMem[pixel_ftx + 3] = 255;               // alpha
				}
			}
		}

		// rotate by 45 degrees for the next orientation
		glRotatef(45, 0, 1, 0);
		glLightfv(GL_LIGHT1, GL_POSITION, mapInfo->light.sunDir);
	}

	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	glDisable(GL_LIGHTING);
	glDisable(GL_TEXTURE_2D);

	glClearColor(mapInfo->atmosphere.fogColor[0], mapInfo->atmosphere.fogColor[1], mapInfo->atmosphere.fogColor[2], 0);
	glViewport(gu->viewPosX, 0, gu->viewSizeX, gu->viewSizeY);
	glLightfv(GL_LIGHT1, GL_AMBIENT,  mapInfo->light.unitAmbientColor);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  mapInfo->light.unitSunColor);
	glLightfv(GL_LIGHT1, GL_SPECULAR, mapInfo->light.unitAmbientColor);

	if (farTexture == 0) {
		glGenTextures(1, &farTexture);
		glBindTexture(GL_TEXTURE_2D, farTexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	}
	const int bx = (usedFarTextures % 8) * 128;
	const int by = (usedFarTextures / 8) * 16;
	glBindTexture(GL_TEXTURE_2D, farTexture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, bx, by, 128, 16, GL_RGBA, GL_UNSIGNED_BYTE, farTextureMem);

	usedFarTextures++;
}
