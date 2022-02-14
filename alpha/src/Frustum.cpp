

#include "Frustum.h"


Frustum::Frustum()
{
	

}

Frustum::~Frustum()
{
	
}

void Frustum::update()
{
	glGetFloatv(GL_PROJECTION_MATRIX, projection);
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
	glPushMatrix();
	glLoadMatrixf(projection);
	glMultMatrixf(modelview);
	glGetFloatv(GL_MODELVIEW_MATRIX, mvp);
	glPopMatrix();

	float a, b, c, d;
	a = mvp[3] + mvp[2];
	b = mvp[7] + mvp[6];
	c = mvp[11] + mvp[10];
	d = mvp[15] + mvp[14];
	planes[0].setAndNormalize(a, b, c, d); //near

	a = mvp[3] - mvp[2];
	b = mvp[7] - mvp[6];
	c = mvp[11] - mvp[10];
	d = mvp[15] - mvp[14];
	planes[1].setAndNormalize(a, b, c, d); //far

	a = mvp[3] + mvp[0];
	b = mvp[7] + mvp[4];
	c = mvp[11] + mvp[8];
	d = mvp[15] + mvp[12];
	planes[2].setAndNormalize(a, b, c, d); //left

	a = mvp[3] - mvp[0];
	b = mvp[7] - mvp[4];
	c = mvp[11] - mvp[8];
	d = mvp[15] - mvp[12];
	planes[3].setAndNormalize(a, b, c, d); //right

	a = mvp[3] + mvp[1];
	b = mvp[7] + mvp[5];
	c = mvp[11] + mvp[9];
	d = mvp[15] + mvp[13];
	planes[4].setAndNormalize(a, b, c, d); // bottom

	a = mvp[3] - mvp[1];
	b = mvp[7] - mvp[5];
	c = mvp[11] - mvp[9];
	d = mvp[15] - mvp[13];
	planes[5].setAndNormalize(a, b, c, d); // top
}


bool Frustum::testPoint(float px, float py, float pz){
	for (int i = 0; i<6; i++){
		if (planes[i].a*px + planes[i].b*py + planes[i].c*pz + planes[i].d < 0){
			return false;
		}
	}
	return true;
}

bool Frustum::testGeom(float * points, int num){
	int out,in;

	// for each plane do ...
	for(int i=0; i < 6; i++) {
		// reset counters for corners in and out
		out=0;in=0;
		// for each corner of the box do ...
		// get out of the cycle as soon as a box as corners
		// both inside and out of the frustum
		for (int k = 0; k < num && (in==0 || out==0); k++) {
			// is the corner outside or inside
			if (planes[i].a*points[3 * k] + planes[i].b*points[3 * k + 1] + planes[i].c* points[3 * k + 2] + planes[i].d < 0)
				out++;
			else
				in++;
		}
		//if all corners are out
		if (!in)
			return false;
	}
	return true;
}
	
