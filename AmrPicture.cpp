// -------------------------------------------------------------------
// AmrPicture.C
// -------------------------------------------------------------------

#include "AmrPicture.H"
#include "PltApp.H"
#include "DataServices.H"
#include <time.h>

// ---------------------------------------------------------------------
AmrPicture::AmrPicture(int mindrawnlevel, GraphicsAttributes *gaptr,
		       PltApp *pltappptr, DataServices *dataservicesptr)
{
  int i, ilev;
  minDrawnLevel = mindrawnlevel;
  GAptr = gaptr;
  pltAppPtr = pltappptr;
  myView = XY;
  isSubDomain = false;
  findSubRange = false;

  dataServicesPtr = dataservicesptr;
  const AmrData &amrData = dataServicesPtr->AmrDataRef();

  if(UseSpecifiedMinMax()) {
    whichRange = USESPECIFIED;
    GetSpecifiedMinMax(dataMinSpecified, dataMaxSpecified);
  } else {
    whichRange = USEGLOBAL;
    dataMinSpecified = 0.0;
    dataMaxSpecified = 0.0;
  }
  showBoxes = PltApp::GetDefaultShowBoxes();

  finestLevel = amrData.FinestLevel();
  maxAllowableLevel =
          DetermineMaxAllowableLevel(amrData.ProbDomain()[finestLevel],
		        finestLevel, MaxPictureSize(), amrData.RefRatio());

  numberOfLevels    = maxAllowableLevel + 1;
  maxDrawnLevel     = maxAllowableLevel;
  maxLevelWithGrids = maxAllowableLevel;

  subDomain.resize(numberOfLevels);
  for(ilev = 0; ilev <= maxAllowableLevel; ilev++) {  // set type
    subDomain[ilev].convert(amrData.ProbDomain()[0].type());
  }

  subDomain[maxAllowableLevel].setSmall
		(amrData.ProbDomain()[maxAllowableLevel].smallEnd());
  subDomain[maxAllowableLevel].setBig
		(amrData.ProbDomain()[maxAllowableLevel].bigEnd());
  for(i = maxAllowableLevel - 1; i >= minDrawnLevel; i--) {
    subDomain[i] = subDomain[maxAllowableLevel];
    subDomain[i].coarsen(CRRBetweenLevels(i, maxAllowableLevel,
			 amrData.RefRatio()));
  }

  dataSizeH = subDomain[maxAllowableLevel].length(XDIR);
  dataSizeV = subDomain[maxAllowableLevel].length(YDIR); 
  dataSize  = dataSizeH * dataSizeV;	 // for a picture (slice).

  vLine = 0;
  hLine = subDomain[maxAllowableLevel].bigEnd(YDIR) * pltAppPtr->CurrentScale();
  subcutY = hLine;
  subcut2ndY = hLine;

#if (BL_SPACEDIM == 2)
  slice = 0;
#else
  slice = subDomain[maxAllowableLevel].smallEnd(YZ-myView);
#endif
  sliceBox.resize(numberOfLevels);

  for(ilev = minDrawnLevel; ilev <= maxAllowableLevel; ilev++) {  // set type
    sliceBox[ilev].convert(subDomain[minDrawnLevel].type());
  }

  sliceBox[maxAllowableLevel] = subDomain[maxAllowableLevel];

  AmrPictureInit();
}  // end AmrPicture constructor


