/*==========================================================================

  Portions (c) Copyright 2008-2014 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    vtkIGTLToMRMLPolyData.cxx

==========================================================================*/

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLPolyData.h"
#include "vtkMRMLIGTLQueryNode.h"

// OpenIGTLink includes
#include <igtl_util.h>
#include <igtlPolyDataMessage.h>

// Slicer includes
//#include <vtkSlicerColorLogic.h>
#include <vtkMRMLColorLogic.h>
#include <vtkMRMLColorTableNode.h>

// MRML includes
#include <vtkMRMLModelNode.h>
#include <vtkMRMLModelDisplayNode.h>

// VTK includes
#include <vtkPolyData.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkVertex.h>
#include <vtkCellArray.h>
#include <vtkPolyLine.h>
#include <vtkPolygon.h>
#include <vtkTriangleStrip.h>
#include <vtkFloatArray.h>
#include <vtkDataSetAttributes.h>
#include <vtkPointData.h>
#include <vtkCellData.h>


// VTKSYS includes
#include <vtksys/SystemTools.hxx>

#include "vtkSlicerOpenIGTLinkIFLogic.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkIGTLToMRMLPolyData);
//---------------------------------------------------------------------------
vtkIGTLToMRMLPolyData::vtkIGTLToMRMLPolyData()
{
  this->InPolyDataMessage = igtl::PolyDataMessage::New();
  this->mrmlNodeTagName = "Model";
}

//---------------------------------------------------------------------------
vtkIGTLToMRMLPolyData::~vtkIGTLToMRMLPolyData()
{
}

//---------------------------------------------------------------------------
void vtkIGTLToMRMLPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkIGTLToMRMLPolyData::CreateNewNode(vtkMRMLScene* scene, const char* name)
{
  // Create a node
  vtkMRMLNode* node = this->CreateMRMLNodeBaseOnTagName(scene);
  if (node)
    {
    vtkSmartPointer<vtkMRMLModelNode> modelNode;
    modelNode = vtkMRMLModelNode::SafeDownCast(node);
    modelNode->SetName(name);

    scene->SaveStateForUndo();

    vtkDebugMacro("Setting scene info");
    modelNode->SetScene(scene);
    modelNode->SetDescription("Received by OpenIGTLink");
    if(strcmp(node->GetNodeTagName(), "Model")==0)
      {      
      double color[3];
      color[0] = 0.5;
      color[1] = 0.5;
      color[2] = 1.0;
      // Display Node
      vtkSmartPointer< vtkMRMLModelDisplayNode > displayNode = vtkSmartPointer< vtkMRMLModelDisplayNode >::New();
      displayNode->SetColor(color);
      displayNode->SetOpacity(1.0);

      displayNode->SliceIntersectionVisibilityOn();  
      displayNode->VisibilityOn();

      scene->SaveStateForUndo();
      scene->AddNode(modelNode);
      scene->AddNode(displayNode);

      displayNode->SetScene(scene);
      modelNode->SetAndObserveDisplayNodeID(displayNode->GetID());
      modelNode->SetHideFromEditors(0);
      }
    else
      {
      modelNode->CreateDefaultDisplayNodes();
      }
    scene->AddNode(modelNode);
    return modelNode;
    }
  
  return NULL;
}


//---------------------------------------------------------------------------
vtkIntArray* vtkIGTLToMRMLPolyData::GetNodeEvents()
{
  vtkIntArray* events;

  events = vtkIntArray::New();
  events->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);

  return events;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::UnpackIGTLMessage(igtl::MessageBase::Pointer message)
{
  if (message.IsNull())
    {
    // TODO: error handling
    return 0;
    }
  this->InPolyDataMessage->Copy(message);
  
  // Deserialize the transform data
  // If CheckCRC==0, CRC check is skipped.
  int c = this->InPolyDataMessage->Unpack(this->CheckCRC);
  if ((c & igtl::MessageHeader::UNPACK_BODY) == 0) // if CRC check fails
    {
    // TODO: error handling
    return 0;
    }
  this->mrmlNodeTagName = "";
  if(this->InPolyDataMessage->GetHeaderVersion()==IGTL_HEADER_VERSION_2)
    {
    this->InPolyDataMessage->GetMetaDataElement(MEMLNodeNameKey, this->mrmlNodeTagName);
    }
  if(!(this->mrmlNodeTagName.compare("")==0))
    {
    // The user specified mrmlnode is not supported by the converter.
    if(!this->CheckIfMRMLSupported(this->mrmlNodeTagName.c_str()))
      {
      return 0;
      }
    }
  else
    {
    //The message is version1 or version 2 without meta information.
    this->mrmlNodeTagName = "Model";
    }
  return 1;
}

