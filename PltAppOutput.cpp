//BL_COPYRIGHT_NOTICE

//
// $Id: PltAppOutput.cpp,v 1.18 2000-06-14 20:08:28 car Exp $
//

#include <Xm/Xm.h>
#include <Xm/SelectioB.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>

// ---------------------------------------------------------------
// PltAppOutput.cpp
// ---------------------------------------------------------------
#include "PltApp.H"
#include "Output.H"

// -------------------------------------------------------------------
void PltApp::DoOutput(Widget w, XtPointer data, XtPointer) {
  int i;
  char tempfilename[BUFSIZ];
  static Widget wGetFileName;
  XmString sMessage;
  sMessage = XmStringCreateSimple("Please enter a filename base:");

  i=0;
  XtSetArg(args[i], XmNselectionLabelString, sMessage); i++;
  XtSetArg(args[i], XmNautoUnmanage, false); i++;
  XtSetArg(args[i], XmNkeyboardFocusPolicy, XmPOINTER); i++;
  wGetFileName = XmCreatePromptDialog(wAmrVisTopLevel, "Save as", args, i);
  XmStringFree(sMessage);

  unsigned long which = (unsigned long) data;
  switch(which) {
  case 0:
    AddStaticCallback(wGetFileName, XmNokCallback,&PltApp::DoCreatePSFile); break;
  case 1:
    AddStaticCallback(wGetFileName, XmNokCallback,&PltApp::DoCreateRGBFile); break;
  case 2:
    AddStaticCallback(wGetFileName, XmNokCallback,&PltApp::DoCreateFABFile); break;
  default:
    cerr << "Error in PltApp::DoOutput:  bad selection = " << data << endl;
    return;
  }

  XtAddCallback(wGetFileName, XmNcancelCallback,
  		(XtCallbackProc)XtDestroyWidget, NULL);
  XtSetSensitive(XmSelectionBoxGetChild(wGetFileName,
  		XmDIALOG_HELP_BUTTON), false);

  char tempstr[BUFSIZ], timestep[BUFSIZ];
  char cder[BUFSIZ];
  if(animating2d) {
    strcpy(tempfilename, fileNames[currentFrame].c_str());
  } else {
    strcpy(tempfilename, fileNames[0].c_str());
  }
  i = strlen(tempfilename) - 1;
  while(i > -1 && tempfilename[i] != '/') {
    --i;
  }
  ++i;  // skip first (bogus) character

  FileType fileType = dataServicesPtr[currentFrame]->GetFileType();
  BL_ASSERT(fileType != INVALIDTYPE);
  if(fileType == FAB || fileType == MULTIFAB) {
    strcpy(timestep, &tempfilename[i]);
  } else {  // plt file
    strcpy(timestep, &tempfilename[i+3]);  // skip plt
  }
  i = 0;
  while(timestep[i] != '\0' && timestep[i] != '.') {
    ++i;
  }
  timestep[i] = '\0';
  strcpy(cder, currentDerived.c_str());  // do this because currentDerive does not
				         // work properly in sprintf
  sprintf(tempstr, "%s.%s", cder, timestep);

  XmTextSetString(XmSelectionBoxGetChild(wGetFileName, XmDIALOG_TEXT), tempstr);
  XtManageChild(wGetFileName);
  XtPopup(XtParent(wGetFileName), XtGrabNone);
}  // end DoOutput