// ---------------------------------------------------------------------
AmrPicture::AmrPicture(int view, int mindrawnlevel,
		GraphicsAttributes *gaptr, Box region,
		AmrPicture *parentPicturePtr, PltApp *pltappptr)
{
  int ilev;
  myView = view;
  minDrawnLevel = mindrawnlevel;
  GAptr = gaptr;
  pltAppPtr = pltappptr;
  isSubDomain = true;
  if(myView == XY) {
    findSubRange = true;
  } else {
    findSubRange = false;
  }
  dataServicesPtr = pltAppPtr->GetDataServicesPtr();
  const AmrData &amrData = dataServicesPtr->AmrDataRef();

  whichRange = parentPicturePtr->GetWhichRange();
  dataMinSpecified = parentPicturePtr->GetSpecifiedMin();
  dataMaxSpecified = parentPicturePtr->GetSpecifiedMax();
  showBoxes = parentPicturePtr->ShowingBoxes();

  finestLevel = amrData.FinestLevel();
  maxAllowableLevel = DetermineMaxAllowableLevel(region, finestLevel,
				   MaxPictureSize(), amrData.RefRatio());

  numberOfLevels = maxAllowableLevel + 1;
  maxDrawnLevel     = maxAllowableLevel;
  maxLevelWithGrids = maxAllowableLevel;

  subDomain.resize(numberOfLevels);
  // region is at the finestLevel
  subDomain[maxAllowableLevel] = region.coarsen(CRRBetweenLevels(maxAllowableLevel,
					     finestLevel, amrData.RefRatio()));

  for(ilev = maxAllowableLevel - 1; ilev >= minDrawnLevel; --ilev) {
    subDomain[ilev] = subDomain[maxAllowableLevel];
    subDomain[ilev].coarsen
	      (CRRBetweenLevels(ilev, maxAllowableLevel, amrData.RefRatio()));
  }

  if(myView==XZ) {
    dataSizeH = subDomain[maxAllowableLevel].length(XDIR);
    dataSizeV = subDomain[maxAllowableLevel].length(ZDIR);
  } else if(myView==YZ) {
    dataSizeH = subDomain[maxAllowableLevel].length(YDIR);
    dataSizeV = subDomain[maxAllowableLevel].length(ZDIR);
  } else {
    dataSizeH = subDomain[maxAllowableLevel].length(XDIR);
    dataSizeV = subDomain[maxAllowableLevel].length(YDIR);
  }
  dataSize = dataSizeH * dataSizeV;

# if (BL_SPACEDIM == 3)
    if(myView==XY) {
      hLine = (subDomain[maxAllowableLevel].bigEnd(YDIR) -
      		subDomain[maxAllowableLevel].smallEnd(YDIR)) *
		pltAppPtr->CurrentScale();
      vLine = 0;
    } else if(myView==XZ) {
      hLine = (subDomain[maxAllowableLevel].bigEnd(ZDIR) -
      		subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
		pltAppPtr->CurrentScale();
      vLine = 0;
    } else {
      hLine = (subDomain[maxAllowableLevel].bigEnd(ZDIR) -
      		subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
		pltAppPtr->CurrentScale();
      vLine = 0;
    }
    slice = subDomain[maxAllowableLevel].smallEnd(YZ - myView);
# else
    vLine = 0;
    hLine = 0;
    slice = 0;
# endif
  subcutY = hLine;
  subcut2ndY = hLine;

  sliceBox.resize(maxAllowableLevel + 1);
  sliceBox[maxAllowableLevel] = subDomain[maxAllowableLevel];

  AmrPictureInit();
}  // end AmrPicture constructor


// ---------------------------------------------------------------------
void AmrPicture::AmrPictureInit() {

  if(myView==XZ) {
    sliceBox[maxAllowableLevel].setSmall(YDIR, slice);
    sliceBox[maxAllowableLevel].setBig(YDIR, slice);
  } else if(myView==YZ) {
    sliceBox[maxAllowableLevel].setSmall(XDIR, slice);
    sliceBox[maxAllowableLevel].setBig(XDIR, slice);
  } else {  // XY
#   if (BL_SPACEDIM == 3)
      sliceBox[maxAllowableLevel].setSmall(ZDIR, slice);
      sliceBox[maxAllowableLevel].setBig(ZDIR, slice);
#   endif
  }

  CoarsenSliceBox();

  sliceFab.resize(sliceBox[maxAllowableLevel], 1);

  imageSizeH = pltAppPtr->CurrentScale() * dataSizeH;  // dont use imageBox.length()
  imageSizeV = pltAppPtr->CurrentScale() * dataSizeV;  // because of node
						       // centered boxes
  imageSize = imageSizeH * imageSizeV;

  imageData = new unsigned char[dataSize];
  scaledImageData = new unsigned char[imageSize];

  pendingTimeOut = 0;
  frameSpeed = 300;
  hdspoint = 0;
  vdspoint = 0;
  datasetPointShowing = false;
  datasetPointColor = 0;
  subCutShowing = false;
  subcutX = 0;
  subcut2ndX = 0;
  framesMade = false;
  maxsFound = false;
  if(myView == XZ) {
    hColor = 255;
    vColor = 65;
  } else if(myView == YZ) {
    hColor = 255;
    vColor = 220;
  } else {
    hColor = 220;
    vColor = 65;
  }
  pixMapCreated = false;

  SetSlice(myView, slice);

}  // end AmrPictureInit()


// ---------------------------------------------------------------------
AmrPicture::~AmrPicture() {
  if(framesMade) {
    for(int i = minDrawnLevel;
        i < subDomain[maxAllowableLevel].length(sliceDir); ++i)
    {
      XDestroyImage(frameBuffer[i]);
    }
  }
  delete [] imageData;
  delete [] scaledImageData;

  if(pixMapCreated) {
    XFreePixmap(GAptr->PDisplay(), pixMap);
  }
}


// ---------------------------------------------------------------------
void AmrPicture::SetSlice(int view, int here) {
  int lev;
  sliceDir = YZ - view;
  const AmrData &amrData = dataServicesPtr->AmrDataRef();
  assert(amrData.RefRatio().length() > 0);

# if (BL_SPACEDIM == 3)
    slice = here;
    sliceBox[maxAllowableLevel].setSmall(sliceDir, slice);
    sliceBox[maxAllowableLevel].setBig(sliceDir, slice);
    CoarsenSliceBox();
    if(sliceFab.box() != sliceBox[maxAllowableLevel]) {
      if(sliceFab.box().numPts() != sliceBox[maxAllowableLevel].numPts()) {
        delete [] imageData;
        imageData = new unsigned char[sliceBox[maxAllowableLevel].numPts()];
      }
      sliceFab.resize(sliceBox[maxAllowableLevel], 1);
    }
# endif

  for(lev = minDrawnLevel; lev < gpArray.length(); lev++) {
    gpArray[lev].clear();
  }
  gpArray.clear();

  gpArray.resize(numberOfLevels);
  maxLevelWithGrids = maxAllowableLevel;

  Array<int> nGrids(numberOfLevels);
  for(lev = minDrawnLevel; lev <= maxAllowableLevel; ++lev) {
    nGrids[lev] = amrData.NIntersectingGrids(lev, sliceBox[lev]);
    gpArray[lev].resize(nGrids[lev]);
    if(nGrids[lev] == 0 && maxLevelWithGrids == maxAllowableLevel) {
      maxLevelWithGrids = lev - 1;
    }
  }

  if(nGrids[minDrawnLevel] == 0) {
    cerr << "Error in AmrPicture::SetSlice:  No Grids intersected." << endl;
    cerr << "slice = " << slice << endl;
    cerr << "sliceBox[maxallowlev] = " << sliceBox[maxAllowableLevel] << endl;
    cerr << "maxAllowableLevel maxLevelWithGrids = " << maxAllowableLevel
	 << "  " << maxLevelWithGrids << endl;
    cerr << "subDomain[maxallowlev] = " << subDomain[maxAllowableLevel] << endl;
    return;
  }

  int gridNumber;
  for(lev = minDrawnLevel; lev <= maxLevelWithGrids; lev++) {
    gridNumber = 0;
    for(int iBox = 0; iBox < amrData.boxArray(lev).length(); ++iBox) {
      Box temp(amrData.boxArray(lev)[iBox]);
      if(sliceBox[lev].intersects(temp)) {
	temp &= sliceBox[lev];
	Box sliceDataBox(temp);
	temp.shift(XDIR, -subDomain[lev].smallEnd(XDIR));
	temp.shift(YDIR, -subDomain[lev].smallEnd(YDIR));
#	if (BL_SPACEDIM==3)
	  temp.shift(ZDIR, -subDomain[lev].smallEnd(ZDIR));
#       endif
        gpArray[lev][gridNumber].GridPictureInit(lev,
		CRRBetweenLevels(lev, maxAllowableLevel, amrData.RefRatio()),
		pltAppPtr->CurrentScale(), imageSizeH, imageSizeV,
		GAptr, temp, sliceDataBox, sliceDir);
        ++gridNumber;
      }
    }
  }
}  // end SetSlice(...)


// ---------------------------------------------------------------------
void AmrPicture::DrawBoxes() {
  int i, level;
  short xbox, ybox;
  unsigned short wbox, hbox;

  if(showBoxes) {
    for(level = minDrawnLevel; level <= maxDrawnLevel; level++) {
      if(level == minDrawnLevel) {
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), palPtr->WhiteIndex());
      } else {
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 255-80*(level-1));
      }
      for(i = 0; i < gpArray[level].length(); i++) {
	xbox = gpArray[level][i].HPositionInPicture();
	ybox = gpArray[level][i].VPositionInPicture();
	wbox = gpArray[level][i].ImageSizeH(); 
	hbox = gpArray[level][i].ImageSizeV(); 

        XDrawRectangle(GAptr->PDisplay(), pictureWindow,  GAptr->PGC(),
			xbox, ybox, wbox, hbox);
      }
    }
  }
  // draw bounding box
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), palPtr->BlackIndex());
  XDrawRectangle(GAptr->PDisplay(), pictureWindow, GAptr->PGC(), 0, 0,
			imageSizeH-1, imageSizeV-1);
}


