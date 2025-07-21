unit InputBox;

{$mode objfpc}{$H+}

interface

uses
  SysUtils; // For String types

// Function to simulate a console-based InputBox
// It will print the caption and prompt, then read a line from the console.
function InputBox(const ACaption, APrompt, ADefault: String): String;

implementation

function InputBox(const ACaption, APrompt, ADefault: String): String;
begin
  // Display the caption and prompt
  Writeln('--- ' + ACaption + ' ---');
  Write(APrompt);

  // If a default value is provided, show it
  if ADefault <> '' then
  begin // Added begin...end block
    Write(' [' + ADefault + ']: ');
  end
  else
  begin // Added begin...end block
    Write(': ');
  end; // Semicolon now correctly terminates the entire if-else statement

  // Read the user's input
  Readln(Result);

  // If the user entered nothing and a default was provided, use the default
  if (Result = '') and (ADefault <> '') then
    Result := ADefault;

  Writeln('--------------------');
end;

end.

