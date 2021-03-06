/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    vtkIGTLToMRMLPolyData.h

==========================================================================*/

#ifndef __vtkIGTLToMRMLPolyData_h
#define __vtkIGTLToMRMLPolyData_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"

// OpenIGTLink includes
#include <igtlPolyDataMessage.h>

// MRML includes
#include <vtkMRMLNode.h>

// VTK includes
#include <vtkObject.h>

class vtkCellArray;
class vtkDataSetAttributes;
class vtkMRMLVolumeNode;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkIGTLToMRMLPolyData : public vtkIGTLToMRMLBase
{
 public:

  static vtkIGTLToMRMLPolyData *New();
  vtkTypeMacro(vtkIGTLToMRMLPolyData,vtkObject);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual const char*  GetIGTLName() VTK_OVERRIDE { return "POLYDATA"; };
  virtual std::vector<std::string>  GetAllMRMLNames() VTK_OVERRIDE
  {
    this->MRMLNames.clear();
    this->MRMLNames.push_back("Model");
    this->MRMLNames.push_back("FiberBundle");
    return this->MRMLNames;
  }
  virtual vtkIntArray* GetNodeEvents() VTK_OVERRIDE;
  virtual vtkMRMLNode* CreateNewNode(vtkMRMLScene* scene, const char* name) VTK_OVERRIDE;

  virtual int          IGTLToMRML(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual int          MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg,  bool useProtocolV2) VTK_OVERRIDE;
  virtual int          UnpackIGTLMessage(igtl::MessageBase::Pointer buffer) VTK_OVERRIDE;

 protected:
  vtkIGTLToMRMLPolyData();
  ~vtkIGTLToMRMLPolyData();

  int IGTLToVTKScalarType(int igtlType);

  // Utility function for MRMLToIGTL(): Convert vtkCellArray to igtl::PolyDataCellArray
  int VTKToIGTLCellArray(vtkCellArray* src, igtl::PolyDataCellArray* dest);

  // Utility function for MRMLToIGTL(): Convert i-th vtkDataSetAttributes (vtkCellData and vtkPointData)
  // to igtl::PolyDataAttribute
  int VTKToIGTLAttribute(vtkDataSetAttributes* src, int i, igtl::PolyDataAttribute* dest);

 protected:
  igtl::PolyDataMessage::Pointer InPolyDataMessage;
  igtl::PolyDataMessage::Pointer OutPolyDataMessage;
  igtl::GetPolyDataMessage::Pointer GetPolyDataMessage;

};


#endif //__vtkIGTLToMRMLPolyData_h