// ---------------------------------------------------------------------
void AmrPicture::Draw(int fromLevel, int toLevel) {
  if(pixMapCreated == true) {
    XFreePixmap(GAptr->PDisplay(), pixMap);
  }  

  pixMap = XCreatePixmap(GAptr->PDisplay(), pictureWindow,
			   imageSizeH, imageSizeV, GAptr->PDepth());
  pixMapCreated = true;

// fixme:  use fromLevel and toLevel

  XPutImage(GAptr->PDisplay(), pixMap, GAptr->PGC(),
            image, 0, 0, 0, 0, imageSizeH, imageSizeV);

  if( ! pltAppPtr->Animating()) {
    DoExposePicture();
  }
}  // end AmrPicture::Draw.


// ---------------------------------------------------------------------
void AmrPicture::SyncPicture() {
  if(pendingTimeOut != 0) {
    SetSlice(myView, slice);
    ChangeDerived(currentDerived, palPtr);
  }
}


// ---------------------------------------------------------------------
void AmrPicture::ToggleBoxes() {
  showBoxes = (showBoxes ? false : true);
  DoExposePicture();
}


// ---------------------------------------------------------------------
void AmrPicture::ToggleShowSubCut() {
  subCutShowing = (subCutShowing ? false : true);
  DoExposePicture();
}


// ---------------------------------------------------------------------
void AmrPicture::SetRegion(int startX, int startY, int endX, int endY) {
  regionX = startX; 
  regionY = startY; 
  region2ndX = endX; 
  region2ndY = endY; 
}


// ---------------------------------------------------------------------
void AmrPicture::SetSubCut(int startX, int startY, int endX, int endY) {
  if(startX != -1) {
    subcutX = startX; 
  }
  if(startY != -1) {
    subcutY = startY; 
  }
  if(endX != -1) {
    subcut2ndX = endX; 
  }
  if(endY != -1) {
    subcut2ndY = endY; 
  }
}


// ---------------------------------------------------------------------
void AmrPicture::DoExposePicture() {
  pltAppPtr->DoExposeRef();
  if(pltAppPtr->Animating()) {
    XPutImage(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
      pltAppPtr->CurrentFrameXImage(), 0, 0, 0, 0, imageSizeH, imageSizeV);
  } else {
    if(pendingTimeOut == 0) {
      XCopyArea(GAptr->PDisplay(), pixMap, pictureWindow, 
 		GAptr->PGC(), 0, 0, imageSizeH, imageSizeV, 0, 0); 

      DrawBoxes();

      if( ! subCutShowing) {   // draw selected region
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 60);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX+1, regionY+1, region2ndX+1, regionY+1); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX+1, regionY+1, regionX+1, region2ndY+1); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX+1, region2ndY+1, region2ndX+1, region2ndY+1); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  region2ndX+1, regionY+1, region2ndX+1, region2ndY+1);

        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 175);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX, regionY, region2ndX, regionY); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX, regionY, regionX, region2ndY); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  regionX, region2ndY, region2ndX, region2ndY); 
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  region2ndX, regionY, region2ndX, region2ndY);
      }

