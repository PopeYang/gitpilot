; GitPilot - Inno Setup Installation Script
; This script will be updated by GitHub Actions during build

#define MyAppName "GitPilot"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "PopeYang"
#define MyAppURL "https://github.com/PopeYang/gitpilot"
#define MyAppExeName "gitpilot.exe"

[Setup]
; Application identifiers
AppId={{A5B4C3D2-E1F0-4A5B-8C9D-0E1F2A3B4C5D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}

; Installation directories
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes

; Output settings
OutputDir=installer_output
OutputBaseFilename=GitPilot-Setup-{#MyAppVersion}
Compression=lzma2/max
SolidCompression=yes

; Permissions and modern interface
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
WizardStyle=modern

; Minimum Windows version (Windows 10 1809)
MinVersion=10.0.17763

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
; Main application and all dependencies
Source: "deploy\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
; Documentation
Source: "README.md"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"

[Run]
; Offer to launch the application after installation
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

[Code]
// Check for existing installation
function InitializeSetup(): Boolean;
var
  ResultCode: Integer;
  UninstallExe: String;
begin
  Result := True;
  
  // Check if already installed
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#SetupSetting("AppId")}_is1',
    'UninstallString', UninstallExe) or
    RegQueryStringValue(HKEY_CURRENT_USER,
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#SetupSetting("AppId")}_is1',
    'UninstallString', UninstallExe) then
  begin
    if MsgBox('Detected existing installation. Uninstall first?', mbConfirmation, MB_YESNO) = IDYES then
    begin
      Exec(RemoveQuotes(UninstallExe), '/SILENT', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
    end;
  end;
end;
