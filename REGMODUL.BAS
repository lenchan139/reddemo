Attribute VB_Name = "REGMODUL"
' --------------------------------------------------------
' This .BAS module contains functions for reading and
'   setting registry values in Windows 95 and Windows NT.
'   They require CALL32.DLL in order to work.
'   Call32.DLL is likely available where you found this demo.
'   The demo is in the form of a functional registry editor,
'   although it is not meant to be a substitute for RegEdit
'   or RegEdt32.  Declares are not provided
'   for some rare registry functions, and not all functions
'   have demo usage implemented.  Only values of type REG_SZ
'   and REG_DWORD may be edited.
'
' Written 6/95 by Don Bradner with the invaluable assitance
'   of Karl Peterson and L.J. Johnson, section leaders in
'   the "32-Bit Bucket" section of the VBPJ Forum on Compuserve.
'
' This material is placed in the public domain.  No guarantees
'   are made, and no support is provided, but comments/bug
'   reports are welcome to Don Bradner at Compuserve 76130,1007
'   or at dbirdman@redshift.com.
'
' Warning:  Editing registry values can seriously impact your
'   computer's operations.  You should only edit values when
'   you know what they should be.  If editing values as a
'   test, make a note of the original value and restore it
'   when you are done.
' --------------------------------------------------------

Option Explicit

' --------------------------------------------------------
' FILETIME type is needed for RegEnumKey and
'   RegQueryInfoKey
' --------------------------------------------------------
Type FILETIME
    lLowDateTime    As Long
    lHighDateTime   As Long
End Type

Type SIZE
        cx As Long
        cy As Long
End Type

' --------------------------------------------------------
' 32-bit registry functions:
' --------------------------------------------------------
Declare Function RegOpenKeyExA& Lib "advapi32.dll" (ByVal hKey&, ByVal lpszSubKey$, dwOptions&, ByVal samDesired&, lpHKey&)
Declare Function RegCloseKey& Lib "advapi32.dll" (ByVal hKey&)
Declare Function RegQueryValueExA& Lib "advapi32.dll" (ByVal hKey&, ByVal lpszValueName$, ByVal lpdwRes&, lpdwType&, ByVal lpDataBuff$, nSize&)
Declare Function RegSetValueExA& Lib "advapi32.dll" (ByVal hKey&, ByVal lpszValueName$, ByVal dwRes&, ByVal dwType&, lpDataBuff As Any, ByVal nSize&)
Declare Function RegConnectRegistry& Lib "advapi32.dll" (ByVal lpMachineName$, ByVal hKey&, phkResult&)
Declare Function RegCreateKeyExA& Lib "advapi32.dll" (ByVal hKey&, ByVal lpSubKey$, ByVal Reserved&, ByVal lpClass$, ByVal dwOptions&, ByVal samDesired&, lpSecurityAttributes&, phkResult&, lpdwDisposition&)
Declare Function RegFlushKey& Lib "advapi32.dll" (ByVal hKey&)
Declare Function RegEnumKeyExA& Lib "advapi32.dll" (ByVal hKey&, ByVal dwIndex&, ByVal lpName$, lpcbName&, ByVal lpReserved&, ByVal lpClass$, lpcbClass&, lpftLastWriteTime As FILETIME)
Declare Function RegEnumValue& Lib "advapi32.dll" Alias "RegEnumValueA" (ByVal hKey&, ByVal dwIndex&, ByVal lpName$, lpcbName&, ByVal lpReserved&, lpdwType&, lpValue As Any, lpcbValue&)
Declare Function RegQueryInfoKey& Lib "advapi32.dll" Alias "RegQueryInfoKeyA" (ByVal hKey&, ByVal lpClass$, lpcbClass&, ByVal lpReserved&, lpcSubKeys&, lpcbMaxSubKeyLen&, lpcbMaxClassLen&, lpcValues&, lpcbMaxValueNameLen&, lpcbMaxValueLen&, lpcbSecurityDescriptor&, lpftLastWriteTime As FILETIME)


' --------------------------------------------------------
' Functions to determine what Windows we are running
' --------------------------------------------------------
Declare Function GetVersion& Lib "Kernel32" ()


' --------------------------------------------------------
' Functions used to set horizontal scrollbars in list
'   boxes
' --------------------------------------------------------
Declare Function GetTextExtentPoint& Lib "GDI32" Alias "GetTextExtentPointA" (ByVal hDC&, ByVal lpString$, ByVal cbString&, lpSize As SIZE)
Declare Function SendMessage& Lib "User32" Alias "SendMessageA" (ByVal hwnd&, ByVal wMsg&, ByVal wParam&, lParam&)


' --------------------------------------------------------
' Functions to pass address for huge memory array when
'   values are too large
' --------------------------------------------------------
Declare Function GlobalAlloc% Lib "Kernel32" (ByVal wFlags%, ByVal dwBytes&)
Declare Function GlobalFree% Lib "Kernel32" (ByVal hMem%)
Declare Function GlobalLock& Lib "Kernel32" (ByVal hMem%)
Declare Function GlobalUnlock% Lib "Kernel32" (ByVal hMem%)


' --------------------------------------------------------
' Functions used to show hourglass
' --------------------------------------------------------
Declare Function LoadCursor& Lib "User32" Alias "LoadCursorA" (ByVal hInstance&, ByVal lpCursor&)
Declare Function SetCursor% Lib "User32" (ByVal hCursor&)
Global Const IDC_WAIT = 32514&
Global Const LIST_INDENT = 2