#     if (BL_SPACEDIM == 3)
        // draw plane "cutting" lines
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), hColor);
        XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), 0, hLine, imageSizeH, hLine); 
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), vColor);
        XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), vLine, 0, vLine, imageSizeV); 

        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), hColor-30);
        XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), 0, hLine+1, imageSizeH, hLine+1); 
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), vColor-30);
        XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), vLine+1, 0, vLine+1, imageSizeV); 

        if(subCutShowing) {
          // draw subvolume cutting border 
          XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 90);
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX+1, subcutY+1, subcut2ndX+1, subcutY+1); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX+1, subcutY+1, subcutX+1, subcut2ndY+1); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX+1, subcut2ndY+1, subcut2ndX+1, subcut2ndY+1); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcut2ndX+1, subcutY+1, subcut2ndX+1, subcut2ndY+1);

          XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 155);
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX, subcutY, subcut2ndX, subcutY); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX, subcutY, subcutX, subcut2ndY); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcutX, subcut2ndY, subcut2ndX, subcut2ndY); 
          XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		    subcut2ndX, subcutY, subcut2ndX, subcut2ndY);
        }
#     endif

      if(datasetPointShowing) {
        int hpoint = hdspoint * pltAppPtr->CurrentScale();
        int vpoint = imageSizeV-1 - vdspoint * pltAppPtr->CurrentScale();
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 0);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  hpoint-4, vpoint+1, hpoint+6, vpoint+1);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  hpoint+1, vpoint-4, hpoint+1, vpoint+6);
        XSetForeground(GAptr->PDisplay(), GAptr->PGC(), datasetPointColor);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  hpoint-5, vpoint, hpoint+5, vpoint);
        XDrawLine(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
		  hpoint, vpoint-5, hpoint, vpoint+5);
      }
    }
  }
}  // end DoExposePicture


// ---------------------------------------------------------------------
void AmrPicture::SetWindow(Window drawPictureHere) {
  pictureWindow = drawPictureHere;
}


// ---------------------------------------------------------------------
void AmrPicture::ChangeSlice(int here) {
  if(pendingTimeOut != 0) {
    DoStop();
  }
  SetSlice(myView, here);
  ChangeDerived(currentDerived, palPtr);
}


// ---------------------------------------------------------------------
void AmrPicture::CreatePicture(Window drawPictureHere, Palette *palptr,
				const aString &derived)
{
  palPtr = palptr;
  currentDerived = derived;
  SetWindow(drawPictureHere);
  ChangeDerived(currentDerived, palptr);
}


// ---------------------------------------------------------------------
void AmrPicture::ChangeDerived(aString derived, Palette *palptr) {
  int lev;
  Real dataMin, dataMax;
  int printDone = false;
  assert(palptr != NULL);
  palPtr = palptr;
  AmrData &amrData = dataServicesPtr->AmrDataRef();

  if(currentDerived != derived || whichRange == USEFILE) {
    maxsFound = false;
    pltAppPtr->PaletteDrawn(false);
  }

  if( ! maxsFound) {
    framesMade = false;
    if(pendingTimeOut != 0) {
      DoStop();
    }
    maxsFound = true;
    currentDerived =  derived;
    if(myView == XY) {
      aString outbuf = "Reading data for ";
      outbuf += pltAppPtr->GetFileName();
      outbuf += " ...";
      strcpy(buffer, outbuf.c_str());
      PrintMessage(buffer);
      if( ! pltAppPtr->IsAnim() || whichRange == USEFILE) {
        dataMinAllGrids =  AV_BIG_REAL;
        dataMaxAllGrids = -AV_BIG_REAL;
        dataMinRegion   =  AV_BIG_REAL;
        dataMaxRegion   = -AV_BIG_REAL;
        dataMinFile     =  AV_BIG_REAL;
        dataMaxFile     = -AV_BIG_REAL;

        // Call Derive for each Grid in AmrData. This finds the
        // global & sub domain min and max.
        for(lev = minDrawnLevel; lev <= maxDrawnLevel; lev++) {
	  if(Verbose()) {
            cout << "Deriving into Grids at level " << lev << endl;
	  }
          //amrData.MinMax(amrData.ProbDomain()[lev], currentDerived, lev,
			     //dataMin, dataMax);
	  bool minMaxValid(false);
          DataServices::Dispatch(DataServices::MinMaxRequest,
			         dataServicesPtr,
			         amrData.ProbDomain()[lev], currentDerived, lev,
			         &dataMin, &dataMax, &minMaxValid);
          dataMinAllGrids = Min(dataMinAllGrids, dataMin);
          dataMaxAllGrids = Max(dataMaxAllGrids, dataMax);
          dataMinFile = Min(dataMinFile, dataMin);
          dataMaxFile = Max(dataMaxFile, dataMax);
	  if(findSubRange && lev <= maxAllowableLevel && lev <= maxLevelWithGrids)
	  {
            DataServices::Dispatch(DataServices::MinMaxRequest,
			           dataServicesPtr,
                                   subDomain[lev], currentDerived, lev,
	                           &dataMin, &dataMax, &minMaxValid);
	    if(minMaxValid) {
              dataMinRegion = Min(dataMinRegion, dataMin);
              dataMaxRegion = Max(dataMaxRegion, dataMax);
	    }
	  }
	}  // end for(lev...)
      } else {
        dataMinAllGrids = pltAppPtr->GlobalMin();
        dataMaxAllGrids = pltAppPtr->GlobalMax();
        dataMinRegion = pltAppPtr->GlobalMin();
        dataMaxRegion = pltAppPtr->GlobalMax();
      }
      sprintf(buffer, "...");
      printDone = true;
      PrintMessage(buffer);
      if( ! findSubRange) {
	dataMinRegion = dataMinAllGrids;
	dataMaxRegion = dataMaxAllGrids;
      }
    } else {
      dataMinAllGrids = pltAppPtr->GetAmrPicturePtr(XY)->GetMin();
      dataMaxAllGrids = pltAppPtr->GetAmrPicturePtr(XY)->GetMax();
      dataMinRegion = pltAppPtr->GetAmrPicturePtr(XY)->GetRegionMin();
      dataMaxRegion = pltAppPtr->GetAmrPicturePtr(XY)->GetRegionMax();
    }

  }  // end if( ! maxsFound)

  switch(whichRange) {
	case USEGLOBAL:
    		minUsing = dataMinAllGrids;
    		maxUsing = dataMaxAllGrids;
	break;
	case USELOCAL:
    		minUsing = dataMinRegion;
    		maxUsing = dataMaxRegion;
	break;
	case USESPECIFIED:
    		minUsing = dataMinSpecified;
    		maxUsing = dataMaxSpecified;
	break;
	case USEFILE:
    		minUsing = dataMinFile;
    		maxUsing = dataMaxFile;
	break;
	default:
    		minUsing = dataMinAllGrids;
    		maxUsing = dataMaxAllGrids;
	break;
  }

  VSHOWVAL(Verbose(), minUsing)
  VSHOWVAL(Verbose(), maxUsing)

  DataServices::Dispatch(DataServices::FillVarOneFab, dataServicesPtr,
		         &sliceFab, sliceFab.box(), maxDrawnLevel, derived);
  CreateImage(sliceFab, imageData, dataSizeH, dataSizeV,
	      minUsing, maxUsing, palPtr);
  CreateScaledImage(&image, pltAppPtr->CurrentScale(),
                    imageData, scaledImageData,
                    dataSizeH, dataSizeV,
                    imageSizeH, imageSizeV);

  if( ! pltAppPtr->PaletteDrawn()) {
    pltAppPtr->PaletteDrawn(true);
    palptr->Draw(minUsing, maxUsing, pltAppPtr->GetFormatString());
  }

  Draw(minDrawnLevel, maxDrawnLevel);
  if(printDone) {
     sprintf(buffer, "done.\n");
     PrintMessage(buffer);
  }

}  // end ChangeDerived


