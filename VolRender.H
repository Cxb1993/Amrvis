// -------------------------------------------------------------------
// VolRender.H
// -------------------------------------------------------------------
#ifndef _VOLRENDER_H_
#define _VOLRENDER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream.h>

#include "aString.H" 
#include "Array.H" 
#include "REAL.H" 
#include "Box.H" 
#include "AmrvisConstants.H" 

#ifdef BL_VOLUMERENDER
#include <volpack.h>
#endif

class DataServices;

class VolRender {
  public:
	VolRender();
	VolRender(const Array<Box> &drawdomain,
		  int mindrawnlevel, int maxdrawnlevel);

	~VolRender();

  private:
	unsigned int	nPoints, bPoints;
	int		minDrawnLevel, maxDataLevel;
	Array<Box>      drawnDomain;
	char		buffer[BUFSIZ];
	Real 		scale[3], aspect, lenRatio, vpLen;
	int		nDenRampPts, nGradRampPts, nShadeRampPts;
	int		maxDenRampPts, maxGradRampPts, maxShadeRampPts;
	bool		bVolRenderDefined;


  private:
	vpContext      *vpc;
	vpResult	vpret;
	float *density_ramp;    // opacity as a function of density
        float *gradient_ramp;   // opacity as a function of grad magnitude
        float *shade_table;     // shading lookup table
	int             rows, cols, planes;
	unsigned char  *swfData;
	int             swfDataSize;
	bool		swfDataAllocated, vpDataValid, swfDataValid, vpCreated;
	Real		xrotang, yrotang, zrotang;
	int		classifyFields, shadeFields;
	int		*densityRampX, *gradientRampX;
	float		*densityRampY, *gradientRampY;
	float		minRayOpacity, maxRayOpacity;

  public:
	vpContext      *GetVPContext()             { return vpc; }
	void	        MakeVPData();
	void            MakeSWFData(DataServices *dataServicesPtr,
				    Real rDataMin, Real rDataMax,
				    const aString &derivedName,
				    int iPaletteStart, int iPaletteEnd,
				    int iBlackIndex, int iWhiteIndex,
				    int iColorSlots);
	void            WriteSWFData(const aString &filenamebase);
	unsigned char  *GetSWFData()               { return swfData; }
	int             GetSWFDataSize()   const   { return swfDataSize; }
	bool      	VPDataValid()      const   { return vpDataValid; }
	void      	InvalidateVPData();
	bool      	SWFDataValid()     const   { return swfDataValid; }
	void      	InvalidateSWFData();
	bool      	SWFDataAllocated() const   { return swfDataAllocated; }
	void     	ReadTransferFile(const aString &rampFileName);
	int             NDenRampPts()  { return nDenRampPts;  }
	int            *DensityRampX() { return densityRampX; }
	float          *DensityRampY() { return densityRampY; }

  private:
	bool     	AllocateSWFData();
	void            MakeSWFDataOneProc(DataServices *dataServicesPtr,
				    Real rDataMin, Real rDataMax,
				    const aString &derivedName,
				    int iPaletteStart, int iPaletteEnd,
				    int iBlackIndex, int iWhiteIndex,
				    int iColorSlots);
	void            MakeSWFDataNProcs(DataServices *dataServicesPtr,
				    Real rDataMin, Real rDataMax,
				    const aString &derivedName,
				    int iPaletteStart, int iPaletteEnd,
				    int iBlackIndex, int iWhiteIndex,
				    int iColorSlots);
};

#endif