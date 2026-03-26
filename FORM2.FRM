VERSION 4.00
Begin VB.Form Form2 
   Appearance      =   0  'Flat
   BackColor       =   &H00C0C0C0&
   BorderStyle     =   1  'Fixed Single
   Caption         =   "Form2"
   ClientHeight    =   2790
   ClientLeft      =   4560
   ClientTop       =   3810
   ClientWidth     =   5520
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
   Height          =   3195
   Left            =   4500
   LinkTopic       =   "Form2"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2790
   ScaleWidth      =   5520
   Top             =   3465
   Width           =   5640
   Begin VB.TextBox Text4 
      Appearance      =   0  'Flat
      Height          =   288
      Left            =   4080
      TabIndex        =   10
      Text            =   "Text4"
      Top             =   2460
      Visible         =   0   'False
      Width           =   492
   End
   Begin VB.TextBox Text3 
      Appearance      =   0  'Flat
      Height          =   288
      Left            =   3180
      TabIndex        =   9
      Text            =   "Text3"
      Top             =   2460
      Visible         =   0   'False
      Width           =   492
   End
   Begin VB.Frame Frame1 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Base"
      ForeColor       =   &H80000008&
      Height          =   912
      Left            =   360
      TabIndex        =   7
      Top             =   1680
      Visible         =   0   'False
      Width           =   2232
      Begin VB.OptionButton Option1 
         Appearance      =   0  'Flat
         BackColor       =   &H00C0C0C0&
         Caption         =   "Decimal"
         ForeColor       =   &H80000008&
         Height          =   252
         Index           =   1
         Left            =   360
         TabIndex        =   0
         Top             =   540
         Width           =   1512
      End
      Begin VB.OptionButton Option1 
         Appearance      =   0  'Flat
         BackColor       =   &H00C0C0C0&
         Caption         =   "Hexadecimal"
         ForeColor       =   &H80000008&
         Height          =   252
         Index           =   0
         Left            =   360
         TabIndex        =   8
         Top             =   240
         Value           =   -1  'True
         Width           =   1512
      End
   End
   Begin VB.CommandButton Command1 
      Appearance      =   0  'Flat
      BackColor       =   &H80000005&
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   312
      Index           =   1
      Left            =   4140
      TabIndex        =   6
      Top             =   1920
      Width           =   912
   End
   Begin VB.CommandButton Command1 
      Appearance      =   0  'Flat
      BackColor       =   &H80000005&
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   312
      Index           =   0
      Left            =   2940
      TabIndex        =   5
      Top             =   1920
      Width           =   912
   End
   Begin VB.TextBox Text2 
      Appearance      =   0  'Flat
      Height          =   312
      Left            =   300
      TabIndex        =   4
      Top             =   1260
      Width           =   4812
   End
   Begin VB.TextBox Text1 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Enabled         =   0   'False
      Height          =   312
      Left            =   300
      TabIndex        =   2
      Top             =   420
      Width           =   4812
   End
   Begin VB.Label Label1 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Data:"
      ForeColor       =   &H80000008&
      Height          =   312
      Left            =   360
      TabIndex        =   3
      Top             =   840
      Width           =   1632
   End
   Begin VB.Label Label2 
      Appearance      =   0  'Flat
      BackColor       =   &H00C0C0C0&
      Caption         =   "Value Name:"
      ForeColor       =   &H80000008&
      Height          =   252
      Left            =   360
      TabIndex        =   1
      Top             =   60
      Width           =   2172
   End
End
Attribute VB_Name = "Form2"
Attribute VB_Creatable = False
Attribute VB_Exposed = False
Option Explicit
Dim bPainted%