// -------------------------------------------------------------------
// convert Real to char in imagedata from fab
void AmrPicture::CreateImage(const FArrayBox &fab, unsigned char *imagedata,
			     int datasizeh, int datasizev,
			     Real globalMin, Real globalMax, Palette *palptr)
{
  int jdsh, jtmp1;
  int dIndex, iIndex;
  Real oneOverGDiff;
  if((globalMax - globalMin) < FLT_MIN) {
    oneOverGDiff = 0.0;
  } else {
    oneOverGDiff = 1.0 / (globalMax - globalMin);
  }
  const Real *dataPoint = fab.dataPtr();
  int blackIndex = palptr->BlackIndex();
  int colorSlots = palptr->ColorSlots();
  int paletteStart = palptr->PaletteStart();
  int paletteEnd = palptr->PaletteEnd();
  int csm1 = colorSlots - 1;

  Real dPoint;

  // flips the image in Vert dir: j => datasizev-j-1
    for(int j = 0; j < datasizev; ++j) {
      jdsh = j * datasizeh;
      jtmp1 = (datasizev-j-1) * datasizeh;
      for(int i = 0; i < datasizeh; ++i) {
        dIndex = i + jtmp1;
        dPoint = dataPoint[dIndex];
        iIndex = i + jdsh;
        if(dPoint > globalMax) {  // clip
          imagedata[iIndex] = paletteEnd;
        } else if(dPoint < globalMin) {  // clip
          imagedata[iIndex] = paletteStart;
        } else {
          imagedata[iIndex] = (unsigned char)
             ((((dPoint - globalMin) * oneOverGDiff) * csm1) );
           //    ^^^^^^^^^^^^^^^^^ Real data
          imagedata[iIndex] += paletteStart;
        }
      }
    }
}  // end CreateImage(...)

// ---------------------------------------------------------------------
void AmrPicture::CreateScaledImage(XImage **ximage, int scale,
				   unsigned char *imagedata,
				   unsigned char *scaledimagedata,
				   int datasizeh, int datasizev,
				   int imagesizeh, int imagesizev)
{ 
  int jish, jtmp;

  for(int j = 0; j < imagesizev; ++j) {
    jish = j*imagesizeh;
    jtmp =  datasizeh * (j/scale);
    for(int i = 0; i < imagesizeh; i++) {
      scaledimagedata[i+jish] = imagedata [ (i/scale) + jtmp ];
    }
  }

  *ximage = XCreateImage(GAptr->PDisplay(), GAptr->PVisual(),
		GAptr->PDepth(), ZPixmap, 0, (char *) scaledimagedata,
		imagesizeh, imagesizev,
		XBitmapPad(GAptr->PDisplay()), imagesizeh);

  (*ximage)->byte_order = MSBFirst;
  (*ximage)->bitmap_bit_order = MSBFirst;

}  // end CreateScaledImage()


// ---------------------------------------------------------------------
void AmrPicture::ChangeScale(int newScale) { 
  XFreePixmap(GAptr->PDisplay(), pixMap);
  pixMapCreated = false;
  framesMade = false;
  if(pendingTimeOut != 0) {
    DoStop();
  }
  imageSizeH = newScale   * dataSizeH;
  imageSizeV = newScale   * dataSizeV;
  imageSize  = imageSizeH * imageSizeV;
  XClearWindow(GAptr->PDisplay(), pictureWindow);

  int lev, grid;
  for(lev = minDrawnLevel; lev <= maxAllowableLevel; lev++) {
   for(grid = 0; grid < gpArray[lev].length(); grid++) {
     gpArray[lev][grid].ChangeScale(newScale, imageSizeH, imageSizeV);
   }
  }
  delete [] scaledImageData;
  scaledImageData = new unsigned char[imageSize];
  CreateScaledImage(&image, newScale,
                    imageData, scaledImageData,
                    dataSizeH, dataSizeV,
                    imageSizeH, imageSizeV);

  hLine = ((hLine / (pltAppPtr->PreviousScale())) * newScale) + (newScale - 1);
  vLine = ((vLine / (pltAppPtr->PreviousScale())) * newScale) + (newScale - 1);
  Draw(minDrawnLevel, maxDrawnLevel);
}


