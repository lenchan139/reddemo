VERSION 4.00
Begin VB.Form Form1 
   Appearance      =   0  'Flat
   BackColor       =   &H00C0C0C0&
   Caption         =   "Win32 RegDemo"
   ClientHeight    =   4980
   ClientLeft      =   1200
   ClientTop       =   1665
   ClientWidth     =   7590
   BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
      Name            =   "Courier New"
      Size            =   8.25
      Charset         =   0
      Weight          =   400
      Underline       =   0   'False
      Italic          =   0   'False
      Strikethrough   =   0   'False
   EndProperty
   ForeColor       =   &H80000008&
   Height          =   5385
   Icon            =   "REGDEMO.frx":0000
   Left            =   1140
   LinkTopic       =   "Form1"
   ScaleHeight     =   4980
   ScaleWidth      =   7590
   Top             =   1320
   Width           =   7710
   Begin VB.CommandButton Command1 
      Appearance      =   0  'Flat
      BackColor       =   &H80000005&
      Caption         =   "Exit"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      Height          =   372
      Left            =   6180
      TabIndex        =   8
      Top             =   4560
      Width           =   972
   End
   Begin VB.ListBox List2 
      Appearance      =   0  'Flat
      Height          =   3735
      Left            =   3540
      TabIndex        =   1
      Top             =   300
      Width           =   3975
   End
   Begin VB.ListBox List1 
      Appearance      =   0  'Flat
      Height          =   3735
      Left            =   0
      TabIndex        =   0
      Top             =   300
      Width           =   3495
   End
   Begin VB.Label Label5 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "* means key cannot be opened"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   252
      Index           =   1
      Left            =   240
      TabIndex        =   7
      Top             =   4680
      Width           =   2772
   End
   Begin VB.Label Label5 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "+ means additional keys"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   252
      Index           =   0
      Left            =   240
      TabIndex        =   6
      Top             =   4440
      Width           =   2772
   End
   Begin VB.Label Label4 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Double-Click a Value to edit it"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   252
      Left            =   4020
      TabIndex        =   5
      Top             =   4200
      Width           =   3312
   End
   Begin VB.Label Label3 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Double-Click a Key to expand it"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   252
      Left            =   120
      TabIndex        =   4
      Top             =   4200
      Width           =   3612
   End
   Begin VB.Label Label2 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Data"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   192
      Left            =   3660
      TabIndex        =   3
      Top             =   60
      Width           =   1692
   End
   Begin VB.Label Label1 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Key Name"
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   700
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
      ForeColor       =   &H80000008&
      Height          =   192
      Left            =   60
      TabIndex        =   2
      Top             =   60
      Width           =   1272
   End
End
Attribute VB_Name = "Form1"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Option Explicit
Dim lMainKey&
Dim sSubKey$
Dim iLevel%
Dim iX%
Dim iY%
Dim sLevelSpaces$
Dim iListIdx%

Private Sub Command1_Click()
    'We want out!
    Unload Form1
End Sub

Private Sub Form_Load()
    Dim lVerNum&
    Dim iVerWord%
    Dim iVersNum%
    
    CenterForm Me
    
    '-----------------------------------------------------
    'First we find out what Windows is running.  This will
    'not work with 16-bit Windows 3.x, and there are a
    'couple of registry keys in Win95 that are not present
    'in Windows NT
    '-----------------------------------------------------
    lVerNum = GetVersion()
    iVerWord = CInt(lVerNum And &HFFFF&)
    iVersNum = (iVerWord And &HFF) * 100 + CInt(iVerWord / 256)
        If lVerNum < &H80000000 Then
            iWinVers = WinNT
        Else
            iWinVers = Win32
        End If
    
    '-----------------------------------------------------
    'Set all fonts to same, so horizontal scrollbars will work
    'Fixed fonts like Small Fonts or Courier will provide even tabs.
    '-----------------------------------------------------
    List1.FontName = FontName
    List1.FontSize = FontSize
    List2.FontName = FontName
    List2.FontSize = FontSize
    
    '-----------------------------------------------------
    'Put in global main keys
    '-----------------------------------------------------
    List1.AddItem "HKEY_CLASSES_ROOT"
    List1.AddItem "HKEY_CURRENT_USER"
    List1.AddItem "HKEY_LOCAL_MACHINE"
    List1.AddItem "HKEY_USERS"
    
    '-----------------------------------------------------
    'Put in version-specific main keys
    'HKEY_PERFORMANCE_DATA does not respond in a normal fashion
    'to key queries.  Note it does not appear in REGEDT32.
    'If iWinVers = WinNT Then
        'list1.AddItem "HKEY_PERFORMANCE_DATA"
    'End If
    '-----------------------------------------------------
    If iWinVers = Win32 Then
        List1.AddItem "HKEY_CURRENT_CONFIG"
        List1.AddItem "HKEY_DYN_DATA"
    End If
    
    '-----------------------------------------------------
    'Setup hourglass cursor
    '-----------------------------------------------------
    iWaitCursor = LoadCursor(0, IDC_WAIT)

End Sub

Private Sub Form_Resize()

    '-----------------------------------------------------
    'Resize controls when the user resizes the form.
    '-----------------------------------------------------
    On Error GoTo ReSizeError
    List1.Move 60, List1.Top, (ScaleWidth / 2) - 300, ScaleHeight - 1200
    List2.Move List1.Width + 90, List1.Top, List1.Width + 480, List1.Height
    
    Label1.Left = List1.Left + 60
    Label2.Left = List2.Left + 60
    
    Label3.Move 90, List1.Top + List1.Height + 150
    Label4.Move List2.Left + 90, Label3.Top
    
    Label5(0).Top = Label3.Top + 240
    Label5(1).Top = Label5(0).Top + 240
    
    Command1.Move ScaleWidth - Command1.Width - 120, Label4.Top + 330
    Exit Sub