Global iWaitCursor&


' --------------------------------------------------------
' Memory Flags for huge memory array functions
' --------------------------------------------------------
Const GMEM_MOVEABLE = &H2
Const GMEM_ZEROINIT = &H40


' --------------------------------------------------------
' Return codes from Registration functions.
' --------------------------------------------------------
Const ERROR_SUCCESS = 0&
Const ERROR_BADDB = 1009&
Const ERROR_BADKEY = 1010&
Const ERROR_CANTOPEN = 1011&
Const ERROR_CANTREAD = 1012&
Const ERROR_CANTWRITE = 1013&
Const ERROR_OUTOFMEMORY = 14&
Const ERROR_INVALID_PARAMETER = 87&
Const ERROR_ACCESS_DENIED = 5&
Const ERROR_NO_MORE_ITEMS = 259&
Const ERROR_MORE_DATA = 234&

Global Const HKEY_CLASSES_ROOT = &H80000000
Global Const HKEY_CURRENT_USER = &H80000001
Global Const HKEY_LOCAL_MACHINE = &H80000002
Global Const HKEY_USERS = &H80000003
Global Const HKEY_PERFORMANCE_DATA = &H80000004
Global Const HKEY_CURRENT_CONFIG = &H80000005
Global Const HKEY_DYN_DATA = &H80000006
   
Global Const LB_SETHORIZONTALEXTENT = &H400 + 21

Const REG_NONE = 0&                        ' No value type
Global Const REG_SZ = 1&                   ' Unicode nul terminated string
Const REG_EXPAND_SZ = 2&                   ' Unicode nul terminated string
                                           ' (with environment variable references)
Const REG_BINARY = 3&                      ' Free form binary
Global Const REG_DWORD = 4&                ' 32-bit number
Const REG_DWORD_LITTLE_ENDIAN = 4&         ' 32-bit number (same as REG_DWORD)
Const REG_DWORD_BIG_ENDIAN = 5&            ' 32-bit number
Const REG_LINK = 6&                        ' Symbolic Link (unicode)
Const REG_MULTI_SZ = 7&                    ' Multiple Unicode strings
Const REG_RESOURCE_LIST = 8&               ' Resource list in the resource map
Const REG_FULL_RESOURCE_DESCRIPTOR = 9&    ' Resource list in the hardware description
Const REG_RESOURCE_REQUIREMENTS_LIST = 10&


' --------------------------------------------------------
' Read/Write permissions:
' --------------------------------------------------------
Const KEY_QUERY_VALUE = &H1&
Const KEY_SET_VALUE = &H2&
Const KEY_CREATE_SUB_KEY = &H4&
Const KEY_ENUMERATE_SUB_KEYS = &H8&
Const KEY_NOTIFY = &H10&
Const KEY_CREATE_LINK = &H20&
Const READ_CONTROL = &H20000
Const WRITE_DAC = &H40000
Const WRITE_OWNER = &H80000
Const SYNCHRONIZE = &H100000
Const STANDARD_RIGHTS_REQUIRED = &HF0000
Const STANDARD_RIGHTS_READ = READ_CONTROL
Const STANDARD_RIGHTS_WRITE = READ_CONTROL
Const STANDARD_RIGHTS_EXECUTE = READ_CONTROL
Const KEY_READ = STANDARD_RIGHTS_READ Or KEY_QUERY_VALUE Or KEY_ENUMERATE_SUB_KEYS Or KEY_NOTIFY
Const KEY_WRITE = STANDARD_RIGHTS_WRITE Or KEY_SET_VALUE Or KEY_CREATE_SUB_KEY
Const KEY_EXECUTE = KEY_READ


' --------------------------------------------------------
' Used with GetVersion and GetWinFlags
' --------------------------------------------------------
Global Const WF_WINNT = &H4000&
Global Const WinNT = 3
Global Const Win32 = 2
Global Const win16 = 1
Global iWinVers%



' --------------------------------------------------------
' Global program variables
' --------------------------------------------------------
Global iTempInt%
Global lTempLong&
Global fTempDbl#
Global sTempString$
Global lCurrentType&
Global iList2Tab%


' --------------------------------------------------------
' BAS module variables
' --------------------------------------------------------

Sub CenterForm(FormName As Form)
    FormName.Move (Screen.Width - FormName.Width) / 2, (Screen.Height - FormName.Height) / 2
End Sub

