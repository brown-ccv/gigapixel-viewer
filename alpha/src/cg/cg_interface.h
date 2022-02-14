#ifndef CG_INTERFACE_H
#define CG_INTERFACE_H

extern int CgGLInit();
extern void CgBindProgramEnableTextureChangeParameter(float gamma, float gmax, float gmin, float alpha, int texid);
extern void CgDisableTextureAndProfile();

#endif