ReSizeError:
    Exit Sub

End Sub

Private Sub List1_Click()

    '-----------------------------------------------------
    'Empty the value list.
    '-----------------------------------------------------
    List2.Clear
    
    '-----------------------------------------------------
    'Remember which line was clicked when we start
    '   moving up and down the list.
    '-----------------------------------------------------
    iListIdx = List1.ListIndex
    
    '-----------------------------------------------------
    'Start by assuming toplevel key:
    '-----------------------------------------------------
    iLevel = 0
    
    If Left$(List1.List(List1.ListIndex), 1) <> Chr$(32) Then
        
        '-------------------------------------------------
        'Subkeys will all have spaces at their beginning (because
        'that is how we will add them), so this is a top level key.
        '-------------------------------------------------
        lMainKey = GetMainKey(CStr(List1.List(List1.ListIndex)))
        sSubKey = ""
    Else
        
        '-------------------------------------------------
        'This must be a subkey.
        '-------------------------------------------------
        sSubKey = List1.List(List1.ListIndex)
    
        '-------------------------------------------------
        'Strip the leading spaces and count how many
        '-------------------------------------------------
        Do Until Left$(sSubKey, 1) <> Chr$(32)
            iLevel = iLevel + 1
            sSubKey = Right$(sSubKey, Len(sSubKey) - 1)
        Loop
    
        '-------------------------------------------------
        'if there are any indicator characters which we have
        'introduced, strip them off.
        '-------------------------------------------------
        If Left$(sSubKey, 1) = "+" Or Left$(sSubKey, 1) = "*" Then
            sSubKey = Right$(sSubKey, Len(sSubKey) - 1)
        End If
    
        '-------------------------------------------------
        'We indented LIST_INDENT = 2 spaces per level when we added this subkey.
        '-------------------------------------------------
        iLevel = iLevel / LIST_INDENT
        For iX = iLevel To 1 Step -1
            
            '---------------------------------------------
            'Build the complete key name by moving up the
            'tree getting subkey names each time the indentation
            'changes, until we run into a main key (no leading spaces).
            '---------------------------------------------
            sLevelSpaces = Space$(iX * LIST_INDENT)
            iY = iListIdx
            Do Until Left$(List1.List(iY), Len(sLevelSpaces)) <> sLevelSpaces
                iY = iY - 1
            Loop
            If Left$(List1.List(iY), 1) <> Chr$(32) Then
                lMainKey = GetMainKey(CStr(List1.List(iY)))
            Else
                sSubKey = LTrim$(List1.List(iY)) & "\" & sSubKey
                If Left$(sSubKey, 1) = "+" Then
                    sSubKey = Right$(sSubKey, Len(sSubKey) - 1)
                End If
            End If
        Next
    End If
    
    '-----------------------------------------------------
    'Now enumerate all values belonging to this key
    '-----------------------------------------------------
    lTempLong = RegEnumValues(lMainKey, sSubKey)
    
    '-----------------------------------------------------
    'If the user double-clicked, we will proceed on
    '    to that event.
    '-----------------------------------------------------
End Sub

Private Sub List1_DblClick()
    
    '----------------------------------------------------------
    'Double-Click event occurs after the Click event, so
    'values for lMainKey and sSubKey have already been determined
    'there.  Now we will deal with additional keys under this
    'one.
    '----------------------------------------------------------
    Dim bAlreadyExpanded%
    Dim iListWidth%
    
    sLevelSpaces = Space$((iLevel + 1) * LIST_INDENT)
    iX = iListIdx + 1
    
    '----------------------------------------------------------
    'If the next item in the list box has more spaces than
    'our current subkey, the tree is already expanded.  Collapse
    'the tree until there are no subkeys to the current subkey.
    '----------------------------------------------------------
    Do Until Left$(List1.List(iX), Len(sLevelSpaces)) <> sLevelSpaces
        List1.RemoveItem iX
        bAlreadyExpanded = True
    Loop
    
    '----------------------------------------------------------
    'If we collapsed the tree, we are done
    '----------------------------------------------------------
    If bAlreadyExpanded Then
        Exit Sub
    End If
    
    '----------------------------------------------------------
    'Otherwise we want to expand the tree one level
    '----------------------------------------------------------
    lTempLong = RegEnumKeys(lMainKey, sSubKey, iLevel, iListIdx + 1)
    
    '----------------------------------------------------------
    'Set Horizontal Scrollbar if necessary
    '----------------------------------------------------------
    iTempInt = SendMessage(List1.hwnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)
    Dim lpSize As SIZE
    For iX = 0 To List1.ListCount - 1
        lTempLong = GetTextExtentPoint(hDC, List1.List(iX), Len(List1.List(iX)), lpSize)
        iTempInt = lpSize.cx
        If iTempInt > iListWidth Then
            iListWidth = iTempInt
            iTempInt = SendMessage(List1.hwnd, LB_SETHORIZONTALEXTENT, iListWidth, 0&)
        End If
    Next iX


End Sub

Private Sub List2_DblClick()

    '----------------------------------------------------------
    'Call EditRegValue to load the value into the editor.
    '----------------------------------------------------------
    EditRegValue lMainKey, sSubKey, CLng(List2.ListIndex)

End Sub

