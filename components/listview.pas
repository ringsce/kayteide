unit listview;

{$mode objfpc}{$H+}

interface

uses
  SysUtils, Classes, Contnrs; // Classes for TStringList, Contnrs for TObjectList

type
  // TListItem: A simple record to hold data for each row in our console list view.
  // You can extend this record with more fields as needed (e.g., Date, Status).
  TListItem = record
    ID: Integer;
    Title: String;
    Description: String;
  end;

  // TListItemObject: A wrapper class for TListItem to allow it to be stored in TObjectList.
  TListItemObject = class(TObject)
  public
    Item: TListItem;
    constructor Create(AID: Integer; const ATitle, ADescription: String);
  end;

  // TConsoleListView: A class to manage and display the list in the console.
  TConsoleListView = class
  private
    FItems: TObjectList; // Stores TListItemObject instances
    procedure DrawLine(Length: Integer);
    procedure PrintHeader;
    procedure PrintItem(const AItem: TListItem);
  public
    constructor Create;
    destructor Destroy; override;
    procedure AddItem(AID: Integer; const ATitle, ADescription: String);
    procedure Display;
    procedure Clear;
  end;

implementation

{ TListItemObject }

constructor TListItemObject.Create(AID: Integer; const ATitle, ADescription: String);
begin
  inherited Create;
  Item.ID := AID;
  Item.Title := ATitle;
  Item.Description := ADescription;
end;

{ TConsoleListView }

constructor TConsoleListView.Create;
begin
  inherited Create;
  // Create TObjectList and set OwnsObjects to True so it automatically frees TListItemObject instances
  FItems := TObjectList.Create(True);
end;

destructor TConsoleListView.Destroy;
begin
  FreeAndNil(FItems); // Free the TObjectList, which in turn frees all TListItemObject instances
  inherited Destroy;
end;

procedure TConsoleListView.DrawLine(Length: Integer);
var
  I: Integer;
begin
  for I := 1 to Length do
    Write('-');
  Writeln;
end;

procedure TConsoleListView.PrintHeader;
begin
  DrawLine(80); // Adjust length as needed
  Writeln(Format('%-5s | %-25s | %-45s', ['ID', 'Title', 'Description']));
  DrawLine(80);
end;

procedure TConsoleListView.PrintItem(const AItem: TListItem);
begin
  Writeln(Format('%-5d | %-25s | %-45s', [AItem.ID, AItem.Title, AItem.Description]));
end;

procedure TConsoleListView.AddItem(AID: Integer; const ATitle, ADescription: String);
begin
  FItems.Add(TListItemObject.Create(AID, ATitle, ADescription));
end;

procedure TConsoleListView.Display;
var
  I: Integer;
begin
  if FItems.Count = 0 then
  begin
    Writeln('No items to display.');
    Exit;
  end;

  PrintHeader;
  for I := 0 to FItems.Count - 1 do
  begin
    PrintItem(TListItemObject(FItems[I]).Item);
  end;
  DrawLine(80);
end;

procedure TConsoleListView.Clear;
begin
  FItems.Clear; // Clears the list and frees the objects because OwnsObjects is True
end;

end.

