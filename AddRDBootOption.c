#include "RD.h"
#include <Library/PcdLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Guid/FileInfo.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootManagerLib.h>
/**
  This function used to get the File Size 
  @param  EfiFileHandle -- filehandle of name.txt
  @retval 0 if read error, others success  
**/
UINTN
GetFileLength(
  IN EFI_FILE_HANDLE              EfiFileHandle
  )
{

  EFI_FILE_INFO                   *Buffer = NULL;
  Buffer = FileHandleGetInfo (EfiFileHandle);
  if (Buffer == NULL) {
    return 0;
  }
  return Buffer->FileSize;
}


CHAR16 *
GetRamDiskDesName (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                            Status;  
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL       *Volume;
  EFI_FILE_PROTOCOL                     *FileVol;
  EFI_FILE_PROTOCOL                     *File;
  UINTN                                DesSize;
  CHAR8                                 *DesData;
  CHAR16                                *DesName;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &Volume
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't load file system by handle status = %r\n", Status));
    return NULL;
  }
  //
  // Open the Volume to get the File System handle
  //
  Status = Volume->OpenVolume (Volume, &FileVol);   
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't open file handle = %r\n", Status));
    return NULL;
  } 
  
  //
  // Open the name.txt to found the boot option desp
  //
  Status = FileVol->Open (
                      FileVol,
                      &File,
                      L"name.txt",
                      EFI_FILE_MODE_READ,
                      0
                      );
  Print (L"Found name.txt status %r\n", Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't open file handle = %r\n", Status));
    return NULL;
  } 
//Print (L"123\n");
  //
  // Get the file size
  // 
  DesSize = (UINT32) GetFileLength (File);
  if (DesSize == 0) {
    return NULL;
  }
//Print (L"2\n");
  //
  // Get the file data
  // 
  DesData = (CHAR8  *)AllocateZeroPool (DesSize+1);
  if (DesData == NULL) {
    return NULL;
  }
//Print (L"3\n");
  Status = FileHandleRead (File, &DesSize, DesData);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't open file handle = %r\n", Status));
    return NULL;
  }
//Print (L"4 %a\n", DesData);
  DesName = (CHAR16  *) AllocateZeroPool ((DesSize+1)*2);
  if (DesName == NULL) {
    return NULL;
  }
//Print (L"5\n");
  UnicodeSPrint (DesName, DesSize*2, L"%a", DesData);
 // Print (L"6\n");
//  Print (DesName);
//  Print (L"6\n");
  if (DesData != NULL) {
    FreePool (DesData);
  }
//Print (L"6\n");
  return DesName;
}


BOOLEAN
IsHaveBootX64 (
  IN EFI_HANDLE  Handle
  )
{
  EFI_STATUS                            Status;  
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL       *Volume;
  EFI_FILE_PROTOCOL                     *FileVol;
  EFI_FILE_PROTOCOL                     *File;
  Status = gBS->HandleProtocol (
                  Handle,
                  &gEfiSimpleFileSystemProtocolGuid,
                  (VOID **) &Volume
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't load file system by handle status = %r\n", Status));
    return FALSE;
  }
  //
  // Open the Volume to get the File System handle
  //
  Status = Volume->OpenVolume (Volume, &FileVol);   
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't open file handle = %r\n", Status));
    return FALSE;
  } 
  
  //
  // Open the name.txt to found the boot option desp
  //
  Status = FileVol->Open (
                      FileVol,
                      &File,
                      L"\\EFI\\BOOT\\BOOTX64.EFI",
                      EFI_FILE_MODE_READ,
                      0
                      );
  //                    Print (L"Found BOOTX64.EFI status %r\n", Status);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "%a(%d) Can't open file handle = %r\n", Status));
    return FALSE;
  } 

  return TRUE;
}
BOOLEAN
IsRamDiskPath (
  IN EFI_DEVICE_PATH_PROTOCOL        *ParentPath
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  if (ParentPath == NULL) {
    DEBUG ((EFI_D_ERROR, "ParentDevicePath is NULL\n"));
    return FALSE;
  }
  ParentDevicePath = ParentPath;
  // Print (L"ParentDevicePath1 = %S\n", ConvertDevicePathToText (ParentDevicePath, TRUE, TRUE));
  do {
    
  //  Print (L"Path = %s\n", ConvertDevicePathToText (ParentDevicePath, TRUE, TRUE));
  //  Print (L"Path type = %x\n", DevicePathType (ParentDevicePath));
  //  Print (L"sub path type = %x\n", DevicePathSubType (ParentDevicePath));
    if ( DevicePathType (ParentDevicePath) == MEDIA_DEVICE_PATH &&
         DevicePathSubType (ParentDevicePath) == MEDIA_RAM_DISK_DP ) {
      DEBUG ((EFI_D_ERROR,"DevicePath match Ramdisk\n"));
      return TRUE;
    }
    ParentDevicePath = NextDevicePathNode (ParentDevicePath);
  } while (!IsDevicePathEnd (ParentDevicePath)) ;
  return FALSE;
}
/**
  Get the Option Number that wasn't used.

  @param  LoadOptionType      The load option type.
  @param  FreeOptionNumber    Return the minimal free option number.

  @retval EFI_SUCCESS           The option number is found and will be returned.
  @retval EFI_OUT_OF_RESOURCES  There is no free option number that can be used.
  @retval EFI_INVALID_PARAMETER FreeOptionNumber is NULL

**/
UINT16
GetFreeOptionNumber (
  VOID
  )
{

  UINTN         OptionNumber;
  UINTN         Index;
  UINT16        *OptionOrder;
  UINTN         OptionOrderSize;
  UINT16        *BootNext;


  GetEfiGlobalVariable2 (L"BootOrder", (VOID **) &OptionOrder, &OptionOrderSize);

  GetEfiGlobalVariable2 (L"BootNext", (VOID**) &BootNext, NULL);


  for (OptionNumber = 0;
       OptionNumber < OptionOrderSize / sizeof (UINT16)
                    + ((BootNext != NULL) ? 1 : 0);
       OptionNumber++
       ) {
    //
    // Search in OptionOrder whether the OptionNumber exists
    //
    for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
      if (OptionNumber == OptionOrder[Index]) {
        break;
      }
    }

    //
    // We didn't find it in the ****Order array and it doesn't equal to BootNext
    // Otherwise, OptionNumber equals to OptionOrderSize / sizeof (UINT16) + 1
    //
    if ((Index == OptionOrderSize / sizeof (UINT16)) &&
        ((BootNext == NULL) || (OptionNumber != *BootNext))
        ) {
      break;
    }
  }
  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  if (BootNext != NULL) {
    FreePool (BootNext);
  }

  return (UINT16) OptionNumber;

}

