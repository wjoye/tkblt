static char *EVREGIONS_C="\n/*\n    NB: MAKE SURE YOU EDIT THE TEMPLATE FILE!!!!\n*/\n\n#ifndef FILTER_PTYPE\n#include <regions.h>\n#endif\n\n\n#ifndef UNUSED\n#ifdef __GNUC__\n#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))\n#else\n#  define UNUSED(x) UNUSED_ ## x\n#endif\n#endif\n\n\n#define USE_ASTRO_ANGLE 0\n\n\n#define USE_FPU_DOUBLE 0\n#if USE_FPU_DOUBLE\n#include <fpu_control.h>\nstatic fpu_control_t _cw;\n#define FPU_DOUBLE {fpu_control_t _cw2; _FPU_GETCW(_cw); _cw2 = _cw & ~_FPU_EXTENDED; _cw2 |= _FPU_DOUBLE; _FPU_SETCW(_cw2);}\n#define FPU_RESTORE {_FPU_SETCW(_cw);}\n#else\n#define FPU_DOUBLE\n#define FPU_RESTORE\n#endif\n\n\n#define USE_FLOAT_COMPARE 0\n\n\nstatic int evregno=0;\nvoid initevregions(void)\n{\n  evregno++;\n  return;\n}\n\nstatic int polypt(double x, double y, double* poly, int count,\n		  double UNUSED(xstart), double ystart, int flag)\n{\n  \n  \n  \n  \n\n  \n  \n  \n  \n  \n  \n  \n\n  \n  int crossings = 0;\n\n  \n  int sign = ((poly[1] - y)>=0) ? 1 : -1;\n\n  \n  int i;\n\n  \n  if( flag && (x == poly[0]) && (y == poly[1]) ) return 1;\n\n  for(i=0; i<count; i++) {\n    int j = (i!=(count-1)) ? i+1 : 0;\n\n    \n    double x1 = poly[i*2] - x;\n    double y1 = poly[i*2+1] - y;\n\n    \n    double x2 = poly[j*2] - x;\n    double y2 = poly[j*2+1] - y;\n\n    \n    int nextSign = (y2>=0) ? 1 : -1;\n\n    \n    if( (y1==0) && (y2==0) ){\n      if( ((x2>=0) && (x1<=0)) || ((x1>=0) && (x2<=0)) ){\n	\n	if( y == ystart )\n	  return 1;\n	else\n	  return fmod((double)crossings+1,2.0) ? 1 : 0;\n      }\n    }\n    \n    else if( (x1==0) && (x2==0) ){\n      if( ((y2>=0) && (y1<=0)) || ((y1>=0) && (y2<=0)) ){\n	return fmod((double)crossings+1,2.0) ? 1 : 0;\n      }\n    }\n    \n    else if( feq((y1*(x2-x1)),(x1*(y2-y1))) ){\n      if( (((x2>=0) && (x1<=0)) || ((x1>=0) && (x2<=0))) &&\n	  (((y2>=0) && (y1<=0)) || ((y1>=0) && (y2<=0))) ){\n	return fmod((double)crossings+1,2.0) ? 1 : 0;\n      }\n    }\n#if 0\n    \n    if( (y1==0) && (y2==0) ){\n      if( ((x2>=0) && (x1<=0)) || ((x1>=0) && (x2<=0)) ){\n	if( y == ystart ){\n	  return 1;\n	}\n      }\n    }\n    \n    else if( (x1==0) && (x2==0) ){\n      if( ((y2>=0) && (y1<=0)) || ((y1>=0) && (y2<=0)) ){\n	if( x == xstart ){\n	  return 1;\n	}\n      }\n    }\n    \n    else if( feq((y1*(x2-x1)),(x1*(y2-y1))) ){\n      if( (((x2>=0) && (x1<=0)) || ((x1>=0) && (x2<=0))) &&\n	  (((y2>=0) && (y1<=0)) || ((y1>=0) && (y2<=0))) ){\n	return 0;\n      }\n    }\n#endif\n    if (sign != nextSign) {\n      if (x1>0 && x2>0)\n	crossings++;\n      else if (x1>0 || x2>0) {\n	if (x1-(y1*(x2-x1)/(y2-y1)) > 0)\n	  crossings++;\n      }\n      sign = nextSign;\n    }\n  }\n\n  return crossings%2 ? 1 : 0;          \n}\n\nstatic void quadeq(double a, double b, double c,\n	    double *x1, double *x2, int *nr, int *nc)\n{\n  double dis, q;\n  if( feq(a,0.0) ){\n    *nc = 0;\n    if( feq(b,0.0) ){\n      *nr = 0; *x1 = 0.0;\n    }\n    else{\n      *nr = 1; *x1 = -c / b;\n    }\n    *x2 = *x1;\n  }\n  else{\n    dis = b*b - 4.0 * a * c;\n    if( dis > 0.0 ){\n      *nr = 2; *nc = 0;\n      dis = sqrt(dis);\n      if( b < 0.0 ) dis = -dis;\n      q = -0.5 * (b + dis);\n      *x1 = q/a; *x2 = c/q;\n      if(*x1 > *x2){\n	q = *x1; *x1 = *x2; *x2 = q;\n      }\n    } \n    else if( feq(dis,0.0) ){\n      *nr = 1; *nc = 0; *x1 = - 0.5 * b / a; *x2 = *x1;\n    }\n    else{\n      *nr = 0; *nc = 2; *x1 = - 0.5 * b / a; *x2 = 0.5 * sqrt(-dis) / a;\n    }\n  }\n}\n\nstatic int corner_vertex(int index, int width, int height,\n			 double *x, double *y)\n{\n  switch (index) {\n  case 1:\n    *x = 0.0;\n    *y = height + 1;\n    break;\n  case 2:\n    *x = 0.0;\n    *y = 0.0;\n    break;\n  case 3:\n    *x = width + 1;\n    *y = 0.0;\n    break;\n  case 4:\n    *x = width + 1;\n    *y = height + 1;\n  default:\n    break;\n  }\n  index = index + 1;\n  if(index > 4) index = 1;\n  return(index);\n}\n\nstatic int pie_intercept(double width, double height, double xcen, double ycen,\n			 double angle, double *xcept, double *ycept)\n{\n  double angl, slope;	\n  angl = angle;\n  \n  while (angl < 0.0)\n    angl = angl + 360.0;\n  while (angl >= 360.0)\n    angl = angl - 360.0;\n  \n#if USE_ASTRO_ANGLE\n  if(fabs(angl - 90.0) < SMALL_NUMBER) {\n#else\n  if(fabs(angl - 180.0) < SMALL_NUMBER) {\n#endif\n    *xcept = 0.0;\n    *ycept = ycen;\n    return(2);\n  }\n#if USE_ASTRO_ANGLE\n  if(fabs(angl - 270.0) < SMALL_NUMBER) {\n#else\n  if(fabs(angl - 0.0) < SMALL_NUMBER) {\n#endif\n    *xcept = width + 1;\n    *ycept = ycen;\n    return(4);\n  }\n  \n#if USE_ASTRO_ANGLE\n  angl = angl + 90.0;\n#endif\n  if(angl >= 360.0)\n    angl = angl - 360.0;\n  if(angl < 180.0) {\n    *ycept = height + 1;\n    \n    if(fabs(angl - 90.0) < SMALL_NUMBER) {\n      *xcept = xcen;\n      return(1);\n    }\n  } else {\n    *ycept = 0.0;\n    \n    if(fabs(angl - 270.0) < SMALL_NUMBER) {\n      *xcept = xcen;\n      return(3);\n    }\n  }\n  \n  angl = (angl / 180.0) * M_PI;\n  \n  slope = tan(angl);\n  \n  *xcept = xcen + ((*ycept - ycen) / slope);\n  if(*xcept < 0) {\n    *ycept = (ycen - (xcen * slope));\n    *xcept = 0.0;\n    return(2);\n  } else if(*xcept > (width + 1)) {\n    *ycept = (ycen + ((width + 1 - xcen) * slope));\n    *xcept = width + 1;\n    return(4);\n  } else {\n    if(*ycept < height)\n      return(3);\n    else\n      return(1);\n  }\n}\n\n\n\nint evannulus(GFilt g, int rno, int sno, int flag, int type,\n	      double x, double y,\n	      double xcen, double ycen, double ri, double ro)\n{\n  \n  if( ri == 0 ){\n    return(evcircle(g, rno, sno, flag, type, x, y, xcen, ycen, ro));\n  }\n\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    g->shapes[sno].ystart = ycen - ro;\n    g->shapes[sno].ystop = ycen + ro;\n    g->shapes[sno].r1sq = ri * ri;\n    g->shapes[sno].r2sq = ro * ro;\n  }\n\n  if((((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))	            &&\n     (((xcen-x)*(xcen-x))+((ycen-y)*(ycen-y))<=g->shapes[sno].r2sq)	    &&\n     (((xcen-x)*(xcen-x))+((ycen-y)*(ycen-y))>g->shapes[sno].r1sq)) == flag ){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else{\n    return 0;\n  }\n}\n\nint evbox(GFilt g, int rno, int sno, int flag, int UNUSED(type),\n	  double x, double y,\n	  double xcen, double ycen, double xwidth, double yheight,\n	  double angle)\n{\n  int i;\n  double angl;			 \n  double half_width, half_height;\n  double cosangl, sinangl;	 \n  double hw_cos, hw_sin;	 \n  double hh_cos, hh_sin;	 \n  double xstart=0.0;\n\n  if( (xwidth == 0) && (yheight==0) ){\n    return(!flag);\n  }\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n#if USE_ASTRO_ANGLE\n    \n    angl = angle + 90.0;\n#else\n    angl = angle;\n#endif\n    while (angl >= 360.0) angl = angl - 360.0;\n    \n    angl = (angl / 180.0) * M_PI;\n    sinangl = sin (angl);\n    cosangl = cos (angl);\n#if USE_ASTRO_ANGLE\n    \n    \n    \n    half_width = yheight / 2.0;\n    half_height = xwidth / 2.0;\n#else\n    half_width = xwidth / 2.0;\n    half_height = yheight / 2.0;\n#endif\n    hw_cos = half_width * cosangl;\n    hw_sin = half_width * sinangl;\n    hh_cos = half_height * cosangl;\n    hh_sin = half_height * sinangl;\n    g->shapes[sno].pts = (double *)calloc(8, sizeof(double));\n#if USE_ASTRO_ANGLE\n    g->shapes[sno].pts[0] = xcen - hw_cos - hh_sin;\n    g->shapes[sno].pts[1] = ycen - hw_sin + hh_cos;\n    g->shapes[sno].pts[2] = xcen + hw_cos - hh_sin;\n    g->shapes[sno].pts[3] = ycen + hw_sin + hh_cos;\n    g->shapes[sno].pts[4] = xcen + hw_cos + hh_sin;\n    g->shapes[sno].pts[5] = ycen + hw_sin - hh_cos;\n    g->shapes[sno].pts[6] = xcen - hw_cos + hh_sin;\n    g->shapes[sno].pts[7] = ycen - hw_sin - hh_cos;\n#else\n    g->shapes[sno].pts[0] = xcen - hw_cos + hh_sin;\n    g->shapes[sno].pts[1] = ycen - hh_cos - hw_sin;\n    g->shapes[sno].pts[2] = xcen - hw_cos - hh_sin;\n    g->shapes[sno].pts[3] = ycen + hh_cos - hw_sin;\n    g->shapes[sno].pts[4] = xcen + hw_cos - hh_sin;\n    g->shapes[sno].pts[5] = ycen + hh_cos + hw_sin;\n    g->shapes[sno].pts[6] = xcen + hw_cos + hh_sin;\n    g->shapes[sno].pts[7] = ycen - hh_cos + hw_sin;\n#endif\n    g->shapes[sno].npt = 8;\n    \n    if( g->shapes[sno].npt ){\n      xstart = g->shapes[sno].pts[0];\n      g->shapes[sno].ystart = g->shapes[sno].pts[1];\n      g->shapes[sno].ystop = g->shapes[sno].ystart;\n      for(i=1; i<g->shapes[sno].npt; i+=2){\n	if(g->shapes[sno].pts[i-1] < xstart)\n	  xstart = g->shapes[sno].pts[i-1];\n	if(g->shapes[sno].pts[i] > g->shapes[sno].ystop)\n	  g->shapes[sno].ystop = g->shapes[sno].pts[i];\n	if(g->shapes[sno].pts[i] < g->shapes[sno].ystart)\n	  g->shapes[sno].ystart = g->shapes[sno].pts[i];\n      }\n    }\n  }\n  if( (((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))	&&\n       polypt(x, y, g->shapes[sno].pts, g->shapes[sno].npt/2,\n	      xstart, g->shapes[sno].ystart, 0)) == flag		){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\nint evcircle(GFilt g, int rno, int sno, int flag, int UNUSED(type),\n	     double x, double y,\n	     double xcen, double ycen, double radius)\n{\n  if( radius == 0 ){\n    return(!flag);\n  }\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    g->shapes[sno].ystart = ycen - radius;\n    g->shapes[sno].ystop = ycen + radius;\n    g->shapes[sno].r1sq = radius * radius;\n  }\n  if( (((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))		    &&\n      (((xcen-x)*(xcen-x))+((ycen-y)*(ycen-y))<=g->shapes[sno].r1sq)) == flag  ){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\nint evellipse(GFilt g, int rno, int sno, int flag, int type,\n	      double x, double y,\n	      double xcen, double ycen, double xrad, double yrad, double angle)\n{\n  double yhi, yoff;\n  double b, c;\n  double b_partial, c_partial;\n  double xboff, xfoff;\n  int nr, nc;\n\n  \n  if( xrad == yrad ){\n    return(evcircle(g, rno, sno, flag, type, x, y, xcen, ycen, xrad));\n  }\n\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    \n#if USE_ASTRO_ANGLE\n    \n    g->shapes[sno].angl = angle + 90.0;\n#else\n    g->shapes[sno].angl = angle;\n#endif\n    while( g->shapes[sno].angl >= 360.0 )\n      g->shapes[sno].angl = g->shapes[sno].angl - 360.0;\n    \n    g->shapes[sno].angl = (g->shapes[sno].angl / 180.0) * M_PI;\n    g->shapes[sno].sinangl = sin(g->shapes[sno].angl);\n    g->shapes[sno].cosangl = cos(g->shapes[sno].angl);\n    \n    \n#if USE_ASTRO_ANGLE\n    yhi = fabs(g->shapes[sno].sinangl * yrad) + \n          fabs(g->shapes[sno].cosangl * xrad);\n#else\n    yhi = fabs(g->shapes[sno].sinangl * xrad) + \n          fabs(g->shapes[sno].cosangl * yrad);\n#endif\n    yhi = min(yhi, max(yrad, xrad));\n    g->shapes[sno].ystart = ycen - yhi;\n    g->shapes[sno].ystop = ycen + yhi;\n    \n    g->shapes[sno].cossq = g->shapes[sno].cosangl * g->shapes[sno].cosangl;\n    g->shapes[sno].sinsq = g->shapes[sno].sinangl * g->shapes[sno].sinangl;\n#if USE_ASTRO_ANGLE\n    \n    \n    \n    \n    g->shapes[sno].xradsq = yrad * yrad;\n    g->shapes[sno].yradsq = xrad * xrad;\n#else\n    g->shapes[sno].xradsq = xrad * xrad;\n    g->shapes[sno].yradsq = yrad * yrad;\n#endif\n    \n    g->shapes[sno].a = (g->shapes[sno].cossq / g->shapes[sno].xradsq) + \n       		       (g->shapes[sno].sinsq / g->shapes[sno].yradsq);\n  }\n  if( ((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop)) ){\n    b_partial = (2.0 * g->shapes[sno].sinangl) * \n                ((g->shapes[sno].cosangl / g->shapes[sno].xradsq) - \n                (g->shapes[sno].cosangl / g->shapes[sno].yradsq));\n    c_partial = (g->shapes[sno].sinsq / g->shapes[sno].xradsq) + \n                (g->shapes[sno].cossq / g->shapes[sno].yradsq);\n    yoff = y - ycen;\n    b = b_partial * yoff;\n    c = (c_partial * yoff * yoff) - 1.0;\n    \n    quadeq (g->shapes[sno].a, b, c, &xboff, &xfoff, &nr, &nc);\n    \n    if( nr != 0 ) {\n      FPU_DOUBLE\n#if USE_FLOAT_COMPARE\n      if( (((float)x>=(float)(xcen+xboff)) && \n	   ((float)x<=(float)(xcen+xfoff))) == flag ){\n#else\n      if( ((x>=(xcen+xboff)) && (x<=(xcen+xfoff))) == flag ){\n#endif\n	if( rno && flag ) g->rid = rno;\n	FPU_RESTORE\n	return 1;\n      }\n      else{\n	FPU_RESTORE\n	return 0;\n      }\n    }\n    else\n      return !flag;\n  }\n  return !flag;\n}\n\nint evfield(GFilt g, int rno, int UNUSED(sno), int flag, int UNUSED(type),\n	    double UNUSED(x), double UNUSED(y))\n{\n  if( flag ){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\nint evline(GFilt g, int rno, int sno, int flag, int UNUSED(type),\n	   double x, double y,\n	   double x1, double y1, double x2, double y2)\n{\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    g->shapes[sno].ystart = min(y1,y2);\n    g->shapes[sno].ystop = max(y1,y2);\n    g->shapes[sno].x1 = x1;\n    g->shapes[sno].x2 = x2;\n    g->shapes[sno].y1 = y1;\n    if( feq(y1,y2) ){\n      g->shapes[sno].xonly = 1;\n      g->shapes[sno].invslope = 0;\n    }\n    else{\n      g->shapes[sno].xonly = 0;\n      g->shapes[sno].invslope = (x1 - x2) / (y1 - y2);\n    }\n  }\n  if( (((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))		 &&\n      ((!g->shapes[sno].xonly &&\n       feq((((y-g->shapes[sno].y1)*g->shapes[sno].invslope)+g->shapes[sno].x1),x)) ||\n      (g->shapes[sno].xonly &&\n       ((x>=g->shapes[sno].x1)&&(x<=g->shapes[sno].x2))))) == flag){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\nint evpie(GFilt g, int rno, int sno, int flag, int type,\n	  double x, double y,\n	  double xcen, double ycen, double angle1, double angle2)\n{\n  int i;\n  int width, height;		\n  double sweep;			\n  int intrcpt1, intrcpt2;	\n  double x2, y2;		\n  double xstart=0.0;\n\n  \n  if( (angle1==0) && (angle2==360) ){\n    return(evfield(g, rno, sno, flag, type, x, y));\n  }\n\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    \n    width = LARGE_NUMBER;\n    height = LARGE_NUMBER;\n    \n    g->shapes[sno].pts = (double *)calloc(14, sizeof(double));\n    g->shapes[sno].pts[0] = xcen;\n    g->shapes[sno].pts[1] = ycen;\n    sweep = angle2 - angle1;\n    \n    if(fabs(sweep) < SMALL_NUMBER)\n      return !flag;\n    if (sweep < 0.0) sweep = sweep + 360.0;\n    intrcpt1 = pie_intercept((double)width, (double)height, xcen, ycen, angle1,\n			     &(g->shapes[sno].pts[2]),\n			     &(g->shapes[sno].pts[3]));\n    intrcpt2 = pie_intercept((double)width, (double)height, xcen, ycen, angle2,\n			     &x2, &y2);\n    g->shapes[sno].npt = 4;\n    \n    \n    if((intrcpt1 != intrcpt2) || (sweep > 180.0)){\n      do{\n	intrcpt1 = corner_vertex(intrcpt1, width, height,  \n				 &(g->shapes[sno].pts[g->shapes[sno].npt]),\n				 &(g->shapes[sno].pts[g->shapes[sno].npt+1]));\n	g->shapes[sno].npt = g->shapes[sno].npt + 2;\n      }while(intrcpt1 != intrcpt2);\n    }\n    g->shapes[sno].pts[g->shapes[sno].npt] = x2;\n    g->shapes[sno].pts[g->shapes[sno].npt+1] = y2;\n    g->shapes[sno].npt = g->shapes[sno].npt + 2;\n    \n    if( g->shapes[sno].npt ){\n      xstart = g->shapes[sno].pts[0];\n      g->shapes[sno].ystart = g->shapes[sno].pts[1];\n      g->shapes[sno].ystop = g->shapes[sno].ystart;\n      for(i=1; i<g->shapes[sno].npt; i+=2){\n	if(g->shapes[sno].pts[i-1] < xstart)\n	  xstart = g->shapes[sno].pts[i-1];\n	if(g->shapes[sno].pts[i] > g->shapes[sno].ystop)\n	  g->shapes[sno].ystop = g->shapes[sno].pts[i];\n	if(g->shapes[sno].pts[i] < g->shapes[sno].ystart)\n	  g->shapes[sno].ystart = g->shapes[sno].pts[i];\n      }\n    }\n  }\n  if( (((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))	&&\n       polypt(x, y, g->shapes[sno].pts, g->shapes[sno].npt/2,\n	      xstart, g->shapes[sno].ystart, 1)) == flag		){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else{\n    return 0;\n  }\n}\n\nint evqtpie(GFilt g, int rno, int sno, int flag, int type,\n	    double x, double y,\n	    double xcen, double ycen, double angle1, double angle2)\n{\n  return evpie(g, rno, sno, flag, type, x, y, xcen, ycen, angle1, angle2);\n}\n\nint evpoint(GFilt g, int rno, int UNUSED(sno), int flag, int UNUSED(type),\n	    double x, double y,\n	    double xcen, double ycen)\n{\n  if( ((x==xcen) && (y==ycen)) == flag ){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\n#ifdef __STDC__\nint\nevpolygon(GFilt g, int rno, int sno, int flag, int UNUSED(type),\n	  double x, double y, ...)\n{\n  int i, maxpts;\n  double xstart=0.0;\n  va_list args;\n  va_start(args, y);\n#else\nint evpolygon(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double xstart=0.0;\n  int i, maxpts;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n#endif\n  if( !g->shapes[sno].init ){\n    g->shapes[sno].init = 1;\n    \n    maxpts = MASKINC;\n    g->shapes[sno].pts = (double *)calloc(maxpts, sizeof(double));\n    \n    g->shapes[sno].npt = 0;\n    while( 1 ){\n      if( g->shapes[sno].npt >= maxpts ){\n	maxpts += MASKINC;\n	g->shapes[sno].pts = (double *)realloc(g->shapes[sno].pts,\n					    maxpts*sizeof(double));\n      }\n      g->shapes[sno].pts[g->shapes[sno].npt] = va_arg(args, double);\n      \n      if( feq(g->shapes[sno].pts[g->shapes[sno].npt],PSTOP)	&&\n	  feq(g->shapes[sno].pts[g->shapes[sno].npt-1],PSTOP) ){\n	g->shapes[sno].npt--;\n	break;\n      }\n      g->shapes[sno].npt++;\n    }\n    va_end(args);\n    \n    g->shapes[sno].pts = (double *)realloc(g->shapes[sno].pts,\n					g->shapes[sno].npt*sizeof(double));\n    \n    if( g->shapes[sno].npt ){\n      xstart = g->shapes[sno].pts[0];\n      g->shapes[sno].ystart = g->shapes[sno].pts[1];\n      g->shapes[sno].ystop = g->shapes[sno].ystart;\n      for(i=1; i<g->shapes[sno].npt; i+=2){\n	if(g->shapes[sno].pts[i-1] < xstart)\n	  xstart = g->shapes[sno].pts[i-1];\n	if(g->shapes[sno].pts[i] > g->shapes[sno].ystop)\n	  g->shapes[sno].ystop = g->shapes[sno].pts[i];\n	if(g->shapes[sno].pts[i] < g->shapes[sno].ystart)\n	  g->shapes[sno].ystart = g->shapes[sno].pts[i];\n      }\n    }\n  }\n  if( (((y>=g->shapes[sno].ystart) && (y<=g->shapes[sno].ystop))	&&\n       polypt(x, y, g->shapes[sno].pts, g->shapes[sno].npt/2,\n	      xstart, g->shapes[sno].ystart, 0)) == flag		){\n    if( rno && flag ) g->rid = rno;\n    return 1;\n  }\n  else\n    return 0;\n}\n\n\n\nint evnannulus(GFilt g, int rno, int sno, int flag, int type,\n	     double x, double y, double xcen, double ycen,\n	     double lo, double hi, int n)\n{\n  int i;\n  int xsno;\n  double dinc;\n\n  \n  dinc = (hi - lo)/(double)n;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evannulus(g, 0, xsno, flag, type, x, y, xcen, ycen, lo, hi) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evannulus(g, rno+i, sno+i, flag, type, x, y,\n		    xcen, ycen, lo+(i*dinc), lo+((i+1)*dinc)) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  else{\n    \n    if( !evannulus(g, 0, xsno, 1, type, x, y, xcen, ycen, lo, hi) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\nint evnbox(GFilt g, int rno, int sno, int flag, int type,\n	   double x, double y, double xcen, double ycen,\n	   double lox, double loy, double hix, double hiy, int n,\n	   double ang)\n{\n  int i;\n  int xsno;\n  double dincx;\n  double dincy;\n\n  \n  dincx = (hix - lox)/n;\n  dincy = (hiy - loy)/n;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evbox(g, 0, xsno, flag, type, x, y, xcen, ycen, hix, hiy, ang) ){\n      return(0);\n    }\n    \n    if( evbox(g, 0, xsno+1, flag, type, x, y, xcen, ycen, lox, loy, ang) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evbox(g, rno+i, sno+i, flag, type, x, y,\n		xcen, ycen, lox+((i+1)*dincx), loy+((i+1)*dincy), ang) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evbox(g, 0, xsno, 1, type, x, y, xcen, ycen, hix, hiy, ang) ){\n      return(1);\n    }\n    \n    if( evbox(g, 0, xsno+1, 1, type, x, y, xcen, ycen, lox, loy, ang) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\nint evnellipse(GFilt g, int rno, int sno, int flag, int type,\n	       double x, double y, double xcen, double ycen,\n	       double lox, double loy, double hix, double hiy, int n,\n	       double ang)\n{\n  int i;\n  int xsno;\n  double dincx;\n  double dincy;\n\n  \n  dincx = (hix - lox)/n;\n  dincy = (hiy - loy)/n;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evellipse(g, 0, xsno, flag, type, x, y, xcen, ycen, hix, hiy, ang) ){\n      return(0);\n    }\n    \n    if( evellipse(g, 0, xsno+1, flag, type, x, y, xcen, ycen, lox, loy, ang) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evellipse(g, rno+i, sno+i, flag, type, x, y,\n		    xcen, ycen, lox+((i+1)*dincx), loy+((i+1)*dincy), ang) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evellipse(g, 0, xsno, 1, type, x, y, xcen, ycen, hix, hiy, ang) ){\n      return(1);\n    }\n    \n    if( evellipse(g, 0, xsno+1, 1, type, x, y, xcen, ycen, lox, loy, ang) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\nint evnpie(GFilt g, int rno, int sno, int flag, int type,\n	 double x, double y, double xcen, double ycen,\n	 double lo, double hi, int n)\n{\n  int i;\n  int xsno;\n  double dinc;\n\n  \n  while( lo > hi ) lo -= 360.0;\n  dinc = (hi - lo)/n;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evpie(g, 0, xsno, flag, type, x, y, xcen, ycen, lo, hi) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evpie(g, rno+i, sno+i, flag, type, x, y,\n		xcen, ycen, lo+(i*dinc), lo+((i+1)*dinc)) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  else{\n    \n    if( !evpie(g, 0, xsno, 1, type, x, y, xcen, ycen, lo, hi) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\nint evpanda(GFilt g, int rno, int sno, int flag, int type,\n	    double x, double y,\n	    double xcen, double ycen,\n	    double anglo, double anghi, double angn,\n	    double radlo, double radhi, double radn)\n{\n    \n  int a, r;\n  int ahi, rhi;\n  int xsno;\n  int n=0;\n  double ainc, rinc;\n\n  \n  ainc = (anghi - anglo)/angn;\n  ahi = (int)angn;\n  rinc = (radhi - radlo)/radn;\n  rhi = (int)radn;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evannulus(g, 0, xsno, flag, type, x, y, xcen, ycen, radlo, radhi) ||\n	!evpie(g, 0, xsno+1, flag, type, x, y, xcen, ycen, anglo, anghi)   ){\n      return(0);\n    }\n    \n    for(a=1; a<=ahi; a++){\n      for(r=1; r<=rhi; r++){\n	if( evannulus(g, rno+n, sno+(2*n), flag, type, x, y,\n		      xcen, ycen, radlo+((r-1)*rinc), radlo+(r*rinc)) &&\n	    evpie(g, rno+n, sno+(2*n+1), flag, type, x, y,\n		  xcen, ycen, anglo+((a-1)*ainc), anglo+(a*ainc))     ){\n	  return(1);\n	}\n	n++;\n      }\n    }\n    return(0);\n  }\n  else{\n    \n    if( !evannulus(g, 0, xsno, 1, type, x, y, xcen, ycen, radlo, radhi) )\n      return(1);\n    else if( !evpie(g, 0, xsno+1, 1, type, x, y, xcen, ycen, anglo, anghi) ){\n      return(1);\n    }\n    else{\n      return(0);\n    }\n  }\n}\n\nint evbpanda(GFilt g, int rno, int sno, int flag, int type,\n	     double x, double y,\n	     double xcen, double ycen,\n	     double anglo, double anghi, double angn,\n	     double xlo, double ylo, double xhi, double yhi, double radn,\n	     double ang)\n{\n    \n  int a, r;\n  int ahi, rhi;\n  int xsno;\n  int n=0;\n  double ainc, xinc, yinc;\n\n  \n  anglo += ang;\n  anghi += ang;\n  ainc = (anghi - anglo)/angn;\n  ahi = (int)angn;\n  xinc = (xhi - xlo)/radn;\n  yinc = (yhi - ylo)/radn;\n  rhi = (int)radn;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evbox(g, 0, xsno, flag, type, x, y, xcen, ycen, xhi, yhi,\n	       ang) ){\n      return(0);\n    }\n    \n    else if( evbox(g, 0, xsno+2, flag, type, x, y, xcen, ycen, xlo, ylo,\n		   ang) ){\n      return(0);\n    }\n    \n    else if( !evpie(g, 0, xsno+1, flag, type, x, y, xcen, ycen, anglo, anghi)){\n      return(0);\n    }\n    \n    for(a=0; a<ahi; a++){\n      for(r=1; r<=rhi; r++){\n	if( evbox(g, rno+n, sno+(2*n), flag, type, x, y,\n		  xcen, ycen, xlo+(r*xinc), ylo+(r*yinc), ang)   &&\n	    evqtpie(g, rno+n, sno+(2*n+1), flag, type, x, y,\n		    xcen, ycen, anglo+(a*ainc), anglo+((a+1)*ainc))  ){\n	  return(1);\n	}\n	n++;\n      }\n    }\n    return(0);\n  }\n  else{\n    \n    if( !evbox(g, 0, xsno, 1, type, x, y, xcen, ycen, xhi, yhi, ang) )\n      return(1);\n    \n    else if( !evbox(g, 0, xsno+2, 1, type, x, y, xcen, ycen, xlo, ylo,\n		    ang) )\n      return(1);\n    \n    else if( !evpie(g, 0, xsno+1, 1, type, x, y, xcen, ycen, anglo, anghi) ){\n      return(1);\n    }\n    else{\n      return(0);\n    }\n  }\n}\n\nint evepanda(GFilt g, int rno, int sno, int flag, int type,\n	     double x, double y,\n	     double xcen, double ycen,\n	     double anglo, double anghi, double angn,\n	     double xlo, double ylo, double xhi, double yhi, double radn,\n	     double ang)\n{\n    \n  int a, r;\n  int ahi, rhi;\n  int xsno;\n  int n=0;\n  double ainc, xinc, yinc;\n\n  \n  anglo += ang;\n  anghi += ang;\n  ainc = (anghi - anglo)/angn;\n  ahi = (int)angn;\n  xinc = (xhi - xlo)/radn;\n  yinc = (yhi - ylo)/radn;\n  rhi = (int)radn;\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( flag ){\n    \n    if( !evellipse(g, 0, xsno, flag, type, x, y, xcen, ycen, xhi, yhi,\n		   ang) ){\n      return(0);\n    }\n    \n    else if( evellipse(g, 0, xsno+2, flag, type, x, y, xcen, ycen, xlo, ylo,\n		       ang) ){\n      return(0);\n    }\n    \n    else if( !evpie(g, 0, xsno+1, flag, type, x, y, xcen, ycen, anglo, anghi)){\n      return(0);\n    }\n    \n    for(a=0; a<ahi; a++){\n      for(r=1; r<=rhi; r++){\n	if( evellipse(g, rno+n, sno+(2*n), flag, type, x, y,\n		      xcen, ycen, xlo+(r*xinc), ylo+(r*yinc), ang) &&\n	    evqtpie(g, rno+n, sno+(2*n+1), flag, type, x, y,\n		    xcen, ycen, anglo+(a*ainc), anglo+((a+1)*ainc))  ){\n	  return(1);\n	}\n	n++;\n      }\n    }\n    return(0);\n  }\n  else{\n    \n    if( !evellipse(g, 0, xsno, 1, type, x, y, xcen, ycen, xhi, yhi, ang) )\n      return(1);\n    \n    else if( !evellipse(g, 0, xsno+2, 1, type, x, y, xcen, ycen, xlo, ylo,\n			ang) )\n      return(1);\n    \n    else if( !evpie(g, 0, xsno+1, 1, type, x, y, xcen, ycen, anglo, anghi) ){\n      return(1);\n    }\n    else{\n      return(0);\n    }\n  }\n}\n\n\n\n#ifdef __STDC__\nint\nevvannulus(GFilt g, int rno, int sno, int flag, int type,\n	   double x, double y, double xcen, double ycen, ...)\n{\n  int i, n;\n  int maxpts;\n  int xsno;\n  double *xv;\n  va_list args;\n  va_start(args, ycen);\n#else\nint evvannulus(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double xcen, ycen;\n  double *xv;\n  int i, n;\n  int maxpts;\n  int xsno;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n  xcen  = va_arg(args, double);\n  ycen  = va_arg(args, double);\n#endif\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( !g->shapes[xsno].xv ){\n    maxpts = MASKINC;\n    g->shapes[xsno].xv = (double *)calloc(maxpts, sizeof(double));\n    g->shapes[xsno].nv = 0;\n    while( 1 ){\n      if( g->shapes[xsno].nv >= maxpts ){\n        maxpts += MASKINC;\n        g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  					      maxpts*sizeof(double));\n      }\n      g->shapes[xsno].xv[g->shapes[xsno].nv] = va_arg(args, double);\n      if( feq(g->shapes[xsno].xv[g->shapes[xsno].nv],PSTOP)   &&\n  	feq(g->shapes[xsno].xv[g->shapes[xsno].nv-1],PSTOP) ){\n        g->shapes[xsno].nv--;\n        break;\n      }\n      g->shapes[xsno].nv++;\n    }\n    va_end(args);\n    g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  	  	        g->shapes[xsno].nv*sizeof(double));\n  }\n  n = g->shapes[xsno].nv;\n  xv = g->shapes[xsno].xv;\n  \n  if( n == 2 ){\n    return(evannulus(g, rno, sno, flag, type, x, y, xcen, ycen, xv[0], xv[1]));\n  }\n  if( flag ){\n    \n    if( !evannulus(g, 0, xsno, flag, type, x, y, xcen, ycen, xv[0], xv[n-1]) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evannulus(g, rno+i, sno+i, flag, type, x, y, xcen, ycen,\n		  xv[i], xv[i+1]) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evannulus(g, 0, xsno, 1, type, x, y, xcen, ycen, xv[0], xv[n-1]) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\n#ifdef __STDC__\nint\nevvbox(GFilt g, int rno, int sno, int flag, int type,\n       double x, double y, double xcen, double ycen, ...)\n{\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  double ang;\n  double *xv;\n  va_list args;\n  va_start(args, ycen);\n#else\nint evvbox(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double xcen, ycen;\n  double ang;\n  double *xv;\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n  xcen  = va_arg(args, double);\n  ycen  = va_arg(args, double);\n#endif\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( !g->shapes[xsno].xv ){\n    maxpts = MASKINC;\n    g->shapes[xsno].xv = (double *)calloc(maxpts, sizeof(double));\n    g->shapes[xsno].nv = 0;\n    while( 1 ){\n      if( g->shapes[xsno].nv >= maxpts ){\n        maxpts += MASKINC;\n        g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  					      maxpts*sizeof(double));\n      }\n      g->shapes[xsno].xv[g->shapes[xsno].nv] = va_arg(args, double);\n      if( feq(g->shapes[xsno].xv[g->shapes[xsno].nv],PSTOP)   &&\n  	feq(g->shapes[xsno].xv[g->shapes[xsno].nv-1],PSTOP) ){\n        g->shapes[xsno].nv--;\n        break;\n      }\n      g->shapes[xsno].nv++;\n    }\n    va_end(args);\n    g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  	  	        g->shapes[xsno].nv*sizeof(double));\n  }\n  n = g->shapes[xsno].nv;\n  xv = g->shapes[xsno].xv;\n  ang = xv[--n];\n  \n  if( n == 2 ){\n    return(evbox(g, rno, sno, flag, type, x, y,\n		 xcen, ycen, xv[0], xv[1], ang));\n  }\n  if( flag ){\n    \n    if( !evbox(g, 0, xsno, flag, type, x, y, xcen, ycen, xv[n-2], xv[n-1],\n	       ang) ){\n      return(0);\n    }\n    \n    if( evbox(g, 0, xsno+1, flag, type, x, y, xcen, ycen, xv[0], xv[1], ang) ){\n      return(0);\n    }\n    \n    for(i=2, j=0; i<n; i+=2, j++){\n      if( evbox(g, rno+j, sno+j, flag, type, x, y, xcen, ycen,\n	      xv[i], xv[i+1], ang) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evbox(g, 0, xsno, 1, type, x, y, xcen, ycen, xv[n-2], xv[n-1], ang) ){\n      return(1);\n    }\n    \n    else if( evbox(g, 0, xsno+1, 1, type, x, y, xcen, ycen, xv[0], xv[1], ang) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\n\n#ifdef __STDC__\nint\nevvellipse(GFilt g, int rno, int sno, int flag, int type,\n	   double x, double y, double xcen, double ycen, ...)\n{\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  double ang;\n  double *xv;\n  va_list args;\n  va_start(args, ycen);\n#else\nint evvellipse(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double xcen, ycen;\n  double ang;\n  double *xv;\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n  xcen  = va_arg(args, double);\n  ycen  = va_arg(args, double);\n#endif\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( !g->shapes[xsno].xv ){\n    maxpts = MASKINC;\n    g->shapes[xsno].xv = (double *)calloc(maxpts, sizeof(double));\n    g->shapes[xsno].nv = 0;\n    while( 1 ){\n      if( g->shapes[xsno].nv >= maxpts ){\n        maxpts += MASKINC;\n        g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  					      maxpts*sizeof(double));\n      }\n      g->shapes[xsno].xv[g->shapes[xsno].nv] = va_arg(args, double);\n      if( feq(g->shapes[xsno].xv[g->shapes[xsno].nv],PSTOP)   &&\n  	feq(g->shapes[xsno].xv[g->shapes[xsno].nv-1],PSTOP) ){\n        g->shapes[xsno].nv--;\n        break;\n      }\n      g->shapes[xsno].nv++;\n    }\n    va_end(args);\n    g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  	  	        g->shapes[xsno].nv*sizeof(double));\n  }\n  n = g->shapes[xsno].nv;\n  xv = g->shapes[xsno].xv;\n  ang = xv[--n];\n  \n  if( n == 2 ){\n    return(evellipse(g, rno, sno, flag, type, x, y,\n		     xcen, ycen, xv[0], xv[1], ang));\n  }\n  if( flag ){\n    \n    if( !evellipse(g, 0, xsno, flag, type, x, y, xcen, ycen, xv[n-2], xv[n-1],\n		   ang) ){\n      return(0);\n    }\n    \n    if( evellipse(g, 0, xsno+1, flag, type, x, y, xcen, ycen, xv[0], xv[1],\n		  ang) ){\n      return(0);\n    }\n    \n    for(i=2, j=0; i<n; i+=2, j++){\n      if( evellipse(g, rno+j, sno+j, flag, type, x, y, xcen, ycen,\n		  xv[i], xv[i+1], ang) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evellipse(g, 0, xsno, 1, type, x, y, xcen, ycen, xv[n-2], xv[n-1],\n		   ang) ){\n      return(1);\n    }\n    \n    if( evellipse(g, 0, xsno+1, 1, type, x, y, xcen, ycen, xv[0], xv[1], ang) ){\n      return(1);\n    }\n    return(0);\n  }\n}\n\n#ifdef __STDC__\nint\nevvpie(GFilt g, int rno, int sno, int flag, int type,\n       double x, double y, double xcen, double ycen, ...)\n{\n  int i, n;\n  int maxpts;\n  int xsno;\n  double *xv;\n  va_list args;\n  va_start(args, ycen);\n#else\nint evvpie(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double xcen, ycen;\n  double *xv;\n  int i, n;\n  int maxpts;\n  int xsno;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n  xcen  = va_arg(args, double);\n  ycen  = va_arg(args, double);\n#endif\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( !g->shapes[xsno].xv ){\n    maxpts = MASKINC;\n    g->shapes[xsno].xv = (double *)calloc(maxpts, sizeof(double));\n    g->shapes[xsno].nv = 0;\n    while( 1 ){\n      if( g->shapes[xsno].nv >= maxpts ){\n        maxpts += MASKINC;\n        g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  					      maxpts*sizeof(double));\n      }\n      g->shapes[xsno].xv[g->shapes[xsno].nv] = va_arg(args, double);\n      if( feq(g->shapes[xsno].xv[g->shapes[xsno].nv],PSTOP)   &&\n  	feq(g->shapes[xsno].xv[g->shapes[xsno].nv-1],PSTOP) ){\n        g->shapes[xsno].nv--;\n        break;\n      }\n      g->shapes[xsno].nv++;\n    }\n    va_end(args);\n    g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  	  	        g->shapes[xsno].nv*sizeof(double));\n  }\n  n = g->shapes[xsno].nv;\n  xv = g->shapes[xsno].xv;\n  \n  if( n == 2 ){\n    return(evpie(g, rno, sno, flag, type, x, y,\n		 xcen, ycen, xv[0], xv[1]));\n  }\n  if( flag ){\n    \n    if( !evpie(g, 0, xsno, flag, type, x, y, xcen, ycen, xv[0], xv[n-1]) ){\n      return(0);\n    }\n    \n    for(i=0; i<n; i++){\n      if( evpie(g, rno+i, sno+i, flag, type, x, y, xcen, ycen, xv[i], xv[i+1]) ){\n	return(1);\n      }\n    }\n    return(0);\n  }\n  \n  else{\n    \n    if( !evpie(g, 0, xsno, 1, type, x, y, xcen, ycen, xv[0], xv[n-1]) ){\n      return(1);\n    }\n    return(1);\n  }\n}\n\n#ifdef __STDC__\nint\nevvpoint(GFilt g, int rno, int sno, int flag, int type,\n	 double x, double y, ...)\n{\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  double *xv;\n  va_list args;\n  va_start(args, y);\n#else\nint evvpoint(va_alist) va_dcl\n{\n  GFilt g;\n  int rno, sno, flag, type;\n  double x, y;\n  double *xv;\n  int i, j, n;\n  int maxpts;\n  int xsno;\n  va_list args;\n  va_start(args);\n  g  = va_arg(args, GFilt);\n  rno  = va_arg(args, int);\n  sno  = va_arg(args, int);\n  flag  = va_arg(args, int);\n  type  = va_arg(args, int);\n  x  = va_arg(args, double);\n  y  = va_arg(args, double);\n#endif\n  xsno = (g->nshapes+1)+((sno-1)*XSNO);\n  if( !g->shapes[xsno].xv ){\n    maxpts = MASKINC;\n    g->shapes[xsno].xv = (double *)calloc(maxpts, sizeof(double));\n    g->shapes[xsno].nv = 0;\n    while( 1 ){\n      if( g->shapes[xsno].nv >= maxpts ){\n        maxpts += MASKINC;\n        g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  					      maxpts*sizeof(double));\n      }\n      g->shapes[xsno].xv[g->shapes[xsno].nv] = va_arg(args, double);\n      if( feq(g->shapes[xsno].xv[g->shapes[xsno].nv],PSTOP)   &&\n  	feq(g->shapes[xsno].xv[g->shapes[xsno].nv-1],PSTOP) ){\n        g->shapes[xsno].nv--;\n        break;\n      }\n      g->shapes[xsno].nv++;\n    }\n    va_end(args);\n    g->shapes[xsno].xv = (double *)realloc(g->shapes[xsno].xv,\n  	  	        g->shapes[xsno].nv*sizeof(double));\n  }\n  n = g->shapes[xsno].nv;\n  xv = g->shapes[xsno].xv;\n  \n  for(i=0, j=0; i<n; i+=2, j++){\n    if( evpoint(g, rno+j, sno+j, flag, type, x, y, xv[i], xv[i+1]) ){\n      return(1);\n    }\n  }\n  return(0);\n}\n\n\n";