//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::IGTLToMRML(vtkMRMLNode* node)
{

  vtkMRMLModelNode* modelNode = vtkMRMLModelNode::SafeDownCast(node);

  if (modelNode==NULL)
    {
    vtkErrorMacro("vtkIGTLToMRMLPolyData::IGTLToMRML failed: invalid node");
    return 0;
    }
  
  igtlUint32 second;
  igtlUint32 nanosecond;
  this->InPolyDataMessage->GetTimeStamp(&second, &nanosecond);
  this->SetNodeTimeStamp(second, nanosecond, node);
  
  vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();

  if (poly.GetPointer()==NULL)
    {
    // TODO: Error handling
    }

  // Points
  igtl::PolyDataPointArray::Pointer pointsArray = this->InPolyDataMessage->GetPoints();
  int npoints = pointsArray->GetNumberOfPoints();
  if (npoints > 0)
    {
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (int i = 0; i < npoints; i ++)
      {
      igtlFloat32 point[3];
      pointsArray->GetPoint(i, point);
      points->InsertNextPoint(point); // TODO: use the id returned by this call?
      }
    poly->SetPoints(points);
    }
  else
    {
    // ERROR: No points defined
    }

  // Vertices
  igtl::PolyDataCellArray::Pointer verticesArray =  this->InPolyDataMessage->GetVertices();
  int nvertices = verticesArray.IsNotNull() ? verticesArray->GetNumberOfCells() : 0;
  if (nvertices > 0)
    {
    vtkSmartPointer<vtkCellArray> vertCells = vtkSmartPointer<vtkCellArray>::New();
    for (int i = 0; i < nvertices; i ++)
      {
      vtkSmartPointer<vtkVertex> vertex = vtkSmartPointer<vtkVertex>::New();

      std::list<igtlUint32> cell;
      verticesArray->GetCell(i, cell);
      //for (unsigned int j = 0; j < cell.size(); j ++) // TODO: is cell.size() always 1?
      //{
      std::list<igtlUint32>::iterator iter; 
      iter = cell.begin();
      vertex->GetPointIds()->SetId(i, *iter);
      //}
      vertCells->InsertNextCell(vertex);
      }
    poly->SetVerts(vertCells);
    }

  // Lines
  igtl::PolyDataCellArray::Pointer linesArray = this->InPolyDataMessage->GetLines();
  int nlines = linesArray.IsNotNull() ? linesArray->GetNumberOfCells() : 0;
  if (nlines > 0)
    {
    vtkSmartPointer<vtkCellArray> lineCells = vtkSmartPointer<vtkCellArray>::New();
    for(int i = 0; i < nlines; i++)
      {
      vtkSmartPointer<vtkPolyLine> polyLine = vtkSmartPointer<vtkPolyLine>::New();
      
      std::list<igtlUint32> cell;
      linesArray->GetCell(i, cell);
      polyLine->GetPointIds()->SetNumberOfIds(cell.size());
      std::list<igtlUint32>::iterator iter;
      int j = 0;
      for (iter = cell.begin(); iter != cell.end(); iter ++)
        {
        polyLine->GetPointIds()->SetId(j, *iter);
        j++;
        }
      lineCells->InsertNextCell(polyLine);    
      }
    poly->SetLines(lineCells);
    }

  // Polygons
  igtl::PolyDataCellArray::Pointer polygonsArray = this->InPolyDataMessage->GetPolygons();
  int npolygons = polygonsArray.IsNotNull() ? polygonsArray->GetNumberOfCells() : 0;
  if (npolygons > 0)
    {
    vtkSmartPointer<vtkCellArray> polygonCells = vtkSmartPointer<vtkCellArray>::New();
    for(int i = 0; i < npolygons; i++)
      {
      vtkSmartPointer<vtkPolygon> polygon = vtkSmartPointer<vtkPolygon>::New();

      std::list<igtlUint32> cell;
      polygonsArray->GetCell(i, cell);
      polygon->GetPointIds()->SetNumberOfIds(cell.size());
      std::list<igtlUint32>::iterator iter;
      int j = 0;
      for (iter = cell.begin(); iter != cell.end(); iter ++)
        {
        polygon->GetPointIds()->SetId(j, *iter);
        j++;
        }
      polygonCells->InsertNextCell(polygon);
      }
    poly->SetPolys(polygonCells);
    }

  // Triangle Strips
  igtl::PolyDataCellArray::Pointer triangleStripsArray = this->InPolyDataMessage->GetTriangleStrips();
  int ntstrips = triangleStripsArray.IsNotNull() ? triangleStripsArray->GetNumberOfCells() : 0;
  if (ntstrips > 0)
    {
    vtkSmartPointer<vtkCellArray> tstripCells = vtkSmartPointer<vtkCellArray>::New();
    for(int i = 0; i < ntstrips; i++)
      {
      vtkSmartPointer<vtkTriangleStrip> tstrip = vtkSmartPointer<vtkTriangleStrip>::New();

      std::list<igtlUint32> cell;
      triangleStripsArray->GetCell(i, cell);
      tstrip->GetPointIds()->SetNumberOfIds(cell.size());
      std::list<igtlUint32>::iterator iter;
      int j = 0;
      for (iter = cell.begin(); iter != cell.end(); iter ++)
        {
        tstrip->GetPointIds()->SetId(j, *iter);
        j++;
        }
      tstripCells->InsertNextCell(tstrip);
      }
    poly->SetStrips(tstripCells);
    }

  // Attribute
  int nAttributes = this->InPolyDataMessage->GetNumberOfAttributes();
  for (int i = 0; i < nAttributes; i ++)
    {
    igtl::PolyDataAttribute::Pointer attribute;    
    attribute = this->InPolyDataMessage->GetAttribute(igtl::PolyDataMessage::AttributeList::size_type(i));

    vtkSmartPointer<vtkFloatArray> data = 
      vtkSmartPointer<vtkFloatArray>::New();

    data->SetName(attribute->GetName()); //set the name of the value
    int n = attribute->GetSize();

    // NOTE: Data types for POINT (igtl::PolyDataMessage::POINT_*) and CELL
    // (igtl::PolyDataMessage::CELL_*) have the same lower 4 bit. 
    // By masking the values with 0x0F, attribute types (either SCALAR, VECTOR, NORMAL,
    // TENSOR, or RGBA) can be obtained. On the other hand, by masking the value
    // with 0xF0, data types (POINT or CELL) can be obtained.
    // See, igtlPolyDataMessage.h in the OpenIGTLink library.
    switch (attribute->GetType() & 0x0F)
      {
      case igtl::PolyDataAttribute::POINT_SCALAR:
        {
        data->SetNumberOfComponents(1);
        break;
        }
      case igtl::PolyDataAttribute::POINT_VECTOR:
      case igtl::PolyDataAttribute::POINT_NORMAL:
        {
        data->SetNumberOfComponents(3);
        break;
        }
      case igtl::PolyDataAttribute::POINT_TENSOR:
        {
        data->SetNumberOfComponents(9); // TODO: Is it valid in Slicer?
        break;
        }
      case igtl::PolyDataAttribute::POINT_RGBA:
        {
        data->SetNumberOfComponents(4); // TODO: Is it valid in Slicer?
        break;
        }
      default:
        {
        // ERROR
        break;
        }
      }
    data->SetNumberOfTuples(n);
    attribute->GetData(static_cast<igtl_float32*>(data->GetPointer(0)));

    if ((attribute->GetType() & 0xF0) == 0) // POINT
      {
      poly->GetPointData()->AddArray(data);
      }
    else // CELL
      {
      poly->GetCellData()->AddArray(data);
      }
    }


  modelNode->SetAndObservePolyData(poly);

  poly->Modified();
  modelNode->Modified();

  return 1;

}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::MRMLToIGTL(unsigned long event, vtkMRMLNode* mrmlNode, int* size, void** igtlMsg, bool useProtocolV2)
{
  if (!mrmlNode)
    {
    return 0;
    }

  // If mrmlNode is PolyData node
  if (event == vtkMRMLModelNode::PolyDataModifiedEvent && this->CheckIfMRMLSupported(mrmlNode->GetNodeTagName()))
    {
    vtkMRMLModelNode* modelNode =
      vtkMRMLModelNode::SafeDownCast(mrmlNode);

    if (!modelNode)
      {
      // TODO: the node not found
      return 0;
      }
    
    vtkSmartPointer<vtkPolyData> poly = modelNode->GetPolyData();
    if (poly.GetPointer() == NULL)
      {
      // TODO: poly data is not available
      return 0;
      }
    
    //------------------------------------------------------------
    // Allocate Status Message Class
    if (this->OutPolyDataMessage.IsNull())
      {
      this->OutPolyDataMessage = igtl::PolyDataMessage::New();
      }
    else
      {
      this->OutPolyDataMessage->Clear();
      }
    unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
    this->OutPolyDataMessage->SetHeaderVersion(headerVersion);
    this->OutPolyDataMessage->SetMetaDataElement(MEMLNodeNameKey, IANA_TYPE_US_ASCII, mrmlNode->GetNodeTagName());
    // Set message name -- use the same name as the MRML node 
    this->OutPolyDataMessage->SetDeviceName(modelNode->GetName());

    // Points
    vtkSmartPointer<vtkPoints> points = poly->GetPoints();
    if (points.GetPointer() != NULL)
      {
      int npoints = points->GetNumberOfPoints();
      if (npoints > 0)
        {
        igtl::PolyDataPointArray::Pointer pointArray = igtl::PolyDataPointArray::New();
        for (int i = 0; i < npoints; i ++)
          {
          double *p = points->GetPoint(i);
          pointArray->AddPoint(static_cast<igtlFloat32>(p[0]),
                               static_cast<igtlFloat32>(p[1]),
                               static_cast<igtlFloat32>(p[2]));
          }
        this->OutPolyDataMessage->SetPoints(pointArray);
        }
      }
    
    // Vertices
    vtkSmartPointer<vtkCellArray> vertCells = poly->GetVerts();
    if (vertCells.GetPointer() != NULL)
      {
      igtl::PolyDataCellArray::Pointer verticesArray = igtl::PolyDataCellArray::New();
      if (this->VTKToIGTLCellArray(vertCells, verticesArray) > 0)
        {
        this->OutPolyDataMessage->SetVertices(verticesArray);
        }
      }
      
    // Lines
    vtkSmartPointer<vtkCellArray> lineCells = poly->GetLines();
    if (lineCells.GetPointer() != NULL)
      {
      igtl::PolyDataCellArray::Pointer linesArray = igtl::PolyDataCellArray::New();
      if (this->VTKToIGTLCellArray(lineCells, linesArray) > 0)
        {
        this->OutPolyDataMessage->SetLines(linesArray);
        }
      }

    // Polygons
    vtkSmartPointer<vtkCellArray> polygonCells = poly->GetPolys();
    if (polygonCells.GetPointer() != NULL)
      {
      igtl::PolyDataCellArray::Pointer polygonsArray = igtl::PolyDataCellArray::New();
      if (this->VTKToIGTLCellArray(polygonCells, polygonsArray) > 0)
        {
        this->OutPolyDataMessage->SetPolygons(polygonsArray);
        }
      }

    // Triangl strips
    vtkSmartPointer<vtkCellArray> triangleStripCells = poly->GetStrips();
    if (triangleStripCells.GetPointer() != NULL)
      {
      igtl::PolyDataCellArray::Pointer triangleStripsArray = igtl::PolyDataCellArray::New();
      if (this->VTKToIGTLCellArray(triangleStripCells, triangleStripsArray) > 0)
        {
        this->OutPolyDataMessage->SetTriangleStrips(triangleStripsArray);
        }
      }

    // Attributes for points
    vtkSmartPointer<vtkPointData> pdata = poly->GetPointData();
    int nPointAttributes = pdata->GetNumberOfArrays();
    if (nPointAttributes > 0)
      {
      for (int i = 0; i < nPointAttributes; i ++)
        {
        igtl::PolyDataAttribute::Pointer attribute = igtl::PolyDataAttribute::New();
        if (this->VTKToIGTLAttribute(pdata, i, attribute) > 0)
          {
          this->OutPolyDataMessage->AddAttribute(attribute);
          }
        }
      }        


    // Attributes for cells
    vtkSmartPointer<vtkCellData> cdata = poly->GetCellData();
    int nCellAttributes = cdata->GetNumberOfArrays();
    if (nCellAttributes > 0)
      {
      for (int i = 0; i < nCellAttributes; i ++)
        {
        igtl::PolyDataAttribute::Pointer attribute = igtl::PolyDataAttribute::New();
        if (this->VTKToIGTLAttribute(cdata, i, attribute) > 0)
          {
          this->OutPolyDataMessage->AddAttribute(attribute);
          }
        }
      }        

    // Pack the message
    this->OutPolyDataMessage->Pack();

    *size = this->OutPolyDataMessage->GetPackSize();
    *igtlMsg = (void*)this->OutPolyDataMessage->GetPackPointer();

    return 1;
    }
  else if (strcmp(mrmlNode->GetNodeTagName(), "IGTLQuery") == 0)   // If mrmlNode is query node
    {
    vtkMRMLIGTLQueryNode* qnode = vtkMRMLIGTLQueryNode::SafeDownCast(mrmlNode);
    if (qnode)
      {
      if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_GET)
        {
        if (this->GetPolyDataMessage.IsNull())
          {
          this->GetPolyDataMessage = igtl::GetPolyDataMessage::New();
          }
        unsigned short headerVersion = useProtocolV2?IGTL_HEADER_VERSION_2:IGTL_HEADER_VERSION_1;
        this->GetPolyDataMessage->SetHeaderVersion(headerVersion);
        this->GetPolyDataMessage->SetDeviceName(qnode->GetIGTLDeviceName());
        this->GetPolyDataMessage->Pack();
        *size = this->GetPolyDataMessage->GetPackSize();
        *igtlMsg = this->GetPolyDataMessage->GetPackPointer();
        return 1;
        }
      /*
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_START)
        {
        *size = 0;
        return 0;
        }
      else if (qnode->GetQueryType() == vtkMRMLIGTLQueryNode::TYPE_STOP)
        {
        *size = 0;
        return 0;
        }
      */
      return 0;
      }
    }
  else
    {
    return 0;
    }

  return 0;
}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::VTKToIGTLCellArray(vtkCellArray* src, igtl::PolyDataCellArray* dest)
{

  if (src && dest)
    {
    int ncells = src->GetNumberOfCells();
    if (ncells > 0)
      {
      vtkSmartPointer<vtkIdList> idList = vtkSmartPointer<vtkIdList>::New();
      src->InitTraversal();
      while (src->GetNextCell(idList))
        {
        std::list<igtlUint32> cell;
        int nIds = idList->GetNumberOfIds();
        for (int i = 0; i < nIds; i ++)
          {
          cell.push_back(idList->GetId(i));
          }
        dest->AddCell(cell);
        }
      }
    return ncells;
    }
  else
    {
    return 0;
    }

}