/**
  Create the Boot####, Driver####, SysPrep####, PlatformRecovery#### variable
  from the load option.

  @param  LoadOption      Pointer to the load option.

  @retval EFI_SUCCESS     The variable was created.
  @retval Others          Error status returned by RT->SetVariable.
**/
EFI_STATUS
EFIAPI
AddOptionToVariable (
  IN CHAR16                         *NameDescription,
  IN EFI_DEVICE_PATH_PROTOCOL       *FilePath,
  IN UINTN                          OptionDataSize,
  IN UINT8                          *OptionData,
  IN UINTN                          OptionNumber          
  )
{
  EFI_STATUS                       Status;
  UINTN                            VariableSize;
  UINT8                            *Variable;
  UINT8                            *Ptr;
  CHAR16                           OptionName[0X10];
  CHAR16                           *Description;
  CHAR16                           NullChar;
  UINT32                           VariableAttributes;

  //
  // Convert NULL description to empty description
  //
  NullChar    = L'\0';
  Description = NameDescription;
  if (Description == NULL) {
    Description = &NullChar;
  }

  
  /*
  UINT32                      Attributes;
  UINT16                      FilePathListLength;
  CHAR16                      Description[];
  EFI_DEVICE_PATH_PROTOCOL    FilePathList[];
  UINT8                       OptionalData[];
TODO: FilePathList[] IS:
A packed array of UEFI device paths.  The first element of the
array is a device path that describes the device and location of the
Image for this load option.  The FilePathList[0] is specific
to the device type.  Other device paths may optionally exist in the
FilePathList, but their usage is OSV specific. Each element
in the array is variable length, and ends at the device path end
structure.
  */
  VariableSize = sizeof (UINT32)
               + sizeof (UINT16)
               + StrSize (Description)
               + GetDevicePathSize (FilePath)
               + OptionDataSize;

  Variable     = AllocatePool (VariableSize);
  ASSERT (Variable != NULL);

  Ptr             = Variable;
  WriteUnaligned32 ((UINT32 *) Ptr, 0x00000001);
  Ptr            += sizeof (UINT32);

  WriteUnaligned16 ((UINT16 *) Ptr, (UINT16) GetDevicePathSize (FilePath));
  Ptr            += sizeof (UINT16);

  CopyMem (Ptr, Description, StrSize (Description));
  Ptr            += StrSize (Description);

  CopyMem (Ptr, FilePath, GetDevicePathSize (FilePath));
  Ptr            += GetDevicePathSize (FilePath);
  if (OptionData != NULL) {
    CopyMem (Ptr, OptionData, OptionDataSize);
  }
  UnicodeSPrint (OptionName, sizeof (OptionName), L"Boot%04x", OptionNumber);

  VariableAttributes = EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE;


  Status = gRT->SetVariable (
                  OptionName,
                  &gEfiGlobalVariableGuid,
                  VariableAttributes,
                  VariableSize,
                  Variable
                  );
  FreePool (Variable);

  return Status;
}

