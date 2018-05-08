/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: vtkMRMLCurveAnalysisNode.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.3 $

=========================================================================auto=*/
#ifndef __vtkMRMLIGTLConnectorNode_h
#define __vtkMRMLIGTLConnectorNode_h

// OpenIGTLinkIF MRML includes
#include "vtkIGTLToMRMLBase.h"
#include "vtkMRMLIGTLQueryNode.h"
#include "vtkSlicerOpenIGTLinkIFModuleMRMLExport.h"
#include "vtkIGTLToMRMLConverterFactory.h"

// OpenIGTLink includes
#include <igtlServerSocket.h>
#include <igtlClientSocket.h>

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>
#include <vtkMRMLStorageNode.h>

// VTK includes
#include <vtkObject.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

// STD includes
#include <string>
#include <map>
#include <vector>
#include <set>

class vtkMultiThreader;
class vtkMutexLock;
class vtkIGTLCircularBuffer;
class vtkSlicerOpenIGTLinkIFLogic;

class VTK_SLICER_OPENIGTLINKIF_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorNode : public vtkMRMLNode
{

 public:

  //----------------------------------------------------------------
  // Constants Definitions
  //----------------------------------------------------------------

  // Events
  enum {
    ConnectedEvent        = 118944,
    DisconnectedEvent     = 118945,
    ActivatedEvent        = 118946,
    DeactivatedEvent      = 118947,
    ReceiveEvent          = 118948,
    NewDeviceEvent        = 118949,
    DeviceModifiedEvent   = 118950
  };

  enum {
    TYPE_NOT_DEFINED,
    TYPE_SERVER,
    TYPE_CLIENT,
    NUM_TYPE
  };

  static const char* ConnectorTypeStr[vtkMRMLIGTLConnectorNode::NUM_TYPE];

  enum {
    STATE_OFF,
    STATE_WAIT_CONNECTION,
    STATE_CONNECTED,
    NUM_STATE
  };

  static const char* ConnectorStateStr[vtkMRMLIGTLConnectorNode::NUM_STATE];
  
  enum {
    IO_UNSPECIFIED = 0x00,
    IO_INCOMING   = 0x01,
    IO_OUTGOING   = 0x02,
  };
  
  enum {
    PERSISTENT_OFF,
    PERSISTENT_ON,
  };

  typedef struct {
    std::string   name;
    std::string   type;
    int           io;
  } DeviceInfoType;

  typedef struct {
    int           lock;
    int           second;
    int           nanosecond;
  } NodeInfoType;

  typedef std::map<int, DeviceInfoType>   DeviceInfoMapType;   // Device list:  index is referred as
                                                               // a device id in the connector.
  typedef std::set<int>                   DeviceIDSetType;
  typedef std::vector< vtkSmartPointer<vtkMRMLNode> >       MRMLNodeListType;
  typedef std::map<std::string, NodeInfoType>       NodeInfoMapType;

 public:

  //----------------------------------------------------------------
  // Standard methods for MRML nodes
  //----------------------------------------------------------------

  static vtkMRMLIGTLConnectorNode *New();
  vtkTypeMacro(vtkMRMLIGTLConnectorNode,vtkMRMLNode);

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  // Description:
  // Set node attributes
  virtual void ReadXMLAttributes( const char** atts) VTK_OVERRIDE;

  // Description:
  // Write this node's information to a MRML file in XML format.
  virtual void WriteXML(ostream& of, int indent) VTK_OVERRIDE;

  // Description:
  // Copy the node's attributes to this object
  virtual void Copy(vtkMRMLNode *node) VTK_OVERRIDE;

  // Description:
  // Get node XML tag name (like Volume, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE
    {return "IGTLConnector";};

  // method to propagate events generated in mrml
  virtual void ProcessMRMLEvents ( vtkObject *caller, unsigned long event, void *callData ) VTK_OVERRIDE;

#ifndef __VTK_WRAP__
  //BTX
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  virtual void OnNodeReferenceRemoved(vtkMRMLNodeReference *reference) VTK_OVERRIDE;

  virtual void OnNodeReferenceModified(vtkMRMLNodeReference *reference) VTK_OVERRIDE;
  //ETX
#endif // __VTK_WRAP__

 protected:
  //----------------------------------------------------------------
  // Constructor and destroctor
  //----------------------------------------------------------------

  vtkMRMLIGTLConnectorNode();
  ~vtkMRMLIGTLConnectorNode();
  vtkMRMLIGTLConnectorNode(const vtkMRMLIGTLConnectorNode&);
  void operator=(const vtkMRMLIGTLConnectorNode&);


 public:
  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------