Private Sub Command1_Click(iIndex%)
    Dim nRtn&
    Dim hKey&
    Dim varTempVar
    Dim sSubKey$
    Dim sValueName$
    Dim sValueData$
    Select Case iIndex
        Case 0
            
            '-------------------------------------------------
            'OK was clicked
            'Values were loaded into visible and
            'hidden text boxes by the EditRegValue function.
            '-------------------------------------------------
            hKey = CLng(text3)
            sSubKey = text4
            sValueData = text2
            sValueName = Text1
            If sValueName = "(Default)" Then
                
                '---------------------------------------------
                'We are visually displaying (Default) when the
                'value name is null.
                '---------------------------------------------
                sValueName = ""
            End If

            '-------------------------------------------------
            'We handle DWORDs differently than strings
            '-------------------------------------------------
            If lCurrentType = REG_DWORD Then
                
                '---------------------------------------------
                'We do some tortuous maneuvers to make sure
                'that all values are hex longs, including decimal
                'values greater than 7FFFFFFF
                '---------------------------------------------
                varTempVar = text2
                If Option1(0) Then
                    varTempVar = "&H" & varTempVar
                End If
                If IsNumeric(varTempVar) Then
                    If Option1(0) Then
                        sValueData = text2
                    Else
                        fTempDbl = Val(text2 & "#")
                        If fTempDbl > &H7FFFFFFF Then
                            sValueData = Hex$(fTempDbl - 4294967296#)
                        Else
                            sValueData = Hex$(fTempDbl)
                        End If
                    End If
                Else
                    '-----------------------------------------
                    'We have a DWORD value, but the text box
                    'does not contain an number.
                    '-----------------------------------------
                    Exit Sub
                End If
            End If
            sTempString = sValueData
            
            '-------------------------------------------------
            'Write new value to the registry
            '-------------------------------------------------
            nRtn = RegSetValue(hKey, sSubKey, sValueName, sValueData)
            
            '-------------------------------------------------
            'If the value was successfully changed, update the list box
            '-------------------------------------------------
            If nRtn = 0 Then
                sValueData = sTempString
                sTempString = Text1    'gets us (Default) back if no value name
                If lCurrentType = REG_SZ Then
                    sValueData = """" & sValueData & """"
                End If
                If lCurrentType = REG_DWORD Then
                    
                    '-----------------------------------------
                    'Tortuous calculations again to display positive
                    'integer values greater than a VB long may contain.
                    '-----------------------------------------
                    If Len(sValueData) < 5 Then
                        sValueData = sValueData & "&"
                    End If
                    fTempDbl = Val("&H" & sValueData)
                    If fTempDbl < 0 Then
                        fTempDbl = fTempDbl + 4294967296#
                    End If
                    If Right$(sValueData, 1) = "&" Then
                        sValueData = Left$(sValueData, Len(sValueData) - 1)
                    End If
                    sValueData = "&H" & UCase(sValueData) & " (" & Format$(fTempDbl) & ")"
                End If

                '---------------------------------------------
                'The iList2Tab value comes from the RegEnumValues function,
                'and tells us how to format our output to match the other
                'values in the same list.
                '---------------------------------------------
                sTempString = sTempString & Space$(iList2Tab - Len(sValueName) + 1) & sValueData
                iTempInt = Form1!List2.ListIndex

                '---------------------------------------------
                'Remove the old, add the new.
                '---------------------------------------------
                Form1!List2.RemoveItem iTempInt
                Form1!List2.AddItem sTempString, iTempInt
                Form1!List2.ListIndex = iTempInt
            End If
            Unload Form2
    
        Case 1
            
            '-------------------------------------------------
            'Cancel was clicked
            '-------------------------------------------------
            Unload Form2
    End Select
End Sub

Private Sub Form_Load()
    CenterForm Me
End Sub

Private Sub Form_Paint()
    If Not bPainted Then
        bPainted = True
        text2.SelStart = 0
        text2.SelLength = Len(text2) + 1
    End If
End Sub

Private Sub Form_Unload(Cancel As Integer)
bPainted = False
End Sub

Private Sub Option1_Click(iIndex%)
Select Case iIndex
    Case 0
        
        '------------------------------------------
        'Convert from decimal to Hex
        '------------------------------------------
        sTempString = text2
        If Val(sTempString) < 0 Then
            
            '--------------------------------------
            'Force values that otherwise would be
            'given negative integer numbers to type
            'long.
            '--------------------------------------
            sTempString = sTempString & "&"
        End If
        fTempDbl = Val(text2 & "#")
            
            '--------------------------------------
            'Hex function will fail on values outside
            'the normal VB long range.  Convert such
            'to a negative value within the range
            '--------------------------------------
            If fTempDbl > &H7FFFFFFF Then
                text2 = Hex$(fTempDbl - 4294967296#)
            Else
                text2 = Hex$(fTempDbl)
            End If
    Case 1
        
        '------------------------------------------
        'Convert from Hex to decimal
        '------------------------------------------
        sTempString = text2
        If Len(sTempString) < 5 Then
            
            '--------------------------------------
            'Force values that otherwise would be
            'given negative integer numbers to type
            'long.
            '--------------------------------------
            sTempString = sTempString & "&"
        End If
        fTempDbl = Val("&H" & sTempString)
        If fTempDbl < 0 Then
            
            '--------------------------------------
            'Val function will return a negative long
            'for hex numbers greater than 7FFFFFFF.
            'Convert these to positive values for
            'display purposes.
            '--------------------------------------
            fTempDbl = fTempDbl + 4294967296#
        End If
        text2 = Format$(fTempDbl)
End Select
End Sub