/**
  Update order variable .

  @param  OptionOrderName     Order variable name which need to be updated.
  @param  OptionNumber        Option number for the new option.
  @param  Position            Position of the new load option to put in the ****Order variable.

  @retval EFI_SUCCESS           The boot#### or driver#### have been successfully registered.
  @retval EFI_ALREADY_STARTED   The option number of Option is being used already.
  @retval EFI_STATUS            Return the status of gRT->SetVariable ().

**/
EFI_STATUS
AddBootOrderVariable (
  IN UINT16               OptionNumber
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINT16                  *OptionOrder;
  UINT16                  *NewOptionOrder;
  UINTN                   OptionOrderSize;
  //
  // Update the option order variable
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **) &OptionOrder, &OptionOrderSize);


  Status = EFI_SUCCESS;
  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    if (OptionOrder[Index] == OptionNumber) {
      Status = EFI_ALREADY_STARTED;
      break;
    }
  }

  if (!EFI_ERROR (Status)) {
    NewOptionOrder = AllocatePool (OptionOrderSize + sizeof (UINT16));
    ASSERT (NewOptionOrder != NULL);
    if (OptionOrderSize != 0) {
      CopyMem (NewOptionOrder, OptionOrder, OptionOrderSize);
    }
    NewOptionOrder[OptionOrderSize / sizeof (UINT16)] = OptionNumber;
    Status = gRT->SetVariable (
                    L"BootOrder",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    OptionOrderSize + sizeof (UINT16),
                    NewOptionOrder
                    );
    FreePool (NewOptionOrder);
  }

  if (OptionOrder != NULL) {
    FreePool (OptionOrder);
  }

  return Status;
}

BOOLEAN
EFIAPI
IsBootOptionCreated (
  IN  CHAR16       *Description,
  IN  EFI_HANDLE   Handle
  )
{
  UINTN                              Index;
  UINTN                              OptionOrderSize;
  UINT16                             *OptionOrder;
  CHAR16                             VariableName;
  UINT8                              *Variable;
  UINT8                              *VariablePtr;
  //
  // Update the option order variable
  //
  GetEfiGlobalVariable2 (L"BootOrder", (VOID **) &OptionOrder, &OptionOrderSize);
  if (Description == NULL) || (Handle == NULL) {
    return FALSE;
  }
  //
  // Boot000
  //
  VariableName = AllocateZeroPool (10);
  if (VariableName == NULL) {
    return FALSE;
  }
  for (Index = 0; Index < OptionOrderSize / sizeof (UINT16); Index++) {
    //
    //
    //
    ZeroMem (VariableName, 10);
    UnicodeSPrint (VariableName, 10, L"Boot%04x", OptionOrder[Index]);
    //
    // Read the variable
    //
    GetVariable2 (VariableName, gEfiGlobalVariableGuid, (VOID **) &Variable, &VariableSize);
    if (Variable == NULL) {
      return FALSE;
    }
  }
}
EFI_STATUS
EFIAPI
EnumerateRamDiskBootOption (
  VOID
  )
{
  EFI_STATUS                            Status;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *Handles;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;

  UINTN                                 Index;
  CHAR16                                *Description;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  BOOLEAN                               IsFound = FALSE;
  UINT16                                OptionNumber;



  //
  // Parse simple file system based on block io
  //
  Status = gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL, 
         &HandleCount,
         &Handles
         );
  if (EFI_ERROR (Status)) return Status;

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
    if (EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle not supports a BlkIo protocol, which we've handled in above
      //
      continue;
    }
    Status = gBS->HandleProtocol (
                    Handles[Index], 
                    &gEfiDevicePathProtocolGuid, 
                    (VOID **)&DevicePath
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }
    //
    // Check if matched RamDisk BootOption
    //
    if (!IsRamDiskPath (DevicePath)) {
      continue;
    }

    //
    // check if it has a //efi//boot//bootx64.efi
    //
    if (IsHaveBootX64 (Handles[Index])) {
      IsFound = TRUE;
      break;
    }
  }
  if (!IsFound) {
    return EFI_SUCCESS;
  }
  Description = GetRamDiskDesName (Handles[Index]);
  //Print (L"Description = %S\n", Description);
  OptionNumber = GetFreeOptionNumber ();
  //Print (L"OptionNumber = %x\n", OptionNumber);
  Print (L"Path = %s\n", ConvertDevicePathToText (DevicePathFromHandle (Handles[Index]), TRUE, TRUE));
  AddOptionToVariable (
    Description, 
    DuplicateDevicePath (DevicePathFromHandle (Handles[Index])),
    0,
    NULL,
    OptionNumber          
    );
  AddBootOrderVariable (OptionNumber);
  return EFI_SUCCESS;
}