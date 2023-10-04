; -- 64Bit.iss --
; Demonstrates installation of a program built for the x64 (a.k.a. AMD64)
; architecture.
; To successfully run this installation and the program it installs,
; you must have a "x64" edition of Windows.

; SEE THE DOCUMENTATION FOR DETAILS ON CREATING .ISS SCRIPT FILES!

[Setup]
AppName=zlstools
AppVersion=0.1.0
WizardStyle=modern
DefaultDirName={userpf}\zlstools
DefaultGroupName=zlstools
Compression=lzma2
SolidCompression=yes
OutputDir=.\
OutputBaseFilename=zlstools-0.1.0-rc1-x64
; "ArchitecturesAllowed=x64" specifies that Setup cannot run on
; anything but x64.
ArchitecturesAllowed=x64
; "ArchitecturesInstallIn64BitMode=x64" requests that the install be
; done in "64-bit mode" on x64, meaning it should use the native
; 64-bit Program Files directory and the 64-bit view of the registry.
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest

[Files]
Source: "bin\x64\Release\zlsthumb.exe"; DestDir: "{app}";
Source: "bin\x64\Release\ZlsThumbnailProvider.dll"; DestDir: "{app}"; Flags: regserver
Source: "{tmp}\vc_redist.x64.exe"; DestDir: "{tmp}"; Flags: external; Check: NeedsToInstallVCRedist

[Run]
Filename: {tmp}\vc_redist.x64.exe; Parameters: "/q /passive /Q:a"; \
  StatusMsg: "Installing VC Redistributalbes..."; Check: NeedsToInstallVCRedist

[Code]
var
  DownloadPage: TDownloadWizardPage;

function OnDownloadProgress(const Url, FileName: String; const Progress, ProgressMax: Int64): Boolean;
begin
  if Progress = ProgressMax then
    Log(Format('Successfully downloaded file to {tmp}: %s', [FileName]));
  Result := True;
end;

function NeedsToInstallVCRedist: Boolean;
var
  installed: Cardinal;
  key: String;
begin
  Result := True;
  key := 'SOFTWARE\Wow6432Node\Microsoft\DevDiv\vc\Servicing\14.0\RuntimeMinimum';
  if RegQueryDWordValue(HKLM, key, 'Install', installed) then begin
      Result := (installed <> 1);
  end;
end;

procedure InitializeWizard;
begin
  DownloadPage := CreateDownloadPage(SetupMessage(msgWizardPreparing), SetupMessage(msgPreparingDesc), @OnDownloadProgress);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  if (CurPageID = wpReady) and NeedsToInstallVCRedist then begin
    DownloadPage.Clear;
    // Use AddEx to specify a username and password
    DownloadPage.Add('https://aka.ms/vs/17/release/vc_redist.x64.exe', 'vc_redist.x64.exe', '');
    DownloadPage.Show;
    try
      try
        DownloadPage.Download; // This downloads the files to {tmp}
        Result := True;
      except
        if DownloadPage.AbortedByUser then
          Log('Aborted by user.')
        else
          SuppressibleMsgBox(AddPeriod(GetExceptionMessage), mbCriticalError, MB_OK, IDOK);
        Result := False;
      end;
    finally
      DownloadPage.Hide;
    end;
  end else
    Result := True;
end;