// -------------------------------------------------------------------
void PltApp::DoCreatePSFile(Widget w, XtPointer, XtPointer call_data) {
  XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) call_data;
  char psfilename[BUFSIZ];
  char *fileNameBase;
  int imageSizeX, imageSizeY;
  XImage *printImage;
  const Array<XColor> colors(pltPaletteptr->GetColorCells());

  if(animating2d) {
    ResetAnimation();
  }

  XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &fileNameBase);

  // write the ZPLANE picture
  sprintf(psfilename, "%s.XY.ps", fileNameBase);
  printImage = amrPicturePtrArray[ZPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[ZPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[ZPLANE]->ImageSizeV();
  WritePSFile(psfilename, printImage, imageSizeX, imageSizeY, colors);

#if (BL_SPACEDIM==3)
  // write the YPLANE picture
  sprintf(psfilename, "%s.XZ.ps", fileNameBase);
  printImage = amrPicturePtrArray[YPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[YPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[YPLANE]->ImageSizeV();
  WritePSFile(psfilename, printImage, imageSizeX, imageSizeY, colors);

  // write the XPLANE picture
  sprintf(psfilename, "%s.YZ.ps", fileNameBase);
  printImage = amrPicturePtrArray[XPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[XPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[XPLANE]->ImageSizeV();
  WritePSFile(psfilename, printImage, imageSizeX, imageSizeY, colors);

  // write the iso picture
#ifdef BL_VOLUMERENDER
  if( ! (XmToggleButtonGetState(wAutoDraw) || showing3dRender ) ) {
    printImage = projPicturePtr->DrawBoxesIntoPixmap(minDrawnLevel, maxDrawnLevel);
  } else {
    printImage = projPicturePtr->GetPictureXImage();
  }
#else
  printImage = projPicturePtr->DrawBoxesIntoPixmap(minDrawnLevel, maxDrawnLevel);
#endif
  sprintf(psfilename, "%s.XYZ.ps", fileNameBase);
  imageSizeX = projPicturePtr->ImageSizeH();
  imageSizeY = projPicturePtr->ImageSizeV();
  WritePSFile(psfilename, printImage, imageSizeX, imageSizeY, colors);
# endif

  // write the palette
  sprintf(psfilename, "%s.pal.ps", fileNameBase);
  printImage = pltPaletteptr->GetPictureXImage();
  imageSizeX = pltPaletteptr->PaletteWidth();
  imageSizeY = pltPaletteptr->PaletteHeight();
  const Array<Real> &pValueList = pltPaletteptr->PaletteDataList();
  aString pNumFormat(pltPaletteptr->PaletteNumberFormat());
  WritePSPaletteFile(psfilename, printImage, imageSizeX, imageSizeY, colors, 
                     pValueList, pNumFormat);

  XtDestroyWidget(w);

}  // end DoCreatePSFile


// -------------------------------------------------------------------
void PltApp::DoCreateRGBFile(Widget w, XtPointer, XtPointer call_data) {
  XmSelectionBoxCallbackStruct *cbs = (XmSelectionBoxCallbackStruct *) call_data;
  char rgbfilename[BUFSIZ];
  char *fileNameBase;
  int imageSizeX, imageSizeY;
  XImage *printImage;
  const Array<XColor> &colors = pltPaletteptr->GetColorCells();

  if(animating2d) {
    ResetAnimation();
  }

  XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &fileNameBase);

  // write the ZPLANE picture
  sprintf(rgbfilename, "%s.XY.rgb", fileNameBase);
  printImage = amrPicturePtrArray[ZPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[ZPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[ZPLANE]->ImageSizeV();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);

#if (BL_SPACEDIM==3)
  // write the YPLANE picture
  sprintf(rgbfilename, "%s.XZ.rgb", fileNameBase);
  printImage = amrPicturePtrArray[YPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[YPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[YPLANE]->ImageSizeV();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);

  // write the XPLANE picture
  sprintf(rgbfilename, "%s.YZ.rgb", fileNameBase);
  printImage = amrPicturePtrArray[XPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[XPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[XPLANE]->ImageSizeV();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);

  // write the iso picture
#ifdef BL_VOLUMERENDER
  if( ! (XmToggleButtonGetState(wAutoDraw) || showing3dRender )) {
    printImage = projPicturePtr->DrawBoxesIntoPixmap(minDrawnLevel, maxDrawnLevel);
  } else {
    printImage = projPicturePtr->GetPictureXImage();
  }
#else
  printImage = projPicturePtr->DrawBoxesIntoPixmap(minDrawnLevel, maxDrawnLevel);
#endif
  sprintf(rgbfilename, "%s.XYZ.rgb", fileNameBase);
  imageSizeX = projPicturePtr->ImageSizeH();
  imageSizeY = projPicturePtr->ImageSizeV();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);
# endif

  // write the palette
  sprintf(rgbfilename, "%s.pal.rgb", fileNameBase);
  printImage = pltPaletteptr->GetPictureXImage();
  imageSizeX = pltPaletteptr->PaletteWidth();
  imageSizeY = pltPaletteptr->PaletteHeight();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);

  XtDestroyWidget(w);

}  // end DoCreateRGBFile


// -------------------------------------------------------------------
void PltApp::DoCreateFABFile(Widget w, XtPointer, XtPointer call_data) {
  XmSelectionBoxCallbackStruct *cbs =
                              (XmSelectionBoxCallbackStruct *) call_data;
  char fabfilename[BUFSIZ];
  char *fileNameBase;
  XmStringGetLtoR(cbs->value, XmSTRING_DEFAULT_CHARSET, &fileNameBase);
  sprintf(fabfilename, "%s.fab", fileNameBase);
  aString fabFileName(fabfilename);
        
  aString derivedQuantity(amrPicturePtrArray[0]->CurrentDerived());
  Array<Box> bx = amrPicturePtrArray[0]->GetSubDomain();
  DataServices::Dispatch(DataServices::WriteFabOneVar,
			 dataServicesPtr[currentFrame],
                         (void *) &fabFileName,
			 (void *) &(bx[maxDrawnLevel]),
			 maxDrawnLevel,
			 (void *) &derivedQuantity);
  XtDestroyWidget(w);
}  // end DoCreateFABFile



// -------------------------------------------------------------------
void PltApp::DoCreateAnimRGBFile() {
  char rgbfilename[BUFSIZ];
  int imageSizeX, imageSizeY;
  XImage *printImage;
  const Array<XColor> &colors = pltPaletteptr->GetColorCells();
  char tempstr[BUFSIZ], timestep[BUFSIZ];
  char tempfilename[BUFSIZ];
  char cder[BUFSIZ];
  int i;

  ResetAnimation();

  strcpy(tempfilename, fileNames[currentFrame].c_str());
  i = strlen(tempfilename) - 1;
  while(i > -1 && tempfilename[i] != '/') {
    --i;
  }
  ++i;  // skip first (bogus) character

  FileType fileType = dataServicesPtr[currentFrame]->GetFileType();
  BL_ASSERT(fileType != INVALIDTYPE);
  if(fileType == FAB || fileType == MULTIFAB) {
    strcpy(timestep, &tempfilename[i]);
  } else {  // plt file
    strcpy(timestep, &tempfilename[i+3]);  // skip plt
  }
  i = 0;
  while(timestep[i] != '\0' && timestep[i] != '.') {
    ++i;
  }
  timestep[i] = '\0';
  strcpy(cder, currentDerived.c_str());
  sprintf(tempstr, "%s.%s", cder, timestep);
  sprintf(rgbfilename, "%s.rgb", tempstr);

  cout << "******* Creating file:  " << rgbfilename << endl;
  // write the picture
  printImage = amrPicturePtrArray[ZPLANE]->GetPictureXImage();
  imageSizeX = amrPicturePtrArray[ZPLANE]->ImageSizeH();
  imageSizeY = amrPicturePtrArray[ZPLANE]->ImageSizeV();
  WriteRGBFile(rgbfilename, printImage, imageSizeX, imageSizeY, colors);
}  // end DoCreateAnimRGBFile
// -------------------------------------------------------------------
// -------------------------------------------------------------------