Sub EditRegValue(lMain_Key&, sSub_Key$, lRegIndex&)

    ' --------------------------------------------------------
    ' Specific to the RegDemo application.  lRegIndex is
    '   the value location in listbox 2.
    ' --------------------------------------------------------
    Dim lRtn&       ' Returned by registry functions, should be 0&
    Dim hKey&       ' Return handle to opened key
    Dim sValueName$
    Dim lValueType&
    Dim sValueString$
    Dim lLenValueName&
    Dim lLenValue&
    Dim lKeyIndx&
   
    ' --------------------------------------------------------
    ' values for QueryInfoKey:
    ' --------------------------------------------------------
    Dim sClassName$
    Dim lClassLen&
    Dim lSubKeys&
    Dim lMaxSubKey&
    Dim lMaxClass&
    Dim lValues&
    Dim lMaxValueName&
    Dim lMaxValueData&
    Dim lSecurityDesc&
    Dim strucLastWriteTime As FILETIME

    
    ' -----------------------------------------------------
    ' Open key
    ' -----------------------------------------------------
    lRtn = RegOpenKeyExA(lMain_Key, sSub_Key, 0&, KEY_READ, hKey)
    If lRtn <> ERROR_SUCCESS Then
        MsgBox RtnRegError(lRtn)
      
        ' --------------------------------------------------
        ' No key open, so leave
        ' --------------------------------------------------
        Exit Sub
    End If
    
    ' -----------------------------------------------------
    ' RegQueryInfoKey is used to get the size of the largest
    '   value name and data string.
    ' Other returned values are ignored.
    ' -----------------------------------------------------
    sClassName = Space$(255) 'initialize these because occasional errors otherwise
    lClassLen = CLng(Len(sClassName))
    lRtn = RegQueryInfoKey(hKey, sClassName, lClassLen, 0&, lSubKeys, lMaxSubKey, lMaxClass, lValues, lMaxValueName, lMaxValueData, lSecurityDesc, strucLastWriteTime)

    '-------------------------------------------------------------------
    'If the enumeration fails due to a buffer over-run, we will loop back
    'to this point with larger buffers.
    '-------------------------------------------------------------------