// ---------------------------------------------------------------------
void AmrPicture::ChangeLevel(int lowLevel, int hiLevel) { 
  minDrawnLevel = lowLevel;
  maxDrawnLevel = hiLevel;
  framesMade = false;
  if(pendingTimeOut != 0) {
    DoStop();
  }
  XClearWindow(GAptr->PDisplay(), pictureWindow);

  cerr << endl;
  cerr << "AmrPicture::ChangeLevel not yet implemented." << endl;
  // fix Draw to use min and max levels
  cerr << endl;

  Draw(minDrawnLevel, maxDrawnLevel);
}


// ---------------------------------------------------------------------
XImage *AmrPicture::GetPictureXImage() {
  int i, level, xbox, ybox, wbox, hbox;
  XImage *image;

  if(showBoxes) {
    for(level = minDrawnLevel; level <= maxDrawnLevel; level++) {
      XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 255-40*level);
      for(i = 0; i < gpArray[level].length(); i++) {
	xbox = gpArray[level][i].HPositionInPicture();
	ybox = gpArray[level][i].VPositionInPicture();
	wbox = gpArray[level][i].ImageSizeH(); 
	hbox = gpArray[level][i].ImageSizeV(); 

        XDrawRectangle(GAptr->PDisplay(), pixMap,  GAptr->PGC(),
			xbox, ybox, wbox, hbox);
      }
    }
  }
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), 255);
  XDrawRectangle(GAptr->PDisplay(), pixMap, GAptr->PGC(), 0, 0,
			imageSizeH-1, imageSizeV-1);

  image = XGetImage(GAptr->PDisplay(), pixMap, 0, 0,
		imageSizeH, imageSizeV, AllPlanes, ZPixmap);

  XFreePixmap(GAptr->PDisplay(), pixMap);
  pixMapCreated = false;
  Draw(minDrawnLevel, maxDrawnLevel);
  return image;
}


// ---------------------------------------------------------------------
void AmrPicture::CoarsenSliceBox() {
  for(int i = maxAllowableLevel - 1; i >= minDrawnLevel; --i) {
    sliceBox[i] = sliceBox[maxAllowableLevel];
    sliceBox[i].coarsen(CRRBetweenLevels(i, maxAllowableLevel,
			dataServicesPtr->AmrDataRef().RefRatio()));
  }
}


