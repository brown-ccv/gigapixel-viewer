#include <Cg/cgGL.h>
#include <iostream>
#include <assert.h>

static CGcontext   cgContext;
static CGprogram   cgFragmentProgram;
static CGprofile   cgFragmentProfile;
static CGparameter color,baseTexture, _gamma, _gmin, _gmax, _alpha;
static int have_cg;



using namespace std;

/*  This initializes the nVidia CG context, and loads the fragment shader
    program(s) */
int CgGLInit(){ 
  cgContext = cgCreateContext();
 
  if (cgContext == NULL){
    CGerror Error = cgGetError();
    cerr << cgGetErrorString(Error) << endl;
    have_cg=0;
    return(0);
  }
 
  cgFragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
  if(cgFragmentProfile == CG_PROFILE_UNKNOWN) {
    have_cg=0;
    return(0);
  }
 
  cgGLSetOptimalOptions(cgFragmentProfile);
 
  char *filename = "./CG/CGContrastEnhancementF.cg";
  cgFragmentProgram = cgCreateProgramFromFile(cgContext,CG_SOURCE,
     filename,cgFragmentProfile,"main",0);
 
  if(cgFragmentProgram == NULL){
    CGerror Error = cgGetError();
    cerr<<  cgGetErrorString(Error) << endl;
    assert(0);
  }
 
  cgGLLoadProgram(cgFragmentProgram);
  color = cgGetNamedParameter(cgFragmentProgram, "color");
  assert(color != NULL);
  baseTexture = cgGetNamedParameter(cgFragmentProgram, "cameraTexture");
 
  if(baseTexture == NULL){
    CGerror Error = cgGetError();
    cerr<< cgGetErrorString(Error) << endl;
    assert (0);
  }
  //cgGLSetTextureParameter(baseTexture, texName);
  _gamma = cgGetNamedParameter(cgFragmentProgram, "gamma");
  assert(_gamma != NULL);
  cgGLSetParameter1f(_gamma,1.0); 
  _gmin = cgGetNamedParameter(cgFragmentProgram, "min");
  assert(_gmin != NULL);
  cgGLSetParameter1f(_gmin,0.0); 
  _gmax = cgGetNamedParameter(cgFragmentProgram, "max");
  assert(_gmax != NULL);
  cgGLSetParameter1f(_gmax,0.0); 
    _alpha = cgGetNamedParameter(cgFragmentProgram, "alpha");
  assert(_alpha != NULL);
  cgGLSetParameter1f(_alpha,1.0); 
  have_cg=1;
  return(1);
}

void CgBindProgramEnableTextureChangeParameter(float gamma, float gmax, float gmin, float alpha, int texid){
    //fprintf(stderr, "CG: Gamma: %f Gmax: %f, Gmin: %f Alpha: %f\n", gamma, gmax, gmin, alpha);
    if (!have_cg)
	return;
  
     cgGLEnableProfile(cgFragmentProfile);
    cgGLBindProgram(cgFragmentProgram);
    cgGLEnableTextureParameter(baseTexture);
    cgGLSetParameter1f(_gamma,gamma); 
    //   cgGLSetParameter1f(gmin,0.0); 
    //   cgGLSetParameter1f(gmax,1.0); 
    cgGLSetParameter1f(_gmin,gmin); 
    cgGLSetParameter1f(_gmax,gmax); 
    cgGLSetParameter1f(_alpha, alpha);
    cgGLSetTextureParameter(baseTexture, texid);

}

void CgDisableTextureAndProfile(){
    if (!have_cg)
	return;
    cgGLDisableTextureParameter(baseTexture);
    cgGLDisableProfile(cgFragmentProfile);
}


