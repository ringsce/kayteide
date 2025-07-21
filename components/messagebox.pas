unit MessageBox;

{$mode objfpc}{$H+}

interface

uses
  SysUtils; // For String types

// A simple console-based message box.
// AType parameter is ignored in this console stub, but kept for compatibility.
procedure ShowMessage(const AText: String);
function MessageBox(const AText, ACaption: String; AType: Integer): Integer;

implementation

procedure ShowMessage(const AText: String);
begin
  Writeln('--- Message ---');
  Writeln(AText);
  Writeln('---------------');
end;

function MessageBox(const AText, ACaption: String; AType: Integer): Integer;
begin
  Writeln('--- ' + ACaption + ' ---');
  Writeln(AText);
  Writeln('--------------------');
  Result := 1; // Simulate a "OK" or "Yes" button press for simplicity
end;

end.