RetryValueHere:
    
    '--------------------------------------------------------
    'Some registry values are too big to handle in a list box
    '--------------------------------------------------------
    If lMaxValueData > 20000 Then
        lMaxValueData = 20000
    End If
    
    ' --------------------------------------------------
    ' Set variables
    ' --------------------------------------------------
    sValueName = Space$(lMaxValueName + 1)
    lLenValueName = CLng(Len(sValueName)) '+ 1
    sValueString = Space$(lMaxValueData + 1)
    lLenValue = CLng(Len(sValueString))       '+ 1
    
    ' --------------------------------------------------
    ' Call the enumeration function to get the indexed value
    ' --------------------------------------------------
    lRtn = RegEnumValue(hKey, lRegIndex, sValueName, lLenValueName, 0&, lValueType, ByVal sValueString, lLenValue)
    
    
    ' --------------------------------------------------
    ' Check for success
    ' --------------------------------------------------
    If lRtn = ERROR_SUCCESS Then
        If lValueType <> REG_SZ And lValueType <> REG_DWORD Then
            
            '--------------------------------------------------------------
            'Tell us what value types may be edited, along with
            '    the type of value found.
            '--------------------------------------------------------------
            sTempString = "This Demo only supports editing of values with types of REG_SZ and REG_DWORD.  This value is of type "
            Select Case lValueType
                Case 2
                    sTempString = sTempString & "REG_EXPAND_SZ."
                Case 3
                    sTempString = sTempString & "REG_BINARY."
                Case 5
                    sTempString = sTempString & "REG_DWORD_BIG_ENDIAN."
                Case 6
                    sTempString = sTempString & "REG_LINK."
                Case 7
                    sTempString = sTempString & "REG_MULTI_SZ."
                Case 8
                    sTempString = sTempString & "REG_RESOURCE_LIST."
                Case 9
                    sTempString = sTempString & "REG_FULL_RESOURCE_DESCRIPTOR."
                Case 10
                    sTempString = sTempString & "REG_RESOURCE_REQUIREMENTS_LIST."
            End Select
            MsgBox sTempString

        Else
            sTempString = Mid$(sValueName, 1, lLenValueName)
            If lLenValueName = 0 Then
                sTempString = "(Default)"
            End If
            sValueString = Mid$(sValueString, 1, lLenValue)
            ' --------------------------------------------
            ' Convert DWORD 4 character value to 32-bit
            '   number.
            ' First character is low byte, and so on.
            ' --------------------------------------------
            Form2.Caption = "Edit String Value"
            If lValueType = REG_DWORD Then
                fTempDbl = Asc(Mid$(sValueString, 1, 1)) + &H100& * Asc(Mid$(sValueString, 2, 1)) + &H10000 * Asc(Mid$(sValueString, 3, 1)) + &H1000000 * CDbl(Asc(Mid$(sValueString, 4, 1)))
                If fTempDbl > &H7FFFFFFF Then
                    sValueString = Hex$(fTempDbl - 4294967296#)
                Else
                    sValueString = Hex$(fTempDbl)
                End If
                ' -----------------------------------------
                ' Turn on Hex/Decimal options
                ' -----------------------------------------
                Form2!Frame1.Visible = True
                Form2.Caption = "Edit DWORD Value"
            End If
            
            '---------------------------------------------
            'Store the type in a global variable for access
            '   from Form2
            '---------------------------------------------
            lCurrentType = lValueType
            
            ' --------------------------------------------
            ' Place the values in the form2 text boxes
            ' --------------------------------------------
            Form2!Text1 = sTempString
            Form2!text2 = sValueString
        
            ' --------------------------------------------
            ' Pass the key variables to form2 via hidden
            '   text boxes
            ' --------------------------------------------
            Form2!text3 = lMain_Key
            Form2!text4 = sSub_Key
            Form2.Show 1
        End If

    ElseIf lRtn = ERROR_MORE_DATA Then
        ' -----------------------------------------------
        ' This error means that, despite querying the key
        '   we have not set one of the buffers large
        '   enough. If the buffer is already 20000 we are
        '   not going to be able to edit it.
        ' -----------------------------------------------
        If lMaxValueData >= 20000 Then
            MsgBox ("Value is too large for this editor!")
        Else
        
            ' --------------------------------------------
            ' Increase the buffer sizes and try again
            ' --------------------------------------------
            lMaxValueData = lMaxValueData + 5
            lMaxValueName = lMaxValueName + 5
            GoTo RetryValueHere
        End If
    Else
        
        ' --------------------------------------------------
        ' Key still open, so display the error and fall
        '   thru to the close function below
        ' --------------------------------------------------
        MsgBox RtnRegError(lRtn)
    End If

    ' -----------------------------------------------------
    ' Always close opened keys!
    ' -----------------------------------------------------
    lRtn = RegCloseKey(hKey)

End Sub

Function GetMainKey&(keyname$)
   
   ' -----------------------------------------------------
   ' Used to convert main key strings to their values
   ' -----------------------------------------------------

    Select Case keyname
        Case "HKEY_CLASSES_ROOT"
            GetMainKey = HKEY_CLASSES_ROOT
        Case "HKEY_CURRENT_USER"
            GetMainKey = HKEY_CURRENT_USER
        Case "HKEY_LOCAL_MACHINE"
            GetMainKey = HKEY_LOCAL_MACHINE
        Case "HKEY_USERS"
            GetMainKey = HKEY_USERS
        Case "HKEY_PERFORMANCE_DATA"
            GetMainKey = HKEY_PERFORMANCE_DATA
        Case "HKEY_CURRENT_CONFIG"
            GetMainKey = HKEY_CURRENT_CONFIG
        Case "HKEY_DYN_DATA"
            GetMainKey = HKEY_DYN_DATA
    End Select

End Function

Function RegEnumKeys&(lMain_Key&, sSub_Key$, iLevels%, iStartlist%)
    iTempInt = SetCursor(iWaitCursor)
    
    ' --------------------------------------------------------
    ' This function will load all subkeys into listbox 1
    '   iLevels tells us how far to indent, while
    '   iStartList tells us where we are in listbox 1
    ' --------------------------------------------------------
    Dim lRtn&       ' Returned by registry functions, should be 0&
    Dim hKey&       ' Return handle to opened key
    Dim strucLastWriteTime    As FILETIME
    Dim sSubKeyName$
    Dim sClassString$
    Dim lLenSubKey&
    Dim lLenClass&
    Dim lKeyIndx&
    Dim lRet&
    Dim hKey2&
    Dim sSubKey2$
    
    '---------------------------------------------
    'values for QueryInfoKey:
    '---------------------------------------------
    Dim sClassName$
    Dim lClassLen&
    Dim lSubKeys&
    Dim lMaxSubKey&
    Dim lMaxClass&
    Dim lMaxSubKey2&  'Used for second QueryInfoKey in loop
    Dim lMaxClass2&   'Same
    Dim lValues&
    Dim lMaxValueName&
    Dim lMaxValueData&
    Dim lSecurityDesc&

    
    ' -----------------------------------------------------
    ' Open key
    ' -----------------------------------------------------
    lRtn = RegOpenKeyExA(lMain_Key, sSub_Key, 0&, KEY_READ, hKey)
    If lRtn <> ERROR_SUCCESS Then
        MsgBox RtnRegError(lRtn)
        RegEnumKeys = lRtn
        
        ' --------------------------------------------------
        ' No key open, so leave
        ' --------------------------------------------------
        Exit Function
    End If
    
    ' -----------------------------------------------------
    ' A call to RegQueryInfoKey will tell us the maximum
    '   keyname length
    ' -----------------------------------------------------
    sClassName = Space$(255)
    lClassLen = CLng(Len(sClassName))
    lRet = RegQueryInfoKey(hKey, sClassName, lClassLen, 0&, lSubKeys, lMaxSubKey, lMaxClass, lValues, lMaxValueName, lMaxValueData, lSecurityDesc, strucLastWriteTime)

    ' -----------------------------------------------------
    ' Enumerate the keys
    ' -----------------------------------------------------
    lKeyIndx = 0&
    Do While lRtn = ERROR_SUCCESS
        
        ' -----------------------------------------------------
        ' If the enumeration fails due to a buffer over-run,
        '   we will loop back to this point with larger buffers.
        ' -----------------------------------------------------
ReTryKeyEnumeration:
            
        ' --------------------------------------------------
        ' Set variables
        ' --------------------------------------------------
        sSubKeyName = Space$(lMaxSubKey + 1)
        lLenSubKey = CLng(Len(sSubKeyName))
        sClassString = Space$(lMaxClass + 1)
        lLenClass = CLng(Len(sClassString))
    
        
        ' --------------------------------------------------
        ' Call the enumeration function
        ' --------------------------------------------------
        lRtn = RegEnumKeyExA(hKey, lKeyIndx, sSubKeyName, lLenSubKey, 0&, sClassString, lLenClass, strucLastWriteTime)
        If InStr(sSubKeyName, Chr$(0)) > 1 Then
            sSubKeyName = Left$(sSubKeyName, InStr(sSubKeyName, Chr$(0)) - 1)
        End If

        
        ' --------------------------------------------------
        ' Check for success
        ' --------------------------------------------------
        If lRtn = ERROR_SUCCESS Then
            sSubKey2 = sSubKeyName
            If sSub_Key <> "" Then
                sSubKey2 = sSub_Key & "\" & sSubKeyName
            End If

            ' -----------------------------------------------
            ' Use RegQueryInfoKey to find out if this key has
            '   subkeys
            ' -----------------------------------------------
            lRet = RegOpenKeyExA(lMain_Key, sSubKey2, 0&, KEY_READ, hKey2)
            
            If lRet = ERROR_SUCCESS Then
                ' --------------------------------------------
                ' Need to initialize string, even though value
                '   not needed
                ' --------------------------------------------
                sClassName = Space$(255)
                lClassLen = CLng(Len(sClassName))
                lRet = RegQueryInfoKey(hKey2, sClassName, lClassLen, 0&, lSubKeys, lMaxSubKey2, lMaxClass2, lValues, lMaxValueName, lMaxValueData, lSecurityDesc, strucLastWriteTime)
                lRet = RegCloseKey(hKey2)
            End If
            
            ' -----------------------------------------------
            ' Indent 2 spaces per level for readability
            ' -----------------------------------------------
            sTempString = Space$((iLevels + 1) * 2)
            
            If lRet = ERROR_SUCCESS Then
                ' --------------------------------------------
                ' If there are subkeys, indicate with a +
                ' --------------------------------------------
                If lSubKeys > 0 Then
                    sTempString = sTempString & "+"
                End If
            ElseIf lRet = ERROR_ACCESS_DENIED Then
                ' --------------------------------------------
                ' If there will be a problem reading the key,
                '   indicate with an "*"
                ' --------------------------------------------
                sTempString = sTempString & "*"
            End If
            
            ' -----------------------------------------------
            ' Finish building the string and add it to the
            '   list box
            ' -----------------------------------------------
            sTempString = sTempString & Mid$(sSubKeyName, 1, lMaxSubKey)
            Form1!List1.AddItem sTempString, iStartlist
            iStartlist = iStartlist + 1
            
            ' -----------------------------------------------
            ' Increment the index and do it again
            ' -----------------------------------------------
            lKeyIndx = lKeyIndx + 1
        ElseIf lRtn = ERROR_MORE_DATA Then
            ' -----------------------------------------------
            ' This error means that, despite querying the key
            '   we have not set one of the buffers large
            '   enough.Increment the buffer sizes and try
            '   again
            ' -----------------------------------------------
            lMaxSubKey = lMaxSubKey + 5
            lMaxClass = lMaxClass + 5
            GoTo ReTryKeyEnumeration
        ElseIf lRtn = ERROR_NO_MORE_ITEMS Then
            ' -----------------------------------------------
            ' Not an error, just end of list -- exit the
            '   loop
            ' -----------------------------------------------
            lRtn = ERROR_SUCCESS
            Exit Do
        ElseIf lRtn <> ERROR_SUCCESS Then
            ' --------------------------------------------------
            ' Key still open, so display the error and fall
            '   thru to the close function below
            ' --------------------------------------------------
            MsgBox RtnRegError(lRtn)
            Exit Do
        End If
    Loop

    
    ' -----------------------------------------------------
    ' Set the return to the last error
    ' -----------------------------------------------------
    RegEnumKeys = lRtn


    ' -----------------------------------------------------
    ' Always close opened keys!
    ' -----------------------------------------------------
    lRtn = RegCloseKey(hKey)

End Function

Function RegEnumValues&(lMain_Key&, sSub_Key$)
    ' --------------------------------------------------------
    'lMain_Key must be one of the HKEY_ values, while sSub_Key
    'must exactly match a subkey string
    '- for instance: Control Panel\Desktop
    ' --------------------------------------------------------
    Dim lRtn&        ' Returned by registry functions, should be 0&
    Dim hKey&       ' Return handle to opened key
    Dim sValueName$
    Dim lValueType&
    Dim sValueString$
    Dim lLenValueName&
    Dim lLenValue&
    Dim lKeyIndx&
    Dim sBinaryString$
    
    '---------------------------
    'values for QueryInfoKey:
    '---------------------------
    Dim sClassName$
    Dim lClassLen&
    Dim lSubKeys&
    Dim lMaxSubKey&
    Dim lMaxClass&
    Dim lValues&
    Dim lMaxValueName&
    Dim lMaxValueData&
    Dim lSecurityDesc&
    Dim strucLastWriteTime As FILETIME
    
    Dim iListWidth%  'Used to set listbox scrollbar

    '---------------------------
    'values for huge memory blocks
    '---------------------------
    Dim lGlblAddr&
    Dim hGlblHnd%
    
    iTempInt = SetCursor(iWaitCursor)

       
    ' -----------------------------------------------------
    ' Open key
    ' -----------------------------------------------------
    lRtn = RegOpenKeyExA(lMain_Key, sSub_Key, 0&, KEY_READ, hKey)
    If lRtn <> ERROR_SUCCESS Then
        MsgBox RtnRegError(lRtn)
        RegEnumValues = lRtn
        
        ' --------------------------------------------------
        ' No key open, so leave
        ' --------------------------------------------------
        Exit Function
    End If
   
   ' -----------------------------------------------------
   ' Use RegQueryInfoKey to get the maximum value data info.
   ' -----------------------------------------------------
    sClassName = Space$(255)
    lClassLen = CLng(Len(sClassName))
    lRtn = RegQueryInfoKey(hKey, sClassName, lClassLen, 0&, lSubKeys, lMaxSubKey, lMaxClass, lValues, lMaxValueName, lMaxValueData, lSecurityDesc, strucLastWriteTime)
   
   ' -----------------------------------------------------
   ' Remove horizontal scrollbars
   ' -----------------------------------------------------
    iListWidth = 0
    iTempInt = SendMessage(Form1!List2.hwnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)

    ' -----------------------------------------------------
    ' Enumerate the keys
    ' -----------------------------------------------------
    lKeyIndx = 0&
    Do While lRtn = ERROR_SUCCESS
        sBinaryString = ""

'-------------------------------------------------------------------
'If the enumeration fails due to a buffer over-run, we will loop back
'to this point with larger buffers.
'-------------------------------------------------------------------
ReTryValueEnumeration:

        ' -----------------------------------------------------
        ' Some registry values can be extremely long, exceeding
        '   the size of a VB string and the capacity of the
        '   list box.  When that is the case, we use API calls
        '   to obtain a handle to a chunk of global memory.
        ' -----------------------------------------------------
        If lMaxValueData > 20000 Then
            hGlblHnd = GlobalAlloc(GMEM_ZEROINIT Or GMEM_MOVEABLE, lMaxValueData + 1)
            lGlblAddr = GlobalLock(hGlblHnd)
            sValueName = Space$(lMaxValueName + 1)
            lLenValueName = CLng(Len(sValueName))
            lLenValue = lMaxValueData + 1
            lRtn = RegEnumValue(hKey, lKeyIndx, sValueName, lLenValueName, 0&, lValueType, ByVal lGlblAddr, lLenValue)
            iTempInt = GlobalUnlock(hGlblHnd)
            hGlblHnd = GlobalFree(hGlblHnd)
            If lRtn = ERROR_NO_MORE_ITEMS Then
                ' -----------------------------------------------
                ' Not an error, just end of list -- exit the
                '   loop
                ' -----------------------------------------------
                lRtn = ERROR_SUCCESS
                Exit Do
            ElseIf lRtn <> ERROR_SUCCESS Then
                ' --------------------------------------------------
                ' Key still open, so display the error and fall
                '   thru to the close function below
                ' --------------------------------------------------
                MsgBox RtnRegError(lRtn)
                Exit Do
            End If
      
            ' --------------------------------------------------
            ' Although the max value data length for this key
            '   exceeds 20000, many or most values in the key
            '   may be under that limit.  Get those which are
            '   under the arbitrary limit.
            ' --------------------------------------------------
            If lLenValue < 20000 Then
                lLenValue = lLenValue + 1
                sValueString = Space$(lLenValue)
                lLenValueName = lLenValueName + 1
                sValueName = Space$(lLenValueName)
                lRtn = RegEnumValue(hKey, lKeyIndx, sValueName, lLenValueName, 0&, lValueType, ByVal sValueString, lLenValue)
            End If
    
        Else
            ' --------------------------------------------------
            ' Set variables
            ' --------------------------------------------------
            sValueName = Space$(lMaxValueName + 1)
            lLenValueName = CLng(Len(sValueName)) '+ 1
            sValueString = Space$(lMaxValueData + 1)
            lLenValue = CLng(Len(sValueString))       '+ 1
    
            
            ' --------------------------------------------------
            ' Call the enumeration function
            ' --------------------------------------------------
            lRtn = RegEnumValue(hKey, lKeyIndx, sValueName, lLenValueName, 0&, lValueType, ByVal sValueString, lLenValue)
            
        End If
        
        ' --------------------------------------------------
        ' Check for success
        ' --------------------------------------------------
        If lRtn = ERROR_SUCCESS Then
         
            ' -----------------------------------------------
            ' Start building the string to put in the list box:
            ' -----------------------------------------------
            sTempString = Mid$(sValueName, 1, lLenValueName)
        
            ' -----------------------------------------------
            ' Default values don't have a name.
            ' -----------------------------------------------
            If lLenValueName = 0 Then
                sTempString = "(Default)": lLenValueName = 9
            End If
         
            ' -----------------------------------------------
            ' This can happen if there is no value name
            '   greater than 8 characters.
            ' -----------------------------------------------
            If lMaxValueName < lLenValueName Then
                lMaxValueName = lLenValueName
            End If
            
            ' -----------------------------------------------
            ' If lLenValue is less than 20000, we have a
            '   value, otherwise just a name.
            ' -----------------------------------------------
            If lLenValue < 20000 Then
                sValueString = Mid$(sValueString, 1, lLenValue)
                Select Case lValueType
                    Case REG_MULTI_SZ
                        ' --------------------------------------
                        ' REG_MULTI_SZ strings are a series of
                        '   zero terminated strings. If we don't
                        '   strip out the zeros, only the first
                        '   one will display.
                        ' We will replace them with spaces.
                        ' --------------------------------------
                        Do While InStr(sValueString, Chr$(0))
                            sValueString = Left$(sValueString, InStr(sValueString, Chr$(0)) - 1) & " " & Right$(sValueString, Len(sValueString) - InStr(sValueString, Chr$(0)))
                        Loop
                    Case REG_SZ
                        ' --------------------------------------
                        ' REG_SZ values are zero-terminated
                        '   strings, and are the most common
                        '   values.
                        ' --------------------------------------
                        If lLenValue = 0 Then
                            lLenValue = 1
                        End If
                        
                        sValueString = """" & Left$(sValueString, lLenValue - 1) & """"
                    
                    Case REG_FULL_RESOURCE_DESCRIPTOR
                        ' --------------------------------------
                        ' Resource Descriptors require a special
                        '   editor to properly be displayed or
                        '   edited.
                        ' --------------------------------------
                        sValueString = "REG_FULL_RESOURCE_DESCRIPTOR"

                    Case REG_DWORD
                        ' --------------------------------------
                        ' REG_DWORD values are 32-bit unsigned
                        '   integers
                        ' Tortuous manipulation to make values
                        '   above 7FFFFFFF appear as positive
                        '   values.
                        ' VB Longs would display them as
                        '   negative numbers.
                        ' --------------------------------------
                        fTempDbl = Asc(Mid$(sValueString, 1, 1)) + &H100& * Asc(Mid$(sValueString, 2, 1)) + &H10000 * Asc(Mid$(sValueString, 3, 1)) + &H1000000 * CDbl(Asc(Mid$(sValueString, 4, 1)))
                        If fTempDbl > &H7FFFFFFF Then
                            sValueString = "&H" & Hex$(fTempDbl - 4294967296#)
                        Else
                            sValueString = "&H" & Hex$(fTempDbl)
                        End If
                        sValueString = sValueString & " (" & Format$(fTempDbl) & ")"

                    Case REG_BINARY
                  
                        ' --------------------------------------
                        ' Binary values may be of any length,
                        '   and may represent text or other data.
                        ' They require a special editor to
                        '   modify them.
                        ' --------------------------------------
                        For iTempInt = 1 To Len(sValueString)
                            sBinaryString = sBinaryString & Format$(Hex(Asc(Mid$(sValueString, iTempInt, 1))), "00") & " "
                        Next iTempInt
                        sValueString = sBinaryString
                End Select

                If Len(sValueString) = 0 Then
                    sValueString = "(value not set)"
                End If

            Else
                ' --------------------------------------------
                ' When lLenValue is greater than 20000
                '    we are'nt going to try to display it.
                ' --------------------------------------------
                sValueString = "(Value too large to display)"
            End If
         
            ' -----------------------------------------------
            ' Finish up the string and display it.
            ' -----------------------------------------------
            sTempString = sTempString & Space$(lMaxValueName - lLenValueName + 1) & sValueString
            Form1!List2.AddItem sTempString
         
            ' -----------------------------------------------
            ' Set listbox scrollbar based on max value size
            ' -----------------------------------------------
            Dim lpSize As SIZE
            GetTextExtentPoint Form1.hDC, sTempString, Len(sTempString), lpSize
            iTempInt = lpSize.cx
            
            If iTempInt > iListWidth Then
                iListWidth = iTempInt
                iTempInt = SendMessage(Form1!List2.hwnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)
            End If
         
            ' -----------------------------------------------
            ' Increment the key and do it again.
            ' -----------------------------------------------
            lKeyIndx = lKeyIndx + 1

        ElseIf lRtn = ERROR_MORE_DATA Then
            ' -----------------------------------------------
            ' This error means that, despite querying the key,
            '   we have not set one of the buffers large
            '   enough.  Increment the buffer sizes and try
            '   again
            ' -----------------------------------------------
            lMaxValueData = lMaxValueData + 5
            lMaxValueName = lMaxValueName + 5
            GoTo ReTryValueEnumeration

        ElseIf lRtn = ERROR_NO_MORE_ITEMS Then
            ' -----------------------------------------------
            ' Not an error, just end of list -- exit the
            '   loop
            ' -----------------------------------------------
            lRtn = ERROR_SUCCESS
            Exit Do

        Else
            ' --------------------------------------------------
            ' Key still open, so display the error and fall
            '   thru to the close function below
            ' --------------------------------------------------
            MsgBox RtnRegError(lRtn)
            Exit Do
        End If
    Loop
   
    ' -----------------------------------------------------
    ' Set the tab stop in case we edit any of the values
    '   and need to write new ones in alignment with the
    '   rest.
    ' -----------------------------------------------------
    iList2Tab = lMaxValueName

    
    ' -----------------------------------------------------
    ' Set the return to the last error
    ' -----------------------------------------------------
    RegEnumValues = lRtn


    ' -----------------------------------------------------
    ' Always close opened keys!
    ' -----------------------------------------------------
    lRtn = RegCloseKey(hKey)

End Function

Function RegGetValue$(lMain_Key&, sSub_Key$, sValue$)
    ' --------------------------------------------------------
    ' lMain_Key& must be one of the Globally declared HKEY
    '   constants.
    ' --------------------------------------------------------
    Dim sKeyType&       'returns the key type.  This function expects REG_SZ
    Dim lRtn&            'returned by registry functions, should be 0&
    Dim hKey&         'return handle to opened key
    Dim lData&       'length of data in returned string
    Dim sReturnedString$ 'returned string value

    '------------------------
    'values for QueryInfoKey:
    '------------------------
    Dim sClassName$
    Dim lClassLen&
    Dim lSubKeys&
    Dim lMaxSubKey&
    Dim lMaxClass&
    Dim lValues&
    Dim lMaxValueName&
    Dim lMaxValueData&
    Dim lSecurityDesc&
    Dim strucLastWriteTime As FILETIME

   
   ' -----------------------------------------------------
   ' Check lMain_Key& for validity
   ' -----------------------------------------------------
   If lMain_Key >= &H80000000 And lMain_Key <= &H80000006 Then
      
        ' --------------------------------------------------
        ' Open key
        ' --------------------------------------------------
        lRtn = RegOpenKeyExA(lMain_Key, sSub_Key, 0&, KEY_READ, hKey)
        If lRtn <> ERROR_SUCCESS Then
            MsgBox RtnRegError(lRtn)
            RegGetValue = ""
            Exit Function     'No key open, so leave
        End If
      
        ' --------------------------------------------------
        'use RegQueryInfoKey to get the maximum value data info.
        ' --------------------------------------------------
        sClassName = Space$(255)
        lClassLen = CLng(Len(sClassName))
        lRtn = RegQueryInfoKey(hKey, sClassName, lClassLen, 0&, lSubKeys, lMaxSubKey, lMaxClass, lValues, lMaxValueName, lMaxValueData, lSecurityDesc, strucLastWriteTime)
        
        ' --------------------------------------------------
        ' Set up buffer for data to be returned in.
        ' --------------------------------------------------
        sReturnedString = Space$(lMaxValueName + 1)
        lData = Len(sReturnedString)
        
        ' --------------------------------------------------
        ' Read key
        ' --------------------------------------------------
        lRtn = RegQueryValueExA(hKey, sValue, 0&, sKeyType, sReturnedString, lData)
        If lRtn <> ERROR_SUCCESS Then
            ' -----------------------------------------------
            ' Key still open, so finish up
            ' -----------------------------------------------
            MsgBox RtnRegError(lRtn)
            RegGetValue = ""
        Else
            RegGetValue = Left$(sReturnedString, lData - 1)
            lCurrentType = sKeyType
        End If


        ' --------------------------------------------------
        ' Always close opened keys!
        ' --------------------------------------------------
        lRtn = RegCloseKey(hKey)
    End If
End Function

Function RegSetValue&(lMain_Key&, sSub_Key$, sValueName$, sValue$)
    ' --------------------------------------------------------
    ' DWORD Values must be in Hex form for this function to
    '   work.
    ' lMain_Key& must be one of the Globally declared HKEY
    '   constants.
    ' --------------------------------------------------------
    Dim lRtn&            'returned by registry functions, should be 0&
    Dim hKey&         'return handle to opened key
    Dim iFirstChar%
    Dim iSecondChar%
    Dim iThirdChar%
    Dim iFourthChar%
    
    If lCurrentType <> REG_SZ And lCurrentType <> REG_DWORD Then
        MsgBox "This demo only supports writing keys of the types REG_SZ and REG_DWORD.  This key uses a different type."
        Exit Function
    End If
    
    ' -----------------------------------------------------
    ' Check MainKey for validity
    ' -----------------------------------------------------
    If lMain_Key >= &H80000000 And lMain_Key <= &H80000006 Then

        ' -----------------------------------------------------
        ' Open key
        ' -----------------------------------------------------
        lRtn = RegOpenKeyExA(lMain_Key, sSub_Key, 0&, KEY_WRITE, hKey)
        If lRtn <> ERROR_SUCCESS Then
            MsgBox RtnRegError(lRtn)
            RegSetValue = lRtn
            Exit Function  'No key open, so leave
        End If
      
        ' -----------------------------------------------------
        ' Write new sValue to key
        ' -----------------------------------------------------
        If lCurrentType = REG_DWORD Then
            sValue = Left(Trim(sValue), 8)
            If Left$(sValue, 2) <> "&H" Then
            sValue = "&H" & Left(Trim(sValue), 8)
            End If
            If Len(sValue) <= 6 Then
            sValue = sValue & "&"
            End If
        
            ' -----------------------------------------------------
                'Convert number string to 32-bit DWORD and save:
            ' -----------------------------------------------------
            lRtn = RegSetValueExA(hKey, sValueName, 0&, lCurrentType, CLng(Val(sValue)), 4&)
        Else

            ' -----------------------------------------------------
                'Save type REG_SZ (strings)
            ' -----------------------------------------------------
            lRtn = RegSetValueExA(hKey, sValueName, 0&, lCurrentType, ByVal sValue, CLng(Len(sValue)))
        End If
        If lRtn <> ERROR_SUCCESS Then
            MsgBox RtnRegError(lRtn) 'Key still open, so finish up
        End If
        RegSetValue = lRtn

        ' -----------------------------------------------------
        ' Always close opened keys!
        ' -----------------------------------------------------
        lRtn = RegCloseKey(hKey)
    End If
End Function

Private Function RtnRegError$(errorcode&)
    Select Case errorcode
        Case 1009, 1015
            ' -----------------------------------------------------
                'We're in trouble now
            ' -----------------------------------------------------
            RtnRegError = "The Registry Database is corrupt!"
        Case 2, 1010
            RtnRegError = "Bad Key Name!"
        Case 1011
            RtnRegError = "Can't Open Key"
        Case 4, 1012
            RtnRegError = "Can't Read Key"
        Case 5
            RtnRegError = "Access to this key is denied."
        Case 1013
            RtnRegError = "Can't Write Key"
        Case 8, 14
            RtnRegError = "Out of memory"
        Case 87
            RtnRegError = "Invalid Parameter"
        Case 234
            RtnRegError = "Error - There is more data than the buffer can handle!"
        Case Else
            RtnRegError = "Undefined Key Error Code" & Str$(errorcode) & "!"
    End Select
End Function

Function WordLo(lLongIn&) As Integer
    If (lLongIn And &HFFFF&) > &H7FFF Then
        WordLo = (lLongIn And &HFFFF&) - &H10000
    Else
        WordLo = lLongIn And &HFFFF&
    End If
End Function