// ---------------------------------------------------------------------
void AmrPicture::CreateFrames(AnimDirection direction) {
  int start, length, cancelled;
  int islice, i, j, lev, gridNumber;
  Array<int> intersectGrids;
  int maxLevelWithGridsHere;
  GridPicture **animPicture;
  int posneg(1);
  if(direction == ANIMNEGDIR) {
    posneg = -1;
  }
  const AmrData &amrData = dataServicesPtr->AmrDataRef();

  sprintf(buffer, "Creating frames..."); 
  PrintMessage(buffer);
  cancelled = false;
  Array<Box> interBox(numberOfLevels);
  interBox[maxAllowableLevel] = subDomain[maxAllowableLevel];
  start = subDomain[maxAllowableLevel].smallEnd(sliceDir);
  length = subDomain[maxAllowableLevel].length(sliceDir);
  if(framesMade) {
    for(int ii = 0; ii < subDomain[maxAllowableLevel].length(sliceDir); ++ii) {
      XDestroyImage(frameBuffer[ii]);
    }
  }
  frameBuffer = new XImage*[length];
  unsigned char *frameImageData = new unsigned char[dataSize];
  //unsigned char *frameScaledImageData = new unsigned char[imageSize];

  for(i = 0; i < length; ++i) {
    islice = ((slice + (posneg * i)) + length) % length;
			          // ^^^^^^^^ + length for negative values
    intersectGrids.resize(maxAllowableLevel + 1);
    interBox[maxAllowableLevel].setSmall(sliceDir, start+islice);
    interBox[maxAllowableLevel].setBig(sliceDir, start+islice);
    for(j = maxAllowableLevel - 1; j >= minDrawnLevel; --j) {
      interBox[j] = interBox[maxAllowableLevel];
      interBox[j].coarsen(CRRBetweenLevels(j, maxAllowableLevel,
			  amrData.RefRatio()));
    }
    maxLevelWithGridsHere = maxDrawnLevel;
    for(lev = minDrawnLevel; lev <= maxAllowableLevel; ++lev) {
      intersectGrids[lev] = 0;
      for(int iBox = 0; iBox < amrData.boxArray(lev).length(); ++iBox) {
        if(interBox[lev].intersects(amrData.boxArray(lev)[iBox])) {
          ++intersectGrids[lev];
        }
      }
    }
    animPicture = new GridPicture*[maxLevelWithGridsHere + 1]; 
    for(lev = minDrawnLevel; lev <= maxLevelWithGridsHere; ++lev) {
      animPicture[lev] = new GridPicture[intersectGrids[lev]];
      if(animPicture[lev] == NULL) {
        sprintf(buffer, "Out of memory."); 
        PrintMessage(buffer);
        return;
      }
    } 


    for(lev = minDrawnLevel; lev <= maxLevelWithGridsHere; ++lev) {
      gridNumber = 0;
      for(int iBox = 0; iBox < amrData.boxArray(lev).length(); ++iBox) {
	Box temp(amrData.boxArray(lev)[iBox]);
        if(interBox[lev].intersects(temp)) {
	  temp &= interBox[lev];
          Box sliceDataBox(temp);
	  temp.shift(XDIR, -subDomain[lev].smallEnd(XDIR));
	  temp.shift(YDIR, -subDomain[lev].smallEnd(YDIR));
          temp.shift(ZDIR, -subDomain[lev].smallEnd(ZDIR));
          animPicture[lev][gridNumber].GridPictureInit(lev,
                  CRRBetweenLevels(lev, maxAllowableLevel, amrData.RefRatio()),
                  pltAppPtr->CurrentScale(), imageSizeH, imageSizeV,
                  GAptr, temp, sliceDataBox, sliceDir);
          //animPicture[lev][gridNumber].Derive(currentDerived.c_str());
          //animPicture[lev][gridNumber].CreateImage(minUsing, maxUsing, palPtr);
          ++gridNumber;
        }
      }
    }   // end for(lev...)

    // get the data for this slice
    FArrayBox imageFab(interBox[maxAllowableLevel], 1);
    DataServices::Dispatch(DataServices::FillVarOneFab, dataServicesPtr,
			   &imageFab, imageFab.box(), maxAllowableLevel,
                           currentDerived);

    CreateImage(imageFab, frameImageData, dataSizeH, dataSizeV,
                minUsing, maxUsing, palPtr);

    // this cannot be deleted because it belongs to the XImage
    unsigned char *frameScaledImageData = new unsigned char[imageSize];
    CreateScaledImage(&(frameBuffer[islice]), pltAppPtr->CurrentScale(),
                      frameImageData, frameScaledImageData,
                      dataSizeH, dataSizeV,
                      imageSizeH, imageSizeV);

/*
    // Send grid image data directly to char array.
    unsigned char *interImageData = new unsigned char[imageSize];
    int jish;
    for(lev = minDrawnLevel; lev <= maxLevelWithGridsHere; ++lev) {
      for(gridNumber = 0; gridNumber < intersectGrids[lev]; ++gridNumber) {
        gridData = animPicture[lev][gridNumber].GetScaledImageData();
        c = 0;
        for(int j = animPicture[lev][gridNumber].VPositionInPicture();
	    j < (animPicture[lev][gridNumber].ImageSizeV() +
		 animPicture[lev][gridNumber].VPositionInPicture()); ++j)
        {
	  jish = j*imageSizeH;
          for(int k = animPicture[lev][gridNumber].HPositionInPicture();
	      k < (animPicture[lev][gridNumber].ImageSizeH() +
                   animPicture[lev][gridNumber].HPositionInPicture()); ++k)
          {
	    // boundary checking
	    if(k<imageSizeH && k+jish < imageSize && k >= 0 && j >= 0) {
              interImageData[k+jish] = gridData[c];
            }
            ++c;
          }
        }
      }
    }  // end for(lev...)
    frameBuffer[islice] = XCreateImage(GAptr->PDisplay(), GAptr->PVisual(),
			GAptr->PDepth(), ZPixmap, 0, (char *) interImageData,
			imageSizeH, imageSizeV,
			XBitmapPad(GAptr->PDisplay()), imageSizeH);
    if(frameBuffer[islice] == NULL) {
      sprintf(buffer, "Out of memory."); 
      PrintMessage(buffer);
      return;
    }
*/

    XPutImage(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
              frameBuffer[islice], 0, 0, 0, 0, imageSizeH, imageSizeV);

    for(lev = minDrawnLevel; lev <= maxLevelWithGridsHere; lev++) {
      delete [] animPicture[lev];
    }
#   if (BL_SPACEDIM == 3)
      XEvent event;
      if(XCheckMaskEvent(GAptr->PDisplay(), ButtonPressMask, &event)) {
        if(event.xany.window == XtWindow(pltAppPtr->GetStopButtonWidget())) {
	  XPutBackEvent(GAptr->PDisplay(), &event);
          cancelled = true;
          break;
	}
      }
#   endif
  }  // end for(i=0; ...)

  delete [] frameImageData;
  //delete [] frameScaledImageData;

  if(cancelled) {
    framesMade = false;
    sprintf(buffer, "Cancelled.\n"); 
    PrintMessage(buffer);
    //cout << "anim cancelled, setting slice to " << start+islice << endl;
    ChangeSlice(start+islice);
    AmrPicture *apXY = pltAppPtr->GetAmrPicturePtr(XY);
    AmrPicture *apXZ = pltAppPtr->GetAmrPicturePtr(XZ);
    AmrPicture *apYZ = pltAppPtr->GetAmrPicturePtr(YZ);
    if(sliceDir == XDIR) {
      apXY->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(XDIR)) *
                                 pltAppPtr->CurrentScale());
      apXY->DoExposePicture();
      apXZ->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(XDIR)) *
                                  pltAppPtr->CurrentScale());
      apXZ->DoExposePicture();
    }
    if(sliceDir == YDIR) {
      apXY->SetHLine(apXY->ImageSizeV()-1 - (slice -
				  subDomain[maxAllowableLevel].smallEnd(YDIR)) *
                       		  pltAppPtr->CurrentScale());
      apXY->DoExposePicture();
      apYZ->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(YDIR)) *
                                  pltAppPtr->CurrentScale());
      apYZ->DoExposePicture();
    }
    if(sliceDir == ZDIR) {
      apYZ->SetHLine(apYZ->ImageSizeV()-1 - (slice -
				  subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
				  pltAppPtr->CurrentScale());
      apYZ->DoExposePicture();
      apXZ->SetHLine(apXZ->ImageSizeV()-1 - (slice -
				  subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
                                  pltAppPtr->CurrentScale());
      apXZ->DoExposePicture();
    }

  } else {
    framesMade = true;
    sprintf(buffer, "Done.\n"); 
    PrintMessage(buffer);
  }
  DoExposePicture();
}  // end CreateFrames