//---------------------------------------------------------------------------
int vtkIGTLToMRMLPolyData::VTKToIGTLAttribute(vtkDataSetAttributes* src, int i, igtl::PolyDataAttribute* dest)
{

  //vtkSmartPointer<vtkPointData> src = poly->GetPointData();
  if ((!src) || (!dest))
    {
    return 0;
    }

  // Check the range of index i
  if (i < 0 || i >= src->GetNumberOfArrays())
    {
    return 0;
    }

  // NOTE: Data types for POINT (igtl::PolyDataMessage::POINT_*) and CELL
  // (igtl::PolyDataMessage::CELL_*) have the same bits exept the 3rd bit (0x10).
  // attrType will contain the 3rd bit based on the type of vtkDataSetAttributes
  // (either vtkCellData or vtkPointData). See, igtlPolyDataMessage.h in the OpenIGTLink library.
  int attrTypeBit;
  if (src->IsTypeOf("vtkCellData"))
    {
    attrTypeBit = 0x10;
    }
  else // vtkPointData
    {
    attrTypeBit = 0x00;
    }
  
  vtkSmartPointer<vtkDataArray> array = src->GetArray(i);
  int ncomps  = array->GetNumberOfComponents();
  if (ncomps == 1)
    {
    dest->SetType(igtl::PolyDataAttribute::POINT_SCALAR | attrTypeBit);
    }
  else if (ncomps == 3)
    {
    // TODO: how to differenciate normal and vector?
    dest->SetType(igtl::PolyDataAttribute::POINT_NORMAL | attrTypeBit);
    }
  else if (ncomps == 9)
    {
    dest->SetType(igtl::PolyDataAttribute::POINT_TENSOR | attrTypeBit);
    }
  else if (ncomps == 4)
    {
    dest->SetType(igtl::PolyDataAttribute::POINT_RGBA | attrTypeBit);
    }
  dest->SetName(array->GetName() ? array->GetName() : "");
  int ntuples = array->GetNumberOfTuples();
  dest->SetSize(ntuples);
  
  for (int j = 0; j < ntuples; j ++)
    {
    double * tuple = array->GetTuple(j);
    igtlFloat32 data[9];
    for (int k = 0; k < ncomps; k ++)
      {
      data[k] = static_cast<igtlFloat32>(tuple[k]);
      }
    dest->SetNthData(j, data);
    }

  return 1;

}
