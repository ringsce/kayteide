program TestSample;

var
  passed, failed: Integer;

procedure AssertEq(a, b: Integer; name: String);
begin
  if a = b then begin
    WriteLn('PASS: ', name);
    Inc(passed);
  end else begin
    WriteLn('FAIL: ', name, ' — expected ', b, ' got ', a);
    Inc(failed);
  end;
end;

begin
  passed := 0;
  failed := 0;

  AssertEq(1 + 1, 2,   'addition');
  AssertEq(10 - 3, 7,  'subtraction');
  AssertEq(3 * 4, 12,  'multiplication');
  AssertEq(10 div 2, 5,'division');

  { Intentional failure }
  AssertEq(42, 99, 'intentional_failure');

  WriteLn('');
  WriteLn('Results: ', passed, ' passed, ', failed, ' failed.');
end.
