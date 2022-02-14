#ifndef FRUSTUM_H_
#define FRUSTUM_H_

#include <GL/glut.h>
#include <math.h>

struct Plane{
	float a;
	float b;
	float c;
	float d;
	void setAndNormalize(float newA, float newB, float newC, float newD){
		float t = sqrt(newA*newA + newB*newB + newC*newC + newD*newD);
		a = newA / t;
		b = newB / t;
		c = newC / t;
		d = newD / t;
	}
};


class Frustum
{
public:
	Frustum();
	~Frustum();

	void update();
	float* getMVP(){return &mvp[0];}
	bool testPoint(float px, float py, float pz);
    bool testGeom(float * points, int num);
private:
	float projection[16];
	GLfloat modelview[16];
	GLfloat mvp[16];

	Plane planes[6];
};

#endif