// ---------------------------------------------------------------------
void AmrPicture::SetDatasetPoint(int hplot, int vplot, int color) {
  hdspoint = hplot;
  vdspoint = vplot;
  datasetPointColor = color;
  datasetPointShowing = true;
}


// ---------------------------------------------------------------------
void AmrPicture::CBFrameTimeOut(XtPointer client_data, XtIntervalId *) {
  ((AmrPicture *) client_data)->DoFrameUpdate();
}


// ---------------------------------------------------------------------
void AmrPicture::DoFrameUpdate() {
  if(sweepDirection == ANIMPOSDIR) {
    if(slice < subDomain[maxAllowableLevel].bigEnd(sliceDir)) {
      slice++;
    } else {
      slice = subDomain[maxAllowableLevel].smallEnd(sliceDir);
    }
  } else {
    if(slice > subDomain[maxAllowableLevel].smallEnd(sliceDir)) {
      slice--;
    } else {
      slice = subDomain[maxAllowableLevel].bigEnd(sliceDir);
    }
  } 
  ShowFrameImage();
  XSync(GAptr->PDisplay(), false);
  pendingTimeOut = XtAppAddTimeOut(pltAppPtr->GetAppContext(),
  				frameSpeed, &AmrPicture::CBFrameTimeOut,
				(XtPointer) this);
}  // end DoFrameUpdate()


// ---------------------------------------------------------------------
void AmrPicture::DoStop() {
  if(pendingTimeOut != 0) {
    XtRemoveTimeOut(pendingTimeOut);
    pendingTimeOut = 0;
    ChangeSlice(slice);
  }
}


// ---------------------------------------------------------------------
void AmrPicture::Sweep(AnimDirection direction) {
  if(pendingTimeOut != 0) {
    DoStop();
  }
  if( ! framesMade) {
    CreateFrames(direction);
  }
  if(framesMade) {
    pendingTimeOut = XtAppAddTimeOut(pltAppPtr->GetAppContext(),
  				frameSpeed, &AmrPicture::CBFrameTimeOut,
				(XtPointer) this);
    sweepDirection = direction;
  }
}


// ---------------------------------------------------------------------
void AmrPicture::ShowFrameImage() {
  AmrPicture *apXY = pltAppPtr->GetAmrPicturePtr(XY);
  AmrPicture *apXZ = pltAppPtr->GetAmrPicturePtr(XZ);
  AmrPicture *apYZ = pltAppPtr->GetAmrPicturePtr(YZ);

  XPutImage(GAptr->PDisplay(), pictureWindow, GAptr->PGC(),
            frameBuffer[slice - subDomain[maxAllowableLevel].smallEnd(sliceDir)],
            0, 0, 0, 0, imageSizeH, imageSizeV);

  // draw plane "cutting" lines
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), hColor);
  XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), 0, hLine, imageSizeH, hLine); 
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), vColor);
  XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), vLine, 0, vLine, imageSizeV); 
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), hColor-30);
  XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), 0, hLine+1, imageSizeH, hLine+1); 
  XSetForeground(GAptr->PDisplay(), GAptr->PGC(), vColor-30);
  XDrawLine(GAptr->PDisplay(), pictureWindow,
		GAptr->PGC(), vLine+1, 0, vLine+1, imageSizeV); 

  if(sliceDir == XDIR) {
    apXY->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(XDIR)) *
                               pltAppPtr->CurrentScale());
    apXY->DoExposePicture();
    apXZ->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(XDIR)) *
                                pltAppPtr->CurrentScale());
    apXZ->DoExposePicture();
  }
  if(sliceDir == YDIR) {
    apXY->SetHLine(apXY->ImageSizeV() - 1 - (slice -
				subDomain[maxAllowableLevel].smallEnd(YDIR)) *
                       		pltAppPtr->CurrentScale());
    apXY->DoExposePicture();
    apYZ->SetVLine((slice - subDomain[maxAllowableLevel].smallEnd(YDIR)) *
                                pltAppPtr->CurrentScale());
    apYZ->DoExposePicture();
  }
  if(sliceDir == ZDIR) {
    apYZ->SetHLine(apYZ->ImageSizeV() - 1 - (slice -
				subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
				pltAppPtr->CurrentScale());
    apYZ->DoExposePicture();
    apXZ->SetHLine(apXZ->ImageSizeV() - 1 - (slice -
				subDomain[maxAllowableLevel].smallEnd(ZDIR)) *
                                pltAppPtr->CurrentScale());
    apXZ->DoExposePicture();
  }
  pltAppPtr->DoExposeRef();
}  // end ShowFrameImage()


// ---------------------------------------------------------------------
Real AmrPicture::GetWhichMin() {
  if(whichRange == USEGLOBAL) {
    return dataMinAllGrids;
  } else if(whichRange == USELOCAL) {
    return dataMinRegion;
  } else if(whichRange == USEFILE) {
    return dataMinFile;
  } else {
    return dataMinSpecified;
  }
}


// ---------------------------------------------------------------------
Real AmrPicture::GetWhichMax() {
  if(whichRange == USEGLOBAL) {
    return dataMaxAllGrids;
  } else if (whichRange == USELOCAL) {
    return dataMaxRegion;
  } else if (whichRange == USEFILE) {
    return dataMaxFile;
  } else {
    return dataMaxSpecified;
  }
}


// ---------------------------------------------------------------------
void AmrPicture::SetWhichRange(Range newRange) {
  whichRange = newRange;
  framesMade = false;
  DoStop();
}
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------