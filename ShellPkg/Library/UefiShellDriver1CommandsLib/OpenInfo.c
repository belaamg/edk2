/** @file
  Main file for OpenInfo shell Driver1 function.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST CHAR16 StringHandProt[]  = L"HandProt ";
STATIC CONST CHAR16 StringGetProt[]   = L"GetProt  ";
STATIC CONST CHAR16 StringTestProt[]  = L"TestProt ";
STATIC CONST CHAR16 StringChild[]     = L"Child    ";
STATIC CONST CHAR16 StringDriver[]    = L"Driver   ";
STATIC CONST CHAR16 StringExclusive[] = L"Exclusive";
STATIC CONST CHAR16 StringDriverEx[]  = L"DriverEx ";
STATIC CONST CHAR16 StringUnknown[]   = L"Unknown  ";

EFI_STATUS
EFIAPI
TraverseHandleDatabase (
  IN CONST EFI_HANDLE TheHandle
  )
{
  EFI_STATUS                          Status;
  EFI_GUID                            **ProtocolGuidArray;
  UINTN                               ArrayCount;
  UINTN                               ProtocolIndex;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY *OpenInfo;
  UINTN                               OpenInfoCount;
  UINTN                               OpenInfoIndex;
  CONST CHAR16                        *OpenTypeString;
  CHAR16                              *TempString;
  UINTN                               HandleIndex;
  CONST CHAR16                        *Name;

  ASSERT(TheHandle != NULL);

  //
  // Retrieve the list of all the protocols on the handle
  //
  Status = gBS->ProtocolsPerHandle (
                TheHandle,
                &ProtocolGuidArray,
                &ArrayCount
               );
  ASSERT_EFI_ERROR(Status);
  if (!EFI_ERROR (Status)) {

    for (ProtocolIndex = 0; ProtocolIndex < ArrayCount; ProtocolIndex++) {
      //
      // print out the human readable name for this one.
      //
      TempString = GetStringNameFromGuid(ProtocolGuidArray[ProtocolIndex], NULL);
      if (TempString == NULL) {
        continue;
      }
      ShellPrintEx(-1, -1, L"%H%s%N\r\n", TempString);
      FreePool(TempString);

      //
      // Retrieve the list of agents that have opened each protocol
      //
      Status = gBS->OpenProtocolInformation (
                    TheHandle,
                    ProtocolGuidArray[ProtocolIndex],
                    &OpenInfo,
                    &OpenInfoCount
                   );
      ASSERT_EFI_ERROR(Status);
      if (!EFI_ERROR (Status)) {
        for (OpenInfoIndex = 0; OpenInfoIndex < OpenInfoCount; OpenInfoIndex++) {
          switch (OpenInfo[OpenInfoIndex].Attributes) {
            case EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL:  OpenTypeString = StringHandProt;  break;
            case EFI_OPEN_PROTOCOL_GET_PROTOCOL:        OpenTypeString = StringGetProt;   break;
            case EFI_OPEN_PROTOCOL_TEST_PROTOCOL:       OpenTypeString = StringTestProt;  break;
            case EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER: OpenTypeString = StringChild;     break;
            case EFI_OPEN_PROTOCOL_BY_DRIVER:           OpenTypeString = StringDriver;    break;
            case EFI_OPEN_PROTOCOL_EXCLUSIVE:           OpenTypeString = StringExclusive; break;
            case EFI_OPEN_PROTOCOL_BY_DRIVER|EFI_OPEN_PROTOCOL_EXCLUSIVE:
                                                        OpenTypeString = StringDriverEx;  break;
            default:                                    OpenTypeString = StringUnknown;   break;
          }
          HandleIndex = ConvertHandleToHandleIndex(OpenInfo[OpenInfoIndex].AgentHandle);
          Name        = GetStringNameFromHandle(OpenInfo[OpenInfoIndex].AgentHandle, "en");
          if (OpenInfo[OpenInfoIndex].ControllerHandle!=NULL) {
            ShellPrintHiiEx(
              -1,
              -1,
              NULL,
              STRING_TOKEN(STR_OPENINFO_LINE),
              gShellDriver1HiiHandle,
              HandleIndex,
              ConvertHandleToHandleIndex(OpenInfo[OpenInfoIndex].ControllerHandle),
              OpenInfo[OpenInfoIndex].OpenCount,
              OpenTypeString,
              Name
             );
          } else {
            ShellPrintHiiEx(
              -1,
              -1,
              NULL,
              STRING_TOKEN(STR_OPENINFO_MIN_LINE),
              gShellDriver1HiiHandle,
              HandleIndex,
              OpenInfo[OpenInfoIndex].OpenCount,
              OpenTypeString,
              Name
             );            
          }
        }
        FreePool (OpenInfo);
      }
    }
    FreePool (ProtocolGuidArray);
  }

  return Status;
}

/**
  Function for 'openinfo' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunOpenInfo (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  EFI_HANDLE          TheHandle;
  CONST CHAR16        *Param1;

  ShellStatus         = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 2){
      //
      // error for too many parameters
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) == 0) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Param1 = ShellCommandLineGetRawValue(Package, 1);
      if (Param1 == NULL || ConvertHandleIndexToHandle(StrHexToUintn(Param1)) == NULL){
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        TheHandle = ConvertHandleIndexToHandle(StrHexToUintn(Param1));
        ASSERT(TheHandle != NULL);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_OPENINFO_HEADER_LINE), gShellDriver1HiiHandle, StrHexToUintn(Param1), TheHandle);

        Status = TraverseHandleDatabase (TheHandle);
        if (!EFI_ERROR(Status)) {
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, Param1);
          ShellStatus = SHELL_NOT_FOUND;
        }
      }
    }
  }
  return (ShellStatus);
}