  vtkGetMacro( ServerPort, int );
  vtkSetMacro( ServerPort, int );
  vtkGetMacro( Type, int );
  vtkSetMacro( Type, int );
  vtkGetMacro( State, int );
  vtkSetMacro( RestrictDeviceName, int );
  vtkGetMacro( RestrictDeviceName, int );
  vtkSetMacro( LogErrorIfServerConnectionFailed, bool);
  vtkGetMacro( LogErrorIfServerConnectionFailed, bool);
  vtkSetMacro( UseProtocolV2, bool);
  vtkGetMacro( UseProtocolV2, bool);

  // Controls if active connection will be resumed when
  // scene is loaded (cf: PERSISTENT_ON/_OFF)
  vtkSetMacro( Persistent, int );
  vtkGetMacro( Persistent, int );

  const char* GetServerHostname();
  void SetServerHostname(std::string str);

  int SetTypeServer(int port);
  int SetTypeClient(std::string hostname, int port);

  vtkGetMacro( CheckCRC, int );
  void SetCheckCRC(int c);


  //----------------------------------------------------------------
  // Thread Control
  //----------------------------------------------------------------

  int Start();
  int Stop();
  static void* ThreadFunction(void* ptr);

  //----------------------------------------------------------------
  // OpenIGTLink Message handlers
  //----------------------------------------------------------------
  int WaitForConnection();
  int ReceiveController();
  int SendData(int size, unsigned char* data);
  int Skip(int length, int skipFully=1);

  //----------------------------------------------------------------
  // Circular Buffer
  //----------------------------------------------------------------

  typedef std::vector<std::string> NameListType;
  unsigned int GetUpdatedBuffersList(NameListType& nameList); // TODO: this will be moved to private
  vtkIGTLCircularBuffer* GetCircularBuffer(std::string& key);     // TODO: Is it OK to use device name as a key?

  //----------------------------------------------------------------
  // Device Lists
  //----------------------------------------------------------------

  // Description:
  // Import received data from the circular buffer to the MRML scne.
  // This is currently called by vtkOpenIGTLinkIFLogic class.
  void ImportDataFromCircularBuffer();

  // Description:
  // Import events from the event buffer to the MRML scene.
  // This is currently called by vtkOpenIGTLinkIFLogic class.
  void ImportEventsFromEventBuffer();

  // Description:
  // Push all outgoing messages to the network stream, if permitted.
  // This function is used, when the connection is established. To permit the OpenIGTLink IF
  // to push individual "outgoing" MRML nodes, set "OpenIGTLinkIF.pushOnConnection" attribute to 1. 
  void PushOutgoingMessages();

  // Description:
  // Set and start observing MRML node for outgoing data.
  // If devType == NULL, a converter is chosen based only on MRML Tag.
  int RegisterOutgoingMRMLNode(vtkMRMLNode* node, const char* devType=NULL);

  // Description:
  // Stop observing and remove MRML node for outgoing data.
  void UnregisterOutgoingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Register MRML node for incoming data.
  // Returns a pointer to the node information in IncomingMRMLNodeInfoList
  NodeInfoType* RegisterIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Unregister MRML node for incoming data.
  void UnregisterIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfOutgoingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetOutgoingMRMLNode(unsigned int i);

  // Description:
  // Get MRML to IGTL converter assigned to the specified MRML node ID
  vtkIGTLToMRMLBase* GetConverterByNodeID(const char* id);

  // Description:
  // Get number of registered outgoing MRML nodes:
  unsigned int GetNumberOfIncomingMRMLNodes();

  // Description:
  // Get Nth outgoing MRML nodes:
  vtkMRMLNode* GetIncomingMRMLNode(unsigned int i);

  // Description:
  // A function to explicitly push node to OpenIGTLink. The function is called either by 
  // external nodes or MRML event hander in the connector node. 
  int PushNode(vtkMRMLNode* node, int event=-1);

  // Description:
  // Push query int the query list.
  void PushQuery(vtkMRMLIGTLQueryNode* query);

  // Description:
  // Removes query from the query list.
  void CancelQuery(vtkMRMLIGTLQueryNode* node);

  //----------------------------------------------------------------
  // For OpenIGTLink time stamp access
  //----------------------------------------------------------------

  // Description:
  // Turn lock flag on to stop updating MRML node. Call this function before
  // reading the content of the MRML node and the corresponding time stamp.
  void LockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Turn lock flag off to start updating MRML node. Make sure to call this function
  // after reading the content / time stamp.
  void UnlockIncomingMRMLNode(vtkMRMLNode* node);

  // Description:
  // Get OpenIGTLink's time stamp information. Returns 0, if it fails to obtain time stamp.
  int GetIGTLTimeStamp(vtkMRMLNode* node, int& second, int& nanosecond);

  // Description:
  // Set the logic of OpenIGTLinkIF
  void SetOpenIGTLinkIFLogic(vtkSlicerOpenIGTLinkIFLogic* logic);


  // Description:
  // Get the logic of OpenIGTLinkIF
  vtkSlicerOpenIGTLinkIFLogic* GetOpenIGTLinkIFLogic();

  vtkIGTLToMRMLBase* GetConverterByMRMLTag(const char* tag);
  vtkIGTLToMRMLBase* GetConverterByIGTLDeviceType(const char* type);

  int RegisterMessageConverter(vtkIGTLToMRMLBase* converter);

  int UnregisterMessageConverter(vtkIGTLToMRMLBase* converter);

 protected:

  // Description:
  // Inserts the eventId to the EventQueue, and the event will be invoked from the main thread
  void RequestInvokeEvent(unsigned long eventId);

  // Description:
  // Reeust to push all outgoing messages to the network stream, if permitted.
  // This function is used, when the connection is established. To permit the OpenIGTLink IF
  // to push individual "outgoing" MRML nodes, set "OpenIGTLinkIF.pushOnConnection" attribute to 1. 
  // The request will be processed in PushOutgonigMessages().
  void RequestPushOutgoingMessages();

 protected:

  //----------------------------------------------------------------
  // Reference role strings
  //----------------------------------------------------------------
  char* IncomingNodeReferenceRole;
  char* IncomingNodeReferenceMRMLAttributeName;

  char* OutgoingNodeReferenceRole;
  char* OutgoingNodeReferenceMRMLAttributeName;

  vtkSetStringMacro(IncomingNodeReferenceRole);
  vtkGetStringMacro(IncomingNodeReferenceRole);
  vtkSetStringMacro(IncomingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(IncomingNodeReferenceMRMLAttributeName);

  vtkSetStringMacro(OutgoingNodeReferenceRole);
  vtkGetStringMacro(OutgoingNodeReferenceRole);
  vtkSetStringMacro(OutgoingNodeReferenceMRMLAttributeName);
  vtkGetStringMacro(OutgoingNodeReferenceMRMLAttributeName);

  //----------------------------------------------------------------
  // Connector configuration
  //----------------------------------------------------------------
  std::string Name;
  int Type;
  int State;
  int Persistent;
  bool LogErrorIfServerConnectionFailed;
  bool UseProtocolV2 = false;

  //----------------------------------------------------------------
  // Thread and Socket
  //----------------------------------------------------------------

  vtkMultiThreader* Thread;
  vtkMutexLock*     Mutex;
  igtl::ServerSocket::Pointer  ServerSocket;
  igtl::ClientSocket::Pointer  Socket;
  int               ThreadID;
  int               ServerPort;
  int               ServerStopFlag;

  std::string       ServerHostname;

  //----------------------------------------------------------------
  // Data
  //----------------------------------------------------------------


  typedef std::map<std::string, vtkIGTLCircularBuffer*> CircularBufferMap;
  CircularBufferMap Buffer;

  vtkMutexLock* CircularBufferMutex;
  int           RestrictDeviceName;  // Flag to restrict incoming and outgoing data by device names

  // Event queueing mechanism is needed to send all event notifications from the main thread.
  // Events can be pushed to the end of the EventQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  std::list<unsigned long> EventQueue;
  vtkMutexLock* EventQueueMutex;

  // Query queueing mechanism is needed to send all queries from the connector thread.
  // Queries can be pushed to the end of the QueryQueue by calling RequestInvoke from any thread,
  // and they will be Invoked in the main thread.
  // Use a weak pointer to make sure we don't try to access the query node after it is deleted from the scene.
  std::list< vtkWeakPointer<vtkMRMLIGTLQueryNode> > QueryWaitingQueue;
  vtkMutexLock* QueryQueueMutex;

  // Flag for the push outoing message request
  // If the flag is ON, the external timer will update the outgoing nodes with 
  // "OpenIGTLinkIF.pushOnConnection" attribute to push the nodes to the network.
  int PushOutgoingMessageFlag;
  vtkMutexLock* PushOutgoingMessageMutex;

  // -- Device Name (same as MRML node) and data type (data type string defined in OpenIGTLink)
  DeviceIDSetType   IncomingDeviceIDSet;
  DeviceIDSetType   OutgoingDeviceIDSet;
  DeviceIDSetType   UnspecifiedDeviceIDSet;

  // Message converter (IGTLToMRML)
  vtkIGTLToMRMLConverterFactory* converterFactory;

  // List of nodes that this connector node is observing.
  //MRMLNodeListType         OutgoingMRMLNodeList;      // TODO: -> ReferenceList
  //NodeInfoListType         IncomingMRMLNodeInfoList;  // TODO: -> ReferenceList with attributes (.lock, .second, .nanosecond)
  NodeInfoMapType          IncomingMRMLNodeInfoMap;

  int CheckCRC;

  vtkSlicerOpenIGTLinkIFLogic* OpenIGTLinkIFLogic;

};

#endif

